/* 
  Scan for all 1Wire devices on the bus
  By: Joel Bartlett
  SparkFun Electronics
  Date: 04/07/17
  License: This code is public domain but you buy me a beer if you use this and we meet someday (Beerware license).

  Scans for all 1Wire devices and prints out their family and ROM address.  
 */
#include <OneWire.h>

OneWire oneWire = OneWire(4);  // on pin 4 (a 4.7K resistor is necessary)
unsigned long lastUpdate = 0;

void setup()
{
  Serial.begin(9600);
}

void loop()
{

 unsigned long now = millis();
    if((now - lastUpdate) > 3000)
    {
        lastUpdate = now;
        byte i;
        byte present = 0;
        
        byte addr[8];

      if ( !oneWire.search(addr)) 
      {
        Serial.println("No more addresses.");
        Serial.println();
        oneWire.reset_search();
        //delay(250);
        return;
      }
            // the first ROM byte indicates which chip
      switch (addr[0]) 
      {
        case 0x10:
          Serial.println("Chip = DS18S20");  // or old DS1820
          break;
        case 0x28:
          Serial.println("Chip = DS18B20");
          break;
        case 0x22:
          Serial.println("Chip = DS1822");
          break;
        case 0x19:
          Serial.println("Chip = 1Wire-to-I2C Bridge");
          break;
        default:
          Serial.println("Device is not a DS18x20 family device.");
          return;
      }


      Serial.print("ROM = ");
      Serial.print("0x");
        Serial.print(addr[0],HEX);
        for( i = 1; i < 8; i++) 
        {
          if(addr[i] <= 0x0F)
          Serial.print(", 0x0");
          else
          Serial.print(", 0x");
          
          Serial.print(addr[i],HEX);
        }
        
      //CRC Check
      if (OneWire::crc8(addr, 7) != addr[7]) 
      {
          Serial.println("CRC is not valid!");
          return;
      }


      Serial.println();
      oneWire.reset();

    }
}

