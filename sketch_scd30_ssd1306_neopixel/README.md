# A C02 tracker device

Displays CO2 levels on an SSD1306 OLED screen, with temperature and hygrometry. 
Visual alert (via NeoPixels) when CO2 levels exceeds thresholds, or continuous mode where the LED color goes from green to red.

Features :
 * LED mode button switches between LED OFF / LED with color gradient / LED with hard thresholds (green < 800ppm < yellow < 1000ppm < red)
 * LED brightness increases contiuously between 1000 and 2000 ppm to catch user's eye
 * Screen mode button switches screen ON/OFF
 * Elevation buttons to adjust sensor position (50m steps)
 * Settings are permanent (saved to EEPROM) 

Parts:
 * Sensirion SCD30 sensor
 * Seeeduino XIAO
 * Adafruit Neopixel jewel
 * four push buttons
 * one diode and a 470µF capacitor (thanks Hackaday for the [trick](https://hackaday.com/2017/01/20/cheating-at-5v-ws2812-control-to-use-a-3-3v-data-line/))

Notes:
 * adjusting elevation seems to require a device reboot to be taken into account
 * to prevent EEPROM wear, it can take up to 10 seconds before settings are persisted into the EEPROM. Please check elevation when rebooting immediately after it's been modified.

