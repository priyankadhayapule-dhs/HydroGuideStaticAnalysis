//
// Copyright 2017 EchoNous, Inc.
//
#pragma once

#include "DauRegisters.h"

#define TBF_NUM_TOP_REGS	        19
#define TBF_NUM_BP_REGS             62

#define TBF_BASE					SE_FB_MEM_START_ADDR
#define TBF_OFFSET                  0
//--------------------------------------------
// Top level reset register
#define TBF_REG_TR0                 (0x00400000 + TBF_OFFSET)

#define TR_SWRST_BIT                0
#define TR_SWRST_LEN                1

#define TR_SPI_SWRST_BIT            1
#define TR_SPI_SWRST_LEN            1

#define TR_AFEIO_SWRST_BIT          2
#define TR_AFEIO_SWRST_LEN          1

#define TR_AFE_SWRST_BIT            3
#define TR_AFE_SWRST_LEN            1

#define TR_TX_SWRST_BIT             4
#define TR_TX_SWRST_LEN             1

#define TR_PSV_SWRST_BIT            5
#define TR_PSV_SWRST_LEN            1

#define TR_HV_FAULT_SWRST_BIT       8
#define TR_HV_FAULT_SWRST_LEN       1

#define TR_HV_INT_SWRST_BIT         9
#define TR_HV_INT_SWRST_LEN         1


//--------------------------------------------
// Register to enable PRI timer for generating LS
#define TBF_REG_LS                  (0x00400004 + TBF_OFFSET)

//--------------------------------------------
#define TBF_REG_EN0                 (0x00400008 + TBF_OFFSET)

#define TR_TX_EN_BIT                0
#define TR_TX_EN_LEN                1

#define TR_RX_EN_BIT                1
#define TR_RX_EN_LEN                1

#define TR_BF_EN_BIT                2
#define TR_BF_EN_LEN                1

#define TR_AFE_AUTO_RST_EN_BIT      5
#define TR_AFE_AUTO_RST_EN_LEN      1

#define TR_AFEIO_AUTO_RST_EN_BIT    6
#define TR_AFEIO_AUTO_RST_EN_LEN    1

#define TR_AFEIO_CONFIG_EN_BIT      7
#define TR_AFEIO_CONFIG_EN_LEN      1

#define TR_THSD_BIT                 8
#define TR_THSD_LEN                 1

#define AFE0_SLOPE_EN_BIT           16
#define AFE0_SLOPE_EN_LEN           1

#define AFE1_SLOPE_EN_BIT           17
#define AFE1_SLOPE_EN_LEN           1

#define AFE0_UPDN_EN_BIT            18
#define AFE0_UPDN_EN_LEN            1

#define AFE1_UPDN_EN_BIT            19
#define AFE1_UPDN_EN_LEN            1

//--------------------------------------------
#define TBF_REG_CK0                 (0x0040000C + TBF_OFFSET)

#define TR_CG4X_EN_BIT              0
#define TR_CG4X_EN_LEN              1

#define TR_CG8X_EN_BIT              1
#define TR_CG8X_EN_LEN              1

#define TR_CGTX_EN_BIT              2
#define TR_CGTX_EN_LEN              1

#define TR_LVDS_EN_BIT              3
#define TR_LVDS_EN_LEN              1

#define TR_TR_EN_EN_BIT             4
#define TR_TR_EN_EN_LEN             1

#define TR_SLEEP_EN_BIT             5
#define TR_SLEEP_EN_LEN             1

#define TR_LVDS_PM_EN_BIT           6
#define TR_LVDS_PM_EN_LEN           1

#define CW_LS_RST_EN_BIT            7
#define CW_LS_RST_EN_LEN            1

#define TR_WAKEUP_EN_BIT            8
#define TR_WAKEUP_EN_LEN            1

#define TR_HV_SPI_PM_EN_BIT         9
#define TR_HV_SPI_PM_EN_LEN         1

#define HVSPIEEN0_DIS_BIT           10
#define HVSPIEEN0_DIS_LEN           1

#define HVSPIEEN1_DIS_BIT           11
#define HVSPIEEN1_DIS_LEN           1

#define DTGC_WAKEUP_BIT             16
#define DTGC_WAKEUP_LEN             1

#define COMP_WAKEUP_BIT             17
#define COMP_WAKEUP_LEN             1

#define BPF_WAKEUP_BIT              18
#define BPF_WAKEUP_LEN              1

#define WF_WAKEUP_BIT               19
#define WF_WAKEUP_LEN               1

#define BLEND_WAKEUP_BIT            20
#define BLEND_WAKEUP_LEN            1

