/*
  Ad hoc network of DS28E17s and DS18B20s
  By: Joel Bartlett
  SparkFun Electronics
  Date: 04/07/17
  License: This code is public domain but you buy me a beer if you use this and we meet someday (Beerware license).

  DS28E17 1Wire to I2C Bridge talking to Si7021.
  Can ping and read humidity and temperature on multiple 
  Si7021s connected to multiple DS28E17s. Also detects DS18B20s
  on the bus and reports their temperature as well. 

  Hardware Connections:
  Connect an RJ11 (https://www.sparkfun.com/products/14021) or 
  RJ45 (https://www.sparkfun.com/products/716) Breakout with associated connector
  to your master device. See wiring diagram below.  


                         _____________
   3.3V                 |o 1     TOP  |
  Arduino               |o 2          |
     or  |------3.3V----|o 3          |
  Photon |------D4------|o 4    RJ45  |-----CAT5 CABLE------> DS28E17 Bridge----->Can add multiple Bridges 
     or  |------GND-----|o 5  BREAKOUT|                          |       |         & other 1Wire devices
   ESP32                |o 6          |                          |       |
                        |o 7          |                          ▼      ▼
                        |o_8__________|                       Si7021  DS18B20

  * Works great with Arduino
  * Crashes on Photon after several reads, still investigating 
  * Not tested on the ESP32

  Gotchas:
  * Had to shift I2C address one left one bit for R/W bit, 
  so 0x40 becomes 0x80 for the Si7021.
  * Had to use use No Hold registers so they send and ACK
  From the datasheet: "While  the  measurement  is  in
  progress, the option of either clock stretching (Hold 
  Master Mode) or Not Acknowledging read requests (No Hold
  Master  Mode)  is  available  to  indicate  to  the  master  th
  at  the  measurement  is  in  progress"
  * Looking into if DS18B20 library is faster/better performing than the DallasTemperature laibrary. 
*/
#include <OneWire.h>
#include <DallasTemperature.h>

/***************DS28E17 commands***************************/
#define Write_Data_Stop         0x4B
#define Write_Data_No_Stop      0x5A
#define Write_Data_Only         0x69
#define Write_Data_Only_Stop    0x78
#define Read_Data_Stop          0x87
#define Write_Read_Data_Stop    0x2D
#define Write_Config            0xD2
#define Read_Config             0xE1
#define Enable_Sleep            0x1E
#define Read_Device_Rev         0xC3


/***************Si7021 commands***************************/
#define I2C_ADDRESS 0x80//Si7021's 0x40 address shifted left one bit

#define TEMP_MEASURE_HOLD  0xE3
#define HUMD_MEASURE_HOLD  0xE5
#define TEMP_MEASURE_NOHOLD  0xF3
#define HUMD_MEASURE_NOHOLD  0xF5
#define TEMP_PREV   0xE0

#define WRITE_USER_REG  0xE6
#define READ_USER_REG  0xE7
#define SOFT_RESET  0xFE

#define HTRE 0x02
#define _BV(bit) (1 << (bit))

/***************1wire commands***************************/
#define ONEWIRE_PIN 4//Can be any digital pin

//1Wire Temperature prescision 9-12
#define TEMPERATURE_PRECISION 12

OneWire oneWire(ONEWIRE_PIN);//Declare instance of 1Wire bus on digital pin

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

uint8_t address[8];//temp place holder address for assigning 

uint8_t temperatureAddress[8][8];
uint8_t bridgeAddress[8][8];


uint8_t temperatureAddressIndex = 0;
uint8_t bridgeAddressIndex = 0;

