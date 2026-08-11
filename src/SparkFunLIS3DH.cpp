/******************************************************************************
SparkFunLIS3DH.cpp
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
//Use VERBOSE_SERIAL to add debug serial to an existing Serial object.
//Note:  Use of VERBOSE_SERIAL adds delays surround RW ops, and should not be used
//for functional testing.
//#define VERBOSE_SERIAL

//See SparkFunLIS3DH.h for additional topology notes.

#include "SparkFunLIS3DH.h"
#include "stdint.h"

#include "Wire.h"
#include "SPI.h"

//****************************************************************************//
//
//  LIS3DHCore functions.
//
//  Construction arguments:
//  ( uint8_t busType, uint8_t inputArg ),
//
//    where inputArg is address for I2C_MODE and chip select pin
//    number for SPI_MODE
//
//  For SPI, construct LIS3DHCore myIMU(SPI_MODE, 10);
//  For I2C, construct LIS3DHCore myIMU(I2C_MODE, 0x6B);
//
//  Default construction is I2C mode, address 0x6B.
//
//****************************************************************************//
LIS3DHCore::LIS3DHCore( uint8_t busType, uint8_t inputArg ) : commInterface(I2C_MODE), I2CAddress(0x19), chipSelectPin(10)
{
	commInterface = busType;
	if( commInterface == I2C_MODE )
	{
		I2CAddress = inputArg;
	}
	if( commInterface == SPI_MODE )
	{
		chipSelectPin = inputArg;
	}

}

status_t LIS3DHCore::beginCore(void)
{
	status_t returnError = IMU_SUCCESS;

	switch (commInterface) {

	case I2C_MODE:
		Wire.begin();
		break;

	case SPI_MODE:
#if defined(ARDUINO_ARCH_ESP32)
		// initalize the chip select pins:
		pinMode(chipSelectPin, OUTPUT);
		digitalWrite(chipSelectPin, HIGH);
		SPI.begin();
		SPI.setFrequency(1000000);
		// Data is read and written MSb first.
		SPI.setBitOrder(SPI_MSBFIRST);
		// Like the standard arduino/teensy comment below, mode0 seems wrong according to standards
		// but conforms to the timing diagrams when used for the ESP32
		SPI.setDataMode(SPI_MODE0);

#elif defined(__MK20DX256__)
		// initalize the chip select pins:
		pinMode(chipSelectPin, OUTPUT);
		digitalWrite(chipSelectPin, HIGH);
		// start the SPI library:
		SPI.begin();
		// Maximum SPI frequency is 10MHz, could divide by 2 here:
		SPI.setClockDivider(SPI_CLOCK_DIV4);
		// Data is read and written MSb first.
		SPI.setBitOrder(MSBFIRST);
		// Data is captured on rising edge of clock (CPHA = 0)
		// Base value of the clock is HIGH (CPOL = 1)

		// MODE0 for Teensy 3.1 operation
		SPI.setDataMode(SPI_MODE0);
#else
// probably __AVR__
		// initalize the chip select pins:
		pinMode(chipSelectPin, OUTPUT);
		digitalWrite(chipSelectPin, HIGH);
		// start the SPI library:
		SPI.begin();
		// Maximum SPI frequency is 10MHz, could divide by 2 here:
		SPI.setClockDivider(SPI_CLOCK_DIV4);
		// Data is read and written MSb first.
		SPI.setBitOrder(MSBFIRST);
		// Data is captured on rising edge of clock (CPHA = 0)
		// Base value of the clock is HIGH (CPOL = 1)

		// MODE3 for 328p operation
		SPI.setDataMode(SPI_MODE3);

#endif
		break;
	default:
		break;
	}

	//Spin for a few ms
	volatile uint8_t temp = 0;
	for( uint16_t i = 0; i < 10000; i++ )
	{
		temp++;
	}

	//Check the ID register to determine if the operation was a success.
	uint8_t readCheck;
	// LIS3DH_WHO_AM_I is the same register address as LIS3DSH_WHO_AM_I so 
	// this line handles both ooptions just fine
	readRegister(&readCheck, LIS3DH_WHO_AM_I);
	// But this line does need to change as the 3dSh has a different ID
	// The 3dh is 0x33, 3dSh is 0x3f
	if( readCheck != 0x33 && readCheck != 0x3F)
	{
		returnError = IMU_HW_ERROR;
	}

	return returnError;
}