#define CCOMP_WAKEUP_BIT            21
#define CCOMP_WAKEUP_LEN            1


//--------------------------------------------
#define TBF_REG_PS0                 (0x00400010 + TBF_OFFSET)

#define PS_ENABLE_BIT               0
#define PS_ENABLE_LEN               8

#define PS_CLK_FH_BIT               8
#define PS_CLK_FH_LEN               4

#define PS_CLK_FL_BIT               12
#define PS_CLK_FL_LEN               4

#define TX_CW_BIT                   16
#define TX_CW_LEN                   8

//--------------------------------------------
#define TBF_REG_PS1                 (0x00400014 + TBF_OFFSET)

#define PS_DIV0_BIT                 0
#define PS_DIV0_LEN                 8

#define PS_DIV1_BIT                 8
#define PS_DIV1_LEN                 8

#define PS_DIV2_BIT                 16
#define PS_DIV2_LEN                 8

#define PS_DIV3_BIT                 24
#define PS_DIV3_LEN                 8


//--------------------------------------------
#define TBF_REG_PS2                 (0x00400018 + TBF_OFFSET)

#define PS_PHASE0_BIT               0
#define PS_PHASE0_LEN               8

#define PS_PHASE1_BIT               8
#define PS_PHASE1_LEN               8

#define PS_PHASE2_BIT               16
#define PS_PHASE2_LEN               8

#define PS_PHASE3_BIT               24
#define PS_PHASE3_LEN               8

//--------------------------------------------
#define TBF_REG_AFE0                (0x0040001C + TBF_OFFSET)

#define AFE_PFULL_BIT               0
#define AFE_PFULL_LEN               8

#define AFE_INIT_COMMANDS_BIT       8
#define AFE_INIT_COMMANDS_LEN       8

//--------------------------------------------
#define TBF_REG_AFE1                (0x00400020 + TBF_OFFSET)

#define AFE_PDGBL_DEL_BIT           0
#define AFE_PDGBL_DEL_LEN           16

//--------------------------------------------
#define TBF_REG_AFE2                (0x00400024 + TBF_OFFSET)

#define AFEIO_RST_DEL_BIT           0
#define AFEIO_RST_DEL_LEN           16

#define AFE_RST_DEL_BIT             16
#define AFE_RST_DEL_LEN             16

//--------------------------------------------
#define TBF_REG_AFE3                (0x00400028 + TBF_OFFSET)
//--------------------------------------------
#define TBF_REG_AFE4                (0x0040002C + TBF_OFFSET)

#define AFE_SLOPE_DEL_BIT           0
#define AFE_SLOPE_DEL_LEN           16

#define AFE_UPDN_DEL_BIT            16
#define AFE_UPDN_DEL_LEN            16

//--------------------------------------------
#define TBF_REG_AFE5                (0x00400030 + TBF_OFFSET)

#define AFE_TRIG_DEL_BIT            0
#define AFE_TRIG_DEL_LEN            16

//--------------------------------------------
#define TBF_REG_HVSPI0              (0x00400034 + TBF_OFFSET)

#define HVSPI_PTR_BIT               0
#define HVSPI_PTR_LEN               13

#define HVSPI_BITSEL_BIT            13
#define HVSPI_BITSEL_LEN            2

#define HVSPI_W_C_BIT               16
#define HVSPI_W_C_LEN               13

//--------------------------------------------
#define TBF_REG_HVPS0               (0x00400038 + TBF_OFFSET)
#define HVPS_DIV_BIT                0
#define HVPS_DIV_LEN                8

//--------------------------------------------
#define TBF_REG_HVPS1               (0x0040003C + TBF_OFFSET)

#define HVCTRLP_ON_BIT              0
#define HVCTRLP_ON_LEN              12

#define HVCTRLP_OFF_BIT             12
#define HVCTRLP_OFF_LEN             12

#define TBF_REG_HVPS2               (0x00400040 + TBF_OFFSET)

#define HVCTRLN_ON_BIT              0
#define HVCTRLN_ON_LEN              12

#define HVCTRLN_OFF_BIT             12
#define HVCTRLN_OFF_LEN             12

//================ BEAM PARAMETER REGISTER FILE ===================================
//--------------------------------------------
#define TBF_REG_BP0                 (0x00404000 + TBF_OFFSET)

#define ADC_S_BIT                   0
#define ADC_S_LEN                   14

#define ADC_S_PRE_BIT               16
#define ADC_S_PRE_LEN               10

//--------------------------------------------
#define TBF_REG_FPS                 (0x00404004 + TBF_OFFSET)

#define FP_S_BIT                    0
#define FP_S_LEN                    14

#define DM_START_BIT                16
#define DM_START_LEN                10

