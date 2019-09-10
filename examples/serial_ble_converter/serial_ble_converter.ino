//  note:
//    use Arduino 1.8.5
//    Arduino board manager
//      https://dl.espressif.com/dl/package_esp32_index.json
//    ボード ESP32 Dev Module
//    Flash Mode  QIO
//    Flash Frequency 80MHz
//    Flash Size  4M (32Mb)
//    Partition Scheme  No OTA (2MB APP/2MB FATFS) <- important!
//    Upload Speed  115200
//    Core Debug Level なし

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include <WiFiMulti.h>
#include <TFT_eSPI.h>

#define TaskCore1 1
#define TaskCore0 0
#define TaskStack4K 4096
#define TaskStack8K 8192
#define Priority1 1
#define Priority2 2
#define Priority3 3
#define Priority4 4
#define Priority5 5

BLEServer *pServer = NULL;
BLECharacteristic * pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

// definition for received data buffer
#define RCV_BUF_NUM 4
#define RCV_BUF_SIZE 100
int writePtr;
int rcvBuffUsedNum;
int readPtr;
char rcvBuffs[RCV_BUF_NUM][RCV_BUF_SIZE];
char *inputBuff;
char *copySrcBuff;
char copiedRcvBuff[RCV_BUF_SIZE];

portMUX_TYPE rcvBuffMux = portMUX_INITIALIZER_UNLOCKED;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

void storeReceivedString(char *rcvStr, int length) {
  int unEnteredSize = length;
  int processedCharCount = 0;

  while( unEnteredSize > 0 ) {
    processedCharCount += input2ReceivedBuff(&(rcvStr[processedCharCount]), unEnteredSize);
    unEnteredSize = length - processedCharCount;
  }

}

int input2ReceivedBuff(char *rcvStr, int length) {
  byte rcvCnt = 0;
  int processedCount = 0;

  portENTER_CRITICAL_ISR(&rcvBuffMux);
  if(rcvBuffUsedNum >= RCV_BUF_NUM) {
    return length; // all buff is used
  }
  portEXIT_CRITICAL_ISR(&rcvBuffMux);

  for (int i = 0; i < length; i++)
  {
      char rcvData = rcvStr[i];
      if( rcvCnt >= (RCV_BUF_SIZE-1) ) {
        break; // rcvCnt is max(this buff is full)
      }
      else if (rcvData > 31 && rcvData < 127) { //filter of displayable ascii data
        if( rcvCnt < (RCV_BUF_SIZE-1) ) {
          inputBuff[rcvCnt] = rcvData;
          rcvCnt++;
        }
      }
      processedCount++;
  }

  if(rcvCnt > 0) {
    inputBuff[rcvCnt] = '\0';
    portENTER_CRITICAL_ISR(&rcvBuffMux);
    rcvBuffUsedNum++;
    if (rcvBuffUsedNum < RCV_BUF_NUM) {
      writePtr = (writePtr + 1) % RCV_BUF_NUM;
      inputBuff = rcvBuffs[writePtr]; //write to next buffer
    }
    portEXIT_CRITICAL_ISR(&rcvBuffMux);
  }

  return processedCount;
}

int getReceivedString(char *dstBuffPtr) {
  int len = 0;
  
  portENTER_CRITICAL_ISR(&rcvBuffMux);
  if (rcvBuffUsedNum > 0) {
    copySrcBuff = rcvBuffs[readPtr];
    len = strlen(copySrcBuff) + 1;
    strncpy(dstBuffPtr, copySrcBuff, len);
    rcvBuffUsedNum--;
    readPtr = (readPtr + 1) % RCV_BUF_NUM;
  }
  portEXIT_CRITICAL_ISR(&rcvBuffMux);
    
  return len;
}

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();

      if (rxValue.length() > 0) {
        storeReceivedString((char*)rxValue.c_str(), rxValue.length());
      }
    }
};


void setup()
{
  Serial.begin(115200); // Debug Print
  WiFi.disconnect(true); //disable wifi

//  Serial2.begin(115200, SERIAL_8N1, 0, 26); // EXT_IO( RX(G0), TX(G26) )
  Serial2.begin(115200, SERIAL_8N1, 26, 27); //rx:G26, tx:G27

  inputBuff = rcvBuffs[writePtr];
  rcvBuffUsedNum = 0;
  writePtr = 0;
  readPtr = 0;
   
  // create tasks
  xTaskCreatePinnedToCore(task0, "Task0", TaskStack4K, NULL, Priority2, NULL, TaskCore0);
}

char sRcvBuff[RCV_BUF_SIZE]; //serial rx buffer
boolean receiveSerialCR = false;
byte rcvCnt = 0;

void loop()
{
  char rcvData;

  // receive uart data
  while (Serial2.available()) {
    rcvData = Serial2.read();
    if (rcvData > 31 && rcvData < 127) { //filter of displayable ascii data
      if( rcvCnt < (RCV_BUF_SIZE-1) ) {
        sRcvBuff[rcvCnt] = rcvData;
        rcvCnt++;
      }
    }
    else if (rcvData == '\r') {
      sRcvBuff[rcvCnt] = '\0';
      receiveSerialCR = true;
    }
    delay(1);
  }
  if(receiveSerialCR) {
    receiveSerialCR = false;
    rcvCnt = 0;
    Serial.println("uart received string:");
    Serial.println(sRcvBuff);
  }
}

void task0(void* arg)
{
  Serial.println("task0");

  // Create the BLE Device
  BLEDevice::init("UART Service");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pTxCharacteristic = pService->createCharacteristic(
                    CHARACTERISTIC_UUID_TX,
                    BLECharacteristic::PROPERTY_NOTIFY
                  );
                      
  pTxCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic * pRxCharacteristic = pService->createCharacteristic(
                       CHARACTERISTIC_UUID_RX,
                      BLECharacteristic::PROPERTY_WRITE
                    );

  pRxCharacteristic->setCallbacks(new MyCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");

  while(1)
  {

    if (deviceConnected) {
      int len = getReceivedString(copiedRcvBuff);
      if (len > 0) {
        Serial.println("received string:");
        Serial.println(copiedRcvBuff);
        pTxCharacteristic->setValue(copiedRcvBuff);
        pTxCharacteristic->notify();          
      }
    }

    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        Serial.println("start advertising");
        oldDeviceConnected = deviceConnected;
    }

    // connecting
    if (deviceConnected && !oldDeviceConnected) {
    // do stuff here on connecting
        oldDeviceConnected = deviceConnected;
    }  

    delay(1); // for clear watch dog timer

  }

}


