// SPDX-License-Identifier: Zlib
//
// Copyright (C) 2023 Adrian "asie" Siekierka

#ifndef LIBNDS_NDS_CAMERA_H__
#define LIBNDS_NDS_CAMERA_H__

#include <stdbool.h>
#include <stddef.h>

// Camera FIFO defines

#define CAMERA_CMD_INIT             0x000
#define CAMERA_CMD_DEINIT           0x001
#define CAMERA_CMD_SELECT           0x002
#define CAMERA_CMD_SEND_SEQ_CMD     0x003

// Aptina camera - I2C registers

#define I2CREG_APT_CHIP_VERSION                 0x0000
#define I2CREG_APT_PLL_DIVS                     0x0010
#define I2CREG_APT_PLL_P_DIVS                   0x0012
#define I2CREG_APT_PLL_CNT                      0x0014
#define I2CREG_APT_CLOCKS_CNT                   0x0016
#define I2CREG_APT_STANDBY_CNT                  0x0018
#define I2CREG_APT_RESET_MISC_CNT               0x001A
#define I2CREG_APT_MCU_BOOT_MODE                0x001C
#define I2CREG_APT_PAD_SLEW                     0x001E
#define I2CREG_APT_VDD_DIS_COUNTER              0x0022
#define I2CREG_APT_GPI_STATUS                   0x0024
#define I2CREG_APT_EN_VDD_DIS_SOFT              0x0028
#define I2CREG_APT_MCU_ADDRESS                  0x098C
#define I2CREG_APT_MCU_DATA0                    0x0990
#define I2CREG_APT_MCU_DATA(n)                  (0x0990 + ((n) << 1))
#define I2CREG_APT_Y_ADDR_START                 0x3002
#define I2CREG_APT_X_ADDR_START                 0x3004
#define I2CREG_APT_Y_ADDR_END                   0x3006
#define I2CREG_APT_X_ADDR_END                   0x3008
#define I2CREG_APT_FRAME_LENGTH_LINES           0x300A
#define I2CREG_APT_LINE_LENGTH_PCLK             0x300C
#define I2CREG_APT_COARSE_INTEGRATION_TIME      0x3012
#define I2CREG_APT_FINE_INTEGRATION_TIME        0x3014
#define I2CREG_APT_ROW_SPEED                    0x3016
#define I2CREG_APT_PIXEL_ORDER                  0x3024
#define I2CREG_APT_ANALOG_GAIN_CODE_GLOBAL      0x3028
#define I2CREG_APT_ANALOG_GAIN_CODE_GREEN1      0x302A
#define I2CREG_APT_ANALOG_GAIN_CODE_RED         0x302C
#define I2CREG_APT_ANALOG_GAIN_CODE_BLUE        0x302E
#define I2CREG_APT_ANALOG_GAIN_CODE_GREEN2      0x3030
#define I2CREG_APT_DIGITAL_GAIN_GREEN1          0x3032
#define I2CREG_APT_DIGITAL_GAIN_RED             0x3034
#define I2CREG_APT_DIGITAL_GAIN_BLUE            0x3036
#define I2CREG_APT_DIGITAL_GAIN_GREEN2          0x3038
#define I2CREG_APT_READ_MODE                    0x3040
#define I2CREG_APT_GREEN1_GAIN                  0x3056
#define I2CREG_APT_BLUE_GAIN                    0x3058
#define I2CREG_APT_RED_GAIN                     0x305A
#define I2CREG_APT_GREEN2_GAIN                  0x305C
#define I2CREG_APT_COLOR_PIPELINE_CNT           0x3210
#define I2CREG_APT_OFIFO_CNT_STATUS             0x321C
#define I2CREG_APT_OFIFO_CNT_STATUS2            0x321E
#define I2CREG_APT_X_ZOOM_BOUND_START           0x3222
#define I2CREG_APT_X_ZOOM_BOUND_END             0x3224
#define I2CREG_APT_Y_ZOOM_BOUND_START           0x3226
#define I2CREG_APT_Y_ZOOM_BOUND_END             0x3228
#define I2CREG_APT_X_SCALING_WEIGHT             0x322C
#define I2CREG_APT_Y_SCALING_WEIGHT             0x322E
#define I2CREG_APT_APERTURE_PARAMS              0x326C
#define I2CREG_APT_DEMOSAIC_EDGE_THRESHOLD      0x328E
#define I2CREG_APT_TEST_PATTERN                 0x3290
#define I2CREG_APT_COLOR_CORR_MATRIX_SCALE(n)   (0x32C0 + (n))
#define I2CREG_APT_COLOR_CORR_MATRIX(n)         (0x32C4 + (n))
#define I2CREG_APT_DIGITAL_GAIN1_RED            0x32D4
#define I2CREG_APT_DIGITAL_GAIN1_GREEN1         0x32D6
#define I2CREG_APT_DIGITAL_GAIN1_GREEN2         0x32D8
#define I2CREG_APT_DIGITAL_GAIN1_BLUE           0x32DA
#define I2CREG_APT_TEST_OUTPUT_FORMAT           0x3330
#define I2CREG_APT_YUV_YCBCR_CONTROL            0x337C
#define I2CREG_APT_Y_RGB_OFFSET                 0x337E
#define I2CREG_APT_KERNEL_CNT                   0x33F4
#define I2CREG_APT_MIPI_CNT                     0x3400
#define I2CREG_APT_MIPI_STATUS                  0x3402
#define I2CREG_APT_MIPI_CUSTOM_SHORT_PKT        0x3404
#define I2CREG_APT_MIPI_LINE_BYTE_COUNT         0x3408
#define I2CREG_APT_MIPI_CUSTOM_SHORT_PKT_WC     0x340C
#define I2CREG_APT_AE_ZONE_X                    0x3580
#define I2CREG_APT_AE_ZONE_Y                    0x3582
#define I2CREG_APT_AE_WINDOW_SIZE_LO            0x3584
#define I2CREG_APT_AE_WINDOW_SIZE_HI            0x3586
// big endian, contains two byte values
#define IC2REG_APT_R_GAMMA_CURVE_KNEES(n)       (0x3604 + ((n) * 2))
// big endian, contains two byte values
#define IC2REG_APT_G_GAMMA_CURVE_KNEES(n)       (0x3618 + ((n) * 2))
// big endian, contains two byte values
#define IC2REG_APT_B_GAMMA_CURVE_KNEES(n)       (0x362C + ((n) * 2))
#define I2CREG_APT_POLY_ORIGIN_X                0x3642
#define I2CREG_APT_POLY_ORIGIN_Y                0x3644
#define I2CREG_APT_P_COEFF_GREEN1(p, n)         (0x364E + ((n) * 2) + ((p) * 40))
#define I2CREG_APT_P_COEFF_RED(p, n)            (0x364E + ((n) * 2) + ((p) * 40))
#define I2CREG_APT_P_COEFF_BLUE(p, n)           (0x364E + ((n) * 2) + ((p) * 40))
#define I2CREG_APT_P_COEFF_GREEN2(p, n)         (0x364E + ((n) * 2) + ((p) * 40))