#define DEL_SCALE_BIT               28
#define DEL_SCALE_LEN               3

//--------------------------------------------
#define TBF_REG_BPS                 (0x00404008 + TBF_OFFSET)

#define BF_S_BIT                    0
#define BF_S_LEN                    14

#define BFOUT_BIT                   16
#define BFOUT_LEN                   11

//--------------------------------------------
#define TBF_REG_FPLUT               (0x0040400C + TBF_OFFSET)

#define FP_PTR_BIT                  0
#define FP_PTR_LEN                  12

#define HV_CLKEN_OFF_BIT            0
#define HV_CLKEN_OFF_LEN            16

#define HV_CLKEN_ON_BIT             16
#define HV_CLKEN_ON_LEN             16

//--------------------------------------------
#define TBF_REG_BENABLE             (0x00404010 + TBF_OFFSET)

#define BP_TX_EN_BIT                0
#define BP_TX_EN_LEN                1

#define BP_RX_EN_BIT                1
#define BP_RX_EN_LEN                1

#define BP_BF_EN_BIT                2
#define BP_BF_EN_LEN                1

#define BP_OUT_EN_BIT               3
#define BP_OUT_EN_LEN               1

#define OUT_TYPE_BIT                4
#define OUT_TYPE_LEN                3

#define OUT_IQ_TYPE_BIT             7
#define OUT_IQ_TYPE_LEN             1

#define CTB_WRITE_EN_BIT            8
#define CTB_WRITE_EN_LEN            1

#define CFP_EN_BIT                  9
#define CFP_EN_LEN                  1

#define B_EOF_BIT                   16
#define B_EOF_LEN                   1

#define C_EOF_BIT                   17
#define C_EOF_LEN                   1

//--------------------------------------------
#define TBF_REG_BPWR                (0x00404014 + TBF_OFFSET)

#define PM_FAST_EN_BIT              0
#define PM_FAST_EN_LEN              1

#define PM_AFE_PWR_ON_BIT           1
#define PM_AFE_PWR_ON_LEN           1

#define PM_AFE_PWR_OFF_BIT          2
#define PM_AFE_PWR_OFF_LEN          1

#define PM_AFE_PWR_SB_BIT           3
#define PM_AFE_PWR_SB_LEN           1

#define PM_AFE_PWR_GBL_TIMER_BIT    4
#define PM_AFE_PWR_GBL_TIMER_LEN    1

#define PM_AFE_PWR_SB_TIMER_BIT     5
#define PM_AFE_PWR_SB_TIMER_LEN     1


#define PM_DIG_CH_PM_BIT            6
#define PM_DIG_CH_PM_LEN            1

#define PM_HV_CONFIG_BIT            8
#define PM_HV_CONFIG_LEN            24

//--------------------------------------------
#define TBF_REG_TXLUT               (0x00404018 + TBF_OFFSET)

#define TXD_PTR_BIT                 0
#define TXD_PTR_LEN                 5

#define TXD_REV_BIT                 8
#define TXD_REV_LEN                 1

#define TXD_LOOP_BIT                9
#define TXD_LOOP_LEN                1

#define TXD_INVERT_BIT              10
#define TXD_INVERT_LEN              1

#define TXD_TXPAIR_BIT              12
#define TXD_TXPAIR_LEN              1

#define TXW_PTR_BIT                 16
#define TXW_PTR_LEN                 14

//--------------------------------------------
#define TBF_REG_TXLEN               (0x0040401C + TBF_OFFSET)

#define TXLEN_BIT                   0
#define TXLEN_LEN                   14

//--------------------------------------------
#define TBF_REG_TXAP_LOW            (0x00404020 + TBF_OFFSET)

#define TXAP_LOW_BIT                0
#define TXAP_LOW_LEN                32

//--------------------------------------------
#define TBF_REG_TXAP_HIGH           (0x00404024 + TBF_OFFSET)

#define TXAP_HIGH_BIT               0
#define TXAP_HIGH_LEN               32

//--------------------------------------------
#define TBF_REG_RXAP_LOW            (0x00404028 + TBF_OFFSET)

#define RXAP_LOW_BIT                0
#define RXAP_LOW_LEN                32

//--------------------------------------------
#define TBF_REG_RXAP_HIGH           (0x0040402C + TBF_OFFSET)

#define RXAP_HIGH_BIT               0
#define RXAP_HIGH_LEN               32

//--------------------------------------------
#define TBF_REG_BPRI                (0x00404030 + TBF_OFFSET)

#define BPRI_BIT                    0
#define BPRI_LEN                    32

//--------------------------------------------
#define TBF_REG_TXRXT               (0x00404034 + TBF_OFFSET)

