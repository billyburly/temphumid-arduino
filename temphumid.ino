#include <SPI.h>
#include <Ethernet.h>
#include <Wire.h>

int humidPin = A0;
int tempAddr = 0x91 >> 1;

int itemp;
int ihumid;

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
byte ip[] = { 192, 168, 1, 80 };
byte gateway[] = { 192, 168, 1, 1 };
byte subnet[] = { 255, 255, 255, 0 };

bool farenheight = true;

EthernetServer server = EthernetServer(23);

/**
 * Converts from celsius to farenheight
 *
 * @param temp Temperature in celsius
 * @return Temperature in farenheight
 */
float c2f(float temp) {
  return (temp * 9 / 5) + 32;
}

/**
 * Calculates humidity, compensates for temperature
 * Equations taken from datasheet: http://www.sparkfun.com/datasheets/Sensors/Weather/SEN-09569-HIH-4030-datasheet.pdf
 * Canceled out Vsupply
 *
 * @param humid sensor humidity
 * @param sensor temperature in degrees celsius
 * @returns temperature compensated humidity
 */
float getHumid(int sensor, float temp) {
  float vout = sensor / 1023.0;
  float humid = (vout + 0.16) / 0.0062;
  return humid / (1.0546 - 0.00216 * temp);
}

/**
 * Reads temparture from TMP102 via I2C
 *
 * @returns Signed int temperature in Celsius. Multiply by 0.0625 to get floating point value. If read fails -9999.
 */
int readTemp(void) {
  byte msb, lsb;
  int temp;
  Wire.requestFrom(tempAddr, 2);
  
  if (2 <= Wire.available()) {
    msb = Wire.read();
    lsb = Wire.read();
    temp = ((msb << 8) | lsb) >> 4;

    //fix 2s compliment
    if(temp & (1 << 11))
      temp |= 0xF800;
    
    return temp;
  }
  
  return -9999;
}

void setup () {
  Serial.begin(9600);
  Wire.begin();
  
  Ethernet.begin(mac, ip, gateway, subnet);
  server.begin();
}

void loop () {
  EthernetClient client = server.available();

  if(client) {
    if(client.available() > 0) {
      char cmd = client.read();
      
      if(cmd == 'i' || cmd == 'f') {
        itemp = readTemp();
	//read humidity from HIH-4030
        ihumid = analogRead(humidPin);
      }
      
      if(cmd == 'i') {
        server.print(itemp);
        server.print(',');
        server.print(ihumid);
      } else if(cmd == 'f') {
        float temp;
        
        if(itemp != -9999) {
          temp = itemp * 0.0625;
          if(farenheight)
	    server.print(c2f(temp), 4);
	  else
	    server.print(temp, 4);

          server.print(',');
          server.print(getHumid(ihumid, temp), 4);
        } else
          server.print("-9999,-9999");
      }
      server.print("\n");
    }
  }  
}
