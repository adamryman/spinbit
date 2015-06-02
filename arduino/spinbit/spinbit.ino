// SpinBit
// Katie Kemp, Isaac Perry, Adam Ryman // // Documentation: // - The InvenSense documents: //   - "MPU-6000 and MPU-6050 Product Specification", //     PS-MPU-6000A.pdf //   - "MPU-6000 and MPU-6050 Register Map and Descriptions", //     RM-MPU-6000A.pdf or RS-MPU-6000A.pdf //   - "MPU-6000/MPU-6050 9-Axis Evaluation Board User Guide" //     AN-MPU-6000EVB.pdf // // The accuracy is 16-bits.  // // Temperature sensor from -40 to +85 degrees Celsius //   340 per degrees, -512 at 35 degrees.  // // At power-up, all registers are zero, except these two: //      Register 0x6B (PWR_MGMT_2) = 0x40  (I read zero).  //      Register 0x75 (WHO_AM_I)   = 0x68.  // // MPU-6050 Short Example Sketch // By Arduino User JohnChi // August 17, 2014 // Public Domain /*
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

// WinBond flash commands
#define WB_WRITE_ENABLE       0x06
#define WB_WRITE_DISABLE      0x04
#define WB_CHIP_ERASE         0xc7
#define WB_READ_STATUS_REG_1  0x05
#define WB_READ_DATA          0x03
#define WB_PAGE_PROGRAM       0x02
#define WB_JEDEC_ID           0x9f

//Change this to 0 to have device switch controlled 1 to have it bluetooth controlled
#define BLUETOOTHCONTROLLED 1


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
  }
  reg;
  struct
  {
    int16_t x_accel;
    int16_t y_accel;
    int16_t z_accel;
    int16_t temperature;
    int16_t x_gyro;
    int16_t y_gyro;
    int16_t z_gyro;
  }
  value;
};

const int sdChipSelectPin = 10;

const int writeSwitchPin = 7;
const int bluetoothSwitchPin = 6;
const int deleteSwitchPin = 5;
const int ledPin = 9;

const int buttonPin = 2;
int buttonState;
int lastButtonState = LOW;

long lastDebounceTime = 0;
long debounceDelay = 50;

int bluetoothSwitchState = 0;
boolean writeSwitchState = 0;
int deleteSwitchState = 0;

boolean writeBreak = false;

long lastSpin;
int bluetoothTx = 3;  // TX - TX
int bluetoothRx = 4;  // RX - RX

SoftwareSerial bluetooth(bluetoothTx, bluetoothRx);

int fileNumber = 0;

char currentFile[14];

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
  Serial.println(c,BIN);
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
    dataFile.close();
    // print to the serial port too:
    Serial.println("START");
  }

  //------------------- Button / Toggle / Debug Setup ----------//

  pinMode(writeSwitchPin, INPUT);
  pinMode(bluetoothSwitchPin, INPUT);
  pinMode(deleteSwitchPin, INPUT);
  pinMode(buttonPin, INPUT);
  pinMode(ledPin, OUTPUT);

  //-------------------- FileManagement ------------------------//

  //setMeta(0);
  //Serial.println("doing meta testing");
  //setMeta(10);
  //int test = readMeta();
  Serial.println(F("Init checking Meta File"));
  fileNumber = readMeta();
  if(fileNumber == -1)
  {
    setMeta(10);
    fileNumber = 10;
  }
  String temp = intToTextFile(fileNumber);
  temp.toCharArray(currentFile, sizeof(currentFile));

}

double runningSum;
double runningSamples[10];
int runningSampleCounter = 0;

double sum = 0;
double spinThreshold = 0.75;
int changeThreshold = 5000;
long beginningOfTime;

double rollOver = 320000;