#define TX_START_BIT                0
#define TX_START_LEN                16

#define RX_START_BIT                16
#define RX_START_LEN                16

//--------------------------------------------
#define TBF_REG_AFE_TR_EN           (0x00404038 + TBF_OFFSET)

#define AFE_TR_EN_BIT               0
#define AFE_TR_EN_LEN               32

//--------------------------------------------
#define TBF_REG_BPRF                (0x0040403C + TBF_OFFSET)

#define RF_FL_BIT                   0
#define RF_FL_LEN                   3

#define RF_FS_BIT                   4
#define RF_FS_LEN                   1

//--------------------------------------------
#define TBF_REG_VBPF0               (0x00404040 + TBF_OFFSET)

#define VBP_FC_BIT                  0
#define VBP_FC_LEN                  1

#define VBP_PR_BIT                  1
#define VBP_PR_LEN                  1

#define VBP_PLC_BIT                 8
#define VBP_PLC_LEN                 8

#define VBP_FL_BIT                  16
#define VBP_FL_LEN                  8

#define VBP_FS_BIT                  24
#define VBP_FS_LEN                  2

#define VBP_OSCALE_BIT              28
#define VBP_OSCALE_LEN              3

//--------------------------------------------
#define TBF_REG_VBPF1               (0x00404044 + TBF_OFFSET)

#define VBP_PINC0_BIT               0
#define VBP_PINC0_LEN               8

#define VBP_PINC1_BIT               8
#define VBP_PINC1_LEN               8

#define VBP_WC0_BIT                 16
#define VBP_WC0_LEN                 8

#define VBP_WC1_BIT                 24
#define VBP_WC1_LEN                 8

//--------------------------------------------
#define TBF_REG_VBPF2               (0x00404048 + TBF_OFFSET)

#define VBP_PHASE_INC_BIT           0
#define VBP_PHASE_INC_LEN           8

#define VBP_W_C_BIT                 16
#define VBP_W_C_LEN                 14

//--------------------------------------------
#define TBF_REG_VBPF3               (0x0040404C + TBF_OFFSET)

#define VBP_I_C_BIT                 0
#define VBP_I_C_LEN                 14

#define VBP_DSEQ_BIT                16
#define VBP_DSEQ_LEN                16

//--------------------------------------------
#define TBF_REG_APOD0               (0x00404050 + TBF_OFFSET)

#define APOD_SCALE_BIT              0
#define APOD_SCALE_LEN              16

//--------------------------------------------
#define TBF_REG_APOD1               (0x00404054 + TBF_OFFSET)

#define APOD_FLAT_BIT               0
#define APOD_FLAT_LEN               1

//--------------------------------------------
#define TBF_REG_ATGC                (0x00404058 + TBF_OFFSET)

#define ATGC_PRO1_BIT               0
#define ATGC_PRO1_LEN               2

#define ATGC_PRO2_BIT               2
#define ATGC_PRO2_LEN               2

//--------------------------------------------
#define TBF_REG_DTGC0               (0x0040405C + TBF_OFFSET)

#define TGC0_LGC_BIT                0
#define TGC0_LGC_LEN                24

#define TGC0_FLAT_BIT               24
#define TGC0_FLAT_LEN               1

#define BGPTR_BIT                   25
#define BGPTR_LEN                   5

//--------------------------------------------
#define TBF_REG_DTGC1               (0x00404060 + TBF_OFFSET)

#define TGC1_LGC_BIT                0
#define TGC1_LGC_LEN                24

#define TGC1_FLAT_BIT               24
#define TGC1_FLAT_LEN               1

#define BG_R_C_BIT                  25
#define BG_R_C_LEN                  5

#define TGC_FLIP_BIT                30
#define TGC_FLIP_LEN                1

//--------------------------------------------
#define TBF_REG_DTGC2               (0x00404064 + TBF_OFFSET)

#define TGC2_LGC_BIT                0
#define TGC2_LGC_LEN                24

#define TGC2_FLAT_BIT               24
#define TGC2_FLAT_LEN               1

//--------------------------------------------
#define TBF_REG_DTGC3               (0x00404068 + TBF_OFFSET)

#define TGC3_LGC_BIT                0
#define TGC3_LGC_LEN                24

#define TGC3_FLAT_BIT               24
#define TGC3_FLAT_LEN               1

//--------------------------------------------
#define TBF_REG_DTGC4               (0x0040406C + TBF_OFFSET)

#define TGCIR_LOW_BIT               0
#define TGCIR_LOW_LEN               32

//--------------------------------------------
#define TBF_REG_DTGC5               (0x00404070 + TBF_OFFSET)

