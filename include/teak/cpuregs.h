// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023 Antonio Niño Díaz

#ifndef LIBNDS_CPUREGS_H__
#define LIBNDS_CPUREGS_H__

// DSi Teak CPU Control/Status Registers
// =====================================

// st0 - Old TL1 Status/Control Register st0
// -----------------------------------------

#define ST0_SATURATION_OFF              (0 << 0)
#define ST0_SATURATION_ON               (1 << 0) // Saturate "Ax to data"

#define ST0_IRQ_DISABLE                 (0 << 1)
#define ST0_IRQ_ENABLE                  (1 << 1) // IE

#define ST0_IRQ_INT0_DISABLE            (0 << 2)
#define ST0_IRQ_INT0_ENABLE             (1 << 2) // Enable if IE=1

#define ST0_IRQ_INT1_DISABLE            (0 << 3)
#define ST0_IRQ_INT1_ENABLE             (1 << 3) // Enable if IE=1

#define ST0_FLAG_R                      (1 << 4)
#define ST0_FLAG_L                      (1 << 5)
#define ST0_FLAG_E                      (1 << 6)
#define ST0_FLAG_C                      (1 << 7)
#define ST0_FLAG_V                      (1 << 8)
#define ST0_FLAG_N                      (1 << 8)
#define ST0_FLAG_M                      (1 << 9)
#define ST0_FLAG_Z                      (1 << 10)

#define ST0_A0E_MASK                    (0xF << 12) // a0e

// st1 - Old TL1 Status/Control Register st1
// -----------------------------------------

#define ST1_DATA_MEMORY_PAGE(p)         ((p) << 0) // 8 bits. See "load page"

#define ST1_PRODUCT_SHIFTER_P0_OFF      (0 << 10) // See "load ps"
#define ST1_PRODUCT_SHIFTER_P0_SHR1     (1 << 10)
#define ST1_PRODUCT_SHIFTER_P0_SHL1     (2 << 10)
#define ST1_PRODUCT_SHIFTER_P0_SHL2     (3 << 10)

#define ST1_A1E_MASK                    (0xF << 12) // a1e

// st2 - Old TL1 Status/Control Register st2
// -----------------------------------------

#define ST2_ENABLE_MODI_Rn(n)           (1 << (n)) // n = 0 to 3 (R0 to R3)
#define ST2_ENABLE_MODJ_Rn(n)           (1 << (n)) // n = 4 to 5 (R4 to R5)

#define ST2_IRQ_INT2_DISABLE            (0 << 6)
#define ST2_IRQ_INT2_ENABLE             (1 << 6) // Enable if IE=1

#define ST2_SHIFT_MODE_ARITHMETIC       (0 << 7)
#define ST2_SHIFT_MODE_LOGIC            (1 << 7)

// OUSER0/OUSER1/IUSER0/IUSER1 unused on DSi?

#define ST2_IRQ_INT2_PENDING            (1 << 13) // Read only
#define ST2_IRQ_INT0_PENDING            (1 << 14) // Read only
#define ST2_IRQ_INT1_PENDING            (1 << 15) // Read only

// icr - Old TL1 Interrupt Context and Repeat Nesting
// --------------------------------------------------

#define ICR_NMI_CTX_SWITCH_ENABLE       (1 << 0)
#define ICR_INT0_CTX_SWITCH_ENABLE      (1 << 1)
#define ICR_INT1_CTX_SWITCH_ENABLE      (1 << 2)
#define ICR_INT2_CTX_SWITCH_ENABLE      (1 << 3)

#define ICR_IN_LOOP                     (1 << 4) // Read only

#define ICR_BLOCK_REPEAT_COUNT_SHIFT    5 // Read only
#define ICR_BLOCK_REPEAT_COUNT_MASK     (0x7 << ICR_BLOCK_REPEAT_COUNT_SHIFT)

// stt0 - New TL2 Status/Control Register stt0 (CPU Flags)
// -------------------------------------------------------

#define STT0_FLAG_LM                    (1 << 0)
#define STT0_FLAG_VL                    (1 << 1)
#define STT0_FLAG_E                     (1 << 2)
#define STT0_FLAG_C                     (1 << 3)
#define STT0_FLAG_V                     (1 << 4)
#define STT0_FLAG_N                     (1 << 5)
#define STT0_FLAG_M                     (1 << 6)
#define STT0_FLAG_Z                     (1 << 7)

#define STT0_FLAG_C1                    (1 << 11)

// stt1 - New TL2 Status/Control Register stt1 (whatever)
// ------------------------------------------------------

#define STT1_FLAG_R                     (1 << 4)

#define STT1_P0E_MASK                   (1 << 14)
#define STT1_P1E_MASK                   (1 << 15)

