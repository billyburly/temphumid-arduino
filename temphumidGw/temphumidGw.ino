#include <JeeLib.h>

bool farenheight = true;

#define NODEID 0
#define NETWORK 47
#define SEN1_NODE 1

typedef struct { int temp, humid; } DataStructure;
DataStructure payload;

void setup () {
  Serial.begin(9600);
  
  rf12_initialize(NODEID,RF12_433MHZ,NETWORK);
}

void loop () {
  
  if (rf12_recvDone() && rf12_crc == 0) {
    if ((rf12_hdr & 0x1F) == SEN1_NODE) {
      payload = *(DataStructure*) rf12_data;
      Serial.print("recv: ");
      Serial.print(payload.temp);
      Serial.print(" ");
      Serial.println(payload.humid);
    } else 
      Serial.println("otherid");
  }
}