#define TGCIR_HIGH_BIT              0
#define TGCIR_HIGH_LEN              32

//--------------------------------------------
#define TBF_REG_CCOMP0              (0x00404074 + TBF_OFFSET)

#define CC_PTR_BIT                  0
#define CC_PTR_LEN                  10

#define CC_OUTLEN_BIT               16
#define CC_OUTLEN_LEN               1

#define CC_WE_BIT                   17
#define CC_WE_LEN                   1

#define CC_SUM_BIT                  18
#define CC_SUM_LEN                  1

#define CC_ONE_BIT                  19
#define CC_ONE_LEN                  1

#define CC_INV_BIT                  20
#define CC_INV_LEN                  1

#define CC_OUTMODE_BIT              21
#define CC_OUTMODE_LEN              1

//--------------------------------------------
#define TBF_REG_B0POX               (0x00404078 + TBF_OFFSET)

#define B0POX_BIT                   0
#define B0POX_LEN                   20

//--------------------------------------------
#define TBF_REG_B0POY               (0x0040407C + TBF_OFFSET)

#define B0POY_BIT                   0
#define B0POY_LEN                   20

//--------------------------------------------
#define TBF_REG_B1POX               (0x00404080 + TBF_OFFSET)

#define B1POX_BIT                   0
#define B1POX_LEN                   20

//--------------------------------------------
#define TBF_REG_B1POY               (0x00404084 + TBF_OFFSET)

#define B1POY_BIT                   0
#define B1POY_LEN                   20

//--------------------------------------------
#define TBF_REG_B2POX               (0x00404088 + TBF_OFFSET)

#define B2POX_BIT                   0
#define B2POX_LEN                   20

//--------------------------------------------
#define TBF_REG_B2POY               (0x0040408C + TBF_OFFSET)

#define B2POY_BIT                   0
#define B2POY_LEN                   20

//--------------------------------------------
#define TBF_REG_B3POX               (0x00404090 + TBF_OFFSET)

#define B3POX_BIT                   0
#define B3POX_LEN                   20

//--------------------------------------------
#define TBF_REG_B3POY               (0x00404094 + TBF_OFFSET)

#define B3POY_BIT                   0
#define B3POY_LEN                   20

//--------------------------------------------
#define TBF_REG_B0SINT              (0x00404098 + TBF_OFFSET)

#define B0SINT_BIT                  0
#define B0SINT_LEN                  18

//--------------------------------------------
#define TBF_REG_B0COST              (0x0040409C + TBF_OFFSET)

#define B0COST_BIT                  0
#define B0COST_LEN                  18

//--------------------------------------------
#define TBF_REG_B1SINT              (0x004040A0 + TBF_OFFSET)

#define B1SINT_BIT                  0
#define B1SINT_LEN                  18

//--------------------------------------------
#define TBF_REG_B1COST              (0x004040A4 + TBF_OFFSET)

#define B1COST_BIT                  0
#define B1COST_LEN                  18

//--------------------------------------------
#define TBF_REG_B2SINT              (0x004040A8 + TBF_OFFSET)

#define B2SINT_BIT                  0
#define B2SINT_LEN                  18

//--------------------------------------------
#define TBF_REG_B2COST              (0x004040AC + TBF_OFFSET)

#define B2COST_BIT                  0
#define B2COST_LEN                  18

//--------------------------------------------
#define TBF_REG_B3SINT              (0x004040B0 + TBF_OFFSET)

#define B3SINT_BIT                  0
#define B3SINT_LEN                  18

//--------------------------------------------
#define TBF_REG_B3COST              (0x004040B4 + TBF_OFFSET)

#define B3COST_BIT                  0
#define B3COST_LEN                  18

//--------------------------------------------
#define TBF_REG_FPYS0               (0x004040B8 + TBF_OFFSET)

#define B0_YS_BIT                   0
#define B0_YS_LEN                   16

#define B1_YS_BIT                   16
#define B1_YS_LEN                   16

//--------------------------------------------
#define TBF_REG_FPYS1               (0x004040BC + TBF_OFFSET)

#define B2_YS_BIT                   0
#define B2_YS_LEN                   16

#define B3_YS_BIT                   16
#define B3_YS_LEN                   16

//--------------------------------------------
#define TBF_REG_FPPTR0              (0x004040C0 + TBF_OFFSET)

#define B0_FPPTR_BIT                0
#define B0_FPPTR_LEN                10

#define B1_FPPTR_BIT                16
#define B1_FPPTR_LEN                10

//--------------------------------------------
#define TBF_REG_FPPTR1              (0x004040C4 + TBF_OFFSET)

#define B2_FPPTR_BIT                0
#define B2_FPPTR_LEN                10

