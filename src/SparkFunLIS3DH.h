/******************************************************************************
SparkFunLIS3DH.h
LIS3DH Arduino and Teensy Driver

Marshall Taylor @ SparkFun Electronics
Nov 16, 2016
https://github.com/sparkfun/LIS3DH_Breakout
https://github.com/sparkfun/SparkFun_LIS3DH_Arduino_Library

Resources:
Uses Wire.h for i2c operation
Uses SPI.h for SPI operation
Either can be omitted if not used

Development environment specifics:
Arduino IDE 1.6.4
Teensy loader 1.23

This code is released under the [MIT License](http://opensource.org/licenses/MIT).

Please review the LICENSE.md file included with this example. If you have any questions
or concerns with licensing, please contact techsupport@sparkfun.com.

Distributed as-is; no warranty is given.
******************************************************************************/

#ifndef __LIS3DH_IMU_H__
#define __LIS3DH_IMU_H__

#include "stdint.h"

// values for commInterface
#define I2C_MODE 0
#define SPI_MODE 1


#define LIS3DH_ADDR_SEL_HIGH	0x18 // with SEL/SDO pulled up to VDD
#define LIS3DH_ADDR_SEL_LOW	0x19	// with SEL/SDO pulled down to VSS

#define LIS3DSH_ADDR_SEL_HIGH	0x1D	
#define LIS3DSH_ADDR_SEL_LOW	0x1E	

// Return values
typedef enum
{
    IMU_SUCCESS,
    IMU_HW_ERROR,
    IMU_NOT_SUPPORTED,
    IMU_GENERIC_ERROR,
    IMU_OUT_OF_BOUNDS,
    IMU_ALL_ONES_WARNING,
    //...
} status_t;

// This is the core operational class of the driver.
//   LIS3DHCore contains only read and write operations towards the IMU.
//   To use the higher level functions, use the class LIS3DH which inherits
//   this class.

class LIS3DHCore
{
  public:
    LIS3DHCore(uint8_t);
    LIS3DHCore(uint8_t, uint8_t);
    ~LIS3DHCore() = default;

    status_t beginCore(void);

    // The following utilities read and write to the IMU

    // ReadRegisterRegion takes a uint8 array address as input and reads
    //   a chunk of memory into that array.
    status_t readRegisterRegion(uint8_t *, uint8_t, uint8_t);

    // readRegister reads one 8-bit register
    status_t readRegister(uint8_t *, uint8_t);

    // Reads two 8-bit regs, LSByte then MSByte order, and concatenates them.
    //   Acts as a 16-bit read operation
    status_t readRegisterInt16(int16_t *, uint8_t offset);

    // Writes an 8-bit byte;
    status_t writeRegister(uint8_t, uint8_t);

  private:
    // Communication stuff
    uint8_t commInterface;
    uint8_t I2CAddress;
    uint8_t chipSelectPin;
};

// This struct holds the settings the driver uses to do calculations
struct SensorSettings
{
  public:
    // ADC and Temperature settings
    uint8_t adcEnabled;
    uint8_t tempEnabled;

    // Accelerometer settings
    uint16_t accelSampleRate; // Hz.  Can be: 0,1,10,25,50,100,200,400,1600,5000 Hz
    uint8_t accelRange;       // Max G force readable.  Can be: 2, 4, 8, 16

    uint8_t xAccelEnabled;
    uint8_t yAccelEnabled;
    uint8_t zAccelEnabled;

    // Fifo settings
    uint8_t fifoEnabled;
    uint8_t fifoMode; // can be 0x0,0x1,0x2,0x3
    uint8_t fifoThreshold;
};

// This is the highest level class of the driver.
//
//   class LIS3DH inherits the core and makes use of the beginCore()
// method through it's own begin() method.  It also contains the
// settings struct to hold user settings.

class LIS3DH : public LIS3DHCore
{
  public:
    // IMU settings
    SensorSettings settings;

    // Error checking
    uint16_t allOnesCounter;
    uint16_t nonSuccessCounter;

    // Constructor generates default SensorSettings.
    //(over-ride after construction if desired)
    LIS3DH(uint8_t busType = I2C_MODE, uint8_t inputArg = 0x19);
    //~LIS3DH() = default;

    // Call to apply SensorSettings
    status_t begin(void);
    void applySettings(void);

    // Returns the raw bits from the sensor cast as 16-bit signed integers
    int16_t readRawAccelX(void);
    int16_t readRawAccelY(void);
    int16_t readRawAccelZ(void);

    // Returns the values as floats.  Inside, this calls readRaw___();
    float readFloatAccelX(void);
    float readFloatAccelY(void);
    float readFloatAccelZ(void);

