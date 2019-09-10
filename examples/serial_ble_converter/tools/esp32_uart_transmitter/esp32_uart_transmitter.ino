#include <M5StickC.h>
 
void setup() {
  Serial.begin(115200); // Debug Print
  Serial2.begin(115200, SERIAL_8N1, 26, 27); //rx:G26, tx:G27
}
 
void loop() {
  static int sendCount = 0;
  Serial2.println("I/ start sending");
  delay(500);
  Serial2.println("1, SWVer, M5StickUARTV001");
  delay(500);
  Serial2.println("2, State, Sending");
  delay(500);
  Serial2.printf("3, Count1, %d\r\n", sendCount);
  delay(500);
  Serial2.printf("4, Count2, %d\r\n", sendCount);
  delay(500);
  Serial2.printf("6, VDD, %d\r\n", (sendCount)%60);
  delay(500);
  sendCount++;
  Serial2.println("I/ wait response");
  delay(500);
  Serial2.println("1, SWVer, M5StickUARTV001");
  delay(500);
  Serial2.println("2, State, Receiving");
  delay(500);
  Serial2.printf("5, Count3, %d\r\n", sendCount);
  delay(500);
  Serial2.printf("6, VDD, %d\r\n", (sendCount+20)%60);
  delay(500);
  sendCount++;
}
