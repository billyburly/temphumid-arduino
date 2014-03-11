#include <JeeLib.h>
#include <Wire.h>

int humidPin = A0;
int tempAddr = 0x91 >> 1;

#define NODEID 2
#define NETWORK 47
typedef struct { int temp, humid; } DataStructure;
DataStructure payload;
int ledPin = 6;
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

void setup () {
  Serial.begin(9600);
  Wire.begin();
  
  pinMode(ledPin, OUTPUT);
  
  rf12_initialize(NODEID,RF12_433MHZ,NETWORK);
}

void loop () {
  payload.temp = readTemp();
  payload.humid = analogRead(humidPin);
  
  rf12_recvDone();
  
  if((millis() > prevMillis + 1000 || prevMillis > millis()) && rf12_canSend()) {
    digitalWrite(ledPin, LOW);
    rf12_sendStart(0, &payload, sizeof(payload));
    prevMillis = millis();
      
    Serial.print("send: ");
    Serial.print(payload.temp);
    Serial.print(" ");
    Serial.println(payload.humid);
    digitalWrite(ledPin, HIGH);
  }
}
