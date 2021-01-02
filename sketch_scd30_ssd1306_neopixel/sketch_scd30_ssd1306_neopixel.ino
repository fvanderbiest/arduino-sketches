
#include "Wire.h"
#include "SparkFun_SCD30_Arduino_Library.h" 
#include "ssd1306.h"
#include "Adafruit_NeoPixel.h"
#include "TaskScheduler.h"
#include "Bounce2.h"
#include "FlashStorage.h"

#define NUMPIXELS              7
#define MAX_BRIGHTNESS_AT_PPM  2000
#define PIN_WS2812             2
#define PIN_BTN_SCREEN_MODE           7
#define PIN_BTN_LED_MODE              10
#define PIN_BTN_ELEVATION_PLUS           8
#define PIN_BTN_ELEVATION_MINUS          9

/*
 * Heart image below is defined directly in flash memory.
 * This reduces SRAM consumption.
 * The image is defined from bottom to top (bits), from left to
 * right (bytes).
 */
const PROGMEM uint8_t heartImage[8] =
{
    0B00001110,
    0B00011111,
    0B00111111,
    0B01111110,
    0B01111110,
    0B00111101,
    0B00011001,
    0B00001110
};

const PROGMEM uint8_t thresholdImage[8] =
{
  B11111000,
  B11111000,
  B00011000,
  B00011000,
  B00011000,
  B00011000,
  B00011111,
  B00011111
};

const PROGMEM uint8_t continuumImage[8] =
{
  0B11000000,
  0B11100000,
  0B01110000,
  0B00111000,
  0B00011100,
  0B00001110,
  0B00000111,
  0B00000011
};

const PROGMEM uint8_t resetImage[8] =
{
  0B00000000,
  0B00000000,
  0B00000000,
  0B00000000,
  0B00000000,
  0B00000000,
  0B00000000,
  0B00000000
};

typedef struct {
  boolean valid;
  /*
   * 0 screen off
   * 1 screen on
   */
  unsigned int screenMode;
  
  /*
   * 0 led off
   * 1 led on, hard tresholds
   * 2 led on, continuous color
   */
  unsigned int ledMode;
  //unsigned int elevation; // TODO
} SETTINGS;
SETTINGS settings;
FlashStorage(flash_settings, SETTINGS);

int BRIGHTNESS = 100; //1-255
int ELEVATION = 300;
int PRESSURE = 1000;
String disp;
boolean screenRequiresRefresh = true;
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
Button ledButton = Button();
Button screenButton = Button();
Button plusButton = Button();
Button minusButton = Button();
SPRITE thresholdSprite;
SPRITE continuumSprite;
SPRITE resetSprite;

void setup() {
  Serial.begin(115200);

  settings = flash_settings.read();
  if (settings.valid == false) {
    settings.ledMode = 2; // initialize with continuous mode
    settings.screenMode = 1; // initialize screen on
    settings.valid = true;
    flash_settings.write(settings);
  }

  // this btn allows to change the LED behavior: off / on-continuous / on-thresholds
  ledButton.attach(PIN_BTN_LED_MODE, INPUT_PULLUP);
  ledButton.interval(10);
  // HIGH state corresponds to physically pressing the button:
  ledButton.setPressedState(HIGH);
  
  // this btn allows to change the screen display: off / on-ppm / on-everything
  screenButton.attach(PIN_BTN_SCREEN_MODE, INPUT_PULLUP);
  screenButton.interval(10);
  screenButton.setPressedState(HIGH);
  
  // this btn increments current elevation
  plusButton.attach(PIN_BTN_ELEVATION_PLUS, INPUT_PULLUP);
  plusButton.interval(10);
  plusButton.setPressedState(HIGH);
  
  // this btn decrements current elevation
  minusButton.attach(PIN_BTN_ELEVATION_MINUS, INPUT_PULLUP);
  minusButton.interval(10);
  minusButton.setPressedState(HIGH);

  neopixel.begin();
  neopixel.setBrightness(BRIGHTNESS);
  neopixel.fill(neopixel.Color(0,255,0), 0, NUMPIXELS);
  if (settings.ledMode > 0) {
    neopixel.show();
  }
  
  ssd1306_128x64_i2c_init();
  ssd1306_fillScreen(0x00);
  if (settings.screenMode > 0) {
    ssd1306_setFixedFont(ssd1306xled_font8x16);
    ssd1306_printFixed (0,  8, "Real time CO2", STYLE_BOLD);
    ssd1306_setFixedFont(ssd1306xled_font6x8);
    ssd1306_printFixed (0, 39, "Initializing...", STYLE_NORMAL);
  }

  SPRITE sprite;
  sprite = ssd1306_createSprite(128-8-1, 8, 8, heartImage);
  sprite.draw();

  thresholdSprite = ssd1306_createSprite(128-8-1, 8, 8, thresholdImage);
  continuumSprite = ssd1306_createSprite(128-8-1, 8, 8, continuumImage);
  resetSprite = ssd1306_createSprite(128-8-1, 8, 8, resetImage);

  Wire.begin();
  Wire.setClock(50000); // 50kHz, recommended for SCD30

  if (airSensor.begin() == false) {
    // should not happen !
    Serial.println("CO2 sensor not detected !");
    while (1);
  }

  // Set altitude of the sensor in m
  // TODO: let the user provide it !
  airSensor.setAltitudeCompensation(ELEVATION);

  // Current ambient pressure in mBar: 700 to 1200
  // TODO: measure it !
  airSensor.setAmbientPressure(1000);

  // Set temperature offset to compensate for self-heating
  airSensor.setTemperatureOffset(3.2);

  runner.init();
  runner.addTask(mainTask);
  mainTask.enable();
}

