// SPDX-License-Identifier: Zlib
// SPDX-FileNotice: Modified from the original version (https://github.com/profi200/dsi_sdmmc)
//
// Copyright (C) 2023 profi200

#include <stdalign.h>
#include <string.h>
#include <nds.h>

// Note on INIT_CLOCK:
// 400 kHz is allowed by the specs. 523 kHz has been proven to work reliably
// for SD cards and eMMC but very early MMCs can fail at init.
// We lose about 5 ms of time on init by using 261 kHz.
#define INIT_CLOCK     (400000u)   // Maximum 400 kHz.
#define DEFAULT_CLOCK  (20000000u) // Maximum 20 MHz.
#define HS_CLOCK       (50000000u) // Maximum 50 MHz.

// ARM7 timer clock = controller clock = CPU clock.
// swiDelay() doesn't seem to be cycle accurate meaning
// one cycle is 4 (?) CPU cycles.
#define SLEEP_MS_FUNC(ms)  swiDelay(8378 * (ms))


#define MMC_OCR_VOLT_MASK  (MMC_OCR_3_2_3_3V)                        // We support 3.3V only.
#define SD_OCR_VOLT_MASK   (SD_OCR_3_2_3_3V)                         // We support 3.3V only.
#define SD_IF_COND_ARG     (SD_CMD8_VHS_2_7_3_6V | SD_CMD8_CHK_PATT)
#define SD_OP_COND_ARG     (SD_OCR_VOLT_MASK)                        // We support 100 mA and 3.3V. Without HCS bit.
#define MMC_OP_COND_ARG    (MMC_OCR_SECT_MODE | MMC_OCR_VOLT_MASK)   // We support sector addressing and 3.3V.

// Note: DEV_TYPE_NONE must be zero.
enum
{
    // Device types.
    DEV_TYPE_NONE  = 0u, // Unitialized/no device.
    DEV_TYPE_MMC   = 1u, // (e)MMC.
    DEV_TYPE_MMCHC = 2u, // High capacity (e)MMC (>2 GB).
    DEV_TYPE_SDSC  = 3u, // SDSC.
    DEV_TYPE_SDHC  = 4u, // SDHC, SDXC.
    DEV_TYPE_SDUC  = 5u  // SDUC.
};

#define IS_DEV_MMC(dev)  ((dev) < DEV_TYPE_SDSC)


typedef struct
{
    TmioPort port;
    u8 type;       // Device type. 0 = none, 1 = (e)MMC, 2 = High capacity (e)MMC,
                   // 3 = SDSC, 4 = SDHC/SDXC, 5 = SDUC.
    u8 prot;       // Protection bits. Each bit 1 = protected.
                   // Bit 0 SD card slider, bit 1 temporary write protection (CSD),
                   // bit 2 permanent write protection (CSD) and bit 3 password protection.
    u16 rca;       // Relative Card Address (RCA).
    u16 ccc;       // (e)MMC/SD command class support from CSD. One per bit starting at 0.
    u32 sectors;   // Size in 512 byte units.
    u32 status;    // R1 card status on error. Only updated on errors.

    // Cached card infos.
    u32 cid[4];    // Raw CID without the CRC.
} SdmmcDev;

static SdmmcDev g_devs[2] = {0};

static u32 sendAppCmd(TmioPort *const port, const u16 cmd, const u32 arg, const u32 rca)
{
    // Send app CMD. Same CMD for (e)MMC/SD.
    // TODO: Check the APP_CMD bit in the response?
    //       Linux does it but is it really necessary? SD spec 4.3.9.1.
    u32 res = TMIO_sendCommand(port, MMC_APP_CMD, rca);
    if (res == 0)
    {
        res = TMIO_sendCommand(port, cmd, arg);
    }

    return res;
}

static u32 goIdleState(TmioPort *const port)
{
    // Enter idle state before we start the init procedure.
    // Works from all but inactive state. CMD is the same for (e)MMC/SD.
    // For (e)MMC there are optional init paths:
    // arg = 0x00000000 -> GO_IDLE_STATE.
    // arg = 0xF0F0F0F0 -> GO_PRE_IDLE_STATE.
    // arg = 0xFFFFFFFA -> BOOT_INITIATION.
    u32 res = TMIO_sendCommand(port, MMC_GO_IDLE_STATE, 0);
    if (res != 0)
        return SDMMC_ERR_GO_IDLE_STATE;

    return SDMMC_ERR_NONE;
}

