#include <EtherCard.h>
#include <IPAddress.h>
#include <NanodeUNIO.h>

int humidPin = A0;
int tempAddr = 0x91 >> 1;

typedef struct { int temp, humid; } DataStructure;
DataStructure payload;
int ledPin = 6;

byte Ethernet::buffer[500];
static byte myip[] = { 192,168,1,81 };
static byte gwip[] = { 192,168,1,1 };
IPAddress sendTo;
boolean send = false;
unsigned long prevMillis = 0;

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
  Wire.begin();
  
  byte macaddr[6];
  NanodeUNIO unio(NANODE_MAC_DEVICE);
  unio.read(macaddr,NANODE_MAC_ADDRESS,6);
  
  if(ether.begin(sizeof Ethernet::buffer, macaddr) == 0)
    Serial.println("failed to initialize ethernet");
  ether.staticSetup(myip, gwip);
  ether.udpServerListenOnPort(&handleUdpRecv, 47);
  
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
  Serial.println("done initializing");
}

void loop () {
  ether.packetLoop(ether.packetReceive());
  
  /*if(prevMillis > millis()) {
  }*/
  
  if(send && (millis() > prevMillis + 1000 || prevMillis > millis())) {
    digitalWrite(ledPin, LOW);

    payload.temp = readTemp();
    payload.humid = analogRead(humidPin);
    
    /*Serial.print("sending: ");
    Serial.print(payload.temp);
    Serial.print(" ");
    Serial.println(payload.humid);*/
    
    uint8_t s[] = {sendTo[0],sendTo[1],sendTo[2],sendTo[3]};
    ether.sendUdp((char*) &payload, sizeof(payload), 4700, s, 47);
    //ether.sendUdp((char*) &payload, sizeof(payload), 4700, (uint8_t*) &sendTo, 47);
    
    digitalWrite(ledPin, HIGH);
    prevMillis = millis();
  }
}
