  /****
  in the previous sketch (333)
  - the sleep works also for esp32 also for the sensor
  - for the BNO055 i added a function to the library: enterTrueSleepMode()
  - it wakes up with the BOOT button (but this causes problems when connecting to USB: you have to enter flashing mode to make it upload)

  In this version, I test
  1) Deactivagte the BOOT button for wake-up - it will be doable with the RESET - DONE
  2) If the suspendMode is necessary. With this mode activated when it goes to sleep the consumption falls from  15mA to ~0.5 mA. - DONE

  3) Add led indication for waking up (3 blinks) and sleep mode (1 blink).
  





/*********
  This instantiates an ESPNow Wifi connection from one Thales controller to the receiver unit.
  Upload as is on the first controller (left hand). When uploading on the second controller change the board ID to 2 (line 162).

  Code is based on Rui Santos ESPNow project at https://RandomNerdTutorials.com/esp-now-many-to-one-esp32/
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*********/


// VERY IMPORTANT: IN ORDER TO UPLOAD THE SKETCH YOU HAVE TO MANUALLY BOOT THE MICROCONTROLLER INTO FLASHING MODE BY PRESSING BOOT (LOW) AND RESET. OTHERWISE THE USB WILL NOT BE RECOGNISED DUE TO THE CHECK FOR THE FALSE WAKE (IT IGNORES INSTANT PRESSES OF BOOT = SPIKES OF VOLTAGE ON THE GPIO0 WHICH ALSO OCCURS WHEN YOU PLUG THE USB)


#include <esp_now.h>
#include <Wire.h>
#include <WiFi.h>
#include <Adafruit_BNO055.h>
//#include "Adafruit_MLX90393.h"

#include <esp_sleep.h>                // Required for deep sleep functions
RTC_DATA_ATTR float last_anglex = 0;  // Persist across deep sleep

#include <Adafruit_NeoPixel.h>
// How many internal neopixels do we have? some boards have more than one!
#define NUMPIXELS 1
Adafruit_NeoPixel pixels(NUMPIXELS, PIN_NEOPIXEL, NEO_GRB + NEO_KHZ800);



//Adafruit_MLX90393 sensor = Adafruit_MLX90393();
Adafruit_BNO055 bno = Adafruit_BNO055(55, 0x28);
//#define MLX90393_CS 10


//timer for transmission freq
unsigned long timer = 0;

// REPLACE WITH THE RECEIVER'S MAC Address
uint8_t broadcastAddress[] = { 0x94, 0xB9, 0x7E, 0xD2, 0x14, 0x1C };

// Structure example to send data
// Must match the receiver structure
typedef struct struct_message {
  int id;  // must be unique for each sender board
           //magnet
  //float hallx;
  //float hally;
  //float hallz;
  //BNO055 angle
  float anglex;
  float angley;
  float anglez;
  //BNO055accelerometer
  float accx;
  float accy;
  float accz;
  int battery;

} struct_message;

// Create a struct_message called myData
struct_message myData;

// Create peer interface
esp_now_peer_info_t peerInfo;

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}


void goToSleepUntilBootPressed() {
  // Put BNO055 into low power suspend mode
  bno.enterTrueSuspendMode();

  digitalWrite(NEOPIXEL_POWER, LOW);  // Cut power to the LED

  //**pixels.setPixelColor(0, pixels.Color(0, 0, 0));  // Blue
  //**esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);  // Clear all previous wake sources
  // # esp_sleep_enable_ext0_wakeup((gpio_num_t)0, 0);         // GPIO0 (BOOT) → wake on LOW
  //**Serial.println("Going to sleep due to no change. Wake on RESET button.");
  //*delay(100);  // Give Serial time to flush
  esp_deep_sleep_start();
}