static u32 initIdleState(TmioPort *const port, u8 *const devTypeOut)
{
    // Tell the card what interfaces and voltages we support.
    // Only SD v2 and up will respond. (e)MMC won't respond.
    u32 res = TMIO_sendCommand(port, SD_SEND_IF_COND, SD_IF_COND_ARG);
    if (res == 0)
    {
        // If the card supports the interfaces and voltages
        // it should echo back the check pattern and set the
        // support bits.
        // Since we don't support anything but the
        // standard SD interface at 3.3V we can check
        // the whole response at once.
        if (port->resp[0] != SD_IF_COND_ARG)
            return SDMMC_ERR_IF_COND_RESP;
    }
    else if (res != SD_STATUS_ERR_CMD_TIMEOUT) // Card responded but an error occured.
        return SDMMC_ERR_SEND_IF_COND;

    // Send the first app CMD. If this times out it's (e)MMC.
    // If SEND_IF_COND timed out tell the SD card we are a v1 host.
    const u32 opCondArg = SD_OP_COND_ARG | (res << 8 ^ SD_ACMD41_HCS); // Caution! Controller specific hack.
    u8 devType = DEV_TYPE_SDSC;
    res = sendAppCmd(port, SD_APP_SD_SEND_OP_COND, opCondArg, 0);
    if (res == SD_STATUS_ERR_CMD_TIMEOUT)
        devType = DEV_TYPE_MMC; // Continue with (e)MMC init.
    else if (res != 0)
        return SDMMC_ERR_SEND_OP_COND; // Unknown error.

    if (devType == DEV_TYPE_MMC) // (e)MMC.
    {
        // Loop until a timeout of 1 second or the card is ready.
        u32 tries = 200;
        u32 ocr;
        while (1)
        {
            res = TMIO_sendCommand(port, MMC_SEND_OP_COND, MMC_OP_COND_ARG);
            if (res != 0)
                return SDMMC_ERR_SEND_OP_COND;

            ocr = port->resp[0];
            if (!--tries || (ocr & MMC_OCR_READY))
                break;

            // Linux uses 10 ms but the card doesn't become ready faster
            // when polling with delay. Use 5 ms as compromise so not much
            // time is wasted when the card becomes ready in the middle of the delay.
            SLEEP_MS_FUNC(5);
        }

        // (e)MMC didn't finish init within 1 second.
        if (tries == 0)
            return SDMMC_ERR_OP_COND_TMOUT;

        // Check if the (e)MMC supports the voltage and if it's high capacity.
        if (!(ocr & MMC_OCR_VOLT_MASK))
            return SDMMC_ERR_VOLT_SUPPORT; // Voltage not supported.
        if (ocr & MMC_OCR_SECT_MODE)
            devType = DEV_TYPE_MMCHC;      // 7.4.3.
    }
    else // SD card.
    {
        // Loop until a timeout of 1 second or the card is ready.
        u32 tries = 200;
        u32 ocr;
        while (1)
        {
            ocr = port->resp[0];
            if (!--tries || (ocr & SD_OCR_READY))
                break;

            // Linux uses 10 ms but the card doesn't become ready faster
            // when polling with delay. Use 5 ms as compromise so not much
            // time is wasted when the card becomes ready in the middle of the delay.
            SLEEP_MS_FUNC(5);

            res = sendAppCmd(port, SD_APP_SD_SEND_OP_COND, opCondArg, 0);
            if (res != 0)
                return SDMMC_ERR_SEND_OP_COND;
        }

        // SD card didn't finish init within 1 second.
        if (tries == 0)
            return SDMMC_ERR_OP_COND_TMOUT;

        if (!(ocr & SD_OCR_VOLT_MASK))
            return SDMMC_ERR_VOLT_SUPPORT; // Voltage not supported.
        if (ocr & SD_OCR_CCS)
            devType = DEV_TYPE_SDHC;
    }

    *devTypeOut = devType;

    return SDMMC_ERR_NONE;
}

static u32 initReadyState(SdmmcDev *const dev)
{
    TmioPort *const port = &dev->port;

    // SD card voltage switch sequence goes here if supported.

    // Get the CID. CMD is the same for (e)MMC/SD.
    u32 res = TMIO_sendCommand(port, MMC_ALL_SEND_CID, 0);
    if (res != 0)
        return SDMMC_ERR_ALL_SEND_CID;
    memcpy(dev->cid, port->resp, 16);

    return SDMMC_ERR_NONE;
}

