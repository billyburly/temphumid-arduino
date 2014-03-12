#include <JeeLib.h>
#include <Wire.h>
#include <rf12-common.h>
#include <tmp102.h>

#define SEND_RATE 500
#define NODEID 2
#define LED_PIN 6

DataStructure payload;
unsigned long prevMillis = 0;

void setup () {
  Serial.begin(9600);
  Wire.begin();
  
  pinMode(LED_PIN, OUTPUT);
  
  rf12_initialize(NODEID,RF12_433MHZ,NETWORK);
}

void loop () {
  payload.temp = readTemp();
  payload.humid = analogRead(A0);
  
  rf12_recvDone();
  
  if((millis() > prevMillis + SEND_RATE || prevMillis > millis()) && rf12_canSend()) {
    digitalWrite(LED_PIN, LOW);
    rf12_sendStart(0, &payload, sizeof(payload));
    prevMillis = millis();
      
    Serial.print("send: ");
    Serial.print(payload.temp);
    Serial.print(" ");
    Serial.println(payload.humid);
    digitalWrite(LED_PIN, HIGH);
  }
}
