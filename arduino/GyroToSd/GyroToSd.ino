// MPU-6050 Accelerometer + Gyro
// -----------------------------
//
// By arduino.cc user "Krodal".
//
// June 2012
//      first version
// July 2013
//      The 'int' in the union for the x,y,z
//      changed into int16_t to be compatible
//      with Arduino Due.
//
// Open Source / Public Domain
//
// Using Arduino 1.0.1
// It will not work with an older version,
// since Wire.endTransmission() uses a parameter
// to hold or release the I2C bus.
//
// Documentation:
// - The InvenSense documents:
//   - "MPU-6000 and MPU-6050 Product Specification",
//     PS-MPU-6000A.pdf
//   - "MPU-6000 and MPU-6050 Register Map and Descriptions",
//     RM-MPU-6000A.pdf or RS-MPU-6000A.pdf
//   - "MPU-6000/MPU-6050 9-Axis Evaluation Board User Guide"
//     AN-MPU-6000EVB.pdf
//
// The accuracy is 16-bits.
//
// Temperature sensor from -40 to +85 degrees Celsius
//   340 per degrees, -512 at 35 degrees.
//
// At power-up, all registers are zero, except these two:
//      Register 0x6B (PWR_MGMT_2) = 0x40  (I read zero).
//      Register 0x75 (WHO_AM_I)   = 0x68.
//
 
 
// MPU-6050 Short Example Sketch
// By Arduino User JohnChi
// August 17, 2014
// Public Domain

/*
  SD card datalogger

 This example shows how to log data from three analog sensors
 to an SD card using the SD library.

 The circuit:
 * analog sensors on analog ins 0, 1, and 2
 * SD card attached to SPI bus as follows:
 ** MOSI - pin 11
 ** MISO - pin 12
 ** CLK - pin 13
 ** CS - pin 10

 created  24 Nov 2010
 modified 9 Apr 2012
 by Tom Igoe

 This example code is in the public domain.

 */

//Includes 
 
#include <Wire.h>

#include <SPI.h>
#include <SD.h>

#include <SoftwareSerial.h>  



#include "defines.h"

// Declaring an union for the registers and the axis values.
// The byte order does not match the byte order of
// the compiler and AVR chip.
// The AVR chip (on the Arduino board) has the Low Byte
// at the lower address.
// But the MPU-6050 has a different order: High Byte at
// lower address, so that has to be corrected.
// The register part "reg" is only used internally,
// and are swapped in code.
typedef union accel_t_gyro_union
{
  struct
  {
    uint8_t x_accel_h;
    uint8_t x_accel_l;
    uint8_t y_accel_h;
    uint8_t y_accel_l;
    uint8_t z_accel_h;
    uint8_t z_accel_l;
    uint8_t t_h;
    uint8_t t_l;
    uint8_t x_gyro_h;
    uint8_t x_gyro_l;
    uint8_t y_gyro_h;
    uint8_t y_gyro_l;
    uint8_t z_gyro_h;
    uint8_t z_gyro_l;
  } reg;
  struct
  {
    int16_t x_accel;
    int16_t y_accel;
    int16_t z_accel;
    int16_t temperature;
    int16_t x_gyro;
    int16_t y_gyro;
    int16_t z_gyro;
  } value;
};

const int sdChipSelectPin = 10;

const int writeSwitchPin = 7;
const int bluetoothSwitchPin = 6;
const int deleteSwitchPin = 5;

int bluetoothSwitchState = 0;
int writeSwitchState = 0;
int deleteSwitchState = 0;

boolean writeBreak = false;

int bluetoothTx = 2;  // TX - TX
int bluetoothRx = 3;  // RX - RX

SoftwareSerial bluetooth(bluetoothTx, bluetoothRx);