//****************************************************************************//
//
//  ReadRegisterRegion
//
//  Parameters:
//    *outputPointer -- Pass &variable (base address of) to save read data to
//    offset -- register to read
//    length -- number of bytes to read
//
//  Note:  Does not know if the target memory space is an array or not, or
//    if there is the array is big enough.  if the variable passed is only
//    two bytes long and 3 bytes are requested, this will over-write some
//    other memory!
//
//****************************************************************************//
status_t LIS3DHCore::readRegisterRegion(uint8_t *outputPointer , uint8_t offset, uint8_t length)
{
	status_t returnError = IMU_SUCCESS;

	//define pointer that will point to the external space
	uint8_t i = 0;
	uint8_t c = 0;
	uint8_t tempFFCounter = 0;

	switch (commInterface) {

	case I2C_MODE:
		Wire.beginTransmission(I2CAddress);
		offset |= 0x80; //turn auto-increment bit on, bit 7 for I2C
		Wire.write(offset);
		if( Wire.endTransmission() != 0 )
		{
			returnError = IMU_HW_ERROR;
		}
		else  //OK, all worked, keep going
		{
			// request 6 bytes from slave device
			Wire.requestFrom(I2CAddress, length);
			while ( (Wire.available()) && (i < length))  // slave may send less than requested
			{
				c = Wire.read(); // receive a byte as character
				*outputPointer = c;
				outputPointer++;
				i++;
			}
		}
		break;

	case SPI_MODE:
		// take the chip select low to select the device:
		digitalWrite(chipSelectPin, LOW);
		// send the device the register you want to read:
		SPI.transfer(offset | 0x80 | 0x40);  //Ored with "read request" bit and "auto increment" bit
		while ( i < length ) // slave may send less than requested
		{
			c = SPI.transfer(0x00); // receive a byte as character
			if( c == 0xFF )
			{
				//May have problem
				tempFFCounter++;
			}
			*outputPointer = c;
			outputPointer++;
			i++;
		}
		if( tempFFCounter == i )
		{
			//Ok, we've recieved all ones, report
			returnError = IMU_ALL_ONES_WARNING;
		}
		// take the chip select high to de-select:
		digitalWrite(chipSelectPin, HIGH);
		break;

	default:
		break;
	}

	return returnError;
}

//****************************************************************************//
//
//  ReadRegister
//
//  Parameters:
//    *outputPointer -- Pass &variable (address of) to save read data to
//    offset -- register to read
//
//****************************************************************************//
status_t LIS3DHCore::readRegister(uint8_t* outputPointer, uint8_t offset) {
	//Return value
	uint8_t result;
	uint8_t numBytes = 1;
	status_t returnError = IMU_SUCCESS;

	switch (commInterface) {

	case I2C_MODE:
		Wire.beginTransmission(I2CAddress);
		Wire.write(offset);
		if( Wire.endTransmission() != 0 )
		{
			returnError = IMU_HW_ERROR;
		}
		Wire.requestFrom(I2CAddress, numBytes);
		while ( Wire.available() ) // slave may send less than requested
		{
			result = Wire.read(); // receive a byte as a proper uint8_t
		}
		break;

	case SPI_MODE:
		// take the chip select low to select the device:
		digitalWrite(chipSelectPin, LOW);
		// send the device the register you want to read:
		SPI.transfer(offset | 0x80);  //Ored with "read request" bit
		// send a value of 0 to read the first byte returned:
		result = SPI.transfer(0x00);
		// take the chip select high to de-select:
		digitalWrite(chipSelectPin, HIGH);
		
		if( result == 0xFF )
		{
			//we've recieved all ones, report
			returnError = IMU_ALL_ONES_WARNING;
		}
		break;

	default:
		break;
	}

	*outputPointer = result;
	return returnError;
}

//****************************************************************************//
//
//  readRegisterInt16
//
//  Parameters:
//    *outputPointer -- Pass &variable (base address of) to save read data to
//    offset -- register to read
//
//****************************************************************************//
status_t LIS3DHCore::readRegisterInt16( int16_t* outputPointer, uint8_t offset )
{
	{
		//offset |= 0x80; //turn auto-increment bit on
		uint8_t myBuffer[2];
		status_t returnError = readRegisterRegion(myBuffer, offset, 2);  //Does memory transfer
		int16_t output = (int16_t)myBuffer[0] | int16_t(myBuffer[1] << 8);
		*outputPointer = output;
		return returnError;
	}

}

//****************************************************************************//
//
//  writeRegister
//
//  Parameters:
//    offset -- register to write
//    dataToWrite -- 8 bit data to write to register
//
//****************************************************************************//
status_t LIS3DHCore::writeRegister(uint8_t offset, uint8_t dataToWrite) {
	status_t returnError = IMU_SUCCESS;
	switch (commInterface) {
	case I2C_MODE:
		//Write the byte
		Wire.beginTransmission(I2CAddress);
		Wire.write(offset);
		Wire.write(dataToWrite);
		if( Wire.endTransmission() != 0 )
		{
			returnError = IMU_HW_ERROR;
		}
		break;

	case SPI_MODE:
		// take the chip select low to select the device:
		digitalWrite(chipSelectPin, LOW);
		// send the device the register you want to read:
		SPI.transfer(offset);
		// send a value of 0 to read the first byte returned:
		SPI.transfer(dataToWrite);
		// decrement the number of bytes left to read:
		// take the chip select high to de-select:
		digitalWrite(chipSelectPin, HIGH);
		break;
		
		//No way to check error on this write (Except to read back but that's not reliable)

	default:
		break;
	}

	return returnError;
}

