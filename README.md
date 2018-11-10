# multi-information-display
Displays multi informations (weather and lotto) with an ESP8266 (NodeMCU)

Used hardware:
* Nodemcu (ESP8266 board)
* 20x4 display (connected via I2C)

Used libraries:
* Arduino
* ESP8266WiFi
* ESP8266HTTPClient
* LiquidCrystal_I2C
* ArduinoJson

Before compiling, configure:
* In settings.h: WLAN and weather api key
* In main.cpp: COUNT_LOTTO_TIPPS, LOTTO_TIPPS, SPIEL_77, SUPERZAHL with your lotto numbers

Connection of display on NodeMCU (via I2C):
* GND: Pin G
* VCC: Pin VU
* SDA: Pin D2
* SCL: Pin D1