void loop()
{
  digitalWrite(ledPin, writeSwitchState);

  int currentButtonValue = digitalRead(buttonPin);

  if (currentButtonValue != lastButtonState)
  {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay)
  {
    if (currentButtonValue != buttonState)
    {
      Serial.println("Button State Changed");
      if( currentButtonValue == 1 && buttonState == 0)
       {
         writeSwitchState = !writeSwitchState;
       }

      buttonState = currentButtonValue;


      }
   }

  if(BLUETOOTHCONTROLLED)
  {
    if(bluetooth.available())
    {
      char gotten = bluetooth.read();
      Serial.println(String(gotten));
      switch(gotten)
      {
        //write to file
      case 'w':
	Serial.println(F("WWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWWW"));
        writeSwitchState = 1;
        bluetoothSwitchState = 0;
        deleteSwitchState = 0;
        break;
        //send file over bluetooth
      case 'b':
	Serial.println(F("BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB"));
        writeSwitchState = 0;
        bluetoothSwitchState = 1;
        deleteSwitchState = 0;
        break;
        //delete file
      case 'd':
	Serial.println(F("DDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD"));
        writeSwitchState = 0;
        bluetoothSwitchState = 0;
        deleteSwitchState = 1;
        break;
        //do nothing
      case 'i':
	Serial.println(F("IIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIIII"));
        writeSwitchState = 0;
        bluetoothSwitchState = 0;
        deleteSwitchState = 0;
        break;

      default:
        Serial.println(F("THIS SHOULD NOT HAPPEN ~~~~~~~~~~~~~~~~~~"));
        break;
      }
    }
  }
  else
  {
    writeSwitchState = digitalRead(writeSwitchPin);
    bluetoothSwitchState = digitalRead(bluetoothSwitchPin);
    deleteSwitchState = digitalRead(deleteSwitchPin);

  }




  //Check if we should be logging to SD card from switch
  if(writeSwitchState)
  {
    if(!writeBreak)
    {
	Serial.println("Entering writeSwitchState");
        sum = 0;
        runningSampleCounter = 0;
        runningSum = 0;
        for(int i = 0; i < 10; i++)
        {
          runningSamples[i] = 0;
        }
        Serial.println(rollOver);
        beginningOfTime = millis();
        lastSpin = millis();
    }
    writeBreak = true;

    int error;
    double dT;
    accel_t_gyro_union accel_t_gyro;

    //Debug information
    //Serial.println(F(""));
    //Serial.println(F("MPU-6050"));

    // Read the raw values.
    // Read 14 bytes at once,
    // containing acceleration, temperature and gyro.
    // With the default settings of the MPU-6050,
    // there is no filter enabled, and the values
    // are not very stable.
    error = MPU6050_read (MPU6050_ACCEL_XOUT_H, (uint8_t *) &accel_t_gyro, sizeof(accel_t_gyro));
    //Serial.print(F("Read accel, temp and gyro, error = "));
    //Serial.println(error,DEC);


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

      sum = sum + accel_t_gyro.value.y_gyro;
      if(sum >= rollOver || sum <= -rollOver)
      {
        File dataFile = SD.open(currentFile, FILE_WRITE);

        // if the file is available, write to it:
        if (dataFile)
        {
          long currentSpin = millis();
          dataFile.print(currentSpin - lastSpin);
          lastSpin = currentSpin;
          dataFile.print(" ");
          dataFile.close();
          // print to the serial port too:
          //Serial.println(dataString);
        }

        if(sum > 0)
        {
          sum = sum - rollOver;
        } else
        {
           sum = sum + rollOver;
        }


      }
      Serial.print("Sum: ");
      Serial.println(sum);
      Serial.println();
//
//
//    runningSum = runningSum + accel_t_gyro.value.y_gyro - runningSamples[runningSampleCounter];
//    runningSamples[runningSampleCounter] = accel_t_gyro.value.y_gyro;
//
//    runningSampleCounter++;
//    if(runningSampleCounter == 10)
//    {
//      runningSampleCounter = 0;
//    }
//    Serial.print("Sum: ");
//    Serial.println(sum);
//      Serial.println();
//    Serial.print("Running Sum:");
//    Serial.println(runningSum);
//    if((sum <= 0 && runningSum < 0) || (sum >= 0 && runningSum > 0))
//    {
//      sum = sum + accel_t_gyro.value.y_gyro;
//      if(abs(sum) >= 360*250)
//      {
//        File dataFile = SD.open(currentFile, FILE_WRITE);
//
//        // if the file is available, write to it:
//        if (dataFile)
//        {
//          dataFile.print(millis() - beginningOfTime);
//          dataFile.print(" ");
//          dataFile.close();
//          // print to the serial port too:
//          //Serial.println(dataString);
//        }
//        sum = sum - 360*250;
//      }
//      else if(abs(runningSum) > changeThreshold*250)
//      {
//        sum = runningSum;
//        for(int i = 0; i < 10; i++)
//        {
//          runningSamples[i] = 0;
//        }
//        runningSum = 0;
//        if(abs(sum) >= 360*spinThreshold*250)
//        {
//          File dataFile = SD.open(currentFile, FILE_WRITE);
//
//          // if the file is available, write to it:
//          if (dataFile)
//          {
//            dataFile.print(millis() - beginningOfTime);
//            dataFile.print(" ");
//            dataFile.close();
//            // print to the serial port too:
//            //Serial.println(dataString);
//          }
//        }
//      }
//    }
  }
  else
  {
    if(writeBreak)
    {
//      if(abs(sum) >= 360*250)
//      {
//        File dataFile = SD.open(currentFile, FILE_WRITE);
//
//        // if the file is available, write to it:
//        if (dataFile)
//        {
//          dataFile.print(millis() - beginningOfTime);
//          dataFile.print(" ");
//          dataFile.close();
//          // print to the serial port too:
//          //Serial.println(dataString);
//        }
//      }

      File dataFile = SD.open(currentFile, FILE_WRITE);

      // if the file is available, write to it:
      if (dataFile)
      {
        dataFile.println("");
        dataFile.close();
        // print to the serial port too:
        //Serial.println("STOPCOLLECTING");
      }

      //Get old meta value and increase it by 1 then set Meta to be that value
      fileNumber = readMeta();
      fileNumber++;
      setMeta(fileNumber);
      Serial.print("Does meta exist?:");
      Serial.println(SD.exists("meta.txt"));

      //Set currentFile to be "fileNumber".txt
      String temp = intToTextFile(fileNumber);
      temp.toCharArray(currentFile, sizeof(currentFile));

      writeBreak = false;
    }

    delay(100);

    if(bluetoothSwitchState)
    {
      Serial.println(F("Attempting to open file to send over bluetooth"));

      int i = 10;
      fileNumber = readMeta();
      char fileToSend[14];
      Serial.println(F("File loop _______________"));
      //Loop from file 0.txt to "meta.value".txt till you find a file that exists
      while(i < fileNumber && !SD.exists(fileToSend))
      {
	String temp = intToTextFile(i);
	temp.toCharArray(fileToSend, sizeof(fileToSend));
	i++;
      }
      //Send the file that you found
      File dataFile = SD.open(fileToSend);

      if(dataFile)
      {
        Serial.println(F("File available, sending"));
        while(dataFile.available())
        {
          byte data = dataFile.read();
          bluetooth.write(data);
        }

        dataFile.close();
      }
      else
      {
        Serial.println(F("No file available"));
      }
      bluetoothSwitchState = 0;
    }
    if(deleteSwitchState)
    {
      Serial.println(F("Trying to delete file"));
      int i = 10;
      fileNumber = readMeta();
      char fileToDelete[14];

      //Loop from file 0.txt to "meta.value".txt till you find a file that exists
      while(i < fileNumber && !SD.exists(fileToDelete))
      {
	String temp = intToTextFile(i);
	temp.toCharArray(fileToDelete, sizeof(fileToDelete));
	i++;
      }

      if(SD.remove(fileToDelete))
      {
	Serial.println(F("File got deleted"));
      }
      else
      {
	Serial.println(F("PROBLEM OH NO, File did not get deleted"));
      }
      deleteSwitchState = 0;
    }
  }
  lastButtonState = currentButtonValue;
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
  char finalString[9];

  uint8_t i;

  for(i = 0; i < 8; i++){
    if(data & mask){
      finalString[i] = '1';
    }
    else {
      finalString[i] = '0';
    }

    mask = mask >> 1;
  }

  finalString[8] = '\0';

  return finalString;

}

int readMeta() {
  Serial.println(F("Checking meta file"));
  File meta = SD.open("meta.txt");
  int metaInt = -1;
  if (meta)
  {
    Serial.println(F("Creating int from meta file"));
    char metaChars[10];
    int i = 0;
    while(meta.available())
    {
      metaChars[i] = meta.read();
      i++;
    }
    metaChars[i] = '\0';
    metaInt = atoi(metaChars);
    Serial.println(String(metaInt));
  }
  meta.close();
  return metaInt;

}

void setMeta(int value)
{
  SD.remove("meta.txt");
  File meta = SD.open("meta.txt", FILE_WRITE);

  if(meta)
  {
    Serial.print(F("Setting Meta to:"));
    Serial.println(String(value));
    meta.print(String(value));
    meta.close();
  }

}

String intToTextFile(int value)
{
  String textFile;
  textFile = String(value);
  textFile.concat(".txt");

  return textFile;
}