// stt2 - New TL2 Status/Control Register stt2 (Interrupt/ProgBank/Bkrep)
// ----------------------------------------------------------------------

#define STT2_IRQ_INT0_PENDING           (1 << 0) // Read only
#define STT2_IRQ_INT1_PENDING           (1 << 1) // Read only
#define STT2_IRQ_INT2_PENDING           (1 << 2) // Read only
#define STT2_IRQ_VINT_PENDING           (1 << 3) // Read only

#define STT2_PROG_MEM_BANK_SHIFT        6 // "load movpd"
#define STT2_PROG_MEM_BANK_MASK         (3 << STT2_PROG_MEM_BANK_SHIFT)

#define STT2_BLOCK_REPEAT_COUNT_SHIFT   12 // Read only
#define STT2_BLOCK_REPEAT_COUNT_MASK    (0x7 << STT2_BLOCK_REPEAT_COUNT_SHIFT)

#define STT2_IN_LOOP                    (1 << 15) // Read only

// mod0 - New TL2 Status/Control Register mod0 (Misc)
// --------------------------------------------------

#define MOD0_SATURATION_OFF             (0 << 0)
#define MOD0_SATURATION_ON              (1 << 0) // Saturate "Ax to data"
#define MOD0_SATURATION_MASK            (1 << 0)

#define MOD0_SATURATION_STORE_OFF       (0 << 1)
#define MOD0_SATURATION_STORE_ON        (1 << 1) // "(Ax op data) to Ax"?
#define MOD0_SATURATION_STORE_MASK      (1 << 1)

#define MOD0_HW_MUL_Y0_Y1               (0 << 5)
#define MOD0_HW_MUL_Y0_SHR_8_Y1_SRH_8   (1 << 5)
#define MOD0_HW_MUL_Y0_AND_FF_Y1_AND_FF (2 << 5)
#define MOD0_HW_MUL_Y0_SHR_8_Y1_AND_FF  (3 << 5)
#define MOD0_HW_MUL_MASK                (3 << 5)

#define MOD0_SHIFT_MODE_ARITHMETIC      (0 << 7)
#define MOD0_SHIFT_MODE_LOGIC           (1 << 7)
#define MOD0_SHIFT_MODE_MASK            (1 << 7)

// OUSER0/OUSER1 not used in DSi?

#define MOD0_PRODUCT_SHIFTER_P0_OFF     (0 << 10) // See "load ps"
#define MOD0_PRODUCT_SHIFTER_P0_SHR1    (1 << 10)
#define MOD0_PRODUCT_SHIFTER_P0_SHL1    (2 << 10)
#define MOD0_PRODUCT_SHIFTER_P0_SHL2    (3 << 10)
#define MOD0_PRODUCT_SHIFTER_P0_MASK    (3 << 10)

#define MOD0_PRODUCT_SHIFTER_P1_OFF     (0 << 13) // See "load ps"
#define MOD0_PRODUCT_SHIFTER_P1_SHR1    (1 << 13)
#define MOD0_PRODUCT_SHIFTER_P1_SHL1    (2 << 13)
#define MOD0_PRODUCT_SHIFTER_P1_SHL2    (3 << 13)
#define MOD0_PRODUCT_SHIFTER_P1_MASK    (3 << 13)

// mod1 - New TL2 Status/Control Register mod1 (Data Page)
// -------------------------------------------------------

#define MOD1_DATA_MEMORY_PAGE(p)        ((p) << 0) // 8 bits. See "load page"

#define MOD1_BANKE_OPCODE               (1 << 12)

#define MOD1_MODULO_MODE_TL2            (0 << 13)
#define MOD1_MODULO_MODE_TL1            (1 << 13)

#define MOD1_EPI                        (1 << 14)
#define MOD1_EPJ                        (1 << 15)

// mod2 - New TL2 Status/Control Register mod2 (Modulo Enable)
// -----------------------------------------------------------

#define MOD2_ENABLE_CFGI_MODI_Rn(n)     (1 << (n)) // 0..3 for R0..R3
#define MOD2_ENABLE_CFGJ_MODJ_Rn(n)     (1 << (n)) // 4..7 for R4..R7

#define MOD2_STEP_CFGI_STEPI_Rn(n)      (0 << ((n) + 8)) // 0..3 for R0..R3
#define MOD2_STEP_STEPI0_Rn(n)          (1 << ((n) + 8))

#define MOD2_STEP_CFGJ_STEPJ_Rn(n)      (0 << ((n) + 8)) // 4..7 for R4..R7
#define MOD2_STEP_STEPJ0_Rn(n)          (1 << ((n) + 8))