static u32 initIdentState(SdmmcDev *const dev, const u8 devType, u32 *const rcaOut)
{
    TmioPort *const port = &dev->port;

    u32 rca;
    if (IS_DEV_MMC(devType)) // (e)MMC.
    {
        // Set the RCA of the (e)MMC to 1. 0 is reserved.
        // The RCA is in the upper 16 bits of the argument.
        rca = 1;
        u32 res = TMIO_sendCommand(port, MMC_SET_RELATIVE_ADDR, rca << 16);
        if (res != 0)
            return SDMMC_ERR_SET_SEND_RCA;
    }
    else // SD card.
    {
        // Ask the SD card to send its RCA.
        u32 res = TMIO_sendCommand(port, SD_SEND_RELATIVE_ADDR, 0);
        if (res != 0)
            return SDMMC_ERR_SET_SEND_RCA;

        // RCA in upper 16 bits. Discards lower status bits of R6 response.
        rca = port->resp[0] >> 16;
    }

    dev->rca = rca;
    *rcaOut = rca << 16;

    return SDMMC_ERR_NONE;
}

// Based on UNSTUFF_BITS from linux/drivers/mmc/core/sd.c.
// Extracts up to 32 bits from a u32[4] array.
static inline u32 extractBits(const u32 resp[4], const u32 start, const u32 size)
{
    const u32 mask = (size < 32 ? 1u << size : 0u) - 1;
    const u32 off = 3 - (start / 32);
    const u32 shift = start & 31u;

    u32 res = resp[off] >> shift;
    if (size + shift > 32)
        res |= resp[off - 1] << ((32u - shift) & 31u);

    return res & mask;
}

static void parseCsd(SdmmcDev *const dev, const u8 devType, u8 *const spec_vers_out)
{
    // Note: The MSBs are in csd[0].
    const u32 *const csd = dev->port.resp;

    const u8 structure = extractBits(csd, 126, 2); // [127:126]
    *spec_vers_out = extractBits(csd, 122, 4);     // [125:122] All 0 for SD cards.
    dev->ccc = extractBits(csd, 84, 12);           // [95:84]
    u32 sectors = 0;
    if (structure == 0 || devType == DEV_TYPE_MMC) // structure = 0 is CSD version 1.0.
    {
        const u32 read_bl_len = extractBits(csd, 80, 4);  // [83:80]
        const u32 c_size      = extractBits(csd, 62, 12); // [73:62]
        const u32 c_size_mult = extractBits(csd, 47, 3);  // [49:47]

        // For SD cards with CSD 1.0 and <=2 GB (e)MMC this calculation is used.
        // Note: READ_BL_LEN is at least 9.
        // Modified/simplified to calculate sectors instead of bytes.
        sectors = (c_size + 1) << (c_size_mult + 2 + read_bl_len - 9);
    }
    else if (devType != DEV_TYPE_MMCHC)
    {
        // SD CSD version 3.0 format.
        // For version 2.0 this is 22 bits however the upper bits
        // are reserved and zero filled so this is fine.
        const u32 c_size = extractBits(csd, 48, 28); // [75:48]

        // Calculation for SD cards with CSD >1.0.
        sectors = (c_size + 1) << 10;
    }
    // Else for high capacity (e)MMC the sectors will be read later from EXT_CSD.
    dev->sectors = sectors;

    // Parse temporary and permanent write protection bits.
    u8 prot = extractBits(csd, 12, 1) << 1; // [12:12] Not checked by Linux.
    prot |= extractBits(csd, 13, 1) << 2;   // [13:13]
    dev->prot |= prot;
}