bool flag = 1;
/////////////////////////////////////////////////////////////////
void setup() 
{
  sensors.begin();//initialize Dallas Temp
  
  Serial.begin(9600);

  Serial.println("\n\nScanning for OneWire Devices...");
  do
  {
    oneWireSearch();
  }
  while(flag == 1);
  
  printOneWireDevices();

  for(int i=0;i<temperatureAddressIndex;i++)
  {
    sensors.setResolution(temperatureAddress[i], TEMPERATURE_PRECISION);
  }
}
/////////////////////////////////////////////////////////////////
void loop() 
{
    for(byte i =0;i<bridgeAddressIndex;i++)
    {
      Serial.print("Si7021_");
      Serial.print(i);
      Serial.print(": RH = ");
      Serial.print(getSi7021_RH(i));
      Serial.print(", TempF = ");
      Serial.print(getSi7021_Temp(i));
      Serial.print(" | ");
    }

    for(byte i =0;i<temperatureAddressIndex;i++)
    {
      Serial.print("1W_");
      Serial.print(i);
      Serial.print(": TempF = ");
      Serial.print(DallasTemperature::toFahrenheit(getOneWireTemperature(i))); 
      Serial.print(" | ");
    }
    //Serial.print("  |  1W_Temp C: ");
    //Serial.print(getOneWireTemperature(0));
    Serial.println();
    delay(100);

}
/////////////////////////////////////////////////////////////////
// function to print the temperature for a device
float getOneWireTemperature(int _temperatureIndex)
{
  sensors.requestTemperatures();
  float tempC = sensors.getTempC(temperatureAddress[_temperatureIndex]);
  return tempC;
}
/////////////////////////////////////////////////////////////////
void printOneWireDevices()
{
  Serial.print("Bridge INDEX after scan = ");
  Serial.println(bridgeAddressIndex);
  
  Serial.print("Temperature INDEX after scan = ");
  Serial.println(temperatureAddressIndex);

  
      Serial.println();
      for(byte i =0;i<bridgeAddressIndex;i++)
      {
      Serial.print("Bridge ");
      Serial.print(i);
      Serial.print(" ROM = ");
      Serial.print("0x");
        Serial.print(bridgeAddress[i][0],HEX);
        for(byte j = 1; j < 8; j++) 
        {
          if(bridgeAddress[i][j] <= 0x0F)
          Serial.print(", 0x0");
          else
          Serial.print(", 0x");
          
          Serial.print(bridgeAddress[i][j],HEX);
        }
        Serial.println();
      }

      Serial.println();
      for(byte i =0;i<temperatureAddressIndex;i++)
      {
      Serial.print("Temperature ");
      Serial.print(i);
      Serial.print(" ROM = ");
      Serial.print("0x");
        Serial.print(temperatureAddress[i][0],HEX);
        for(byte j = 1; j < 8; j++) 
        {
          if(temperatureAddress[i][j] <= 0x0F)
          Serial.print(", 0x0");
          else
          Serial.print(", 0x");
          
          Serial.print(temperatureAddress[i][j],HEX);
        }
        Serial.println();
      }
}
//////////////////////////////////////////////////////////
void oneWireSearch()
{
        byte i;
        byte address[8];
        
        
      if ( !oneWire.search(address)) 
      {
        Serial.println("No more addresses.");
        Serial.println();
        oneWire.reset_search();
        flag = 0;//set flag to 0 as there are no more 1wire devices left to find
        return;
      }
      
      
      // the first ROM byte indicates which chip
      switch (address[0]) 
      {
        case 0x10:
          Serial.println("Chip = DS18S20");  // or old DS1820
          break;
        case 0x28:
          Serial.println("Chip = DS18B20");
              for( i = 0; i < 8; i++) 
              {
                temperatureAddress[temperatureAddressIndex][i] = address[i];
              }
              temperatureAddressIndex++;//increment index for more than one device 
           break;
        case 0x22:
          Serial.println("Chip = DS1822");
          break;
        case 0x19:
          Serial.println("Chip = 1Wire-to-I2C Bridge");
              for( i = 0; i < 8; i++) 
              {
                bridgeAddress[bridgeAddressIndex][i] = address[i];
              }

              bridgeAddressIndex++;//increment index for more than one device 
          break;
        default:
          Serial.println("Device is not a DS18x20 family device.");
          return;
      }


      Serial.print("ROM = ");
      Serial.print("0x");
        Serial.print(address[0],HEX);
        for( i = 1; i < 8; i++) 
        {
          if(address[i] <= 0x0F)
          Serial.print(", 0x0");
          else
          Serial.print(", 0x");
          
          Serial.print(address[i],HEX);
        }
        
      //CRC Check
      if (OneWire::crc8(address, 7) != address[7]) 
      {
          Serial.println("CRC is not valid!");
          return;
      }


      Serial.println();
      oneWire.reset();
}

////////////////////////////////////////////////////

uint16_t makeMesurment_Si7021(byte _i2c_address, byte reg, int _bridgeIndex) 
{
  byte packet[7];                           // Reserve bytes to transmit data
  uint16_t nBytes = 3;
  if (reg == 0xE0) nBytes = 2;

  packet[0] = Write_Read_Data_Stop;         // Command ("Write_Read_Data_Stop") 0x2D
  packet[1] = _i2c_address;                   // I2C slave to address
  packet[2] = 0x01;                         // Number of data bytes to write
  packet[3] = reg;                      // The data to write
  packet[4] = nBytes;   //number of bytes to read

  uint16_t CRC16 = oneWire.crc16(&packet[0], 5);
  CRC16 = ~CRC16;                           // Invert the crc16 value

  packet[6] = CRC16 >> 8;                   // Most significant byte of 16 bit CRC
  packet[5] = CRC16 & 0xFF;                 // Least significant byte of 16 bit CRC

 // Serial.println("\n\nStarting 1Wire Transmission");
  
/////Start Transmission/////////////////
  oneWire.reset();
  oneWire.select(bridgeAddress[_bridgeIndex]);  
  oneWire.write_bytes(packet, sizeof(packet), 1); //Write the packet and hold the bus
  
  while(oneWire.read_bit() == true)         // Wait for not busy
  {
    delay(1);
  }

  //comment thes two lines if uncommenting the prints below 
  oneWire.read();//status
  oneWire.read();//write status
  //Serial.print("Status =");
  //Serial.println(oneWire.read(),BIN);
  //Serial.print("Write Status =");
  //Serial.println(oneWire.read(),BIN);

  unsigned int msb = oneWire.read();
  unsigned int lsb = oneWire.read();
  // Clear the last to bits of LSB to 00.
  // According to datasheet LSB of RH is always xxxxxx10
  lsb &= 0xFC;
  unsigned int mesurment = msb << 8 | lsb;
  
  oneWire.depower();// Release the bus

  return mesurment;  //return temp or humidity measurment
}
////////////////////////////////////////////////////
float getSi7021_RH(int bridgeindex)
{
  uint16_t RH_Code = makeMesurment_Si7021(I2C_ADDRESS, 0xE5, bridgeindex);
  float result = (125.0*RH_Code/65536)-6;
  return result;
}
////////////////////////////////////////////////////
//pulls temperature from previous humidity reading 
float getSi7021_Temp(int bridgeindex)
{
  uint16_t temp_Code = makeMesurment_Si7021(I2C_ADDRESS, 0xE0, bridgeindex);
  float temp_result = (175.72*temp_Code/65536)-46.85;
  float result = (temp_result * 1.8) + 32;
  return result;
}

