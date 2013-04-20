#include <JeeLib.h>
#include <EtherCard.h>
#include <IPAddress.h>
#include <NanodeUNIO.h>

bool farenheight = true;

#define NODEID 1
#define NETWORK 47
#define SEN1_NODE 2

typedef struct { int temp, humid; } DataStructure;
DataStructure payload;
int ledPin = 6;


byte Ethernet::buffer[500];
static byte myip[] = { 192,168,1,81 };
static byte gwip[] = { 192,168,1,1 };
IPAddress sendTo;
boolean send = false;
unsigned long prevMillis = 0;

void handleUdpRecv(word port, byte ip[4], const char *data, word len) {
  IPAddress src(ip[0], ip[1], ip[2], ip[3]);
  Serial.print("recieved packet from: ");
  Serial.println(src);
  if(len == 1 && data[0] == 'r') {
    Serial.print("Registering: ");
    Serial.println(src);
    sendTo = ip;
    send = true;
  } else if(len == 1 && data[0] == 'd') {
    if(src == sendTo) {
      Serial.print("Deregistering: ");
      Serial.println(src);
      send = false;
      prevMillis = 0;
    }
  }
}

void setup () {
  Serial.begin(9600);
  
  byte macaddr[6];
  NanodeUNIO unio(NANODE_MAC_DEVICE);
  unio.read(macaddr,NANODE_MAC_ADDRESS,6);
  
  if(ether.begin(sizeof Ethernet::buffer, macaddr) == 0)
    Serial.println("failed to initialize ethernet");
  ether.staticSetup(myip, gwip);
  ether.udpServerListenOnPort(&handleUdpRecv, 47);
  
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
  
  rf12_initialize(NODEID,RF12_433MHZ,NETWORK);
  Serial.println("init done");
}

void loop () {
  ether.packetLoop(ether.packetReceive());
  
  if (rf12_recvDone() && rf12_crc == 0) {
    if ((rf12_hdr & 0x1F) == SEN1_NODE) {
      payload = *(DataStructure*) rf12_data;
      /*Serial.print("recv: ");
      Serial.print(payload.temp);
      Serial.print(" ");
      Serial.println(payload.humid);*/
    }
  }
  
  if(send && (millis() > prevMillis + 1000 || prevMillis > millis())) {
    digitalWrite(ledPin, LOW);
    
    uint8_t s[] = {sendTo[0],sendTo[1],sendTo[2],sendTo[3]};
    ether.sendUdp((char*) &payload, sizeof(payload), 4700, s, 47);
    
    digitalWrite(ledPin, HIGH);
    prevMillis = millis();
  }
}