// mod3 - New TL2 Status/Control Register mod3 (Interrupt Control)
// ---------------------------------------------------------------

#define MOD3_NMI_CTX_SWITCH_ENABLE      (1 << 0)
#define MOD3_INT0_CTX_SWITCH_ENABLE     (1 << 1)
#define MOD3_INT1_CTX_SWITCH_ENABLE     (1 << 2)
#define MOD3_INT2_CTX_SWITCH_ENABLE     (1 << 3)

#define MOD3_IRQ_DISABLE                (0 << 7)
#define MOD3_IRQ_ENABLE                 (1 << 7) // IE

#define MOD3_IRQ_INT0_DISABLE           (0 << 8)
#define MOD3_IRQ_INT0_ENABLE            (1 << 8) // Enable if IE=1

#define MOD3_IRQ_INT1_DISABLE           (0 << 9)
#define MOD3_IRQ_INT1_ENABLE            (1 << 9) // Enable if IE=1

#define MOD3_IRQ_INT2_DISABLE           (0 << 10)
#define MOD3_IRQ_INT2_ENABLE            (1 << 10) // Enable if IE=1

#define MOD3_IRQ_VINT_DISABLE           (0 << 11)
#define MOD3_IRQ_VINT_ENABLE            (1 << 11) // Enable if IE=1

// Normal: push low word then push high word on call; pop high word then pop low
// word on ret.
#define MOD3_STACK_ORDER_NORMAL         (0 << 14)
#define MOD3_STACK_ORDER_REVERSE        (1 << 14)

// DSi Teak CPU Address Config
// ===========================

// ar0
// ---

#define AR0_PM1_POST_MODIFY_STEP_P0     (0 << 0) // +0
#define AR0_PM1_POST_MODIFY_STEP_P1     (1 << 0) // +1
#define AR0_PM1_POST_MODIFY_STEP_M1     (2 << 0) // -1
#define AR0_PM1_POST_MODIFY_STEP_PS     (3 << 0) // +s
#define AR0_PM1_POST_MODIFY_STEP_P2     (4 << 0) // +2
#define AR0_PM1_POST_MODIFY_STEP_M2     (5 << 0) // -2
#define AR0_PM1_POST_MODIFY_STEP_P2_    (6 << 0) // +2
#define AR0_PM1_POST_MODIFY_STEP_M2_    (7 << 0) // -2

#define AR0_CS1_OFFSET_P0               (0 << 3) // +0
#define AR0_CS1_OFFSET_P1               (1 << 3) // +1
#define AR0_CS1_OFFSET_M1               (2 << 3) // -1
#define AR0_CS1_OFFSET_M1_              (3 << 3) // -1

#define AR0_PM0_POST_MODIFY_STEP_P0     (0 << 5) // +0
#define AR0_PM0_POST_MODIFY_STEP_P1     (1 << 5) // +1
#define AR0_PM0_POST_MODIFY_STEP_M1     (2 << 5) // -1
#define AR0_PM0_POST_MODIFY_STEP_PS     (3 << 5) // +s
#define AR0_PM0_POST_MODIFY_STEP_P2     (4 << 5) // +2
#define AR0_PM0_POST_MODIFY_STEP_M2     (5 << 5) // -2
#define AR0_PM0_POST_MODIFY_STEP_P2_    (6 << 5) // +2
#define AR0_PM0_POST_MODIFY_STEP_M2_    (7 << 5) // -2

#define AR0_CS0_OFFSET_P0               (0 << 8) // +0
#define AR0_CS0_OFFSET_P1               (1 << 8) // +1
#define AR0_CS0_OFFSET_M1               (2 << 8) // -1
#define AR0_CS0_OFFSET_M1_              (3 << 8) // -1

#define AR0_RN1_REGISTER_Rn(n)          ((n) << 10) // 0..7 = R0..R7
#define AR0_RN0_REGISTER_Rn(n)          ((n) << 13) // 0..7 = R0..R7

// ar1
// ---

#define AR1_PM3_POST_MODIFY_STEP_P0     (0 << 0) // +0
#define AR1_PM3_POST_MODIFY_STEP_P1     (1 << 0) // +1
#define AR1_PM3_POST_MODIFY_STEP_M1     (2 << 0) // -1
#define AR1_PM3_POST_MODIFY_STEP_PS     (3 << 0) // +s
#define AR1_PM3_POST_MODIFY_STEP_P2     (4 << 0) // +2
#define AR1_PM3_POST_MODIFY_STEP_M2     (5 << 0) // -2
#define AR1_PM3_POST_MODIFY_STEP_P2_    (6 << 0) // +2
#define AR1_PM3_POST_MODIFY_STEP_M2_    (7 << 0) // -2