// I2CREG_APT_CHIP_VERSION register values
#define I2CREG_APT_CHIP_VERSION_MT9V113         0x2280

// I2CREG_APT_PLL_DIVS register values
#define I2CREG_APT_PLL_M(n)             (n)
#define I2CREG_APT_PLL_N(n)             ((n) << 8)

// I2CREG_APT_PLL_P_DIVS register values
#define I2CREG_APT_PLL_P1(n)            (n)
#define I2CREG_APT_PLL_P3(n)            ((n) << 8)
#define I2CREG_APT_PLL_CLOCK_DIV(n)     ((n) << 12)

// I2CREG_APT_PLL_CNT register values
#define I2CREG_APT_PLL_BYPASS           (1 << 0)
#define I2CREG_APT_PLL_ENABLE           (1 << 1)
#define I2CREG_APT_PLL_RESET_CNTR       (1 << 8)
#define I2CREG_APT_PLL_LOCK             (1 << 15)

// I2CREG_APT_CLOCKS_CNT register values
#define I2CREG_APT_CLKIN_ENABLE         (1 << 9)

// I2CREG_APT_STANDBY_CNT register values
#define I2CREG_APT_STANDBY_ENABLE       (1 << 0)
#define I2CREG_APT_STANDBY_STOP         (1 << 2)
#define I2CREG_APT_STANDBY_IRQ_ENABLE   (1 << 3)
#define I2CREG_APT_STANDBY_STATUS       (1 << 14)

// I2CREG_APT_RESET_MISC_CNT register values
#define I2CREG_APT_I2C_RESET            (1 << 0)
#define I2CREG_APT_MIPI_TX_RESET        (1 << 1)
#define I2CREG_APT_MIPI_TX_ENABLE       (1 << 3)
#define I2CREG_APT_IP_PARALLEL_ENABLE   (1 << 4)
#define I2CREG_APT_IP_SENSOR_FULL_RES   (1 << 6)
#define I2CREG_APT_OE_N_ENABLE          (1 << 8)
#define I2CREG_APT_PARALLEL_ENABLE      (1 << 9)

// I2CREG_APT_MCU_BOOT_MODE register values
#define I2CREG_APT_MCU_RESET            (1 << 0)

// I2CREG_APT_PAD_SLEW register values
#define I2CREG_APT_PARALLEL_OUT_SLEW_RATE(n)    (n)
#define I2CREG_APT_GPIO_SLEW_RATE(n)            ((n) << 4)
#define I2CREG_APT_PCLK_SLEW_RATE(n)            ((n) << 8)

// I2CREG_APT_READ_MODE register values
#define I2CREG_APT_READ_X_MIRROR                (1 << 0)
#define I2CREG_APT_READ_Y_MIRROR                (1 << 1)
#define I2CREG_APT_READ_Y_ODD_INC(n)            ((n) << 2)
#define I2CREG_APT_READ_X_ODD_INC(n)            ((n) << 5)
#define I2CREG_APT_READ_LOW_POWER               (1 << 9)
#define I2CREG_APT_READ_XY_BIN_ENABLE           (1 << 10)
#define I2CREG_APT_READ_X_BIN_ENABLE            (1 << 11)
#define I2CREG_APT_READ_BIN_SUM_ENABLE          (1 << 12)
#define I2CREG_APT_READ_Y_SUM_ENABLE            (1 << 13)

// I2CREG_APT_..._GAIN register values
#define I2CREG_APT_GAIN_INITIAL(n)              (n)
#define I2CREG_APT_GAIN_ANALOG(n)               ((n) << 7)
#define I2CREG_APT_GAIN_DIGITAL(n)              ((n) << 9)

// I2CREG_APT_COLOR_PIPELINE_CNT register values
#define I2CREG_APT_PGA_PIXEL_SHADING_CORRECT_ENABLE     (1 << 3)
#define I2CREG_APT_APERTURE_CORRECT_ENABLE              (1 << 4)
#define I2CREG_APT_COLOR_CORRECT_ENABLE                 (1 << 5)
#define I2CREG_APT_GAMMA_CORRECT_ENABLE                 (1 << 7)
#define I2CREG_APT_SCALING_ENABLE                       (1 << 8)

// I2CREG_APT_APERTURE_PARAMS register values
#define I2CREG_APT_APERTURE_THRESHOLD(n)        (n)
#define I2CREG_APT_APERTURE_GAIN(n)             ((n) << 8)
#define I2CREG_APT_APERTURE_GAIN_EXP(n)         ((n) << 11)
#define I2CREG_APT_APERTURE_GAIN_ABSOLUTE       (1 << 14)

// I2CREG_APT_TEST_PATTERN register values
#define I2CREG_APT_TEST_PATTERN_ENABLE          (1 << 5)
#define I2CREG_APT_TEST_PATTERN_8_BIT           (0)
#define I2CREG_APT_TEST_PATTERN_10_BIT          (1 << 6)