void refreshLedModeSprite() {
  switch (settings.ledMode) {
    case 1:
      thresholdSprite.draw();
      break;
    case 2:
      continuumSprite.draw();
      break;
    default:
      resetSprite.draw();
      break;
  }
}

void measure() {

  if (!airSensor.dataAvailable()) {
    return;
  }

  taux_co2 = airSensor.getCO2();
  if (taux_co2 < 400) {
    // happens sometimes at startup
    return;
  }
  if (screenRequiresRefresh == true) {
    ssd1306_fillScreen(0x00);
    screenRequiresRefresh = false;
  }
  temp = airSensor.getTemperature();
  dtostrf(temp, 4, 1, formattedTemp);
  int taux_hum = airSensor.getHumidity();


  if (settings.screenMode > 0) {
    refreshLedModeSprite();
    ssd1306_setFixedFont(ssd1306xled_font8x16);
    disp = String(taux_co2) + " ppm";
    ssd1306_printFixed (0,  8, disp.c_str(), STYLE_BOLD);
    ssd1306_setFixedFont(ssd1306xled_font6x8);
    disp = "temperature: " + String(formattedTemp) + " C";
    ssd1306_printFixed (0,  39, disp.c_str(), STYLE_NORMAL);
    
    disp = "hygrometry: " + String(taux_hum) + " %";
    ssd1306_printFixed (0,  54, disp.c_str(), STYLE_NORMAL);
  }

  if (settings.ledMode == 0) {
    return;
  }
  
  if (taux_co2 < 600) {
    red = 0;
  } else if (taux_co2 > 800) {
    red = 255;
  } else {
    if (settings.ledMode == 2) {
      // continuum mode
      red = int(255*(taux_co2-600)/200);
    } else {
      // threshold mode
      red = 0;
    }
  }

  if (taux_co2 < 800) {
    green = 255;
  } else if (taux_co2 > 1000) {
    green = 0;
  } else {
    if (settings.ledMode == 2) {
      green = int(255*(1000-taux_co2)/200);
    } else {
      green = 255;
    }
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

  ledButton.update();
  screenButton.update();
  plusButton.update();
  minusButton.update();

  if (ledButton.pressed()) {
    settings.ledMode += 1;
    // this btn allows to change the LED behavior: off / on-thresholds / on-continuous
    settings.ledMode = settings.ledMode % 3;

    ssd1306_fillScreen(0x00);
    ssd1306_setFixedFont(ssd1306xled_font8x16);
    if (settings.ledMode == 0) {
      neopixel.clear();
      neopixel.show();
      ssd1306_printFixed (0,  8, "LED OFF", STYLE_BOLD);
    } else {
      ssd1306_printFixed (0,  8, "LED ON", STYLE_BOLD);
      ssd1306_setFixedFont(ssd1306xled_font6x8);
    }
    if (settings.ledMode == 1) {
      ssd1306_printFixed (0, 39, "Threshold mode", STYLE_NORMAL);
    } else if (settings.ledMode == 2) {
      ssd1306_printFixed (0, 39, "Continuum mode", STYLE_NORMAL);
    }
    refreshLedModeSprite();
    delay(1000);
    screenRequiresRefresh = true;
    flash_settings.write(settings);
  }

  if (screenButton.pressed()) {
    // this btn allows to change the screen display: off / on-ppm
    settings.screenMode += 1;
    settings.screenMode = settings.screenMode % 2;
    if (settings.screenMode == 0) {
      ssd1306_fillScreen(0x00);
    } else {
      ssd1306_setFixedFont(ssd1306xled_font8x16);
      ssd1306_printFixed (0,  8, "Display ON", STYLE_BOLD);
      ssd1306_setFixedFont(ssd1306xled_font6x8);
      ssd1306_printFixed (0, 39, "Initializing...", STYLE_NORMAL);
      screenRequiresRefresh = true;
    }
    flash_settings.write(settings); // TODO: check if write is required every minute, and perform it (soft debounce to prevent wear)
  }

  if (plusButton.pressed()) {
    // TODO
  }

  if (minusButton.pressed()) {
    // TODO
    //elevation -= 10;
    //airSensor.setAltitudeCompensation(elevation);
  }
  
}