//****************************************************************************//
//
//  Main user class -- wrapper for the core class + maths
//
//  Construct with same rules as the core ( uint8_t busType, uint8_t inputArg )
//
//****************************************************************************//
LIS3DH::LIS3DH( uint8_t busType, uint8_t inputArg ) : LIS3DHCore( busType, inputArg )
{
	//Construct with these default settings
	//ADC stuff
	settings.adcEnabled = 1;
	
	//Temperature settings
	settings.tempEnabled = 1;

	//Accelerometer settings
	settings.accelSampleRate = 50;  //Hz.  Can be: 0,1,10,25,50,100,200,400,1600,5000 Hz
	settings.accelRange = 2;      //Max G force readable.  Can be: 2, 4, 8, 16

	settings.xAccelEnabled = 1;
	settings.yAccelEnabled = 1;
	settings.zAccelEnabled = 1;

	//FIFO control settings
	settings.fifoEnabled = 0;
	settings.fifoThreshold = 20;  //Can be 0 to 32
	settings.fifoMode = 0;  //FIFO mode.
  
	allOnesCounter = 0;
	nonSuccessCounter = 0;

}

//****************************************************************************//
//
//  Begin
//
//  This starts the lower level begin, then applies settings
//
//****************************************************************************//
status_t LIS3DH::begin( void )
{
	//Begin the inherited core.  This gets the physical wires connected
	status_t returnError = beginCore();

	applySettings();
	
	return returnError;
}

//****************************************************************************//
//
//  Configuration section
//
//  This uses the stored SensorSettings to start the IMU
//  Use statements such as "myIMU.settings.commInterface = SPI_MODE;" or
//  "myIMU.settings.accelEnabled = 1;" to configure before calling .begin();
//
//****************************************************************************//
void LIS3DH::applySettings( void )
{
	uint8_t dataToWrite = 0;  //Temporary variable

	//Build TEMP_CFG_REG
	dataToWrite = 0; //Start Fresh!
	dataToWrite = ((settings.tempEnabled & 0x01) << 6) | ((settings.adcEnabled & 0x01) << 7);
	//Now, write the patched together data
#ifdef VERBOSE_SERIAL
	Serial.print("LIS3DH_TEMP_CFG_REG: 0x");
	Serial.println(dataToWrite, HEX);
#endif
	writeRegister(LIS3DH_TEMP_CFG_REG, dataToWrite);
	
	//Build CTRL_REG1
	dataToWrite = 0; //Start Fresh!
	//  Convert ODR
	switch(settings.accelSampleRate)
	{
		case 1:
		dataToWrite |= (0x01 << 4);
		break;
		case 10:
		dataToWrite |= (0x02 << 4);
		break;
		case 25:
		dataToWrite |= (0x03 << 4);
		break;
		case 50:
		dataToWrite |= (0x04 << 4);
		break;
		case 100:
		dataToWrite |= (0x05 << 4);
		break;
		case 200:
		dataToWrite |= (0x06 << 4);
		break;
		default:
		case 400:
		dataToWrite |= (0x07 << 4);
		break;
		case 1600:
		dataToWrite |= (0x08 << 4);
		break;
		case 5000:
		dataToWrite |= (0x09 << 4);
		break;
	}
	
	dataToWrite |= (settings.zAccelEnabled & 0x01) << 2;
	dataToWrite |= (settings.yAccelEnabled & 0x01) << 1;
	dataToWrite |= (settings.xAccelEnabled & 0x01);
	//Now, write the patched together data
#ifdef VERBOSE_SERIAL
	Serial.print("LIS3DH_CTRL_REG1: 0x");
	Serial.println(dataToWrite, HEX);
#endif
	writeRegister(LIS3DH_CTRL_REG1, dataToWrite);

	//Build CTRL_REG4
	dataToWrite = 0; //Start Fresh!
	//  Convert scaling
	switch(settings.accelRange)
	{
		case 2:
		dataToWrite |= (0x00 << 4);
		break;
		case 4:
		dataToWrite |= (0x01 << 4);
		break;
		case 8:
		dataToWrite |= (0x02 << 4);
		break;
		default:
		case 16:
		dataToWrite |= (0x03 << 4);
		break;
	}
	dataToWrite |= 0x80; //set block update
	dataToWrite |= 0x08; //set high resolution
#ifdef VERBOSE_SERIAL
	Serial.print("LIS3DH_CTRL_REG4: 0x");
	Serial.println(dataToWrite, HEX);
#endif
	//Now, write the patched together data
	writeRegister(LIS3DH_CTRL_REG4, dataToWrite);

}
//****************************************************************************//
//
//  Accelerometer section
//
//****************************************************************************//
int16_t LIS3DH::readRawAccelX( void )
{
	int16_t output;
	status_t errorLevel = readRegisterInt16( &output, LIS3DH_OUT_X_L );
	if( errorLevel != IMU_SUCCESS )
	{
		if( errorLevel == IMU_ALL_ONES_WARNING )
		{
			allOnesCounter++;
		}
		else
		{
			nonSuccessCounter++;
		}
	}
	return output;
}
float LIS3DH::readFloatAccelX( void )
{
	float output = calcAccel(readRawAccelX());
	return output;
}

int16_t LIS3DH::readRawAccelY( void )
{
	int16_t output;
	status_t errorLevel = readRegisterInt16( &output, LIS3DH_OUT_Y_L );
	if( errorLevel != IMU_SUCCESS )
	{
		if( errorLevel == IMU_ALL_ONES_WARNING )
		{
			allOnesCounter++;
		}
		else
		{
			nonSuccessCounter++;
		}
	}
	return output;
}