// I2CREG_APT_YUV_YCBCR_CONTROL register values
#define I2CREG_APT_YUV_NORMALIZE                (1 << 0)
#define I2CREG_APT_YUV_COEFF_CONTROL            (1 << 1)
#define I2CREG_APT_YUV_UV_UNSIGNED              (1 << 2)
#define I2CREG_APT_YUV_CLIP                     (1 << 3)

// I2CREG_APT_Y_RGB_OFFSET register values
#define I2CREG_APT_Y_OFFSET(n)                  ((n) << 8)

// I2CREG_APT_KERNEL_CNT register values
#define I2CREG_APT_KERNEL_DC_ENABLE             (1 << 0)
#define I2CREG_APT_KERNEL_NR_ENABLE             (1 << 3)

// I2CREG_APT_MIPI_CNT register values
#define I2CREG_APT_MIPI_RESET_ENABLE            (1 << 0)
#define I2CREG_APT_MIPI_STANDBY_ENABLE          (1 << 1)
#define I2CREG_APT_MIPI_CONTINUOUS_CLOCK        (1 << 2)
#define I2CREG_APT_MIPI_FRAME_BOUNDARY_SYNC     (1 << 3)
#define I2CREG_APT_MIPI_STANDBY_WAIT_EOF        (1 << 4)
#define I2CREG_APT_MIPI_CHANNEL(n)              ((n) << 6)
#define I2CREG_APT_MIPI_DATA_TYPE_YUV422_8_BIT  (0x1E << 10)
#define I2CREG_APT_MIPI_DATA_TYPE_RGB444        (0x20 << 10)
#define I2CREG_APT_MIPI_DATA_TYPE_RGB555        (0x21 << 10)
#define I2CREG_APT_MIPI_DATA_TYPE_RGB565        (0x22 << 10)
#define I2CREG_APT_MIPI_DATA_TYPE_RAW_8_BIT     (0x2A << 10)
#define I2CREG_APT_MIPI_DATA_TYPE_RAW_10_BIT    (0x2B << 10)
#define I2CREG_APT_MIPI_DATA_TYPE_MASK          (0x3F << 10)

// I2CREG_APT_MIPI_STATUS register values
#define I2CREG_APT_MIPI_STANDBY                 (1 << 0)
#define I2CREG_APT_MIPI_IDLE                    (1 << 4)
#define I2CREG_APT_MIPI_READY_RX                (1 << 5)

// I2CREG_APT_AE_ZONE register values
#define I2CREG_APT_AE_ZONE_START(n)             (n)
#define I2CREG_APT_AE_ZONE_WIDTH(n)             ((n) << 8)