#define B3_FPPTR_BIT                16
#define B3_FPPTR_LEN                10

//--------------------------------------------
#define TBF_REG_TXPTR0              (0x004040C8 + TBF_OFFSET)

#define B0_TXPTR_BIT                0
#define B0_TXPTR_LEN                10

#define B1_TXPTR_BIT                16
#define B1_TXPTR_LEN                10

//--------------------------------------------
#define TBF_REG_TXPTR1              (0x004040CC + TBF_OFFSET)

#define B2_TXPTR_BIT                0
#define B2_TXPTR_LEN                10

#define B3_TXPTR_BIT                16
#define B3_TXPTR_LEN                10

//--------------------------------------------
#define TBF_REG_IRPTR0              (0x004040D0 + TBF_OFFSET)

#define B0_IRPTR_BIT                0
#define B0_IRPTR_LEN                10
//--------------------------------------------
#define TBF_REG_ELPTR0              (0x004040D4 + TBF_OFFSET)

#define EL_PTR_BIT                  0
#define EL_PTR_LEN                  8

#define APEL_PTR_BIT                8
#define APEL_PTR_LEN                8

//--------------------------------------------
//=== CORNER TURN BUFFER WRITE PARAMETERS ====

#define TBF_REG_CF0                 (0x0040408D + TBF_OFFSET)

#define CTBW_RPTR_BIT               0
#define CTBW_RPTR_LEN               14

#define CTBW_IPTR_BIT               16
#define CTBW_IPTR_LEN               14

//--------------------------------------------
#define TBF_REG_CF1                 (0x004040DC + TBF_OFFSET)

#define CTBW_RI_BIT                 0
#define CTBW_RI_LEN                 14

#define CTBW_II_BIT                 16
#define CTBW_II_LEN                 14

//--------------------------------------------
#define TBF_REG_CF2                 (0x004040E0 + TBF_OFFSET)

#define CTBW_R_C_BIT                0
#define CTBW_R_C_LEN                14

#define CTBW_I_C_BIT                16
#define CTBW_I_C_LEN                14

//--------------------------------------------
//==== CORNER TURN BUFFER READ PARAMETERS ====

#define TBF_REG_CF3                 (0x004040E4 + TBF_OFFSET)

#define CTBR_RPTR_BIT               0
#define CTBR_RPTR_LEN               14

#define CTBR_IPTR_BIT               16
#define CTBR_IPTR_LEN               14

//--------------------------------------------
#define TBF_REG_CF4                 (0x004040E8 + TBF_OFFSET)

#define CTBR_RI_BIT                 0
#define CTBR_RI_LEN                 14

#define CTBR_II_BIT                 16
#define CTBR_II_LEN                 14

//--------------------------------------------
#define TBF_REG_CF5                 (0x004040EC + TBF_OFFSET)

#define CTBR_R_C_BIT                0
#define CTBR_R_C_LEN                14

#define CTBR_I_C_BIT                16
#define CTBR_I_C_LEN                14

//========= WALL FILTER PARAMETERS ===========

#define TBF_REG_CF6                 (0x004040F0 + TBF_OFFSET)

#define CF_ENS_BIT                  0
#define CF_ENS_LEN                  5

#define CF_WFL_BIT                  8
#define CF_WFL_LEN                  5

//============ READ ONLY REGISTERS ===========
#define TBF_RO_ID                   (0x00408000 + TBF_OFFSET)
#define TBF_VERSION_V1              0x00000000
#define TBF_VERSION_V2              0x11111111
#define TBF_VERSION_ASIC            0x22222222

#define TBF_RO_AFE                  (0x00408004 + TBF_OFFSET)
#define TBF_RO_TX                   (0x00408008 + TBF_OFFSET)
#define TBF_RO_VR                   (0x0040800C + TBF_OFFSET)
#define TBF_RO_VA                   (0x00408010 + TBF_OFFSET)

//============ TRIGGER REGISTERS =============

// Software line sync
#define TBF_TRIG_LS                 (0x0040C000 + TBF_OFFSET)

//--------------------------------------------
// Generate pulse on AFE tx_trig pin
#define TBF_TRIG_AFE_TX             (0x0040C004 + TBF_OFFSET)

//--------------------------------------------
// Generate pulse on AFE tx_slope pin
#define TBF_TRIG_AFE_SLOPE          (0x0040C008 + TBF_OFFSET)

//--------------------------------------------
// Generate pulse on AFE updn pin
#define TBF_TRIG_AFE_UPDN           (0x0040C00C + TBF_OFFSET)

//--------------------------------------------
#define TBF_TRIG_TX                 (0x0040C010 + TBF_OFFSET)

