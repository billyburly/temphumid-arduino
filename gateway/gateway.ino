#include <SPI.h>
#include <JeeLib.h>
#include <EtherCard.h>
#include <Wire.h>
#include <NanodeUNIO.h>
#include <rf12-common.h>
#include <tmp102.h>

byte ip[] = { 10, 47, 0, 80 };
byte gateway[] = { 10, 47, 0, 1 };
byte subnet[] = { 255, 255, 255, 0 };

byte Ethernet::buffer[500];
BufferFiller bfill;
#define NODEID 1
#define MAXNODES 2 //max nodes for network is 30. ran into stability issues. ethernet buffer too small?
//#define DEBUG

DataStructure payload;
#define LED_PIN 6

char okHeader[] PROGMEM =
  "HTTP/1.0 200 OK\r\n"
  "Content-Type: text/plain\r\n"
  "Pragma: no-cache\r\n\r\n"
;

struct Node {
  int temp;
  int humid;
  unsigned long updated_at;
};
struct Node Nodes[MAXNODES];

void setup () {
#ifdef DEBUG
  Serial.begin(9600);
#endif
  Wire.begin();
  
  byte macaddr[6];
  NanodeUNIO unio(NANODE_MAC_DEVICE);
  unio.read(macaddr,NANODE_MAC_ADDRESS,6);

  ether.begin(sizeof Ethernet::buffer, macaddr);
  ether.staticSetup(ip, gateway);
  
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  
  rf12_initialize(NODEID,RF12_433MHZ,NETWORK);
  
  for(int i = 0; i < MAXNODES; i++) {
    Nodes[i].temp = -9999;
    Nodes[i].humid = -9999;
    Nodes[i].updated_at = 0;
  }
#ifdef DEBUG
  Serial.println("init done");
#endif
}

void loop () {
  word len = ether.packetReceive();
  word pos = ether.packetLoop(len);

  if (rf12_recvDone() && rf12_crc == 0) {
    byte recvId = (rf12_hdr & 0x1F) -1;
    payload = *(DataStructure*) rf12_data;
    
    Nodes[recvId].temp = payload.temp;
    Nodes[recvId].humid = payload.humid;
    Nodes[recvId].updated_at = millis();
#ifdef DEBUG
    Serial.print("rcv:");
    Serial.print(recvId);
    Serial.print(":");
    Serial.print(payload.temp);
    Serial.print(",");
    Serial.print(payload.humid);
    Serial.print(",");
    Serial.println(Nodes[recvId].updated_at);
#endif
  }
  
  if(pos) {
#ifdef DEBUG
    Serial.println("reading local sensors");
#endif
    Nodes[NODEID-1].temp = readTemp();
    //read humidity from HIH-4030
    Nodes[NODEID-1].humid = analogRead(A0);
    Nodes[NODEID-1].updated_at = millis();
    
#ifdef DEBUG
    Serial.println("preparing file to send");
#endif
    bfill = ether.tcpOffset();
    bfill.emit_p(PSTR("$F"), okHeader);
    for(int i = 0; i < MAXNODES; i++)
      bfill.emit_p(PSTR("$D:$D,$D,$D\r\n"), i, Nodes[i].temp, Nodes[i].humid, Nodes[i].updated_at);
    ether.httpServerReply(bfill.position());
#ifdef DEBUG
    Serial.println("sending data over http");
#endif
  }  
}