#define MCUREG_APT_16BIT                        0x0000
#define MCUREG_APT_8BIT                         0x8000
// Aptina camera - MCU registers
#define MCUREG_APT_SFR_WATCHDOG                     (MCUREG_APT_16BIT | 0x1040)
#define MCUREG_APT_SFR_HIGH_PREC_TIMER_LO           (MCUREG_APT_16BIT | 0x1048)
#define MCUREG_APT_SFR_HIGH_PREC_TIMER_HI           (MCUREG_APT_16BIT | 0x104A)
#define MCUREG_APT_SFR_VIRT_PAGETABLE_ADDR          (MCUREG_APT_16BIT | 0x1050)
#define MCUREG_APT_SFR_GPIO_DATA                    (MCUREG_APT_16BIT | 0x1070)
#define MCUREG_APT_SFR_GPIO_OUTPUT_SET              (MCUREG_APT_16BIT | 0x1074)
#define MCUREG_APT_SFR_GPIO_OUTPUT_CLEAR            (MCUREG_APT_16BIT | 0x1076)
#define MCUREG_APT_SFR_GPIO_DIR                     (MCUREG_APT_16BIT | 0x1078)
#define MCUREG_APT_SEQ_MODE                         (MCUREG_APT_8BIT  | 0x2102)
#define MCUREG_APT_SEQ_CMD                          (MCUREG_APT_8BIT  | 0x2103)
#define MCUREG_APT_SEQ_STATE                        (MCUREG_APT_8BIT  | 0x2104)
#define MCUREG_APT_SEQ_FLASH_TYPE                   (MCUREG_APT_8BIT  | 0x2105)
#define MCUREG_APT_SEQ_AE_FAST_BUFF                 (MCUREG_APT_8BIT  | 0x2109)
#define MCUREG_APT_SEQ_AE_FAST_STEP                 (MCUREG_APT_8BIT  | 0x210A)
#define MCUREG_APT_SEQ_AWB_CONT_BUFF                (MCUREG_APT_8BIT  | 0x210B)
#define MCUREG_APT_SEQ_AWB_CONT_STEP                (MCUREG_APT_8BIT  | 0x210C)
#define MCUREG_APT_SEQ_CNT                          (MCUREG_APT_8BIT  | 0x2111)
#define MCUREG_APT_SEQ_FLASH_TH                     (MCUREG_APT_16BIT | 0x2113)
#define MCUREG_APT_SEQ_CAP_MODE                     (MCUREG_APT_8BIT  | 0x2115)
#define MCUREG_APT_SEQ_CAP_FRAMES                   (MCUREG_APT_8BIT  | 0x2116)
#define MCUREG_APT_SEQ_PREVIEW0_AE                  (MCUREG_APT_8BIT  | 0x2117)
#define MCUREG_APT_SEQ_PREVIEW0_FD                  (MCUREG_APT_8BIT  | 0x2118)
#define MCUREG_APT_SEQ_PREVIEW0_AWB                 (MCUREG_APT_8BIT  | 0x2119)
#define MCUREG_APT_SEQ_PREVIEW0_HG                  (MCUREG_APT_8BIT  | 0x211A)
#define MCUREG_APT_SEQ_PREVIEW0_FLASH               (MCUREG_APT_8BIT  | 0x211B)
#define MCUREG_APT_SEQ_PREVIEW0_SKIPFRAME           (MCUREG_APT_8BIT  | 0x211C)
#define MCUREG_APT_SEQ_PREVIEW1_AE                  (MCUREG_APT_8BIT  | 0x211D)
#define MCUREG_APT_SEQ_PREVIEW1_FD                  (MCUREG_APT_8BIT  | 0x211E)
#define MCUREG_APT_SEQ_PREVIEW1_AWB                 (MCUREG_APT_8BIT  | 0x211F)
#define MCUREG_APT_SEQ_PREVIEW1_HG                  (MCUREG_APT_8BIT  | 0x2120)
#define MCUREG_APT_SEQ_PREVIEW1_FLASH               (MCUREG_APT_8BIT  | 0x2121)
#define MCUREG_APT_SEQ_PREVIEW1_SKIPFRAME           (MCUREG_APT_8BIT  | 0x2122)
#define MCUREG_APT_SEQ_PREVIEW2_AE                  (MCUREG_APT_8BIT  | 0x2123)
#define MCUREG_APT_SEQ_PREVIEW2_FD                  (MCUREG_APT_8BIT  | 0x2124)
#define MCUREG_APT_SEQ_PREVIEW2_AWB                 (MCUREG_APT_8BIT  | 0x2125)
#define MCUREG_APT_SEQ_PREVIEW2_HG                  (MCUREG_APT_8BIT  | 0x2126)
#define MCUREG_APT_SEQ_PREVIEW2_FLASH               (MCUREG_APT_8BIT  | 0x2127)
#define MCUREG_APT_SEQ_PREVIEW2_SKIPFRAME           (MCUREG_APT_8BIT  | 0x2128)
#define MCUREG_APT_SEQ_PREVIEW3_AE                  (MCUREG_APT_8BIT  | 0x2129)
#define MCUREG_APT_SEQ_PREVIEW3_FD                  (MCUREG_APT_8BIT  | 0x212A)
#define MCUREG_APT_SEQ_PREVIEW3_AWB                 (MCUREG_APT_8BIT  | 0x212B)
#define MCUREG_APT_SEQ_PREVIEW3_HG                  (MCUREG_APT_8BIT  | 0x212C)
#define MCUREG_APT_SEQ_PREVIEW3_FLASH               (MCUREG_APT_8BIT  | 0x212D)
#define MCUREG_APT_SEQ_PREVIEW3_SKIPFRAME           (MCUREG_APT_8BIT  | 0x212E)
#define MCUREG_APT_AE_WINDOW_POS                    (MCUREG_APT_8BIT  | 0x2202)
#define MCUREG_APT_AE_WINDOW_SIZE                   (MCUREG_APT_8BIT  | 0x2203)
#define MCUREG_APT_AE_TARGET                        (MCUREG_APT_8BIT  | 0x2206)
#define MCUREG_APT_AE_GATE                          (MCUREG_APT_8BIT  | 0x2207)
#define MCUREG_APT_AE_MIN_INDEX                     (MCUREG_APT_8BIT  | 0x220B)
#define MCUREG_APT_AE_MAX_INDEX                     (MCUREG_APT_8BIT  | 0x220C)
#define MCUREG_APT_AE_MIN_VIRTUAL_GAIN              (MCUREG_APT_8BIT  | 0x220D)
#define MCUREG_APT_AE_MAX_VIRTUAL_GAIN              (MCUREG_APT_8BIT  | 0x220E)
#define MCUREG_APT_AE_MAX_DIGITAL_GAIN_AE1          (MCUREG_APT_16BIT | 0x2212)
#define MCUREG_APT_AE_STATUS                        (MCUREG_APT_8BIT  | 0x2217)
#define MCUREG_APT_AE_LAST_LUMA                     (MCUREG_APT_8BIT  | 0x2218)
#define MCUREG_APT_AE_CURRENT_SHUTTER_DELAY         (MCUREG_APT_16BIT | 0x2219)
#define MCUREG_APT_AE_CURRENT_ZONE_INTEGRATION_TIME (MCUREG_APT_8BIT  | 0x221B)
#define MCUREG_APT_AE_CURRENT_VIRTUAL_GAIN          (MCUREG_APT_8BIT  | 0x221C)
#define MCUREG_APT_AE_CURRENT_DIGITAL_GAIN_AE1      (MCUREG_APT_16BIT | 0x221F)
#define MCUREG_APT_AE_R9                            (MCUREG_APT_16BIT | 0x2222)
#define MCUREG_APT_AE_R9_STEP                       (MCUREG_APT_16BIT | 0x222D)
#define MCUREG_APT_AE_MIN_TARGET                    (MCUREG_APT_8BIT  | 0x224A)
#define MCUREG_APT_AE_MAX_TARGET                    (MCUREG_APT_8BIT  | 0x224B)
#define MCUREG_APT_AE_TARGET_BUFFER_SPEED           (MCUREG_APT_8BIT  | 0x224C)
#define MCUREG_APT_AE_TARGET_BASE                   (MCUREG_APT_8BIT  | 0x224F)
#define MCUREG_APT_AWB_WINDOW_POS                   (MCUREG_APT_8BIT  | 0x2302)
#define MCUREG_APT_AWB_WINDOW_SIZE                  (MCUREG_APT_8BIT  | 0x2303)
#define MCUREG_APT_AWB_CCM_L(n)                     (MCUREG_APT_16BIT | (0x2306 + ((n) * 2)))
#define MCUREG_APT_AWB_CCM_RL(n)                    (MCUREG_APT_16BIT | (0x231C + ((n) * 2)))
#define MCUREG_APT_AWB_CCM(n)                       (MCUREG_APT_16BIT | (0x2332 + ((n) * 2)))
#define MCUREG_APT_AWB_GAIN_BUFFER_SPEED            (MCUREG_APT_8BIT  | 0x2348)
#define MCUREG_APT_AWB_JUMP_DIVISOR                 (MCUREG_APT_8BIT  | 0x2348)
#define MCUREG_APT_AWB_GAIN_MIN_R                   (MCUREG_APT_8BIT  | 0x2348)
#define MCUREG_APT_AWB_GAIN_MAX_R                   (MCUREG_APT_8BIT  | 0x2348)
#define MCUREG_APT_AWB_GAIN_MIN_B                   (MCUREG_APT_8BIT  | 0x2348)
#define MCUREG_APT_AWB_GAIN_MAX_B                   (MCUREG_APT_8BIT  | 0x2348)
#define MCUREG_APT_AWB_GAIN_R                       (MCUREG_APT_8BIT  | 0x2348)
#define MCUREG_APT_AWB_GAIN_G                       (MCUREG_APT_8BIT  | 0x2348)
#define MCUREG_APT_AWB_GAIN_B                       (MCUREG_APT_8BIT  | 0x2348)
#define MCUREG_APT_AWB_CCM_POS_MIN                  (MCUREG_APT_8BIT  | 0x2351)
#define MCUREG_APT_AWB_CCM_POS_MAX                  (MCUREG_APT_8BIT  | 0x2352)
#define MCUREG_APT_AWB_CCM_POS                      (MCUREG_APT_8BIT  | 0x2353)
#define MCUREG_APT_AWB_SATURATION                   (MCUREG_APT_8BIT  | 0x2354)
#define MCUREG_APT_AWB_CNT                          (MCUREG_APT_8BIT  | 0x2355)
#define MCUREG_APT_AWB_GAIN_R_BUF                   (MCUREG_APT_16BIT | 0x2356)
#define MCUREG_APT_AWB_GAIN_B_BUF                   (MCUREG_APT_16BIT | 0x2358)
#define MCUREG_APT_AWB_STEADY_B_GAIN_OUT_MIN        (MCUREG_APT_8BIT  | 0x235D)
#define MCUREG_APT_AWB_STEADY_B_GAIN_OUT_MAX        (MCUREG_APT_8BIT  | 0x235E)
#define MCUREG_APT_AWB_STEADY_B_GAIN_IN_MIN         (MCUREG_APT_8BIT  | 0x235F)
#define MCUREG_APT_AWB_STEADY_B_GAIN_IN_MAX         (MCUREG_APT_8BIT  | 0x2360)
#define MCUREG_APT_AWB_TRUE_GRAY_MIN_0              (MCUREG_APT_8BIT  | 0x2363)
#define MCUREG_APT_AWB_TRUE_GRAY_MAX_0              (MCUREG_APT_8BIT  | 0x2364)
#define MCUREG_APT_AWB_X0                           (MCUREG_APT_8BIT  | 0x2365)
#define MCUREG_APT_AWB_KR_L                         (MCUREG_APT_8BIT  | 0x2366)
#define MCUREG_APT_AWB_KG_L                         (MCUREG_APT_8BIT  | 0x2367)
#define MCUREG_APT_AWB_KB_L                         (MCUREG_APT_8BIT  | 0x2368)
#define MCUREG_APT_AWB_KR_R                         (MCUREG_APT_8BIT  | 0x2369)
#define MCUREG_APT_AWB_KG_R                         (MCUREG_APT_8BIT  | 0x236A)
#define MCUREG_APT_AWB_KB_R                         (MCUREG_APT_8BIT  | 0x236B)
#define MCUREG_APT_FD_WINDOW_X                      (MCUREG_APT_8BIT  | 0x2402)
#define MCUREG_APT_FD_WINDOW_HEIGHT                 (MCUREG_APT_8BIT  | 0x2403)
#define MCUREG_APT_FD_MODE                          (MCUREG_APT_8BIT  | 0x2404)
#define MCUREG_APT_FD_SEARCH_F1_50HZ                (MCUREG_APT_8BIT  | 0x2408)
#define MCUREG_APT_FD_SEARCH_F2_50HZ                (MCUREG_APT_8BIT  | 0x2409)
#define MCUREG_APT_FD_SEARCH_F1_60HZ                (MCUREG_APT_8BIT  | 0x240A)
#define MCUREG_APT_FD_SEARCH_F2_60HZ                (MCUREG_APT_8BIT  | 0x240B)
#define MCUREG_APT_FD_STAT_MIN                      (MCUREG_APT_8BIT  | 0x240D)
#define MCUREG_APT_FD_STAT_MAX                      (MCUREG_APT_8BIT  | 0x240E)
#define MCUREG_APT_FD_MIN_AMPLITUDE                 (MCUREG_APT_8BIT  | 0x2410)
#define MCUREG_APT_FD_R9_STEP_F60_A                 (MCUREG_APT_16BIT | 0x2411)
#define MCUREG_APT_FD_R9_STEP_F50_A                 (MCUREG_APT_16BIT | 0x2413)
#define MCUREG_APT_FD_R9_STEP_F60_B                 (MCUREG_APT_16BIT | 0x2415)
#define MCUREG_APT_FD_R9_STEP_F50_B                 (MCUREG_APT_16BIT | 0x2417)
#define MCUREG_APT_MODE_A_OUTPUT_WIDTH              (MCUREG_APT_16BIT | 0x2703)
#define MCUREG_APT_MODE_A_OUTPUT_HEIGHT             (MCUREG_APT_16BIT | 0x2705)
#define MCUREG_APT_MODE_B_OUTPUT_WIDTH              (MCUREG_APT_16BIT | 0x2707)
#define MCUREG_APT_MODE_B_OUTPUT_HEIGHT             (MCUREG_APT_16BIT | 0x2709)
#define MCUREG_APT_MODE_A_MIPI_VC                   (MCUREG_APT_8BIT  | 0x270B)
#define MCUREG_APT_MODE_B_MIPI_VC                   (MCUREG_APT_8BIT  | 0x270C)
#define MCUREG_APT_MODE_A_SENSOR_ROW_START          (MCUREG_APT_16BIT | 0x270D)
#define MCUREG_APT_MODE_A_SENSOR_COL_START          (MCUREG_APT_16BIT | 0x270F)
#define MCUREG_APT_MODE_A_SENSOR_ROW_END            (MCUREG_APT_16BIT | 0x2711)
#define MCUREG_APT_MODE_A_SENSOR_COL_END            (MCUREG_APT_16BIT | 0x2713)
#define MCUREG_APT_MODE_A_SENSOR_ROW_SPEED          (MCUREG_APT_16BIT | 0x2715)
#define MCUREG_APT_MODE_A_SENSOR_READ_MODE          (MCUREG_APT_16BIT | 0x2717)
#define MCUREG_APT_MODE_A_SENSOR_FINE_CORRECTION    (MCUREG_APT_16BIT | 0x2719)
#define MCUREG_APT_MODE_A_SENSOR_FINE_IT_MIN        (MCUREG_APT_16BIT | 0x271B)
#define MCUREG_APT_MODE_A_SENSOR_FINE_IT_MAX_MARGIN (MCUREG_APT_16BIT | 0x271D)
#define MCUREG_APT_MODE_A_SENSOR_FRAME_LENGTH       (MCUREG_APT_16BIT | 0x271F)
#define MCUREG_APT_MODE_A_SENSOR_LINE_LENGTH_PCK    (MCUREG_APT_16BIT | 0x2721)
#define MCUREG_APT_MODE_B_SENSOR_ROW_START          (MCUREG_APT_16BIT | 0x2723)
#define MCUREG_APT_MODE_B_SENSOR_COL_START          (MCUREG_APT_16BIT | 0x2725)
#define MCUREG_APT_MODE_B_SENSOR_ROW_END            (MCUREG_APT_16BIT | 0x2727)
#define MCUREG_APT_MODE_B_SENSOR_COL_END            (MCUREG_APT_16BIT | 0x2729)
#define MCUREG_APT_MODE_B_SENSOR_ROW_SPEED          (MCUREG_APT_16BIT | 0x272B)
#define MCUREG_APT_MODE_B_SENSOR_READ_MODE          (MCUREG_APT_16BIT | 0x272D)
#define MCUREG_APT_MODE_B_SENSOR_FINE_CORRECTION    (MCUREG_APT_16BIT | 0x272F)
#define MCUREG_APT_MODE_B_SENSOR_FINE_IT_MIN        (MCUREG_APT_16BIT | 0x2731)
#define MCUREG_APT_MODE_B_SENSOR_FINE_IT_MAX_MARGIN (MCUREG_APT_16BIT | 0x2733)
#define MCUREG_APT_MODE_B_SENSOR_FRAME_LENGTH       (MCUREG_APT_16BIT | 0x2735)
#define MCUREG_APT_MODE_B_SENSOR_LINE_LENGTH_PCK    (MCUREG_APT_16BIT | 0x2737)
#define MCUREG_APT_MODE_A_CROP_X_START              (MCUREG_APT_16BIT | 0x2739)
#define MCUREG_APT_MODE_A_CROP_X_END                (MCUREG_APT_16BIT | 0x273B)
#define MCUREG_APT_MODE_A_CROP_Y_START              (MCUREG_APT_16BIT | 0x273D)
#define MCUREG_APT_MODE_A_CROP_Y_END                (MCUREG_APT_16BIT | 0x273F)
#define MCUREG_APT_MODE_B_CROP_X_START              (MCUREG_APT_16BIT | 0x2747)
#define MCUREG_APT_MODE_B_CROP_X_END                (MCUREG_APT_16BIT | 0x2749)
#define MCUREG_APT_MODE_B_CROP_Y_START              (MCUREG_APT_16BIT | 0x274B)
#define MCUREG_APT_MODE_B_CROP_Y_END                (MCUREG_APT_16BIT | 0x274D)
#define MCUREG_APT_MODE_A_OUTPUT_FORMAT             (MCUREG_APT_16BIT | 0x2755)
#define MCUREG_APT_MODE_B_OUTPUT_FORMAT             (MCUREG_APT_16BIT | 0x2757)
#define MCUREG_APT_MODE_A_SPECIAL_EFFECTS           (MCUREG_APT_16BIT | 0x2759)
#define MCUREG_APT_MODE_B_SPECIAL_EFFECTS           (MCUREG_APT_16BIT | 0x275B)
#define MCUREG_APT_MODE_A_Y_RGB_OFFSET              (MCUREG_APT_8BIT  | 0x275D)
#define MCUREG_APT_MODE_B_Y_RGB_OFFSET              (MCUREG_APT_8BIT  | 0x275E)
#define MCUREG_APT_MODE_BOTH_BRIGHT_COLOR_KILL      (MCUREG_APT_16BIT | 0x275F)
#define MCUREG_APT_MODE_BOTH_DARK_COLOR_KILL        (MCUREG_APT_16BIT | 0x2761)
#define MCUREG_APT_MODE_BOTH_SPECIAL_EFFECT_SEPIA   (MCUREG_APT_16BIT | 0x2763)
#define MCUREG_APT_MODE_BOTH_FILTER_MODE            (MCUREG_APT_8BIT  | 0x2765)
#define MCUREG_APT_MODE_BOTH_TEST_MODE              (MCUREG_APT_8BIT  | 0x2766)
#define MCUREG_APT_HG_MAX_DARK_LEVEL                (MCUREG_APT_8BIT  | 0x2B04)
#define MCUREG_APT_HG_PERCENT                       (MCUREG_APT_8BIT  | 0x2B06)
#define MCUREG_APT_HG_DARK_LEVEL                    (MCUREG_APT_8BIT  | 0x2B08)
#define MCUREG_APT_HG_AVG_LUMA                      (MCUREG_APT_8BIT  | 0x2B17)
#define MCUREG_APT_HG_BRIGHTNESS_METRIC             (MCUREG_APT_16BIT | 0x2B1B)
#define MCUREG_APT_HG_LLMODE                        (MCUREG_APT_8BIT  | 0x2B1F)
#define MCUREG_APT_HG_LL_SAT1                       (MCUREG_APT_8BIT  | 0x2B20)
#define MCUREG_APT_HG_LL_AP_CORR1                   (MCUREG_APT_8BIT  | 0x2B22)
#define MCUREG_APT_HG_LL_SAT2                       (MCUREG_APT_8BIT  | 0x2B24)
#define MCUREG_APT_HG_LL_INTERP_THRESHOLD2          (MCUREG_APT_8BIT  | 0x2B25)
#define MCUREG_APT_HG_LL_AP_CORR2                   (MCUREG_APT_8BIT  | 0x2B26)
#define MCUREG_APT_HG_LL_AP_THRESHOLD2              (MCUREG_APT_8BIT  | 0x2B27)
#define MCUREG_APT_HG_LL_BRIGHTNESS_START           (MCUREG_APT_16BIT | 0x2B28)
#define MCUREG_APT_HG_LL_BRIGHTNESS_STOP            (MCUREG_APT_16BIT | 0x2B2A)
#define MCUREG_APT_HG_NR_START_R                    (MCUREG_APT_8BIT  | 0x2B2C)
#define MCUREG_APT_HG_NR_START_G                    (MCUREG_APT_8BIT  | 0x2B2D)
#define MCUREG_APT_HG_NR_START_B                    (MCUREG_APT_8BIT  | 0x2B2E)
#define MCUREG_APT_HG_NR_START_OL                   (MCUREG_APT_8BIT  | 0x2B2F)
#define MCUREG_APT_HG_NR_STOP_R                     (MCUREG_APT_8BIT  | 0x2B30)
#define MCUREG_APT_HG_NR_STOP_G                     (MCUREG_APT_8BIT  | 0x2B31)
#define MCUREG_APT_HG_NR_STOP_B                     (MCUREG_APT_8BIT  | 0x2B32)
#define MCUREG_APT_HG_NR_STOP_OL                    (MCUREG_APT_8BIT  | 0x2B33)
#define MCUREG_APT_HG_NR_GAIN_START                 (MCUREG_APT_8BIT  | 0x2B34)
#define MCUREG_APT_HG_NR_GAIN_STOP                  (MCUREG_APT_8BIT  | 0x2B35)
#define MCUREG_APT_HG_CLUSTERDC_THRESHOLD           (MCUREG_APT_8BIT  | 0x2B36)
#define MCUREG_APT_HG_GAMMA_MORPH_CNT               (MCUREG_APT_8BIT  | 0x2B37)
#define MCUREG_APT_HG_GAMMA_MORPH_START             (MCUREG_APT_16BIT | 0x2B38)
#define MCUREG_APT_HG_GAMMA_MORPH_STOP              (MCUREG_APT_16BIT | 0x2B3A)
#define MCUREG_APT_HG_GAMMA_TABLE_A(n)              ((MCUREG_APT_8BIT | 0x2B3C) + (n))
#define MCUREG_APT_HG_GAMMA_TABLE_B(n)              ((MCUREG_APT_8BIT | 0x2B4F) + (n))
#define MCUREG_APT_HG_FTB_START_BM                  (MCUREG_APT_16BIT | 0x2B62)
#define MCUREG_APT_HG_FTB_STOP_BM                   (MCUREG_APT_16BIT | 0x2B64)
#define MCUREG_APT_HG_CLUSTER_DC_BM                 (MCUREG_APT_16BIT | 0x2B66)

