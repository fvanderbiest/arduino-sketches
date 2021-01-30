#include <Wire.h>
#include <TFT_eSPI.h>
#include <SPI.h>
#include "SparkFun_SCD30_Arduino_Library.h"
#include "bmp.h"

SCD30 airSensor;
TFT_eSPI tft = TFT_eSPI(135, 240); // Invoke custom library
int co2;

//! Long time delay, it is recommended to use shallow sleep, which can effectively reduce the current consumption
void espDelay(int ms)
{
    esp_sleep_enable_timer_wakeup(ms * 1000);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    esp_light_sleep_start();
}

void setup()
{
  Serial.begin(115200);
  Serial.println("SCD30");
  Wire.begin();
  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_WHITE);

  // splash screen:
  tft.setSwapBytes(true);
  tft.pushImage(0, 0,  135, 240, c2c);
  espDelay(5000);

  if (airSensor.begin() == false)
  {
    Serial.println("Air sensor not detected. Please check wiring. Freezing...");
    while (1)
      ;
  }
}

void loop()
{
  if (airSensor.dataAvailable())
  {
    
    Serial.print("co2(ppm):");
    co2 = airSensor.getCO2();
    Serial.print(co2);
/*
    Serial.print(" temp(C):");
    Serial.print(airSensor.getTemperature(), 1);

    Serial.print(" humidity(%):");
    Serial.print(airSensor.getHumidity(), 1);
*/
    Serial.println();

    if (co2 > 1000) {
      tft.setTextColor(TFT_BLACK, TFT_BLACK);
      tft.fillScreen(TFT_RED);
    } else if (co2 < 800) { 
      tft.setTextColor(TFT_BLACK, TFT_BLACK);
      tft.fillScreen(TFT_GREEN);
    } else {
      tft.setTextColor(TFT_BLACK, TFT_BLACK);
      tft.fillScreen(TFT_ORANGE);
    }
    tft.setTextDatum(MC_DATUM);

    /*
    tft.setTextSize(5);
    tft.drawString(String(co2), tft.width() / 2, 30);

    tft.setTextSize(3);
    tft.drawString("ppm", tft.width() - 40, 60);

    tft.setTextSize(5);
    tft.drawString("CO2", tft.width()/2, tft.height()/2);
    */

    tft.setTextSize(5);
    tft.drawString("CO2", tft.width() / 2, 30);

    tft.setTextSize(5);
    tft.drawString(String(co2), tft.width()/2, tft.height()/2);

    tft.setTextSize(3);
    tft.drawString("ppm", tft.width() - 50, tft.height()/2 + 30);
    
  }

  delay(2000);
}