static u32 initStandbyState(SdmmcDev *const dev, const u8 devType, const u32 rca, u8 *const spec_vers_out)
{
    TmioPort *const port = &dev->port;

    // Get the CSD. CMD is the same for (e)MMC/SD.
    u32 res = TMIO_sendCommand(port, MMC_SEND_CSD, rca);
    if (res != 0)
        return SDMMC_ERR_SEND_CSD;
    parseCsd(dev, devType, spec_vers_out);

    // CMD is the same for (e)MMC/SD however both R1 and R1b responses are used.
    // We assume R1b and hope it doesn't time out.
    res = TMIO_sendCommand(port, MMC_SELECT_CARD, rca);
    if (res != 0)
        return SDMMC_ERR_SELECT_CARD;

    // The SD card spec mentions that we should check the lock bit in the
    // response to CMD7 to identify cards requiring a password to unlock.
    // Same seems to apply for (e)MMC.
    // Same bit for (e)MMC/SD R1 card status.
    dev->prot |= (port->resp[0] & MMC_R1_CARD_IS_LOCKED) >> 22; // Bit 3.

    return SDMMC_ERR_NONE;
}

// TODO: Set the timeout based on clock speed (Tmio uses SDCLK for timeouts).
//       The tmio driver sets a sane default but we should calculate it anyway.
static u32 initTranState(SdmmcDev *const dev, const u8 devType, const u32 rca,
                         const u8 spec_vers)
{
    TmioPort *const port = &dev->port;

    if (IS_DEV_MMC(devType)) // (e)MMC.
    {
        // EXT_CSD, non-1 bit bus width and HS timing are only
        // supported by (e)MMC SPEC_VERS 4.1 and higher.
        if (spec_vers > 3) // Version 4.1–4.2–4.3 or higher.
        {
            // The (e)MMC spec says to check the card status after a SWITCH CMD (7.6.1).
            // I think we can get away without checking this because support for HS timing
            // and 4 bit bus width is mandatory for this spec version. If the card is
            // non-standard we will encounter errors on the next CMD anyway.
            // Switch to 4 bit bus mode.
            const u32 busWidthArg = MMC_SWITCH_ARG(MMC_SWITCH_ACC_WR_BYTE,
                                                   EXT_CSD_BUS_WIDTH, 1, 0);
            u32 res = TMIO_sendCommand(port, MMC_SWITCH, busWidthArg);
            if (res != 0)
                return SDMMC_ERR_SET_BUS_WIDTH;
            TMIO_setBusWidth(port, 4);

            // We should also check in the EXT_CSD the power budget for the card.
            // Nintendo seems to leave it on default (no change).

            if (devType == DEV_TYPE_MMCHC)
            {
                // Note: The EXT_CSD is normally read before touching HS timing and bus width.
                //       We can take advantage of the faster data transfer with this order.
                alignas(4) u8 ext_csd[512];
                TMIO_setBuffer(port, (u32 *)ext_csd, 1, NULL);
                res = TMIO_sendCommand(port, MMC_SEND_EXT_CSD, 0);
                if (res != 0)
                    return SDMMC_ERR_SEND_EXT_CSD;

                // Get sector count from EXT_CSD only if sector addressing is used because
                // byte addressed (e)MMC may set sector count to 0.
                dev->sectors = ext_csd[EXT_CSD_SEC_COUNT + 3] << 24 |
                               ext_csd[EXT_CSD_SEC_COUNT + 2] << 16 |
                               ext_csd[EXT_CSD_SEC_COUNT + 1] << 8 |
                               ext_csd[EXT_CSD_SEC_COUNT + 0];
            }
        }
    }
    else // SD card.
    {
        // Remove DAT3 pull-up. Linux doesn't do it but the SD spec recommends it.
        u32 res = sendAppCmd(port, SD_APP_SET_CLR_CARD_DETECT, 0, rca); // arg = 0 removes the pull-up.
        if (res != 0)
            return SDMMC_ERR_SET_CLR_CD;

        // Switch to 4 bit bus mode.
        res = sendAppCmd(port, SD_APP_SET_BUS_WIDTH, 2, rca); // arg = 2 is 4 bit bus width.
        if (res != 0)
            return SDMMC_ERR_SET_BUS_WIDTH;
        TMIO_setBusWidth(port, 4);
    }

    // SD:     The description for CMD SET_BLOCKLEN says 512 bytes is the default.
    // (e)MMC: The description for READ_BL_LEN (CSD) says 512 bytes is the default.
    // So it's not required to set the block length.

    return SDMMC_ERR_NONE;
}