float LIS3DH::readFloatAccelY( void )
{
	float output = calcAccel(readRawAccelY());
	return output;
}

int16_t LIS3DH::readRawAccelZ( void )
{
	int16_t output;
	status_t errorLevel = readRegisterInt16( &output, LIS3DH_OUT_Z_L );
	if( errorLevel != IMU_SUCCESS )
	{
		if( errorLevel == IMU_ALL_ONES_WARNING )
		{
			allOnesCounter++;
		}
		else
		{
			nonSuccessCounter++;
		}
	}
	return output;

}

float LIS3DH::readFloatAccelZ( void )
{
	float output = calcAccel(readRawAccelZ());
	return output;
}

float LIS3DH::calcAccel( int16_t input )
{
	float output;
	switch(settings.accelRange)
	{
		case 2:
		output = (float)input / 15987;
		break;
		case 4:
		output = (float)input / 7840;
		break;
		case 8:
		output = (float)input / 3883;
		break;
		case 16:
		output = (float)input / 1280;
		break;
		default:
		output = 0;
		break;
	}
	return output;
}

//****************************************************************************//
//
//  Accelerometer section
//
//****************************************************************************//
uint16_t LIS3DH::read10bitADC1( void )
{
	int16_t intTemp;
	uint16_t uintTemp;
	readRegisterInt16( &intTemp, LIS3DH_OUT_ADC1_L );
	intTemp = 0 - intTemp;
	uintTemp = intTemp + 32768;
	return uintTemp >> 6;
}

uint16_t LIS3DH::read10bitADC2( void )
{
	int16_t intTemp;
	uint16_t uintTemp;
	readRegisterInt16( &intTemp, LIS3DH_OUT_ADC2_L );
	intTemp = 0 - intTemp;
	uintTemp = intTemp + 32768;
	return uintTemp >> 6;
}

uint16_t LIS3DH::read10bitADC3( void )
{
	int16_t intTemp;
	uint16_t uintTemp;
	readRegisterInt16( &intTemp, LIS3DH_OUT_ADC3_L );
	intTemp = 0 - intTemp;
	uintTemp = intTemp + 32768;
	return uintTemp >> 6;
}

//****************************************************************************//
//
//  FIFO section
//
//****************************************************************************//
void LIS3DH::fifoBegin( void )
{
	uint8_t dataToWrite = 0;  //Temporary variable

	//Build LIS3DH_FIFO_CTRL_REG
	readRegister( &dataToWrite, LIS3DH_FIFO_CTRL_REG ); //Start with existing data
	dataToWrite &= 0x20;//clear all but bit 5
	dataToWrite |= (settings.fifoMode & 0x03) << 6; //apply mode
	dataToWrite |= (settings.fifoThreshold & 0x1F); //apply threshold
	//Now, write the patched together data
#ifdef VERBOSE_SERIAL
	Serial.print("LIS3DH_FIFO_CTRL_REG: 0x");
	Serial.println(dataToWrite, HEX);
#endif
	writeRegister(LIS3DH_FIFO_CTRL_REG, dataToWrite);

	//Build CTRL_REG5
	readRegister( &dataToWrite, LIS3DH_CTRL_REG5 ); //Start with existing data
	dataToWrite &= 0xBF;//clear bit 6
	dataToWrite |= (settings.fifoEnabled & 0x01) << 6;
	//Now, write the patched together data
#ifdef VERBOSE_SERIAL
	Serial.print("LIS3DH_CTRL_REG5: 0x");
	Serial.println(dataToWrite, HEX);
#endif
	writeRegister(LIS3DH_CTRL_REG5, dataToWrite);
}

void LIS3DH::fifoClear( void ) {
	//Drain the fifo data and dump it
	while( (fifoGetStatus() & 0x20 ) == 0 ) {
		readRawAccelX();
		readRawAccelY();
		readRawAccelZ();
	}
}

void LIS3DH::fifoStartRec( void )
{
	uint8_t dataToWrite = 0;  //Temporary variable
	
	//Turn off...
	readRegister( &dataToWrite, LIS3DH_FIFO_CTRL_REG ); //Start with existing data
	dataToWrite &= 0x3F;//clear mode
#ifdef VERBOSE_SERIAL
	Serial.print("LIS3DH_FIFO_CTRL_REG: 0x");
	Serial.println(dataToWrite, HEX);
#endif
	writeRegister(LIS3DH_FIFO_CTRL_REG, dataToWrite);	
	//  ... then back on again
	readRegister( &dataToWrite, LIS3DH_FIFO_CTRL_REG ); //Start with existing data
	dataToWrite &= 0x3F;//clear mode
	dataToWrite |= (settings.fifoMode & 0x03) << 6; //apply mode
	//Now, write the patched together data
#ifdef VERBOSE_SERIAL
	Serial.print("LIS3DH_FIFO_CTRL_REG: 0x");
	Serial.println(dataToWrite, HEX);
#endif
	writeRegister(LIS3DH_FIFO_CTRL_REG, dataToWrite);
}

