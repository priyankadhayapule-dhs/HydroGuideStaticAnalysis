package com.echonous.kLink;

/**
 * Contains the Register definitions for the Bosch BMI160 Gyro/Accelerometer.
 */
public final class Bmi160RegisterDefs {

//==============================================================================
// Register Addresses
//==============================================================================
public static final int BMI160_REG_PMU_STATUS       = 0x03; //!< Power management status register
public static final int BMI160_REG_INT_STATUS0      = 0x1C; //!< Interrupt Status register 0
public static final int BMI160_REG_INT_STATUS1      = 0x1D; //!< Interrupt Status register 1

public static final int BMI160_REG_ACC_CONF         = 0x40; //!< Accelerometer Configuration
public static final int BMI160_REG_ACC_RANGE        = 0x41; //!< Accelerometer Range

public static final int BMI160_REG_INT_EN0          = 0x50; //!< Interrupt enable register 0
public static final int BMI160_REG_INT_EN2          = 0x52; //!< Interrupt enable register 2
public static final int BMI160_REG_INT_OUT_CTRL     = 0x53; //!< Interrupt output control register
public static final int BMI160_REG_INT_MAP0         = 0x55; //!< Interrupt mapping register 0

public static final int BMI160_REG_INT_MOTION0      = 0x5F; //!< Motion interrupt register 0
public static final int BMI160_REG_INT_MOTION1      = 0x60; //!< Motion interrupt register 1
public static final int BMI160_REG_INT_MOTION2      = 0x61; //!< Motion interrupt register 2
public static final int BMI160_REG_INT_MOTION3      = 0x62; //!< Motion interrupt register 3

//==============================================================================
// Bit definitions for BMI160_REG_ACC_RANGE
public static final int BMI160_ACC_RANGE_2G  = 0x3;    //!< +/-2g Accelerometer range

//==============================================================================
// Bit definitions for BMI160_REG_ACC_CONF

public static final int BMI160_ACC_ODR_25HZ   = 0x6;  //!< 25Hz ODR value

public static final int BMI160_ACC_AVG_SHIFT_BITS = 4; //!< Number of bits to shift for sampling averaging cycles.
public static final int BMI160_ACC_US_AVG4   = 0x02 << BMI160_ACC_AVG_SHIFT_BITS; //!< Accelerometer 4 cycle averaging

public static final int BMI_ACC_US = 0x80; //!< Undersampling bit flag

//==============================================================================
// Bit definitions for BMI160_REG_INT_EN

// INT_EN[0] Bits
public static final int BMI160_INT_ANYMO_X_EN       = 0x01; //!< Any motion X interrupt enable
public static final int BMI160_INT_ANYMO_Y_EN       = 0x02; //!< Any motion Y interrupt enable
public static final int BMI160_INT_ANYMO_Z_EN       = 0x04; //!< Any motion Z interrupt enable

// INT_EN[2] Bits
public static final int BMI160_INT_NOMOX_EN         = 0x01; //!< No motion X interrupt enable
public static final int BMI160_INT_NOMOY_EN         = 0x02; //!< No motion Y interrupt enable
public static final int BMI160_INT_NOMOZ_EN         = 0x04; //!< No motion Z interrupt enable

//==============================================================================
// Bit definitions for BMI160_REG_INT_MAP

// INT_MAP[0] Bits
public static final int BMI160_INT_MAP_MOTION_INT1      = 0x04; //!< Map motion interrupt to INT1
public static final int BMI160_INT_MAP_NO_MOTION_INT1   = 0x08; //!< Map no motion interrupt to INT1

//==============================================================================
// Bit definitions for BMI160_REG_INT_OUT_CTRL

public static final int BMI160_INT1_EDGE_CRTL       = 0x01; //!< INT1 use edge based interrupt
public static final int BMI160_INT1_LVL             = 0x02; //!< INT1 interrupt level (low/high/falling/rising)
public static final int BMI160_INT1_OUTPUT_EN       = 0x08; //!< INT1 interrupt output enable

//==============================================================================
// Bit definitions for BMI160_REG_INT_STATUS

// INT_STATUS[0] Bits
public static final int BMI160_INT_STATUS_ANYMOTION = 0x04; //!< Any motion interrupt active

// INT_STATUS[1] Bits
public static final int BMI160_INT_STATUS_NOMOTION = 0x80; //!< No motion interrupt active

//==============================================================================
// Bit definitions for BMI160_REG_INT_MOTION[0-3]

//! Bit shift for the no motion duration value in MOTION0
public static final int BMI160_NO_MOTION_DURATION_SHIFT = 2;
//! BIt value for the int_not_mot_sel bit in MOTION3
public static final int BMI160_NO_MOTION_SEL = 0x01;

//==============================================================================
// Commands
public static final int BMI160_CMD_ACC_SET_PMU_MODE = 0x10; //!< Set the Accelerometer Power Mode command

// Accelerometer PMU modes
public static final int BMI160_ACC_PMU_MODE_SUSPEND    = 0x0; //!< Accelerometer Suspend power mode
public static final int BMI160_ACC_PMU_MODE_LOW_POWER  = 0x2; //!< Accelerometer Low power mode

}
