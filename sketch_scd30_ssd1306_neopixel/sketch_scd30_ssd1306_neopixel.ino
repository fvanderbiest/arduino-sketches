
#include "Wire.h"
#include "SparkFun_SCD30_Arduino_Library.h" 
#include "ssd1306.h"
#include "Adafruit_NeoPixel.h"
#include "TaskScheduler.h"

#define BRIGHTNESS             100 //1-255
#define NUMPIXELS              7
#define MAX_BRIGHTNESS_AT_PPM  2000
#define PIN_WS2812             2

String disp;
boolean started = false;
int taux_co2;
int red;
int green;
int blue = 0;
int brightness;
float temp;
char formattedTemp[4];
void measure();
Task mainTask(1000, TASK_FOREVER, &measure);
SCD30 airSensor;
Adafruit_NeoPixel neopixel = Adafruit_NeoPixel(NUMPIXELS, PIN_WS2812, NEO_GRB + NEO_KHZ800);
Scheduler runner;

void setup() {
  Serial.begin(115200);

  neopixel.begin(); // INITIALIZE NeoPixel object (REQUIRED)
  neopixel.setBrightness(BRIGHTNESS);
  neopixel.fill(neopixel.Color(0,255,0), 0, NUMPIXELS);
  neopixel.show();
  
  ssd1306_128x64_i2c_init();
  ssd1306_fillScreen(0x00);
  ssd1306_setFixedFont(ssd1306xled_font8x16);
  ssd1306_printFixed (0,  8, "Real time CO2", STYLE_BOLD);
  ssd1306_setFixedFont(ssd1306xled_font6x8);
  ssd1306_printFixed (0, 39, "Initializing...", STYLE_NORMAL);

  Wire.begin();
  Wire.setClock(50000); // 50kHz, recommended for SCD30

  if (airSensor.begin() == false) {
    Serial.println("CO2 sensor not detected !");
    while (1);
  }

  airSensor.setAltitudeCompensation(300); //Set altitude of the sensor in m
  airSensor.setAmbientPressure(1020); //Current ambient pressure in mBar: 700 to 1200

  //float offset = airSensor.getTemperatureOffset();
  //Serial.print("Current temp offset: ");
  //Serial.print(offset, 2);

  runner.init();
  runner.addTask(mainTask);
  mainTask.enable();
}

void measure() {
  //Serial.println("callback");
  if (!airSensor.dataAvailable()) {
    return;
  }
    
  taux_co2 = airSensor.getCO2();
  if (taux_co2 < 400) {
    // happens at startup
    return;
  }
  if (started == false) {
    ssd1306_fillScreen(0x00);
    started = true;
  }
  temp = airSensor.getTemperature();
  dtostrf(temp, 4, 1, formattedTemp);
  int taux_hum = airSensor.getHumidity();

  ssd1306_setFixedFont(ssd1306xled_font8x16);
  disp = String(taux_co2) + " ppm";    
  ssd1306_printFixed (0,  8, disp.c_str(), STYLE_BOLD);
  ssd1306_setFixedFont(ssd1306xled_font6x8);
  disp = "temperature: " + String(formattedTemp) + " C";
  ssd1306_printFixed (0,  39, disp.c_str(), STYLE_NORMAL);

  disp = "hygrometry: " + String(taux_hum) + " %";
  ssd1306_printFixed (0,  54, disp.c_str(), STYLE_NORMAL);

  if (taux_co2 < 600){
    red = 0;
  } else if (taux_co2 > 800){
    red = 255;
  } else {
    red = int(255*(taux_co2-600)/200);
  }

  if (taux_co2 < 800){
    green = 255;
  } else if (taux_co2 > 1000){
    green = 0;
  } else {
    green = int(255*(1000-taux_co2)/200);
  }

  if (taux_co2 < 1000) {
    brightness = BRIGHTNESS;
  } else if (taux_co2 > MAX_BRIGHTNESS_AT_PPM) {
    brightness = 255;
  } else {
    brightness = BRIGHTNESS + int((255-BRIGHTNESS)*(taux_co2-1000)/(MAX_BRIGHTNESS_AT_PPM-1000));
  }

  neopixel.setBrightness(brightness);
  neopixel.fill(neopixel.Color(red, green, blue), 0, NUMPIXELS);
  neopixel.show();
}

void loop() {
  runner.execute();
}