uint8_t LIS3DH::fifoGetStatus( void )
{
	//Return some data on the state of the fifo
	uint8_t tempReadByte = 0;
	readRegister(&tempReadByte, LIS3DH_FIFO_SRC_REG);
#ifdef VERBOSE_SERIAL
	Serial.print("LIS3DH_FIFO_SRC_REG: 0x");
	Serial.println(tempReadByte, HEX);
#endif
	return tempReadByte;  
}

void LIS3DH::fifoEnd( void )
{
	uint8_t dataToWrite = 0;  //Temporary variable

	//Turn off...
	readRegister( &dataToWrite, LIS3DH_FIFO_CTRL_REG ); //Start with existing data
	dataToWrite &= 0x3F;//clear mode
#ifdef VERBOSE_SERIAL
	Serial.print("LIS3DH_FIFO_CTRL_REG: 0x");
	Serial.println(dataToWrite, HEX);
#endif
	writeRegister(LIS3DH_FIFO_CTRL_REG, dataToWrite);	
}

// ******************************************************************
// LIS3dSh section
// ******************************************************************

//****************************************************************************//
//
//  Main user class -- wrapper for the core class + maths
//
//  Construct with same rules as the core ( uint8_t busType, uint8_t inputArg )
//
////****************************************************************************//
LIS3DSH::LIS3DSH( uint8_t busType, uint8_t inputArg ) : LIS3DHCore( busType, inputArg )
{
	// Use device defaults for settings except where marked.

	//Accelerometer settings
	// Device default is 0, CHANGING to 100 Hz
	settings.accelSampleRate = 100;  //Hz.  Can be: 0,1,10,25,50,100,200,400,1600,5000 Hz
	settings.blockDataUpdate = 0;

	settings.accelRange = 2;      //Max G force readable.  Can be: 2, 4, 8, 16

	settings.xAccelEnabled = 1;
	settings.yAccelEnabled = 1;
	settings.zAccelEnabled = 1;

	
	settings.dataReadySignal = 0;
	settings.intHighOrLow = 0;
	settings.intLatches = 0;
	// Changing defaults for interrupts
	settings.int1Enabled = 1;
	settings.int2Enabled = 1;
	settings.vectorFilterEnabled = 0;

	// Using a non-default filter here to go with the default sampling
	// rate of 100 Hz.
	settings.antiAliasingBandwidth = 50; //Hz

	settings.spiWireMode = 0;

	//FIFO control settings
	settings.fifoEnabled = 0;
	settings.fifoLimitToWatermark = 0; 
	settings.fifoWatermarkIntEnabled = 0;
	settings.fifoOverrunIntEnabled = 0;
	settings.fifoEmptyIntEnabled = 0;
	settings.autoIncrement = 1;
	
	settings.fifoMode = 0; 
  	settings.fifoWatermarkLevel = 31;
	
	// SM2 Filter settings;
	settings.constantShiftX = 0;
	settings.constantShiftY = 0;
	settings.constantShiftZ = 0;

	settings.vectorFilterCoefficient_1 = 0;
	settings.vectorFilterCoefficient_2 = 0;
	settings.vectorFilterCoefficient_3 = 0;
	settings.vectorFilterCoefficient_4 = 0;

	settings.threshold3 = 0;

	allOnesCounter = 0;
	nonSuccessCounter = 0;

	//
	// State machine 1 settings
	//
	sm1.settings.enabled = 0;
	sm1.settings.hysteresis = 0;
	sm1.settings.intPin = 0;
	
	sm1.settings.threshold1 = 0;
	sm1.settings.threshold2 = 0;
	
	sm1.settings.maskA = 0;
	sm1.settings.maskB = 0;

	sm1.settings.peakDetectionEnabled = 0;
	sm1.settings.resetOnThreshold3MaskA = 0;
	sm1.settings.resetOnThreshold3MaskB = 0;
	sm1.settings.absThresholds = 0;
	sm1.settings.sitr = 0;
	//
	// State machine 2 settings
	settings.sm2UseDiff = 0;
	settings.sm2DiffMode = 0;
	
	sm2.settings.enabled = 0;
	sm2.settings.hysteresis = 0;
	sm2.settings.intPin = 0;
	
	sm2.settings.threshold1 = 0;
	sm2.settings.threshold2 = 0;
	
	sm2.settings.maskA = 0;
	sm2.settings.maskB = 0;

	sm2.settings.peakDetectionEnabled = 0;
	sm2.settings.resetOnThreshold3MaskA = 0;
	sm2.settings.resetOnThreshold3MaskB = 0;
	sm2.settings.absThresholds = 0;
	sm2.settings.sitr = 0;	
}
//
////****************************************************************************//
////
////  Begin
////
////  This starts the lower level begin, then applies settings
////
////****************************************************************************//
status_t LIS3DSH::begin( void )
{
	//Begin the inherited core.  This gets the physical wires connected
	status_t returnError = beginCore();

	applyGlobalSettings();
	
	return returnError;
}