//--------------------------------------------
#define TBF_TRIG_HVSPI              (0x0040C014 + TBF_OFFSET)

//========== Diagnostic Parameters ===========
#define TBF_DP0                     (0x00410000 + TBF_OFFSET)
#define TBF_DP1                     (0x00410004 + TBF_OFFSET)
#define TBF_DP2                     (0x00410008 + TBF_OFFSET)
#define TBF_DP3                     (0x0041000C + TBF_OFFSET)
#define TBF_DP4                     (0x00410010 + TBF_OFFSET)
#define TBF_DP5                     (0x00410014 + TBF_OFFSET)
#define TBF_DP6                     (0x00410018 + TBF_OFFSET)
#define TBF_DP7                     (0x0041001C + TBF_OFFSET)
#define TBF_DP8                     (0x00410020 + TBF_OFFSET)
#define TBF_DP9                     (0x00410024 + TBF_OFFSET)
#define TBF_DP10                    (0x00410028 + TBF_OFFSET)
#define TBF_DP11                    (0x0041002C + TBF_OFFSET)
#define TBF_DP12                    (0x00410030 + TBF_OFFSET)
#define TBF_DP13                    (0x00410034 + TBF_OFFSET)
#define TBF_DP14                    (0x00410038 + TBF_OFFSET)
#define TBF_DP15                    (0x0041003C + TBF_OFFSET)
#define TBF_DP16                    (0x00410040 + TBF_OFFSET)
#define TBF_DP17                    (0x00410044 + TBF_OFFSET)
#define TBF_DP18                    (0x00410048 + TBF_OFFSET)
#define TBF_DP19                    (0x0041004C + TBF_OFFSET)
#define TBF_DP20                    (0x00410050 + TBF_OFFSET)
#define TBF_DP21                    (0x00410054 + TBF_OFFSET)
#define TBF_DP22                    (0x00410058 + TBF_OFFSET)
#define TBF_DP23                    (0x0041005C + TBF_OFFSET)
#define TBF_DP24                    (0x00410060 + TBF_OFFSET)
#define TBF_DP25                    (0x00410064 + TBF_OFFSET)

//================ LUTs ======================

// Delay element location LUTs
#define TBF_LUT_ELOC_X              (0x00680000 + TBF_OFFSET)
#define TBF_LUT_ELOC_X_SIZE         256

#define TBF_LUT_ELOC_Y              (0x00681000 + TBF_OFFSET)
#define TBF_LUT_ELOC_Y_SIZE         256

//--------------------------------------------
// Apodization element location LUT
#define TBF_LUT_APODEL              (0x00687000 + TBF_OFFSET)
#define TBF_LUT_APODEL_SIZE         64

//--------------------------------------------
// Apodization weight LUT
#define TBF_LUT_MSAPW               (0x00686000 + TBF_OFFSET)
#define TBF_LUT_MSAPW_SIZE          2048

//--------------------------------------------
// Beam RX focal point LUTs
#define TBF_LUT_B0FPX               (0x00682000 + TBF_OFFSET)
#define TBF_LUT_B0FPX_SIZE          1024

#define TBF_LUT_B0FPY               (0x00683000 + TBF_OFFSET)
#define TBF_LUT_B0FPY_SIZE          1024

#define TBF_LUT_B1FPX               (0x00692000 + TBF_OFFSET)
#define TBF_LUT_B1FPX_SIZE          1024

#define TBF_LUT_B1FPY               (0x00693000 + TBF_OFFSET)
#define TBF_LUT_B1FPY_SIZE          1024

#define TBF_LUT_B2FPX               (0x006A2000 + TBF_OFFSET)
#define TBF_LUT_B2FPX_SIZE          1024

#define TBF_LUT_B2FPY               (0x006A3000 + TBF_OFFSET)
#define TBF_LUT_B2FPY_SIZE          1024

#define TBF_LUT_B3FPX               (0x006B2000 + TBF_OFFSET)
#define TBF_LUT_B3FPX_SIZE          1024

#define TBF_LUT_B3FPY               (0x006B3000 + TBF_OFFSET)
#define TBF_LUT_B3FPY_SIZE          1024

//--------------------------------------------
// Focal point transmit delay LUTs
#define TBF_LUT_B0FTXD              (0x00684000 + TBF_OFFSET)
#define TBF_LUT_B0FTXD_SIZE         1024

#define TBF_LUT_B1FTXD              (0x00694000 + TBF_OFFSET)
#define TBF_LUT_B1FTXD_SIZE         1024

#define TBF_LUT_B2FTXD              (0x006A4000 + TBF_OFFSET)
#define TBF_LUT_B2FTXD_SIZE         1024