// MCUREG_APT_SFR_GPIO register values
#define MCUREG_APT_SFR_GPIO(n)              (1 << (9 + (n)))

// MCUREG_APT_SEQ_MODE register values
#define MCUREG_APT_SEQ_MODE_AE_ENABLE       (1 << 0)
#define MCUREG_APT_SEQ_MODE_FD_ENABLE       (1 << 1)
#define MCUREG_APT_SEQ_MODE_AWB_ENABLE      (1 << 2)
#define MCUREG_APT_SEQ_MODE_HG_ENABLE       (1 << 3)

// MCUREG_APT_SEQ_CMD register values
#define MCUREG_APT_SEQ_CMD_RUN              (0)
#define MCUREG_APT_SEQ_CMD_PREVIEW          (1)
#define MCUREG_APT_SEQ_CMD_CAPTURE          (2)
#define MCUREG_APT_SEQ_CMD_STANDBY          (3)
#define MCUREG_APT_SEQ_CMD_LOCK             (4)
#define MCUREG_APT_SEQ_CMD_REFRESH          (5)
#define MCUREG_APT_SEQ_CMD_REFRESH_MODE     (6)
#define MCUREG_APT_SEQ_CMD_MASK             (0xFF)

// MCUREG_APT_SEQ_CAP_MODE register values
#define MCUREG_APT_SEQ_CAP_MODE_XENON_FLASH         (1 << 0)
#define MCUREG_APT_SEQ_CAP_MODE_VIDEO_ENABLE        (1 << 1)
#define MCUREG_APT_SEQ_CAP_MODE_FLASH_OFF           (1 << 2)
#define MCUREG_APT_SEQ_CAP_MODE_VIDEO_AE_ENABLE     (1 << 3)
#define MCUREG_APT_SEQ_CAP_MODE_VIDEO_AWB_ENABLE    (1 << 4)
#define MCUREG_APT_SEQ_CAP_MODE_VIDEO_HG_ENABLE     (1 << 5)