void setup() {

  #if defined(NEOPIXEL_POWER)
 // If this board has a power control pin, we must set it to output and high
 // in order to enable the NeoPixels. We put this in an #if defined so it can
 // be reused for other boards without compilation errors
 pinMode(NEOPIXEL_POWER, OUTPUT);
 digitalWrite(NEOPIXEL_POWER, HIGH);
#endif
 pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
 pixels.setBrightness(20); // not so bright



  pinMode(0, INPUT_PULLUP);  // Ensure stable HIGH level

  // Init Serial Monitor
  Serial.begin(115200);
  //while (!Serial);  // Wait for Serial Monitor to open

  //Debugging for the wake reason. When you do it with the RESET the code is 0 ( = undefined reason)
  /*
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
  Serial.print("Wakeup reason: ");
  Serial.println((int)wakeup_reason);
  */

  analogReadResolution(12);

  Wire.begin(41, 40);  //I2C address for the qt connector of the ESP32-S2 Qt controller

  if (!bno.begin()) {
    while (1);
    { delay(10); }
  }

  //if (! sensor.begin_I2C())
  //{
  //  while (1) { delay(10); }
  //}

// THIS PART CHECKS IF BOOT BUTTON IS REALLY PRESSED OR IT IS THE EFFECT OF A FLOATING VALUE. (after wake-up is the button still pressed? If not, ignore it). This is not used since we de-activated the BOOT button wake-up option

/*
  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0) {
    if (digitalRead(0) == LOW) {
      Serial.println("Confirmed BOOT button press");

      // Refresh last_anglex to reset comparison baseline
      sensors_event_t orientationWake;
      bno.getEvent(&orientationWake, Adafruit_BNO055::VECTOR_EULER);
      last_anglex = orientationWake.orientation.x;
      Serial.print("Updated last_anglex after wake: ");
      Serial.println(last_anglex);
    } else {
      Serial.println("False wakeup – going back to sleep");
      goToSleepUntilBootPressed();
    }
  }
*/

  delay(10);

  /* Use external crystal for better accuracy */
  bno.setExtCrystalUse(true);

  //SETUP FOR THE MAGNETOMETER MLX90393:
  //Set gain
  //sensor.setGain(MLX90393_GAIN_1X);
  // Set resolution, per axis
  //sensor.setResolution(MLX90393_X, MLX90393_RES_19);
  //sensor.setResolution(MLX90393_Y, MLX90393_RES_19);
  //sensor.setResolution(MLX90393_Z, MLX90393_RES_19);
  // Set oversampling
  //sensor.setOversampling(MLX90393_OSR_2);
  // Set digital filtering
  //sensor.setFilter(MLX90393_FILTER_0);

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  //esp_now_register_send_cb(OnDataSent);

  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  // Add peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
}

void loop() {

  if ((millis() - timer) > 10)

  {

    int Voltout = analogRead(A2);
    Voltout = Voltout * 5000 / 4096;  //read battery charge

    //Get MXL event, normalized to uTesla */
    //sensors_event_t event, orientationData, linearAccelData;
    //sensor.getEvent(&event);
    sensors_event_t orientationData, linearAccelData;
    bno.getEvent(&orientationData, Adafruit_BNO055::VECTOR_EULER);
    bno.getEvent(&linearAccelData, Adafruit_BNO055::VECTOR_LINEARACCEL);


    // Set values to send
    myData.id = 2;  //Set ID board number
    //myData.hallx = event.magnetic.x;
    //myData.hally = event.magnetic.y;
    //myData.hallz = event.magnetic.z;
    myData.anglex = orientationData.orientation.x;
    myData.angley = orientationData.orientation.y;
    myData.anglez = orientationData.orientation.z;
    myData.accx = linearAccelData.orientation.x;
    myData.accy = linearAccelData.orientation.y;
    myData.accz = linearAccelData.orientation.z;
    myData.battery = Voltout;

    // Turn LED on dim blue when awake
    pixels.setPixelColor(0, pixels.Color(0, 0, 50));  // Blue
    pixels.show();
    


    // Send message via ESP-NOW
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&myData, sizeof(myData));
    if (result == ESP_OK) {
      Serial.println("Delivered");

    } else {
      Serial.println("Error sending the data");
    }
    timer = millis();
  }
  // Every 30 seconds, check if anglex changed
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck > 60000) {
    lastCheck = millis();

    sensors_event_t orientationCheck;
    bno.getEvent(&orientationCheck, Adafruit_BNO055::VECTOR_EULER);
    float current_anglex = orientationCheck.orientation.x;

    
    if (current_anglex == last_anglex) {
      //Serial.println("anglex unchanged → going to deep sleep (wake on BOOT)");
      goToSleepUntilBootPressed();
    } else {
      // Update the stored angle value when it changes
      last_anglex = current_anglex;
      //Serial.println("anglex changed → staying awake");
    }
    
  }
}
