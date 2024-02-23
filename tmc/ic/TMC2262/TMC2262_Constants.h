/*******************************************************************************
* Copyright © 2023 Analog Devices Inc. All Rights Reserved. This software is
* proprietary & confidential to Analog Devices, Inc. and its licensors.
*******************************************************************************/


#ifndef TMC_IC_TMC2262_TMC2262_CONSTANTS_H_
#define TMC_IC_TMC2262_TMC2262_CONSTANTS_H_

#include "tmc/helpers/Constants.h"

#define TMC2262_REGISTER_COUNT   TMC_REGISTER_COUNT
#define TMC2262_MOTORS           1
#define TMC2262_WRITE_BIT        TMC_WRITE_BIT
#define TMC2262_ADDRESS_MASK     TMC_ADDRESS_MASK
#define TMC2262_MAX_VELOCITY     8388096
#define TMC2262_MAX_ACCELERATION u16_MAX

// ramp modes (Register TMC2262_RAMPMODE)
#define TMC2262_MODE_POSITION  0
#define TMC2262_MODE_VELPOS    1
#define TMC2262_MODE_VELNEG    2
#define TMC2262_MODE_HOLD      3

// limit switch mode bits (Register TMC2262_SWMODE)
#define TMC2262_SW_STOPL_ENABLE    0x0001
#define TMC2262_SW_STOPR_ENABLE    0x0002
#define TMC2262_SW_STOPL_POLARITY  0x0004
#define TMC2262_SW_STOPR_POLARITY  0x0008
#define TMC2262_SW_SWAP_LR         0x0010
#define TMC2262_SW_LATCH_L_ACT     0x0020
#define TMC2262_SW_LATCH_L_INACT   0x0040
#define TMC2262_SW_LATCH_R_ACT     0x0080
#define TMC2262_SW_LATCH_R_INACT   0x0100
#define TMC2262_SW_LATCH_ENC       0x0200
#define TMC2262_SW_SG_STOP         0x0400
#define TMC2262_SW_SOFTSTOP        0x0800

// Status bits (Register TMC2262_RAMPSTAT)
#define TMC2262_RS_STOPL          0x0001
#define TMC2262_RS_STOPR          0x0002
#define TMC2262_RS_LATCHL         0x0004
#define TMC2262_RS_LATCHR         0x0008
#define TMC2262_RS_EV_STOPL       0x0010
#define TMC2262_RS_EV_STOPR       0x0020
#define TMC2262_RS_EV_STOP_SG     0x0040
#define TMC2262_RS_EV_POSREACHED  0x0080
#define TMC2262_RS_VELREACHED     0x0100
#define TMC2262_RS_POSREACHED     0x0200
#define TMC2262_RS_VZERO          0x0400
#define TMC2262_RS_ZEROWAIT       0x0800
#define TMC2262_RS_SECONDMOVE     0x1000
#define TMC2262_RS_SG             0x2000

// Encoderbits (Register TMC2262_ENCMODE)
#define TMC2262_EM_DECIMAL     0x0400
#define TMC2262_EM_LATCH_XACT  0x0200
#define TMC2262_EM_CLR_XENC    0x0100
#define TMC2262_EM_NEG_EDGE    0x0080
#define TMC2262_EM_POS_EDGE    0x0040
#define TMC2262_EM_CLR_ONCE    0x0020
#define TMC2262_EM_CLR_CONT    0x0010
#define TMC2262_EM_IGNORE_AB   0x0008
#define TMC2262_EM_POL_N       0x0004
#define TMC2262_EM_POL_B       0x0002
#define TMC2262_EM_POL_A       0x0001

#endif /* TMC_IC_TMC2262_TMC2262_CONSTANTS_H_ */
