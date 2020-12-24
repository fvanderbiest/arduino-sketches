
#include "Wire.h"
#include "SparkFun_SCD30_Arduino_Library.h" 
#include "ssd1306.h"
#include "Adafruit_NeoPixel.h"
#include "TaskScheduler.h"

#define NUMPIXELS              7
#define MAX_BRIGHTNESS_AT_PPM  2000
#define PIN_WS2812             2
#define PIN_BTN_MODE           7
#define PIN_BTN_PLUS           8
#define PIN_BTN_MINUS          9

int BRIGHTNESS = 100; //1-255
int ELEVATION = 300;
int PRESSURE = 1000;
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
int modeButtonState = 0;
int plusButtonState = 0;
int minusButtonState = 0;
Task mainTask(1000, TASK_FOREVER, &measure);
SCD30 airSensor;
Adafruit_NeoPixel neopixel = Adafruit_NeoPixel(NUMPIXELS, PIN_WS2812, NEO_GRB + NEO_KHZ800);
Scheduler runner;

// A callback contains both a function and a pointer to arbitrary data
// that will be passed as argument to the function.
// see https://arduino.stackexchange.com/questions/23063/user-callback-functions
struct Callback {
    Callback(void (*f)(void *) = 0, void *d = 0)
        : function(f), data(d) {}
    void (*function)(void *);
    void *data;
};


void update_elevation(void *data)
{
  int elevation = * (int *) data;
  //ELEVATION = elevation;
  airSensor.setAltitudeCompensation(elevation);
  Serial.println("elevation updated");
}

void update_pressure(void *data)
{
  int pressure = * (int *) data;
  airSensor.setAmbientPressure(pressure);
  Serial.println("pressure updated");
}

void update_brightness(void *data)
{
  int brightness = * (int *) data;
  BRIGHTNESS = brightness;
  Serial.println("brightness updated");
}


class SetupScreen {
  private:
    String caption;
    unsigned int value;
    unsigned int minValue;
    unsigned int maxValue;
    unsigned int increment;
    void (*myCallback)();

  public:
    SetupScreen(String caption, unsigned int value, unsigned int minValue, unsigned int maxValue, void (*callback)(), unsigned int increment = 1) {

      this->caption = caption;
      this->value = value;
      this->minValue = minValue;
      this->maxValue = maxValue;
      this->increment = increment;
      this->myCallback = callback;
    }

    void plus() {
      if (value <= maxValue - increment) {
        value += increment;
        myCallback.function(value);
        display();
        //callbacks[i].function(callbacks[i].data);
      }
    }

    void minus() {
      if (value >= minValue + increment) {
        value -= increment;
        myCallback.function(value);
        display();
        //myCallback(value);
      }
    }

    void display() {
      ssd1306_fillScreen(0x00);
      ssd1306_setFixedFont(ssd1306xled_font8x16);
      ssd1306_printFixed (0,  8, caption.c_str(), STYLE_BOLD);
      ssd1306_setFixedFont(ssd1306xled_font6x8);
      ssd1306_printFixed (0, 39, String(value).c_str(), STYLE_NORMAL);
    }
};


class ModeSwitcher {
  private:
    unsigned int setupScreenNumber = 0;
    SetupScreen setupScreen1 = SetupScreen("Altitude", 300, 0, 8800, Callback(update_elevation, &ELEVATION), 10);
    SetupScreen setupScreen2 = SetupScreen("Pression", 1000, 700, 1200, Callback(update_pressure, &PRESSURE), 10);
    SetupScreen setupScreen3 = SetupScreen("Luminosite", 50, 0, 100, Callback(update_brightness, &BRIGHTNESS), 1);
    SetupScreen screensArray[3] = {setupScreen1, setupScreen2, setupScreen3}; 
    
  public:
    ModeSwitcher() {
      /*
      SetupScreen setupScreen1 = SetupScreen("Altitude", 300, 0, 8800, 10);
      SetupScreen setupScreen2 = SetupScreen("Pression", 1000, 700, 1200, 10);
      SetupScreen setupScreen3 = SetupScreen("Luminosite", 50, 0, 100);
      this->screensArray = {setupScreen1, setupScreen2, setupScreen3};
      */
      pinMode(PIN_BTN_MODE, INPUT); // this btn allows to circle through setup screens
      pinMode(PIN_BTN_PLUS, INPUT); // this btn increments the value of the screen currently displayed
      pinMode(PIN_BTN_MINUS, INPUT); // this btn decrements the value of the screen currently displayed
    }

    void next() {
      setupScreenNumber += 1;
      screensArray[setupScreenNumber].display();
    }

    void plus() {
      screensArray[setupScreenNumber].plus();
    }

    void minus() {
      screensArray[setupScreenNumber].minus();
    }
};


ModeSwitcher switcher;

void setup() {
  Serial.begin(115200);

  //SetupScreen setupScreen1 = SetupScreen("Altitude", 300, 0, 8800, 10);
  //ModeSwitcher switcher;

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
    // should not happen !
    Serial.println("CO2 sensor not detected !");
    while (1);
  }

  // Set altitude of the sensor in m
  // TODO: let the user provide it !
  airSensor.setAltitudeCompensation(300);

  // Current ambient pressure in mBar: 700 to 1200
  // TODO: measure it !
  airSensor.setAmbientPressure(1020);

  // Set temperature offset to compensate for self-heating
  airSensor.setTemperatureOffset(3.2);

  runner.init();
  runner.addTask(mainTask);
  mainTask.enable();
}

void measure() {
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

  if (taux_co2 < 600) {
    red = 0;
  } else if (taux_co2 > 800) {
    red = 255;
  } else {
    red = int(255*(taux_co2-600)/200);
  }

  if (taux_co2 < 800) {
    green = 255;
  } else if (taux_co2 > 1000) {
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

  modeButtonState = digitalRead(PIN_BTN_MODE);
  plusButtonState = digitalRead(PIN_BTN_PLUS);
  minusButtonState = digitalRead(PIN_BTN_MINUS);

  if (modeButtonState) {
    switcher.next();
    delay(500);
  }
  if (plusButtonState) {
    switcher.plus();
    delay(500);
  }
  if (minusButtonState) {
    switcher.minus();
    delay(500);
  }
  
}