	//ADC related calls
	uint16_t read10bitADC1( void );
	uint16_t read10bitADC2( void );
	uint16_t read10bitADC3( void );
	
	//FIFO stuff
	void fifoBegin( void );
	void fifoClear( void );
	uint8_t fifoGetStatus( void );
	void fifoStartRec();
	void fifoEnd( void );
	
	float calcAccel( int16_t );
	
private:

};

class LIS3DSH : public LIS3DHCore
{
public:
	//IMU settings
	SensorSettings settings;

	//Error checking
	uint16_t allOnesCounter;
	uint16_t nonSuccessCounter;

	//Constructor generates default SensorSettings.
	//(over-ride after construction if desired)
	LIS3DSH( uint8_t busType = I2C_MODE, uint8_t inputArg = LIS3DSH_ADDR_SEL_LOW);
	//~LIS3DSH() = default;
	
	//Call to apply SensorSettings
	status_t begin( void );
	void applySettings( void );

	//Returns the raw bits from the sensor cast as 16-bit signed integers
	int16_t readRawAccelX( void );
	int16_t readRawAccelY( void );
	int16_t readRawAccelZ( void );

	//Returns the values as floats.  Inside, this calls readRaw___();
	float readFloatAccelX( void );
	float readFloatAccelY( void );
	float readFloatAccelZ( void );

	//ADC related calls
	uint16_t read10bitADC1( void );
	uint16_t read10bitADC2( void );
	uint16_t read10bitADC3( void );
	
	//FIFO stuff
	void fifoBegin( void );
	void fifoClear( void );
	uint8_t fifoGetStatus( void );
	void fifoStartRec();
	void fifoEnd( void );
	
	float calcAccel( int16_t );
	
private:

};

// Device Registers
#define LIS3DH_STATUS_REG_AUX 0x07
#define LIS3DH_OUT_ADC1_L 0x08
#define LIS3DH_OUT_ADC1_H 0x09
#define LIS3DH_OUT_ADC2_L 0x0A
#define LIS3DH_OUT_ADC2_H 0x0B
#define LIS3DH_OUT_ADC3_L 0x0C
#define LIS3DH_OUT_ADC3_H 0x0D
#define LIS3DH_INT_COUNTER_REG 0x0E
#define LIS3DH_WHO_AM_I 0x0F

#define LIS3DH_TEMP_CFG_REG 0x1F
#define LIS3DH_CTRL_REG1 0x20
#define LIS3DH_CTRL_REG2 0x21
#define LIS3DH_CTRL_REG3 0x22
#define LIS3DH_CTRL_REG4 0x23
#define LIS3DH_CTRL_REG5 0x24
#define LIS3DH_CTRL_REG6 0x25
#define LIS3DH_REFERENCE 0x26
#define LIS3DH_STATUS_REG2 0x27
#define LIS3DH_OUT_X_L 0x28
#define LIS3DH_OUT_X_H 0x29
#define LIS3DH_OUT_Y_L 0x2A
#define LIS3DH_OUT_Y_H 0x2B
#define LIS3DH_OUT_Z_L 0x2C
#define LIS3DH_OUT_Z_H 0x2D
#define LIS3DH_FIFO_CTRL_REG 0x2E
#define LIS3DH_FIFO_SRC_REG 0x2F
#define LIS3DH_INT1_CFG 0x30
#define LIS3DH_INT1_SRC 0x31
#define LIS3DH_INT1_THS 0x32
#define LIS3DH_INT1_DURATION 0x33

#define LIS3DH_CLICK_CFG 0x38
#define LIS3DH_CLICK_SRC 0x39
#define LIS3DH_CLICK_THS 0x3A
#define LIS3DH_TIME_LIMIT 0x3B
#define LIS3DH_TIME_LATENCY 0x3C
#define LIS3DH_TIME_WINDOW 0x3D

#define LIS3DSH_OUT_T					0x0C
#define LIS3DSH_INFO1					0x0D
#define LIS3DSH_INFO2					0x0E
#define LIS3DSH_WHO_AM_I	            0x0F
#define LIS3DSH_OFF_X					0x10
#define LIS3DSH_OFF_Y					0x11
#define LIS3DSH_OFF_Z					0x12
#define LIS3DSH_CS_X					0x13
#define LIS3DSH_CS_Y					0x14
#define LIS3DSH_CS_Z					0x15

#define LIS3DSH_LC_L					0x16
#define LIS3DSH_LC_H					0x17

#define LIS3DSH_STAT					0x18

#define LIS3DSH_PEAK1					0x19
#define LIS3DSH_PEAK2					0x1A

