//  note:
//    use Arduino 1.8.5
//    Arduino board manager
//      https://dl.espressif.com/dl/package_esp32_index.json
//    ボード ESP32 Dev Module
//    Flash Mode  QIO
//    Flash Frequency 80MHz
//    Flash Size  4M (32Mb)
//    Partition Scheme  No OTA (2MB APP/2MB FATFS)
//    Upload Speed  115200
//    Core Debug Level なし

void setup() {
  Serial.begin(115200); // Debug Print
  Serial2.begin(115200, SERIAL_8N1, 26, 27); //rx:G26, tx:G27
}
 
void loop() {
  static int sendCount = 0;
  Serial2.printf("send %d\r\n", sendCount);
  delay(500);
  sendCount = (sendCount+1) % 100;
}
