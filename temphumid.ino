#include <SPI.h>
#include <Ethernet.h>
#include <Wire.h>

int humidPin = A0;
int tempAddr = 0x91 >> 1;

int itemp;
int ihumid;

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
byte ip[] = { 192, 168, 1, 80 };
byte gateway[] = { 192, 168, 1, 1 };
byte subnet[] = { 255, 255, 255, 0 };

EthernetServer server = EthernetServer(23);

float c2f(float temp) {
  return (temp * 9 / 5) + 32;
}

int readTemp(void) {
  byte msb, lsb;
  int temp;
  Wire.requestFrom(tempAddr, 2);
  
  if (2 <= Wire.available()) {
    msb = Wire.read();
    lsb = Wire.read();
    temp = ((msb << 8) | lsb) >> 4;
    
    if(temp & (1 << 11))
      temp |= 0xF800;
    
    return temp;
  }
  
  return -9999;
}

void setup () {
  Serial.begin(9600);
  Wire.begin();
  
  delay(200);
  Ethernet.begin(mac, ip, gateway, subnet);
  server.begin();
}

void loop () {
  EthernetClient client = server.available();

  if(client) {
    if(client.available() > 0) {
      char cmd = client.read();
      
      if(cmd == 'i' || cmd == 'f') {
        itemp = readTemp();
        ihumid = analogRead(humidPin);
      }
      
      if(cmd == 'i') {
        server.print(itemp);
        server.print(',');
        server.print(ihumid);
      } else if(cmd == 'f') {
        if(itemp != -9999)
          server.print(c2f(itemp * 0.0625), 4);
        else
          server.print(-9999);
        server.print(',');
        server.print(100.0 * ihumid / 1023, 4);
      }
      server.print("\n");
    }
  }  
}