u32 SDMMC_init(const u8 devNum)
{
    if (devNum > SDMMC_MAX_DEV_NUM)
        return SDMMC_ERR_INVAL_PARAM;

    SdmmcDev *const dev = &g_devs[devNum];
    if (dev->type != DEV_TYPE_NONE)
        return SDMMC_ERR_INITIALIZED;

    // Check SD card write protection slider.
    if (devNum == SDMMC_DEV_CARD)
        dev->prot = !TMIO_cardWritable();

    // Init port, enable clock output and wait 74 clocks.
    TmioPort *const port = &dev->port;
    TMIO_initPort(port, devNum);
    TMIO_powerupSequence(port); // Setup continuous clock and wait 74 clocks.

    u32 res = goIdleState(port);
    if (res != SDMMC_ERR_NONE)
        return res;

    // (e)MMC/SD now in idle state (idle).
    u8 devType;
    res = initIdleState(port, &devType);
    if (res != SDMMC_ERR_NONE)
        return res;

    // Stop clock at idle, init clock.
    TMIO_setClock(port, INIT_CLOCK);

    // (e)MMC/SD now in ready state (ready).
    res = initReadyState(dev);
    if (res != SDMMC_ERR_NONE)
        return res;

    // (e)MMC/SD now in identification state (ident).
    u32 rca;
    res = initIdentState(dev, devType, &rca);
    if (res != SDMMC_ERR_NONE)
        return res;

    // (e)MMC/SD now in stand-by state (stby).
    // Maximum at this point would be 20 MHz for (e)MMC and 25 for SD.
    // SD: We can increase the clock after end of identification state.
    // TODO: eMMC spec section 7.6
    // "Until the contents of the CSD register is known by the host,
    // the fPP clock rate must remain at fOD. (See Section 12.7 on page 176.)"
    // Since the absolute minimum clock rate is 20 MHz and we are in push-pull
    // mode already can we cheat and switch to <=20 MHz before getting the CSD?
    // Note: This seems to be working just fine in all tests.
    TMIO_setClock(port, DEFAULT_CLOCK);

    u8 spec_vers;
    res = initStandbyState(dev, devType, rca, &spec_vers);
    if (res != SDMMC_ERR_NONE)
        return res;

    // (e)MMC/SD now in transfer state (tran).
    res = initTranState(dev, devType, rca, spec_vers);
    if (res != SDMMC_ERR_NONE)
        return res;

    // Only set dev type on successful init.
    dev->type = devType;

    return SDMMC_ERR_NONE;
}

u32 SDMMC_setSleepMode(const u8 devNum, const bool enabled)
{
    if (devNum > SDMMC_MAX_DEV_NUM)
        return SDMMC_ERR_INVAL_PARAM;

    SdmmcDev *const dev = &g_devs[devNum];
    TmioPort *const port = &dev->port;
    const u32 rca = (u32)dev->rca << 16;
    const u8 devType = dev->type;
    if (enabled)
    {
        // Deselect card to go back to stand-by state.
        // CMD is the same for (e)MMC/SD.
        u32 res = TMIO_sendCommand(port, MMC_DESELECT_CARD, 0);
        if (res != 0)
            return SDMMC_ERR_SELECT_CARD;

        // Only (e)MMC can go into true sleep mode.
        if (IS_DEV_MMC(devType))
        {
            // Switch (e)MMC into sleep mode.
            res = TMIO_sendCommand(port, MMC_SLEEP_AWAKE, rca | 1u << 15);
            if (res != 0)
                return SDMMC_ERR_SLEEP_AWAKE;
            // TODO: Power down eMMC. This is confirmed working on 3DS.
        }
    }
    else
    {
        if (IS_DEV_MMC(devType))
        {
            // TODO: Power up eMMC. This is confirmed working on 3DS.
            // Wake (e)MMC up from sleep mode.
            u32 res = TMIO_sendCommand(port, MMC_SLEEP_AWAKE, rca);
            if (res != 0)
                return SDMMC_ERR_SLEEP_AWAKE;
        }

        // Select card to go back to transfer state.
        // CMD is the same for (e)MMC/SD.
        u32 res = TMIO_sendCommand(port, MMC_SELECT_CARD, rca);
        if (res != 0)
            return SDMMC_ERR_SELECT_CARD;
    }

    return SDMMC_ERR_NONE;
}

// TODO: Is there any "best practice" way of deinitializing cards?
//       Kick the card back into idle state maybe?
//       Linux seems to deselect cards on "suspend".
u32 SDMMC_deinit(const u8 devNum)
{
    if (devNum > SDMMC_MAX_DEV_NUM)
        return SDMMC_ERR_INVAL_PARAM;

    memset(&g_devs[devNum], 0, sizeof(SdmmcDev));

    return SDMMC_ERR_NONE;
}

