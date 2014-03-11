#include <JeeLib.h>
#include <Wire.h>
#include <rf12-common.h>
#include <tmp102.h>

int humidPin = A0;

#define NODEID 2
DataStructure payload;
int ledPin = 6;
unsigned long prevMillis = 0;

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