// MCUREG_APT_AE_WINDOW_POS register values
#define MCUREG_APT_AE_WINDOW_X0(n)          (n)
#define MCUREG_APT_AE_WINDOW_Y0(n)          ((n) << 4)

// MCUREG_APT_AE_WINDOW_SIZE register values
#define MCUREG_APT_AE_WINDOW_WIDTH(n)       (n)
#define MCUREG_APT_AE_WINDOW_HEIGHT(n)      ((n) << 4)

// MCUREG_APT_AE_STATUS register values
#define MCUREG_APT_AE_STATUS_AT_LIMIT       (1 << 0)
#define MCUREG_APT_AE_STATUS_R9_CHANGED     (1 << 1)
#define MCUREG_APT_AE_STATUS_READY          (1 << 2)

// MCUREG_APT_AWB_CNT register values
#define MCUREG_APT_AWB_STEADY               (1 << 0)
#define MCUREG_APT_AWB_LIMIT_REACHED        (1 << 1)
#define MCUREG_APT_AWB_FORCE_UNIT_DGAINS    (1 << 5)
#define MCUREG_APT_AWB_NORMALIZE_CCM_OFF    (1 << 6)

// MCUREG_APT_MODE_x_SENSOR_READ_MODE register values
#define MCUREG_APT_READ_X_MIRROR            (1 << 0)
#define MCUREG_APT_READ_Y_MIRROR            (1 << 1)
#define MCUREG_APT_READ_Y_ODD_INC(n)        ((n) << 2)
#define MCUREG_APT_READ_X_ODD_INC(n)        ((n) << 5)
#define MCUREG_APT_READ_LOW_POWER           (1 << 9)
#define MCUREG_APT_READ_XY_BIN_ENABLE       (1 << 10)
#define MCUREG_APT_READ_X_BIN_ENABLE        (1 << 11)

