#include "SoftwareSerial.h"
#include "Adafruit_NeoPixel.h"
#include "TaskScheduler.h"

#define RX_PIN 3
#define TX_PIN 4
#define NUMPIXELS 1
#define PIN_WS2812 7
#define BRIGHTNESS 50
#define MEASURE_PERIOD 5000

void measure();
void blinkLed();
Task mainTask(MEASURE_PERIOD, TASK_FOREVER, &measure);
Task blinkTask(500, TASK_FOREVER, &blinkLed);
Scheduler runner;
SoftwareSerial SoftSerial(RX_PIN, TX_PIN);
Adafruit_NeoPixel neopixel = Adafruit_NeoPixel(NUMPIXELS, PIN_WS2812);

int getCO2() {
  // Inspired from https://github.com/airgradienthq/arduino
  int retry = 0;
  const byte CO2Command[] = {0xFE, 0X44, 0X00, 0X08, 0X02, 0X9F, 0X25};
  byte CO2Response[] = {0,0,0,0,0,0,0};

  while(!(SoftSerial.available())) {
    retry++;
    SoftSerial.write(CO2Command, 7);
    delay(50);
    if (retry > 10) {
      return -1;
    }
  }

  int timeout = 0;
  while (SoftSerial.available() < 7) {
    timeout++;
    if (timeout > 10) {
      while(SoftSerial.available())
      SoftSerial.read();
      break;
    }
    delay(50);
  }

  for (int i=0; i < 7; i++) {
    int byte = SoftSerial.read();
    if (byte == -1) {
      return -1;
    }
    CO2Response[i] = byte;
  }
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

  if (taux_co2 < 430) {
    blinkTask.disable();
    neopixel.fill(neopixel.Color(255, 255, 255), 0, NUMPIXELS);
    neopixel.show();
  } else if (taux_co2 < 1000) {
    blinkTask.disable();
    if (taux_co2 < 800) {
      fillLed(false);
    } else {
      fillLed(true);
    }
    neopixel.show();
  } else {
    if (taux_co2 > 2000) {
      blinkTask.setInterval(50);
    } else {
      int period = 500 - 450 * (taux_co2 - 1000) / 1000;
      if (period != blinkTask.getInterval()) {
        blinkTask.setInterval(period);
      }
    }
    blinkTask.enable();
  }
}

void setup(){
  neopixel.begin();
  neopixel.setBrightness(BRIGHTNESS);

  SoftSerial.begin(9600);
  
  runner.init();
  runner.addTask(mainTask);
  mainTask.enable();
  runner.addTask(blinkTask);
}

void loop(){
  runner.execute();
}
