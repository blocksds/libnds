#include <nds/disc_io.h>
#include <nds/arm9/sdmmc.h>
#include <nds/system.h>

//#define SECTOR_SIZE 512
#define CRYPT_BUF_LEN 64

static u8 crypt_buf[SECTOR_SIZE * CRYPT_BUF_LEN] ALIGN(32);

static u32 fat_sig_fix_offset = 0;

static u32 sector_buf32[SECTOR_SIZE/sizeof(u32)];
static u8 *sector_buf = (u8*)sector_buf32;

static void getConsoleID(u8 *consoleID)
{
    // TODO
}

bool nandfs_Startup(void)
{
    printf("%s\n", __func__);

    if (!nand_Startup())
        return false;

    printf("Startup ok\n");


    return true;
}

static bool read_sectors(sec_t start, sec_t len, void *buffer)
{
    if (!nand_ReadSectors(start, len, crypt_buf))
    {
        printf("NANDIO: read error\n");
        return false;
    }

    memcpy(buffer, crypt_buf, len * SECTOR_SIZE);
#if 0
    dsi_nand_crypt(buffer, crypt_buf, start * SECTOR_SIZE / AES_BLOCK_SIZE,
                   len * SECTOR_SIZE / AES_BLOCK_SIZE);

    if (fat_sig_fix_offset && (start == fat_sig_fix_offset)
        && (((u8*)buffer)[0x36] == 0) && (((u8*)buffer)[0x37] == 0)
        && (((u8*)buffer)[0x38] == 0))
    {
        ((u8*)buffer)[0x36] = 'F';
        ((u8*)buffer)[0x37] = 'A';
        ((u8*)buffer)[0x38] = 'T';
    }
#endif
    return true;
}

static bool nandfs_ReadSectors(sec_t sector, sec_t numSectors, void *buffer)
{
    printf("%u %u %p\n", (unsigned int)sector, (unsigned int)numSectors, buffer);

    while (numSectors >= CRYPT_BUF_LEN)
    {
        if (!read_sectors(sector, CRYPT_BUF_LEN, buffer))
            return false;

        sector += CRYPT_BUF_LEN;
        numSectors -= CRYPT_BUF_LEN;
        buffer = ((u8*)buffer) + (SECTOR_SIZE * CRYPT_BUF_LEN);
    }

    if (numSectors > 0)
        return read_sectors(sector, numSectors, buffer);
    else
        return true;

    return false;
}

static bool nandfs_WriteSectors(sec_t sector, sec_t numSectors, const void *buffer)
{
    // Let's not do this for now
    (void)sector;
    (void)numSectors;
    (void)buffer;
    return false;
}

static bool nandfs_IsInserted(void)
{
    return true;
}

static bool nandfs_ClearStatus(void)
{
    return true;
}

static bool nandfs_Shutdown(void)
{
    return true;
}

static const DISC_INTERFACE __io_dsinand = {
    DEVICE_TYPE_DSI_NAND,
    FEATURE_MEDIUM_CANREAD | FEATURE_MEDIUM_CANWRITE,
    &nandfs_Startup,
    &nandfs_IsInserted,
    &nandfs_ReadSectors,
    &nandfs_WriteSectors,
    &nandfs_ClearStatus,
    &nandfs_Shutdown
};

const DISC_INTERFACE *get_io_dsinand(void)
{
    return isDSiMode() ? &__io_dsinand : NULL;
}