u32 SDMMC_lockUnlock(const u8 devNum, const u8 mode, const u8 *const pwd, const u8 pwdLen)
{
    // Password length is maximum 16 bytes except when replacing a password.
    if (devNum > SDMMC_MAX_DEV_NUM || pwdLen > 32)
        return SDMMC_ERR_INVAL_PARAM;

    // Set block length on (e)MMC/SD side and host.
    // Same CMD for (e)MMC/SD.
    SdmmcDev *const dev = &g_devs[devNum];
    TmioPort *const port = &dev->port;
    const u32 blockLen = (mode != SDMMC_LK_ERASE ? 2 + pwdLen : 1);
    u32 res = TMIO_sendCommand(port, MMC_SET_BLOCKLEN, blockLen);
    if (res != 0)
        return SDMMC_ERR_SET_BLOCKLEN;
    TMIO_setBlockLen(port, blockLen);

    do
    {
        // Prepare lock/unlock data block.
        alignas(4) u8 buf[36] = {0}; // Size multiple of 4 (TMIO driver limitation).
        buf[0] = mode;
        buf[1] = pwdLen;
        memcpy(&buf[2], pwd, pwdLen);

        // Dirty hack to extend the data timeout to a bit over 4 minutes with TMIO controller.
        // We need 3 minutes minimum for erase.
        const u16 clk_ctrl_backup = port->sd_clk_ctrl;
        TMIO_setClock(port, 130913);

        // Note: Command class 7 support is mandatory for (e)MMC. Not for SD cards until 2.00.
        // Same CMD for (e)MMC/SD.
        TMIO_setBuffer(port, (u32 *)buf, 1, NULL);
        res = TMIO_sendCommand(port, MMC_LOCK_UNLOCK, 0);
        port->sd_clk_ctrl = clk_ctrl_backup; // Undo the data timeout hack.
        if (res != 0)
        {
            res = SDMMC_ERR_LOCK_UNLOCK;
            break;
        }

        // Restore default block length and get the R1 status.
        // Same CMD for (e)MMC/SD.
        res = TMIO_sendCommand(port, MMC_SET_BLOCKLEN, 512);
        if (res != 0)
        {
            res = SDMMC_ERR_SET_BLOCKLEN;
            break;
        }
        TMIO_setBlockLen(port, 512);

        // Check if lock/unlock worked.
        // Same bit for (e)MMC/SD R1 card status.
        const u32 status = port->resp[0];
        if (status & MMC_R1_LOCK_UNLOCK_FAILED)
            res = SDMMC_ERR_LOCK_UNLOCK_FAIL;

        // Update lock status.
        const u8 prot = dev->prot & ~(1u << 3);
        dev->prot = prot | (status >> 22 & 1u << 3);
    } while (0);

    return res;
}

// People should not mess with the state which is the reason
// why the struct is not exposed directly.
static_assert(sizeof(SdmmcDev) == 68, "Wrong SDMMC dev export/import size.");

u32 SDMMC_exportDevState(const u8 devNum, u8 devOut[64])
{
    if (devNum > SDMMC_MAX_DEV_NUM)
        return SDMMC_ERR_INVAL_PARAM;

    // Check if the device is initialized.
    const SdmmcDev *const dev = &g_devs[devNum];
    if (dev->type == DEV_TYPE_NONE)
        return SDMMC_ERR_NO_CARD;

    memcpy(devOut, dev, 64);

    return SDMMC_ERR_NONE;
}

u32 SDMMC_importDevState(const u8 devNum, const u8 devIn[64])
{
    if (devNum > SDMMC_MAX_DEV_NUM)
        return SDMMC_ERR_INVAL_PARAM;

    // Make sure there is a card inserted.
    if (devNum == SDMMC_DEV_CARD && !TMIO_cardDetected())
        return SDMMC_ERR_NO_CARD;

    // Check if the device is initialized.
    SdmmcDev *const dev = &g_devs[devNum];
    if (dev->type != DEV_TYPE_NONE)
        return SDMMC_ERR_INITIALIZED;

    memcpy(dev, devIn, 64);

    // Update write protection slider state just in case.
    dev->prot |= !TMIO_cardWritable();

    return SDMMC_ERR_NONE;
}