// MCUREG_APT_MODE_x_OUTPUT_FORMAT register values
#define MCUREG_APT_MODE_OUTPUT_FORMAT_SWAP_CHANNELS     (1 << 0)
#define MCUREG_APT_MODE_OUTPUT_FORMAT_SWAP_LUMA_CHROMA  (1 << 1)
#define MCUREG_APT_MODE_OUTPUT_FORMAT_PROGRESSIVE_BAYER (1 << 2)
#define MCUREG_APT_MODE_OUTPUT_FORMAT_MONOCHROME        (1 << 3)
#define MCUREG_APT_MODE_OUTPUT_FORMAT_YUV               (0)
#define MCUREG_APT_MODE_OUTPUT_FORMAT_RGB565            (1 << 5)
#define MCUREG_APT_MODE_OUTPUT_FORMAT_RGB555            ((1 << 5) | (1 << 6))
#define MCUREG_APT_MODE_OUTPUT_FORMAT_RGBX4444          ((1 << 5) | (2 << 6))
#define MCUREG_APT_MODE_OUTPUT_FORMAT_XRGB4444          ((1 << 5) | (3 << 6))
#define MCUREG_APT_MODE_OUTPUT_FORMAT_MASK              (7 << 5)
#define MCUREG_APT_MODE_OUTPUT_FORMAT_PROCESSED_BAYER   (1 << 8)

