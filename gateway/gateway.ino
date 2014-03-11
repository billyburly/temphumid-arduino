#include <SPI.h>
#include <JeeLib.h>
#include <EtherCard.h>
#include <Wire.h>
#include <NanodeUNIO.h>

int humidPin = A0;
int tempAddr = 0x91 >> 1;

int itemp;
int ihumid;

byte ip[] = { 10, 47, 0, 80 };
byte gateway[] = { 10, 47, 0, 1 };
byte subnet[] = { 255, 255, 255, 0 };

bool farenheight = true;

byte Ethernet::buffer[500];
BufferFiller bfill;
#define NODEID 1
#define NETWORK 47
#define SEN1_NODE 2

typedef struct { int temp, humid; } DataStructure;
DataStructure payload;
int ledPin = 6;

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
  
  byte macaddr[6];
  NanodeUNIO unio(NANODE_MAC_DEVICE);
  unio.read(macaddr,NANODE_MAC_ADDRESS,6);

  ether.begin(sizeof Ethernet::buffer, macaddr);
  ether.staticSetup(ip, gateway);
  
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
  
  rf12_initialize(NODEID,RF12_433MHZ,NETWORK);
  Serial.println("init done");
}

void loop () {
  word len = ether.packetReceive();
  word pos = ether.packetLoop(len);

  if (rf12_recvDone() && rf12_crc == 0) {
    if ((rf12_hdr & 0x1F) == SEN1_NODE) {
      payload = *(DataStructure*) rf12_data;
    }
  }
  
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