#define LIS3DSH_VFC_1					0x1B
#define LIS3DSH_VFC_2					0x1C
#define LIS3DSH_VFC_3					0x1D
#define LIS3DSH_VFC_4					0x1E

#define LIS3DSH_THRS3					0x1F

#define LIS3DSH_CTRL_REG4              0x20
#define LIS3DSH_CTRL_REG1              0x21
#define LIS3DSH_CTRL_REG2              0x22
#define LIS3DSH_CTRL_REG3              0x23
#define LIS3DSH_CTRL_REG5              0x24
#define LIS3DSH_CTRL_REG6              0x25

#define LIS3DSH_STATUS					0x27

#define LIS3DSH_OUT_X_L                0x28
#define LIS3DSH_OUT_X_H                0x29
#define LIS3DSH_OUT_Y_L                0x2A
#define LIS3DSH_OUT_Y_H                0x2B
#define LIS3DSH_OUT_Z_L                0x2C
#define LIS3DSH_OUT_Z_H                0x2D

#define LIS3DSH_FIFO_CTRL				0x2E
#define LIS3DSH_FIFO_SRC				0x2F

// Very annoying to me that these 16 registers are enumerated in the datasheet
// in decimal instead of hex.
#define LIS3DSH_ST1_1					0x40
#define LIS3DSH_ST1_2					0x41
#define LIS3DSH_ST1_3					0x42
#define LIS3DSH_ST1_4					0x43
#define LIS3DSH_ST1_5					0x44
#define LIS3DSH_ST1_6					0x45
#define LIS3DSH_ST1_7					0x46
#define LIS3DSH_ST1_8					0x47
#define LIS3DSH_ST1_9					0x48
#define LIS3DSH_ST1_10					0x49
#define LIS3DSH_ST1_11					0x4A
#define LIS3DSH_ST1_12					0x4B
#define LIS3DSH_ST1_13					0x4C
#define LIS3DSH_ST1_14					0x4D
#define LIS3DSH_ST1_15					0x4E
#define LIS3DSH_ST1_16					0x4F

#define LIS3DSH_TIM4_1					0x50
#define LIS3DSH_TIM3_1					0x51
#define LIS3DSH_TIM2_1_L				0x52
#define LIS3DSH_TIM2_1_H				0x53
#define LIS3DSH_TIM1_1_L				0x54
#define LIS3DSH_TIM1_1_H				0x55

#define LIS3DSH_THRS2_1					0x56
#define LIS3DSH_THRS1_1					0x57

#define LIS3DSH_MASK1_B					0x59
#define LIS3DSH_MASK1_A					0x5A
#define LIS3DSH_SETT1					0x5B
#define LIS3DSH_PR1						0x5C
#define LIS3DSH_TC1_L					0x5D
#define LIS3DSH_TC1_H					0x5E
#define LIS3DSH_OUTS1					0x5F

#define LIS3DSH_ST2_1					0x60
#define LIS3DSH_ST2_2					0x61
#define LIS3DSH_ST2_3					0x62
#define LIS3DSH_ST2_4					0x63
#define LIS3DSH_ST2_5					0x64
#define LIS3DSH_ST2_6					0x65
#define LIS3DSH_ST2_7					0x66
#define LIS3DSH_ST2_8					0x67
#define LIS3DSH_ST2_9					0x68
#define LIS3DSH_ST2_10					0x69
#define LIS3DSH_ST2_11					0x6A
#define LIS3DSH_ST2_12					0x6B
#define LIS3DSH_ST2_13					0x6C
#define LIS3DSH_ST2_14					0x6D
#define LIS3DSH_ST2_15					0x6E
#define LIS3DSH_ST2_16					0x6F

#define LIS3DSH_TIM4_2					0x70
#define LIS3DSH_TIM3_2					0x71
#define LIS3DSH_TIM2_2_L				0x72
#define LIS3DSH_TIM2_2_H				0x73
#define LIS3DSH_TIM1_2_L				0x74
#define LIS3DSH_TIM1_2_H				0x75

#define LIS3DSH_THRS2_2					0x76
#define LIS3DSH_THRS1_2					0x77

#define LIS3DSH_DES2					0x78

#define LIS3DSH_MASK2_B					0x79
#define LIS3DSH_MASK2_A					0x7A
#define LIS3DSH_SETT2					0x7B
#define LIS3DSH_PR2						0x7C
#define LIS3DSH_TC2_L					0x7D
#define LIS3DSH_TC2_H					0x7E
#define LIS3DSH_OUTS2					0x7F

#endif  // End of __LIS3DH_IMU_H__ definition check