#define TBF_LUT_B3FTXD              (0x006B4000 + TBF_OFFSET)
#define TBF_LUT_B3FTXD_SIZE         1024

//--------------------------------------------
// Transmit delay profile LUT
#define TBF_LUT_MSTXD               (0x00480000 + TBF_OFFSET)
#define TBF_LUT_MSTXD_SIZE          2048

//--------------------------------------------
// Transmit config SPI comman LUT (proto V2 & ASIC
#define TBF_LUT_MSSTTXC             (0x00480000 + TBF_OFFSET)
#define TBF_LUT_MSSTTXC_SIZE        8192

//--------------------------------------------
// Transmit waveform LUT
#define TBF_LUT_MSTXW               (0x00500000 + TBF_OFFSET)
#define TBF_LUT_MSTXW_SIZE          2048

//--------------------------------------------
// Compression LUT
#define TBF_LUT_MSCOMP              (0x006F4000 + TBF_OFFSET)
#define TBF_LUT_MSCOMP_SIZE         256

//--------------------------------------------
// Coherent beam compounding weighting LUT
#define TBF_LUT_MSCBC               (0x006F0000 + TBF_OFFSET)
#define TBF_LUT_MSCBC_SIZE          1024

#define TBF_LUT_MSCBCBUF            (0x006F4000 + TBF_OFFSET)
#define TBF_LUT_MSCBCBUF_SIZE       2048

//--------------------------------------------
// RF interpolation filter coefficient LUT
#define TBF_LUT_MSRF                (0x006C0000 + TBF_OFFSET)
#define TBF_LUT_MSRF_SIZE           512

//--------------------------------------------
// Bandpass filter coefficient LUT
#define TBF_LUT_MSBPFI              (0x006E0000 + TBF_OFFSET)
#define TBF_LUT_MSBPFI_SIZE         2048

#define TBF_LUT_MSBPFQ              (0x006E2000 + TBF_OFFSET)
#define TBF_LUT_MSBPFQ_SIZE         2048

//--------------------------------------------
// Frequency compounding coefficient LUT
#define TBF_LUT_MSFC                (0x006F2000 + TBF_OFFSET)
#define TBF_LUT_MSFC_SIZE           512

#define TBF_LUT_MSFC_BUF            (0x006F2000 + TBF_OFFSET)
#define TBF_LUT_MSFC_BUF_SIZE       2048

//--------------------------------------------
// Digital TGC related LUTs
#define TBF_LUT_MSDBL               (0x006D8000 + TBF_OFFSET)
#define TBF_LUT_MSDBL_SIZE          512

#define TBF_LUT_MSDBG01             (0x006D0000 + TBF_OFFSET)
#define TBF_LUT_MSDBG01_SIZE        1024

#define TBF_LUT_MSDBG23             (0x006D4000 + TBF_OFFSET)
#define TBF_LUT_MSDBG23_SIZE        1024

//--------------------------------------------
// Apodization Scale
#define TBF_LUT_MSB0APS             (0x00685000 + TBF_OFFSET)
#define TBF_LUT_MSB0APS_SIZE        1024

#define TBF_LUT_MSB1APS             (0x00695000 + TBF_OFFSET)
#define TBF_LUT_MSB1APS_SIZE        1024

#define TBF_LUT_MSB2APS             (0x006A5000 + TBF_OFFSET)
#define TBF_LUT_MSB2APS_SIZE        1024

#define TBF_LUT_MSB3APS             (0x006B5000 + TBF_OFFSET)
#define TBF_LUT_MSB3APS_SIZE        1024

//--------------------------------------------
// Delay and apodization interpolation rate LUT
#define TBF_LUT_MSDIS               (0x00688000 + TBF_OFFSET)
#define TBF_LUT_MSDIS_SIZE          128

//--------------------------------------------
// Color dlow wall filter coefficients
#define TBF_LUT_MSWFC               (0x006F6000 + TBF_OFFSET)
#define TBF_LUT_MSWFC_SIZE          256

//--------------------------------------------
// Colorflow corner turn buffer
#define TBF_LUT_MSCTB               (0x006F8000 + TBF_OFFSET)

//--------------------------------------------
// AFE SPI write queue
#define TBF_LUT_MSRTAFE             (0x00700000 + TBF_OFFSET)
#define TBF_LUT_MSRTAFE_SIZE        256

//--------------------------------------------
// AFE initialization commands
#define TBF_LUT_MSAFEINIT           (0x00700400 + TBF_OFFSET)
#define TBF_LUT_MSAFEINIT_SIZE      256

//--------------------------------------------
//--------------------------------------------
//--------------------------------------------