#define AR1_CS3_OFFSET_P0               (0 << 3) // +0
#define AR1_CS3_OFFSET_P1               (1 << 3) // +1
#define AR1_CS3_OFFSET_M1               (2 << 3) // -1
#define AR1_CS3_OFFSET_M1_              (3 << 3) // -1

#define AR1_PM2_POST_MODIFY_STEP_P0     (0 << 5) // +0
#define AR1_PM2_POST_MODIFY_STEP_P1     (1 << 5) // +1
#define AR1_PM2_POST_MODIFY_STEP_M1     (2 << 5) // -1
#define AR1_PM2_POST_MODIFY_STEP_PS     (3 << 5) // +s
#define AR1_PM2_POST_MODIFY_STEP_P2     (4 << 5) // +2
#define AR1_PM2_POST_MODIFY_STEP_M2     (5 << 5) // -2
#define AR1_PM2_POST_MODIFY_STEP_P2_    (6 << 5) // +2
#define AR1_PM2_POST_MODIFY_STEP_M2_    (7 << 5) // -2

#define AR1_CS2_OFFSET_P0               (0 << 8) // +0
#define AR1_CS2_OFFSET_P1               (1 << 8) // +1
#define AR1_CS2_OFFSET_M1               (2 << 8) // -1
#define AR1_CS2_OFFSET_M1_              (3 << 8) // -1

#define AR1_RN3_REGISTER_Rn(n)          ((n) << 10) // 0..7 = R0..R7
#define AR1_RN2_REGISTER_Rn(n)          ((n) << 13) // 0..7 = R0..R7

// arp0/arp1/arp2/arp3
// -------------------

#define ARPn_PMIn_POST_MODIFY_STEP_P0   (0 << 0) // +0
#define ARPn_PMIn_POST_MODIFY_STEP_P1   (1 << 0) // +1
#define ARPn_PMIn_POST_MODIFY_STEP_M1   (2 << 0) // -1
#define ARPn_PMIn_POST_MODIFY_STEP_PS   (3 << 0) // +s
#define ARPn_PMIn_POST_MODIFY_STEP_P2   (4 << 0) // +2
#define ARPn_PMIn_POST_MODIFY_STEP_M2   (5 << 0) // -2
#define ARPn_PMIn_POST_MODIFY_STEP_P2_  (6 << 0) // +2
#define ARPn_PMIn_POST_MODIFY_STEP_M2_  (7 << 0) // -2

#define ARPn_CIn_OFFSET_P0              (0 << 3) // +0
#define ARPn_CIn_OFFSET_P1              (1 << 3) // +1
#define ARPn_CIn_OFFSET_M1              (2 << 3) // -1
#define ARPn_CIn_OFFSET_M1_             (3 << 3) // -1

#define ARPn_PMJn_POST_MODIFY_STEP_P0   (0 << 5) // +0
#define ARPn_PMJn_POST_MODIFY_STEP_P1   (1 << 5) // +1
#define ARPn_PMJn_POST_MODIFY_STEP_M1   (2 << 5) // -1
#define ARPn_PMJn_POST_MODIFY_STEP_PS   (3 << 5) // +s
#define ARPn_PMJn_POST_MODIFY_STEP_P2   (4 << 5) // +2
#define ARPn_PMJn_POST_MODIFY_STEP_M2   (5 << 5) // -2
#define ARPn_PMJn_POST_MODIFY_STEP_P2_  (6 << 5) // +2
#define ARPn_PMJn_POST_MODIFY_STEP_M2_  (7 << 5) // -2

#define ARPn_CJn_OFFSET_P0              (0 << 8) // +0
#define ARPn_CJn_OFFSET_P1              (1 << 8) // +1
#define ARPn_CJn_OFFSET_M1              (2 << 8) // -1
#define ARPn_CJn_OFFSET_M1_             (3 << 8) // -1

#define ARPn_RIn_REGISTER_Rn(n)         ((n) << 10) // 0..3 = R0..R3
#define ARPn_RJn_REGISTER_Rn(n)         (((n) - 4) << 13) // 4..7 = R4..R7

// DSi Teak CPU Address Step/Modulo
// ================================

// cfgi - Step and Mod I (for R0..R3)
// ----------------------------------

#define CFGI_STEPI(s)                   ((s) & 0x7F)
#define CFGI_MODI(M)                    (((m) & 0x1FF) << 7)

// cfgj - Step and Mod J (for R4..R7)
// ----------------------------------

#define CFGJ_STEPJ(s)                   ((s) & 0x7F)
#define CFGJ_MODJ(M)                    (((m) & 0x1FF) << 7)

#endif // LIBNDS_CPUREGS_H__
