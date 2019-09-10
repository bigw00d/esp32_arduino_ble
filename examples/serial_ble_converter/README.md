# serial_ble_converter

Converter from serial UART to BluetoothLE

## Requirement

- Arduino Version: 1.8.5


## Hardware Connections

|M5Stick-C (UART Receiver)  |ESP32 (UART Transmitter)  |
|---|---|
|IO26  |IO27  |
|GND  |GND  |

## Task information in M5Stick-C application

|   |Core   |Job  |
|---|---|---|
|MainTask  |core 0 (main loop)  |UART Receiver |
|Task0 |core 0  |BLE Transmitter |