// TODO: Less controller dependent code.
u32 SDMMC_getDevInfo(const u8 devNum, SdmmcInfo *const infoOut)
{
    if (devNum > SDMMC_MAX_DEV_NUM)
        return SDMMC_ERR_INVAL_PARAM;

    const SdmmcDev *const dev = &g_devs[devNum];
    const TmioPort *const port = &dev->port;

    infoOut->type    = dev->type;
    infoOut->prot    = dev->prot;
    infoOut->rca     = dev->rca;
    infoOut->sectors = dev->sectors;

    const u32 clkSetting = port->sd_clk_ctrl & 0xFFu;
    infoOut->clock       = TMIO_HCLK / (clkSetting ? clkSetting << 2 : 2);

    memcpy(infoOut->cid, dev->cid, 16);
    infoOut->ccc      = dev->ccc;
    infoOut->busWidth = (port->sd_option & SD_OPTION_BUS_WIDTH1 ? 1 : 4);

    return SDMMC_ERR_NONE;
}

u32 SDMMC_getCid(const u8 devNum, u32 cidOut[4])
{
    if (devNum > SDMMC_MAX_DEV_NUM)
        return SDMMC_ERR_INVAL_PARAM;

    if (cidOut != NULL)
        memcpy(cidOut, g_devs[devNum].cid, 16);

    return SDMMC_ERR_NONE;
}

u32 SDMMC_getCidRaw(const u8 devNum, u32 cidOut[4])
{
    u32 cidFixed[4];
    u32 err = SDMMC_getCid(devNum, cidFixed);
    if (err != SDMMC_ERR_NONE)
        return err;

    if (cidOut != NULL)
    {
        cidOut[0] = (cidFixed[3] >> 8) | (cidFixed[2] << 24);
        cidOut[1] = (cidFixed[2] >> 8) | (cidFixed[1] << 24);
        cidOut[2] = (cidFixed[1] >> 8) | (cidFixed[0] << 24);
        cidOut[3] = (cidFixed[0] >> 8);
    }

    return SDMMC_ERR_NONE;
}

u8 SDMMC_getDiskStatus(const u8 devNum)
{
    if (devNum > SDMMC_MAX_DEV_NUM)
        return SDMMC_STATUS_NODISK | SDMMC_STATUS_NOINIT;

    u8 status = 0;
    if (devNum == SDMMC_DEV_CARD && !TMIO_cardDetected())
        return SDMMC_STATUS_NODISK | SDMMC_STATUS_NOINIT;

    const SdmmcDev *const dev = &g_devs[devNum];
    if (dev->type == DEV_TYPE_NONE)
        status |= SDMMC_STATUS_NOINIT;
    else if (!(status & SDMMC_STATUS_NODISK) && dev->prot)
        status |= SDMMC_STATUS_PROTECT;

    return status;
}

u32 SDMMC_getSectors(const u8 devNum)
{
    if (devNum > SDMMC_MAX_DEV_NUM)
        return 0;

    return g_devs[devNum].sectors;
}

static u32 updateStatus(SdmmcDev *const dev, const bool stopTransmission)
{
    TmioPort *const port = &dev->port;

    // MMC_STOP_TRANSMISSION: Same CMD for (e)MMC/SD. Relies on the driver returning a proper response.
    // MMC_SEND_STATUS:       Same CMD for (e)MMC/SD but the argument format differs slightly.
    u32 res;
    if (stopTransmission)
        res = TMIO_sendCommand(port, MMC_STOP_TRANSMISSION, 0);
    else
        res = TMIO_sendCommand(port, MMC_SEND_STATUS, (u32)dev->rca << 16);
    dev->status = (res == 0 ? port->resp[0] : 0); // Don't update the status with stale data.

    return res;
}

