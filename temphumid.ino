#include <SPI.h>
#include <EtherCard.h>
#include <Wire.h>

int humidPin = A0;
int tempAddr = 0x91 >> 1;

int itemp;
int ihumid;

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
byte ip[] = { 10, 47, 0, 80 };
byte gateway[] = { 10, 47, 0, 1 };
byte subnet[] = { 255, 255, 255, 0 };

bool farenheight = true;

byte Ethernet::buffer[500];
BufferFiller bfill;

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
  float humid = (vout - 0.16) / 0.0062;
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
  
  ether.begin(sizeof Ethernet::buffer, mac);
  ether.staticSetup(ip, gateway);
}

void loop () {
  word len = ether.packetReceive();
  word pos = ether.packetLoop(len);

  if(pos) {
    itemp = readTemp();
    //read humidity from HIH-4030
    ihumid = analogRead(humidPin);
    
    bfill = ether.tcpOffset();
    bfill.emit_p(PSTR(
      "HTTP/1.0 200 OK\r\n"
      "Content-Type: text/plain\r\n"
      "Pragma: no-cache\r\n"
      "\r\n"
      "0:$D,$D"),
      itemp, ihumid);
    ether.httpServerReply(bfill.position());
  }  
}
