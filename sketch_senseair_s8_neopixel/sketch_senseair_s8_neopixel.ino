#include "SoftwareSerial.h"
#include "Adafruit_NeoPixel.h"
#include "TaskScheduler.h"

#define RX_PIN 3
#define TX_PIN 4
#define NUMPIXELS 1
#define PIN_WS2812 2
#define BRIGHTNESS 50
#define MEASURE_PERIOD 5000

void measure();
void blinkLed();
Task mainTask(MEASURE_PERIOD, TASK_FOREVER, &measure);
Task blinkTask(500, TASK_FOREVER, &blinkLed);
Scheduler runner;
SoftwareSerial SoftSerial_CO2(RX_PIN, TX_PIN);
Adafruit_NeoPixel neopixel = Adafruit_NeoPixel(NUMPIXELS, PIN_WS2812);

int getCO2(){
  int retry = 0;
    const byte CO2Command[] = {0xFE, 0X44, 0X00, 0X08, 0X02, 0X9F, 0X25};
    byte CO2Response[] = {0,0,0,0,0,0,0};

    while(!(SoftSerial_CO2.available())) {
        retry++;
        // keep sending request until we start to get a response
        SoftSerial_CO2.write(CO2Command, 7);
        delay(50);
        if (retry > 10) {
            return -1;
        }
    }

    int timeout = 0; 
    
    while (SoftSerial_CO2.available() < 7) {
        timeout++; 
        if (timeout > 10) {
            while(SoftSerial_CO2.available())  
            SoftSerial_CO2.read();
            break;                    
        }
        delay(50);
    }

    for (int i=0; i < 7; i++) {
        int byte = SoftSerial_CO2.read();
        if (byte == -1) {
            return -1;
        }
        CO2Response[i] = byte;
    }  
    int valMultiplier = 1;
    int high = CO2Response[3];
    int low = CO2Response[4];
    unsigned long val = high*256 + low;

    return val;
}

void fillLed(boolean red) {
  if (red) {
    neopixel.fill(neopixel.Color(255, 0, 0), 0, NUMPIXELS);
  } else {
    neopixel.fill(neopixel.Color(0, 255, 0), 0, NUMPIXELS);
  }
}

boolean ledOn = false;
void blinkLed() {
  if (ledOn) {
    neopixel.clear();
    ledOn = false;
  } else {
    fillLed(true);
    ledOn = true;
  }
  neopixel.show();
}

void measure() {
  int taux_co2 = getCO2();

  if (taux_co2 < 1000) {
    blinkTask.disable();
    if (taux_co2 < 800) {
      fillLed(false);
    } else {
      fillLed(true);
    }
    neopixel.show();
  } else {
    if (taux_co2 >= 2000) {
      blinkTask.setInterval(100);
    } else {
      blinkTask.setInterval(1000-900*(taux_co2-1000)/1000);
    }
    blinkTask.enable();
  }
}

void setup(){
  neopixel.begin();
  neopixel.setBrightness(BRIGHTNESS);

  SoftSerial_CO2.begin(9600);
  
  runner.init();
  runner.addTask(mainTask);
  mainTask.enable();
  runner.addTask(blinkTask);
}

void loop(){
  runner.execute();
}