////****************************************************************************//
////
////  Configuration section
////
////  This uses the stored SensorSettings to start the IMU
////  Use statements such as "myIMU.settings.commInterface = SPI_MODE;" or
////  "myIMU.settings.accelEnabled = 1;" to configure before calling .begin();
////
////****************************************************************************//
// This will likely need to change significantly
void LIS3DSH::applyGlobalSettings( void )
{
	uint8_t dataToWrite = 0;  //Temporary variable
	uint8_t optionCode = 0;

	//Build TEMP_CFG_REG
	dataToWrite = 0; //Start Fresh!

	// Constant shift vars
	writeRegister(LIS3DSH_CS_X,settings.constantShiftX);
	writeRegister(LIS3DSH_CS_Y,settings.constantShiftY);
	writeRegister(LIS3DSH_CS_Z,settings.constantShiftZ);
#ifdef VERBOSE_SERIAL
	Serial.print("LIS3DSH_CS_X: 0x");
	Serial.println(settings.constantShiftX, HEX);
	Serial.print("LIS3DSH_CS_Y: 0x");
	Serial.println(settings.constantShiftY, HEX);
	Serial.print("LIS3DSH_CS_Z: 0x");
	Serial.println(settings.constantShiftZ, HEX);
#endif
	// Vector filter coefficients
	writeRegister(LIS3DSH_VFC_1,settings.vectorFilterCoefficient_1);
	writeRegister(LIS3DSH_VFC_2,settings.vectorFilterCoefficient_2);
	writeRegister(LIS3DSH_VFC_3,settings.vectorFilterCoefficient_3);
	writeRegister(LIS3DSH_VFC_4,settings.vectorFilterCoefficient_4);
#ifdef VERBOSE_SERIAL
	Serial.print("LIS3DSH_VFC_1: 0x");
	Serial.println(settings.vectorFilterCoefficient_1, HEX);
	Serial.print("LIS3DSH_VFC_2: 0x");
	Serial.println(settings.vectorFilterCoefficient_2, HEX);
	Serial.print("LIS3DSH_VFC_3: 0x");
	Serial.println(settings.vectorFilterCoefficient_3, HEX);
	Serial.print("LIS3DSH_VFC_4: 0x");
	Serial.println(settings.vectorFilterCoefficient_4, HEX);
#endif

	// THRS3
#ifdef VERBOSE_SERIAL
	Serial.print("LIS3DSH_THRS3: 0x");
	Serial.println(settings.threshold3, HEX);
#endif
	writeRegister(LIS3DSH_THRS3,settings.threshold3);

	// CTRL_REG4
	// output data rate
	// block data update flag
	// x,y,z enable
	dataToWrite = 0;
	switch (settings.accelSampleRate) {
		case 0:
			optionCode = 0;
			break;
		case 3:
			optionCode = 0x01;
			break;
		case 6:
			optionCode = 0x02;
			break;
		case 12:
			optionCode = 0x03;
			break;
		case 25:
			optionCode = 0x04;
			break;
		case 50:
			optionCode = 0x05;
			break;
		default:
#ifdef VERBOSE_SERIAL
			Serial.println("Unrecognized option for ODR, defaulting to 100 Hz");
#endif
		case 100:
			optionCode = 0x6;
			break;
		case 400:
			optionCode = 0x7;
			break;
		case 800:
			optionCode = 0x08;
			break;
		case 1600:
			optionCode = 0x09;
			break;
	}
	dataToWrite |= (optionCode << 4);

	dataToWrite |= settings.blockDataUpdate << 3;
	dataToWrite |= settings.zAccelEnabled << 2;
	dataToWrite |= settings.yAccelEnabled << 1;
	dataToWrite |= settings.xAccelEnabled;
#ifdef VERBOSE_SERIAL
	Serial.print("LIS3DSH_CTRL_REG4: 0x");
	Serial.println(dataToWrite, HEX);
#endif
	writeRegister(LIS3DSH_CTRL_REG4,dataToWrite);

	// CTRL_REG1
	dataToWrite = 0x00;
	dataToWrite |= (0x07 & sm1.settings.hysteresis) << 5;
	dataToWrite |= sm1.settings.intPin << 3;
	dataToWrite |= sm1.settings.enabled;

#ifdef VERBOSE_SERIAL
	Serial.print("LIS3DSH_CTRL_REG1: 0x");
	Serial.println(dataToWrite, HEX);
#endif
	writeRegister(LIS3DSH_CTRL_REG1, dataToWrite);

	// CTRL_REG2
	dataToWrite = 0x00;
	dataToWrite |= (0x07 & sm2.settings.hysteresis) << 5;
	dataToWrite |= sm2.settings.intPin << 3;
	dataToWrite |= sm2.settings.enabled;

#ifdef VERBOSE_SERIAL
	Serial.print("LIS3DSH_CTRL_REG2: 0x");
	Serial.println(dataToWrite, HEX);
#endif
	writeRegister(LIS3DSH_CTRL_REG2, dataToWrite);

	// CTRL_REG3
	dataToWrite = 0x00;
	dataToWrite |= settings.dataReadySignal << 7;
	dataToWrite |= settings.intHighOrLow << 6;
	dataToWrite |= settings.intLatches << 5;
	dataToWrite |= settings.int2Enabled << 4;
	dataToWrite |= settings.int1Enabled << 3;
	dataToWrite |= settings.vectorFilterEnabled <<2;

#ifdef VERBOSE_SERIAL
	Serial.print("LIS3DSH_CTRL_REG3: 0x");
	Serial.println(dataToWrite, HEX);
#endif
	writeRegister(LIS3DSH_CTRL_REG3, dataToWrite);
	
	// CTRL_REG5
	dataToWrite = 0x00;
	switch (settings.antiAliasingBandwidth) {
		default:
#ifdef VERBOSE_SERIAL
			Serial.println("Unrecognized anti-aliasing filter frequency, defaulting to 50 Hz");
#endif
		case 50:
			optionCode = 0x3;
			break;
		case 200:
			optionCode = 0x1;
			break;
		case 400:
			optionCode = 0x2;
			break;
		case 800:
			optionCode = 0x0;
			break;
	}
	dataToWrite |= optionCode << 6;
	
	switch (settings.accelRange) {
		default:
#ifdef VERBOSE_SERIAL
			Serial.println("Unrecognized full scale range, defaulting to 2g");
#endif
		case 2:
			optionCode = 0x0;
			break;
		case 4:
			optionCode = 0x1;
			break;
		case 6:
			optionCode = 0x2;
			break;
		case 8:
			optionCode = 3;
			break;
		case 16:
			optionCode = 4;
			break;
	}
	dataToWrite |= optionCode << 3;
	dataToWrite |= settings.spiWireMode;
#ifdef VERBOSE_SERIAL
	Serial.print("LIS3DSH_CTRL_REG5: 0x");
	Serial.println(dataToWrite, HEX);
#endif
	writeRegister(LIS3DSH_CTRL_REG5, dataToWrite);

	// CTRL_REG6
	dataToWrite = 0x00;
	dataToWrite |= settings.fifoEnabled << 6;
	dataToWrite |= settings.fifoLimitToWatermark << 5;
	dataToWrite |= settings.autoIncrement << 4;
	dataToWrite |= settings.fifoEmptyIntEnabled << 3;
	dataToWrite |= settings.fifoWatermarkIntEnabled << 2;
	dataToWrite |= settings.fifoOverrunIntEnabled << 1;
#ifdef VERBOSE_SERIAL
	Serial.print("LIS3DSH_CTRL_REG6: 0x");
	Serial.println(dataToWrite, HEX);
#endif
	writeRegister(LIS3DSH_CTRL_REG6, dataToWrite);

	// FIFO_CTRL
	dataToWrite = 0x00;
	dataToWrite |= settings.fifoMode << 5;
	dataToWrite |= (settings.fifoWatermarkLevel & 0x1F);
#ifdef VERBOSE_SERIAL
	Serial.print("LIS3DSH_FIFO_CTRL: 0x");
	Serial.println(dataToWrite, HEX);
#endif
	writeRegister(LIS3DSH_FIFO_CTRL, dataToWrite);

}
////****************************************************************************//
////
////  Accelerometer section
////
////****************************************************************************//
int16_t LIS3DSH::readRawAccelX( void )
{
	int16_t output;
	status_t errorLevel = readRegisterInt16( &output, LIS3DSH_OUT_X_L );
	if( errorLevel != IMU_SUCCESS )
	{
		if( errorLevel == IMU_ALL_ONES_WARNING )
		{
			allOnesCounter++;
		}
		else
		{
			nonSuccessCounter++;
		}
	}
	return output;
}
float LIS3DSH::readFloatAccelX( void )
{
	float output = calcAccel(readRawAccelX());
	return output;
}

