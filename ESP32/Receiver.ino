/*********
  This is code instantiates one ESPNow Wifi receiver with the following custom MAC address: 0x94, 0xB9, 0x7E, 0xD2, 0x14, 0x1C
  It matches the sender code on this repo for a total of two Thales controllers.
  Works on any ESP32 board viare serial.
  
  Code is based on Rui Santos EspNow projectat https://RandomNerdTutorials.com/esp-now-many-to-one-esp32/
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*********/
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

#include <Adafruit_TinyUSB.h>
#include <MIDI.h>
#include <USB.h>

// MIDI over USB setup
Adafruit_USBD_MIDI usb_midi;
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, MIDI);

// Set your new MAC Address
uint8_t newMACAddress[] = {0x94, 0xB9, 0x7E, 0xD2, 0x14, 0x1C};

// Data structure received via ESPNow
typedef struct struct_message {
  int id;
  float anglex, angley, anglez;
  float accx, accy, accz;
  int battery;
} struct_message;

struct_message myData;
struct_message boardsStruct[2];

// === CALLBACK ===
void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
  memcpy(&myData, incomingData, sizeof(myData));
  boardsStruct[myData.id - 1] = myData;

  //Serial.printf("Board %u | ANGl: %.2f %.2f %.2f | ACC: %.2f %.2f %.2f | Battery: %u\n",
  //              myData.id, myData.anglex, myData.angley, myData.anglez,
  //              myData.accx, myData.accy, myData.accz, myData.battery);

  // === SEND MIDI via USB ===
  byte channel = myData.id;  // or just 1 if you want to keep it fixed

  // Map and send angles (example CCs: 10,11,12)
  MIDI.sendControlChange(10, map(myData.anglex, 0, 360, 0, 127), channel);
  MIDI.sendControlChange(11, map(myData.angley, -180, 180, 0, 127), channel);
  MIDI.sendControlChange(12, map(myData.anglez, -180, 180, 0, 127), channel);

  // Map and send acceleration (example CCs: 13,14,15)
  MIDI.sendControlChange(13, map(myData.accx, -15, 15, 0, 127), channel);
  MIDI.sendControlChange(14, map(myData.accy, -15, 15, 0, 127), channel);
  MIDI.sendControlChange(15, map(myData.accz, -15, 15, 0, 127), channel);

  // Optionally, send battery level as CC (e.g., CC#20)
  MIDI.sendControlChange(20, map(myData.battery, 0.5, 4700, 0, 127), channel);
}

void setup() {

 // if (!TinyUSBDevice.isInitialized()){
 //   TinyUSBDevice.begin(0);
  //}

  //Serial.begin(115200);
  // while (!Serial);  // Wait for Serial Monitor to open

  USB.begin();
  
 
  // Optional: helps with USB device name
  TinyUSBDevice.setManufacturerDescriptor("Privato & Skialivas");
  TinyUSBDevice.setProductDescriptor("MidiSense");

  //usb_midi.begin();   // Start USB MIDI

  //if (TinyUSBDevice.mounted()) {
  //  TinyUSBDevice.detach();
  //  delay(100);
  //  TinyUSBDevice.attach();
  //}

  MIDI.begin(MIDI_CHANNEL_OMNI);  // Listen to all MIDI channels

  // Force USB to re-enumerate (important!)
  if (TinyUSBDevice.mounted()) {
    TinyUSBDevice.detach();
    delay(100);
    TinyUSBDevice.attach();
  }



  WiFi.mode(WIFI_STA);
  esp_wifi_set_mac(WIFI_IF_STA, &newMACAddress[0]);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  
 
  }

  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
  // Nothing to do here — everything happens in OnDataRecv()
}