void setup()
{
  //---------------------- General Setup -------------------------------//
  //vars for printing
  int error;
  uint8_t c;
  
  Serial.begin(9600);
  bluetooth.begin(9600);
  
  //------------------------------ Gyro Setup ---------------------------//
  Wire.begin();
  
  //Check WHO_AM_I Register
  error = MPU6050_read (MPU6050_WHO_AM_I, &c, 1);
  //F() macro stores on FLASH Memory rather than both SRAM and Flash Memory (why is that the default?)
  Serial.print(F("WHO_AM_I : "));
  //Serial.print(data,(display type)
  Serial.print(c,HEX);
  Serial.print(F(", error = "));
  Serial.println(error,DEC);
  
  //Check 'sleep' bit
  error = MPU6050_read (MPU6050_PWR_MGMT_1, &c, 1);
  Serial.print(F("PWR_MGMT_1 : "));
  Serial.print(c,HEX);
  Serial.print(F(", error = "));
  Serial.println(error,DEC);
   
  //Change Gyro config register for FS_SEL
  MPU6050_write_reg(MPU6050_GYRO_CONFIG,MPU6050_FS_SEL_2000);
  //Check Gyro Config register
  error = MPU6050_read (MPU6050_GYRO_CONFIG, &c, 1);
  Serial.print(F("FS_SLEL3: "));
  Serial.println(print_binary(c));

  // Clear the 'sleep' bit to start the sensor.
  MPU6050_write_reg (MPU6050_PWR_MGMT_1, 0);
  
  //---------------------- SD Card Setup ------------------------//
  
  Serial.print("Initializing SD card...");
  
  // see if the card is present and can be initialized:
  if (!SD.begin(sdChipSelectPin)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  Serial.println("card initialized.");
  
  File dataFile = SD.open("datalog.txt", FILE_WRITE);
  
  // if the file is available, write to it:
  if (dataFile) {
    dataFile.println("START");
    dataFile.close();
    // print to the serial port too:
    Serial.println("START");
  }
  
  //------------------- Button / Toggle / Debug Setup ----------//
  
  pinMode(writeSwitchPin, INPUT);
  pinMode(bluetoothSwitchPin, INPUT);
  pinMode(deleteSwitchPin, INPUT);
  
}

void loop()
{
  
  writeSwitchState = digitalRead(writeSwitchPin);
  bluetoothSwitchState = digitalRead(bluetoothSwitchPin);
  deleteSwitchState = digitalRead(deleteSwitchPin);
  
  //Check if we should be logging to SD card from switch
  if(writeSwitchState)
  {
    
    writeBreak = true;
    
    int error;
    double dT;
    accel_t_gyro_union accel_t_gyro;
    
    //Debug information
    Serial.println(F(""));
    Serial.println(F("MPU-6050"));
   
    // Read the raw values.
    // Read 14 bytes at once,
    // containing acceleration, temperature and gyro.
    // With the default settings of the MPU-6050,
    // there is no filter enabled, and the values
    // are not very stable.
    error = MPU6050_read (MPU6050_ACCEL_XOUT_H, (uint8_t *) &accel_t_gyro, sizeof(accel_t_gyro));
    Serial.print(F("Read accel, temp and gyro, error = "));
    Serial.println(error,DEC);
   
   
    // Swap all high and low bytes.
    // After this, the registers values are swapped,
    // so the structure name like x_accel_l does no
    // longer contain the lower byte.
    uint8_t swap;
    #define SWAP(x,y) swap = x; x = y; y = swap
   
    SWAP (accel_t_gyro.reg.x_accel_h, accel_t_gyro.reg.x_accel_l);
    SWAP (accel_t_gyro.reg.y_accel_h, accel_t_gyro.reg.y_accel_l);
    SWAP (accel_t_gyro.reg.z_accel_h, accel_t_gyro.reg.z_accel_l);
    SWAP (accel_t_gyro.reg.t_h, accel_t_gyro.reg.t_l);
    SWAP (accel_t_gyro.reg.x_gyro_h, accel_t_gyro.reg.x_gyro_l);
    SWAP (accel_t_gyro.reg.y_gyro_h, accel_t_gyro.reg.y_gyro_l);
    SWAP (accel_t_gyro.reg.z_gyro_h, accel_t_gyro.reg.z_gyro_l);
   
  
    // make a string for assembling the data to log:
    String dataString = String(accel_t_gyro.value.x_gyro) + ',' + 
                        String(accel_t_gyro.value.y_gyro) + ',' + 
                        String(accel_t_gyro.value.z_gyro);
  
    // open the file. note that only one file can be open at a time,
    // so you have to close this one before opening another.
    File dataFile = SD.open("datalog.txt", FILE_WRITE);
  
    // if the file is available, write to it:
    if (dataFile) 
    {
      dataFile.println(dataString);
      dataFile.close();
      // print to the serial port too:
      Serial.println(dataString);
    }
    // if the file isn't open, pop up an error:
    else {
      Serial.println("error opening datalog.txt");
    } 
  } 
  else 
  {
    if(writeBreak) 
    {
      File dataFile = SD.open("datalog.txt", FILE_WRITE);
  
      // if the file is available, write to it:
      if (dataFile) 
      {
        dataFile.println("STOPCOLLECTING");
        dataFile.close();
        // print to the serial port too:
        Serial.println("STOPCOLLECTING");
      }
      writeBreak = false;
    }
    Serial.println("Not writing");
    delay(100);
    
    if(bluetoothSwitchState)
    {
      Serial.println(F("Attempting to open file to send over bluetooth"));
      File dataFile = SD.open("datalog.txt");
      
      if(dataFile)
      {
       Serial.println(F("File available, sending"));
       if(dataFile.available()){
         bluetooth.println("START");
       }
       while(dataFile.available())
       {
         byte data = dataFile.read();
         bluetooth.write(data);
         Serial.println((char)data);
       }
       
       dataFile.close();
      } 
      else
      {
       Serial.println(F("No file available")); 
      }
    }
    if(deleteSwitchState)
    {
      Serial.println(F("Deleting datalog.txt"));
      SD.remove("DATALOG.TXT");
    }
  }
}


//MPU6050 READ/WRITE

// --------------------------------------------------------
// MPU6050_read
//
// This is a common function to read multiple bytes
// from an I2C device.
//
// It uses the boolean parameter for Wire.endTransMission()
// to be able to hold or release the I2C-bus.
// This is implemented in Arduino 1.0.1.
//
// Only this function is used to read.
// There is no function for a single byte.
//

int MPU6050_read(int start, uint8_t *buffer, int size)
{
  int i, n, error;
 
  Wire.beginTransmission(MPU6050_I2C_ADDRESS);
  n = Wire.write(start);
  if (n != 1)
    return (-10);
 
  n = Wire.endTransmission(false);    // hold the I2C-bus
  if (n != 0)
    return (n);
 
  // Third parameter is true: relase I2C-bus after data is read.
  Wire.requestFrom(MPU6050_I2C_ADDRESS, size, true);
  i = 0;
  while(Wire.available() && i<size)
  {
    buffer[i++]=Wire.read();
  }
  if ( i != size)
    return (-11);
 
  return (0);  // return : no error
}

// --------------------------------------------------------
// MPU6050_write
//
// This is a common function to write multiple bytes to an I2C device.
//
// If only a single register is written,
// use the function MPU_6050_write_reg().
//
// Parameters:
//   start : Start address, use a define for the register
//   pData : A pointer to the data to write.
//   size  : The number of bytes to write.
//
// If only a single register is written, a pointer
// to the data has to be used, and the size is
// a single byte:
//   int data = 0;        // the data to write
//   MPU6050_write (MPU6050_PWR_MGMT_1, &c, 1);
//
int MPU6050_write(int start, const uint8_t *pData, int size)
{
  int n, error;
 
  Wire.beginTransmission(MPU6050_I2C_ADDRESS);
  n = Wire.write(start);        // write the start address
  if (n != 1)
    return (-20);
 
  n = Wire.write(pData, size);  // write data bytes
  if (n != size)
    return (-21);
 
  error = Wire.endTransmission(true); // release the I2C-bus
  if (error != 0)
    return (error);
 
  return (0);         // return : no error
}
 
// --------------------------------------------------------
// MPU6050_write_reg
//
// An extra function to write a single register.
// It is just a wrapper around the MPU_6050_write()
// function, and it is only a convenient function
// to make it easier to write a single register.
//
int MPU6050_write_reg(int reg, uint8_t data)
{
  int error;
 
  error = MPU6050_write(reg, &data, 1);
 
  return (error);
}

//Prints a uint8_t as binary rather with leading zeros
char* print_binary(uint8_t data)
{
  uint8_t mask = 128;
  char finalString[8];
  
  uint8_t i;
  
  for(i = 0; i < 8; i++){
    if(data & mask){
      finalString[i] = '1';
    } else {
      finalString[i] = '0';
    }     
    
    mask = mask >> 1;
  }
  
  return finalString;
  
}