int16_t LIS3DSH::readRawAccelY( void )
{
	int16_t output;
	status_t errorLevel = readRegisterInt16( &output, LIS3DSH_OUT_Y_L );
	if( errorLevel != IMU_SUCCESS )
	{
		if( errorLevel == IMU_ALL_ONES_WARNING )
		{
			allOnesCounter++;
		}
		else
		{
			nonSuccessCounter++;
		}
	}
	return output;
}

float LIS3DSH::readFloatAccelY( void )
{
	float output = calcAccel(readRawAccelY());
	return output;
}

int16_t LIS3DSH::readRawAccelZ( void )
{
	int16_t output;
	status_t errorLevel = readRegisterInt16( &output, LIS3DSH_OUT_Z_L );
	if( errorLevel != IMU_SUCCESS )
	{
		if( errorLevel == IMU_ALL_ONES_WARNING )
		{
			allOnesCounter++;
		}
		else
		{
			nonSuccessCounter++;
		}
	}
	return output;

}

float LIS3DSH::readFloatAccelZ( void )
{
	float output = calcAccel(readRawAccelZ());
	return output;
}

float LIS3DSH::calcAccel( int16_t input )
{
	float output;
	// The LIS3DH code uses different scale factors that are not
	// obvious. E.g. 15847 or something versus the expected 16384. Why?
	switch(settings.accelRange)
	{
		// max value 2^15 - 1 corresponds to Ng
		case 2:
		output = (float)input/ 16383.5;
		break;
		case 4:
		output = (float)input / 8191.75;
		break;
		case 6:
		output = (float)input / 5461.167;
		break;
		case 8:
		output = (float)input / 4095.875;
		break;
		case 16:
		output = (float)input / 2047.9375;
		break;
		default:
		output = 0;
		break;
	}
	return output;
}
//
////****************************************************************************//
////
////  Accelerometer section
////
////****************************************************************************//
//uint16_t LIS3DSH::read10bitADC1( void )
//{
//	int16_t intTemp;
//	uint16_t uintTemp;
//	readRegisterInt16( &intTemp, LIS3DSH_OUT_ADC1_L );
//	intTemp = 0 - intTemp;
//	uintTemp = intTemp + 32768;
//	return uintTemp >> 6;
//}
//
//uint16_t LIS3DSH::read10bitADC2( void )
//{
//	int16_t intTemp;
//	uint16_t uintTemp;
//	readRegisterInt16( &intTemp, LIS3DSH_OUT_ADC2_L );
//	intTemp = 0 - intTemp;
//	uintTemp = intTemp + 32768;
//	return uintTemp >> 6;
//}
//
//uint16_t LIS3DSH::read10bitADC3( void )
//{
//	int16_t intTemp;
//	uint16_t uintTemp;
//	readRegisterInt16( &intTemp, LIS3DSH_OUT_ADC3_L );
//	intTemp = 0 - intTemp;
//	uintTemp = intTemp + 32768;
//	return uintTemp >> 6;
//}
//
////****************************************************************************//
////
////  FIFO section
////
////****************************************************************************//
//void LIS3DSH::fifoBegin( void )
//{
//	uint8_t dataToWrite = 0;  //Temporary variable
//
//	//Build LIS3DSH_FIFO_CTRL_REG
//	readRegister( &dataToWrite, LIS3DSH_FIFO_CTRL_REG ); //Start with existing data
//	dataToWrite &= 0x20;//clear all but bit 5
//	dataToWrite |= (settings.fifoMode & 0x03) << 6; //apply mode
//	dataToWrite |= (settings.fifoThreshold & 0x1F); //apply threshold
//	//Now, write the patched together data
//#ifdef VERBOSE_SERIAL
//	Serial.print("LIS3DSH_FIFO_CTRL_REG: 0x");
//	Serial.println(dataToWrite, HEX);
//#endif
//	writeRegister(LIS3DSH_FIFO_CTRL_REG, dataToWrite);
//
//	//Build CTRL_REG5
//	readRegister( &dataToWrite, LIS3DSH_CTRL_REG5 ); //Start with existing data
//	dataToWrite &= 0xBF;//clear bit 6
//	dataToWrite |= (settings.fifoEnabled & 0x01) << 6;
//	//Now, write the patched together data
//#ifdef VERBOSE_SERIAL
//	Serial.print("LIS3DSH_CTRL_REG5: 0x");
//	Serial.println(dataToWrite, HEX);
//#endif
//	writeRegister(LIS3DSH_CTRL_REG5, dataToWrite);
//}
//
//void LIS3DSH::fifoClear( void ) {
//	//Drain the fifo data and dump it
//	while( (fifoGetStatus() & 0x20 ) == 0 ) {
//		readRawAccelX();
//		readRawAccelY();
//		readRawAccelZ();
//	}
//}
//
//void LIS3DSH::fifoStartRec( void )
//{
//	uint8_t dataToWrite = 0;  //Temporary variable
//	
//	//Turn off...
//	readRegister( &dataToWrite, LIS3DSH_FIFO_CTRL_REG ); //Start with existing data
//	dataToWrite &= 0x3F;//clear mode
//#ifdef VERBOSE_SERIAL
//	Serial.print("LIS3DSH_FIFO_CTRL_REG: 0x");
//	Serial.println(dataToWrite, HEX);
//#endif
//	writeRegister(LIS3DSH_FIFO_CTRL_REG, dataToWrite);	
//	//  ... then back on again
//	readRegister( &dataToWrite, LIS3DSH_FIFO_CTRL_REG ); //Start with existing data
//	dataToWrite &= 0x3F;//clear mode
//	dataToWrite |= (settings.fifoMode & 0x03) << 6; //apply mode
//	//Now, write the patched together data
//#ifdef VERBOSE_SERIAL
//	Serial.print("LIS3DSH_FIFO_CTRL_REG: 0x");
//	Serial.println(dataToWrite, HEX);
//#endif
//	writeRegister(LIS3DSH_FIFO_CTRL_REG, dataToWrite);
//}
//
//uint8_t LIS3DSH::fifoGetStatus( void )
//{
//	//Return some data on the state of the fifo
//	uint8_t tempReadByte = 0;
//	readRegister(&tempReadByte, LIS3DSH_FIFO_SRC_REG);
//#ifdef VERBOSE_SERIAL
//	Serial.print("LIS3DSH_FIFO_SRC_REG: 0x");
//	Serial.println(tempReadByte, HEX);
//#endif
//	return tempReadByte;  
//}
//
//void LIS3DSH::fifoEnd( void )
//{
//	uint8_t dataToWrite = 0;  //Temporary variable
//
//	//Turn off...
//	readRegister( &dataToWrite, LIS3DSH_FIFO_CTRL_REG ); //Start with existing data
//	dataToWrite &= 0x3F;//clear mode
//#ifdef VERBOSE_SERIAL
//	Serial.print("LIS3DSH_FIFO_CTRL_REG: 0x");
//	Serial.println(dataToWrite, HEX);
//#endif
//	writeRegister(LIS3DSH_FIFO_CTRL_REG, dataToWrite);	
//}
//