// Note: On multi-block read from the last 2 sectors there are no errors reported by the controller
//       however the R1 card status may report ADDRESS_OUT_OF_RANGE on next(?) status read.
//       This error is normal for (e)MMC and can be ignored.
u32 SDMMC_readSectorsCrypt(const u8 devNum, u32 sect, void *const buf, const u16 count, tmio_callback_t crypt_callback)
{
    if (devNum > SDMMC_MAX_DEV_NUM || count == 0)
        return SDMMC_ERR_INVAL_PARAM;

    // Check if the device is initialized.
    SdmmcDev *const dev = &g_devs[devNum];
    const u8 devType = dev->type;
    if (devType == DEV_TYPE_NONE)
        return SDMMC_ERR_NO_CARD;

    // Set destination buffer and sector count.
    TmioPort *const port = &dev->port;
    TMIO_setBuffer(port, buf, count, crypt_callback);

    // Read a single 512 bytes block. Same CMD for (e)MMC/SD.
    // Read multiple 512 bytes blocks. Same CMD for (e)MMC/SD.
    const u16 readCmd = (count == 1 ? MMC_READ_SINGLE_BLOCK : MMC_READ_MULTIPLE_BLOCK);
    if (devType == DEV_TYPE_MMC || devType == DEV_TYPE_SDSC)
        sect *= 512; // Byte addressing.

    u32 res = TMIO_sendCommand(port, readCmd, sect);
    if (res != 0)
    {
        // On error in the middle of multi-block reads the card will be stuck
        // in data state and we need to send STOP_TRANSMISSION to bring it
        // back to tran state.
        // Otherwise for single-block reads just update the status.
        updateStatus(dev, count > 1);

        return SDMMC_ERR_SECT_RW;
    }

    return SDMMC_ERR_NONE;
}

// Note: On multi-block write to the last 2 sectors there are no errors reported by the controller
//       however the R1 card status may report ADDRESS_OUT_OF_RANGE on next(?) status read.
//       This error is normal for (e)MMC and can be ignored.
u32 SDMMC_writeSectorsCrypt(const u8 devNum, u32 sect, const void *const buf, const u16 count, tmio_callback_t crypt_callback)
{
    if (devNum > SDMMC_MAX_DEV_NUM || count == 0)
        return SDMMC_ERR_INVAL_PARAM;

    // Check if the device is initialized.
    SdmmcDev *const dev = &g_devs[devNum];
    const u8 devType = dev->type;
    if (devType == DEV_TYPE_NONE)
        return SDMMC_ERR_NO_CARD;

    // Check if the device is write protected.
    if (dev->prot != 0)
        return SDMMC_ERR_WRITE_PROT;

    // Set source buffer and sector count.
    TmioPort *const port = &dev->port;
    TMIO_setBuffer(port, (void *)buf, count, crypt_callback);

    // Write a single 512 bytes block. Same CMD for (e)MMC/SD.
    // Write multiple 512 bytes blocks. Same CMD for (e)MMC/SD.
    const u16 writeCmd = (count == 1 ? MMC_WRITE_BLOCK : MMC_WRITE_MULTIPLE_BLOCK);
    if (devType == DEV_TYPE_MMC || devType == DEV_TYPE_SDSC)
        sect *= 512; // Byte addressing.

    const u32 res = TMIO_sendCommand(port, writeCmd, sect);
    if (res != 0)
    {
        // On error in the middle of multi-block writes the card will be stuck
        // in data state and we need to send STOP_TRANSMISSION to bring it
        // back to tran state.
        // Otherwise for single-block writes just update the status.
        updateStatus(dev, count > 1);

        return SDMMC_ERR_SECT_RW;
    }

    return SDMMC_ERR_NONE;
}

u32 SDMMC_sendCommand(const u8 devNum, MmcCommand *const mmcCmd)
{
    if (devNum > SDMMC_MAX_DEV_NUM)
        return SDMMC_ERR_INVAL_PARAM;

    SdmmcDev *const dev = &g_devs[devNum];
    TmioPort *const port = &dev->port;
    TMIO_setBlockLen(port, mmcCmd->blkLen);
    TMIO_setBuffer(port, mmcCmd->buf, mmcCmd->count, NULL);

    const u32 res = TMIO_sendCommand(port, mmcCmd->cmd, mmcCmd->arg);
    TMIO_setBlockLen(port, 512); // Restore default block length.
    if (res != 0)
    {
        updateStatus(dev, false);
        return SDMMC_ERR_SEND_CMD;
    }

    memcpy(mmcCmd->resp, port->resp, 16);

    return SDMMC_ERR_NONE;
}

u32 SDMMC_getLastR1error(const u8 devNum)
{
    if (devNum > SDMMC_MAX_DEV_NUM)
        return 0;

    SdmmcDev *const dev = &g_devs[devNum];
    const u32 status = dev->status;
    dev->status = 0;

    return status;
}
