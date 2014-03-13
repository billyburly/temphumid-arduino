#include <SPI.h>
#include <JeeLib.h>
#include <EtherCard.h>
#include <Wire.h>
#include <NanodeUNIO.h>
#include <rf12-common.h>
#include <tmp102.h>

int humidPin = A0;

int itemp;
int ihumid;

byte ip[] = { 10, 47, 0, 80 };
byte gateway[] = { 10, 47, 0, 1 };
byte subnet[] = { 255, 255, 255, 0 };

bool farenheight = true;

byte Ethernet::buffer[500];
BufferFiller bfill;
#define NODEID 1
#define SEN1_NODE 2

DataStructure payload;
int ledPin = 6;

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
      "0:$D,$D\r\n"
      "2:$D,$D"),
      itemp, ihumid, payload.temp, payload.humid);
    ether.httpServerReply(bfill.position());
  }  
}