// MCUREG_APT_MODE_x_SPECIAL_EFFECTS register values
#define MCUREG_APT_MODE_SPECIAL_EFFECT_NONE                 (0)
#define MCUREG_APT_MODE_SPECIAL_EFFECT_MONO                 (1)
#define MCUREG_APT_MODE_SPECIAL_EFFECT_SEPIA                (2)
#define MCUREG_APT_MODE_SPECIAL_EFFECT_NEGATIVE             (3)
#define MCUREG_APT_MODE_SPECIAL_EFFECT_SOLARIZATION         (4)
#define MCUREG_APT_MODE_SPECIAL_EFFECT_SOLARIZATION_UV      (5)
#define MCUREG_APT_MODE_SPECIAL_EFFECT_MASK                 (7)
#define MCUREG_APT_MODE_SPECIAL_EFFECT_DITHER_BITWIDTH(n)   ((n) << 3)
#define MCUREG_APT_MODE_SPECIAL_EFFECT_DITHER_LUMA          (1 << 6)

// MCUREG_APT_MODE_x_BRIGHT_COLOR_KILL register values
#define MCUREG_APT_MODE_BRIGHT_COLOR_KILL_SATURATION_POINT(n)   (n)
#define MCUREG_APT_MODE_BRIGHT_COLOR_KILL_GAIN(n)               ((n) << 3)
#define MCUREG_APT_MODE_BRIGHT_COLOR_KILL_THRESHOLD(n)          ((n) << 6)
#define MCUREG_APT_MODE_BRIGHT_COLOR_KILL_USE_LUMA              (1 << 9)
#define MCUREG_APT_MODE_BRIGHT_COLOR_KILL_ENABLE                (1 << 10)

// MCUREG_APT_MODE_x_DARK_COLOR_KILL register values
#define MCUREG_APT_MODE_DARK_COLOR_KILL_GAIN(n)         (n)
#define MCUREG_APT_MODE_DARK_COLOR_KILL_THRESHOLD(n)    ((n) << 3)
#define MCUREG_APT_MODE_DARK_COLOR_KILL_USE_LUMA        (1 << 6)
#define MCUREG_APT_MODE_DARK_COLOR_KILL_ENABLE          (1 << 7)

// MCUREG_APT_MODE_x_SPECIAL_EFFECT_SEPIA register values
#define MCUREG_APT_MODE_SPECIAL_EFFECT_SEPIA_CR(x)      (x)
#define MCUREG_APT_MODE_SPECIAL_EFFECT_SEPIA_CB(x)      ((x) << 8)

// MCUREG_APT_MODE_x_TEST_MODE register values
#define MCUREG_APT_MODE_TEST_MODE_NONE                  (0)
#define MCUREG_APT_MODE_TEST_MODE_FLAT                  (1)
#define MCUREG_APT_MODE_TEST_MODE_RAMP                  (2)
#define MCUREG_APT_MODE_TEST_MODE_COLOR_BARS            (3)
#define MCUREG_APT_MODE_TEST_MODE_VERTICAL_STRIPES      (4)
#define MCUREG_APT_MODE_TEST_MODE_NOISE                 (5)
#define MCUREG_APT_MODE_TEST_MODE_HORIZONTAL_STRIPES    (6)

#endif // LIBNDS_NDS_CAMERA_H__
