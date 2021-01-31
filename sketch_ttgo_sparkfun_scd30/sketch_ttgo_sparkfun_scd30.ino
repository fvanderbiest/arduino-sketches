#include "Wire.h"
#include "TFT_eSPI.h"
#include "SPI.h"
#include "SparkFun_SCD30_Arduino_Library.h"
#include "TaskScheduler.h"
// splash screen:
#include "bmp.h"
// fonts:
#include "Purisa_48.h"
#include "Purisa_24.h"
#include "Purisa_12.h"
/*
Font files in the .vlw format can be created using Processing out of any font which is installed in your system.

The process is simple: In the processing IDE,
- go to Tools / Create Font
- pick a font and a size and accept.

If you click then Sketch / Show sketch folder you'll be able to see the font in the data folder. 
Now you can convert it to an array using https://tomeko.net/online_tools/file_to_hex.php?lang=en
Fonts as arrays are loaded as tabs.
*/

void measure();
Task mainTask(2000, TASK_FOREVER, &measure);

SCD30 scd30;
TFT_eSPI tft = TFT_eSPI(135, 240); // pins defined in User_Setup.h
Scheduler runner;
int co2;

// For long time delay, it is recommended to use shallow sleep, which can effectively reduce the current consumption
void espDelay(int ms)
{
    esp_sleep_enable_timer_wakeup(ms * 1000);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    esp_light_sleep_start();
}

void setup()
{
  Serial.begin(115200);
  Wire.setClock(50000); // 50kHz, recommended for SCD30
  Serial.println("SCD30");
  Wire.begin();
  tft.init();
  tft.setRotation(0);
  
  tft.fillScreen(TFT_WHITE);

  // splash screen:
  tft.setSwapBytes(true);
  tft.pushImage(0, 0,  135, 240, c2c);
  espDelay(5000);

  if (scd30.begin() == false) {
    Serial.println("Air sensor not detected.");
    while (1);
  }
  scd30.setAmbientPressure(1000);
  scd30.setTemperatureOffset(3);

  runner.init();
  runner.addTask(mainTask);
  mainTask.enable();
}

void measure() {
  if (!scd30.dataAvailable()) {
    return;
  }

  Serial.print("co2(ppm):");
  co2 = scd30.getCO2();
  Serial.print(co2);
  Serial.print(" temp(C):");
  Serial.print(scd30.getTemperature(), 1);
  Serial.print(" humidity(%):");
  Serial.print(scd30.getHumidity(), 1);
  Serial.println();

  if (co2 > 1000) {
    tft.setTextColor(TFT_BLACK, TFT_RED);
    tft.fillScreen(TFT_RED);
  } else if (co2 < 800) { 
    tft.setTextColor(TFT_BLACK, TFT_GREEN);
    tft.fillScreen(TFT_GREEN);
  } else {
    tft.setTextColor(TFT_BLACK, TFT_ORANGE);
    tft.fillScreen(TFT_ORANGE);
  }
  tft.setTextDatum(MC_DATUM);

  tft.loadFont(Purisa_48);
  tft.drawString("CO2", tft.width() / 2, 40);
  tft.drawString(String(co2), tft.width()/2, tft.height()/2);
  tft.unloadFont(); // Unload the font to recover used RAM
  
  tft.loadFont(Purisa_24);
  tft.drawString("ppm", tft.width() - 55, tft.height()/2 + 40);
  tft.unloadFont();


  tft.loadFont(Purisa_12);
  if (co2 > 1000) {
    tft.drawString("Leave room now !", tft.width()- 70, tft.height() - 30);
  } else if (co2 < 800) { 
    tft.drawString("All good !", tft.width()- 70, tft.height() - 30);
  } else {
    tft.drawString("Open the windows...", tft.width()- 70, tft.height() - 30);
  }
  tft.unloadFont();
}

void loop()
{
  runner.execute();
}
