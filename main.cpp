#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Lotto.h>
#include <Settings.h>

// Bitmuster fuer grosse Umlaute
byte Auml[8] =  { B01010, B00000, B01110, B10001, B10001, B11111, B10001, B10001 };
byte Uuml[8] =  { B01010, B00000, B10001, B10001, B10001, B10001, B10001, B01110 };
byte Ouml[8] =  { B01010, B00000, B01110, B10001, B10001, B10001, B10001, B01110 };

// constants

const char* WEATHER_URL_TEMPLATE = "http://api.openweathermap.org/data/2.5/weather?units=metric&lang=de&q=%1,de&appid=%2";
const char* FORECAST_URL_TEMPLATE = "http://api.openweathermap.org/data/2.5/forecast?units=metric&lang=de&q=%1,de&appid=%2&cnt=10";
const unsigned int DISPLAY_MAX_PAGE_SIZE = 10;
const int COUNT_LOTTO_TIPPS = 12;
const int LOTTO_TIPPS[COUNT_LOTTO_TIPPS][6] = {
  {3, 5, 11, 19, 22, 48},
  {19, 26, 27, 36, 42, 43},
  {6, 7, 10, 17, 21, 30},
  {5, 8, 24, 37, 43, 49},
  {3, 9, 17, 23, 32, 45},
  {7, 9, 35, 37, 41, 49},
  {3, 6, 19, 26, 39, 41},
  {1, 6, 9, 11, 27, 29},
  {18, 20, 24, 30, 44, 45},
  {6, 11, 27, 33, 37, 45},
  {5, 14, 36, 38, 41, 45},
  {3, 7, 9, 19, 22, 31}
};
const String SPIEL_77 = "9940957";
const int SUPERZAHL = 7;

// variables
LiquidCrystal_I2C lcd(0x27, 20, 4);
String cities[] = {WEATHER_CITY};
Lotto lotto = Lotto();
unsigned int cityCount = 1;
unsigned long lastWeatherUpdateMillis = 0;
unsigned long updateWeatherIntervalMillis = WEATHER_UPDATE_INTERVAL_MILLIS;
unsigned int updateDisplayIntervalMillis = DISPLAY_UPDATE_INTERVAL_MILLIS;
String displayLine1Array[DISPLAY_MAX_PAGE_SIZE];
String displayLine2Array[DISPLAY_MAX_PAGE_SIZE];
String displayLine3Array[DISPLAY_MAX_PAGE_SIZE];
String displayLine4Array[DISPLAY_MAX_PAGE_SIZE];
unsigned int displayLastPageNo = 0;
bool displayTextChanged = false;
int lotto6Aus49[6] = {0, 0, 0, 0, 0, 0};
int superZahl = 0;
String spiel77 = "";

String replaceSpecialCharactersForLcd(String text) {
  // text is encoded in UTF-8
  text.replace("\303\204", "\001"); // Ä
  text.replace("\303\226", "\003"); // Ö
  text.replace("\303\234", "\002"); // Ü
  text.replace("\303\244", "\341"); // ä
  text.replace("\303\266", "\357"); // ö
  text.replace("\303\274", "\365"); // ü
  text.replace("\303\237", "\342"); // ß
  text.replace("\302\260", "\337"); // °
  return text;
}

// prints the line and cut it to 20 if needed
void printLineCut(byte lineIndex, String text) {
  String replacedText = replaceSpecialCharactersForLcd(text);
  lcd.setCursor(0, lineIndex);
  if (replacedText.length() > 20) {
    replacedText = replacedText.substring(0, 20);
  }
  lcd.print(replacedText);
}

void storeTextForDisplay(int pageNo, int lineNo, String line) {
  if (lineNo == 0) {
    displayLine1Array[pageNo] = line;
  } else if (lineNo == 1) {
    displayLine2Array[pageNo] = line;
  } else if (lineNo == 2) {
    displayLine3Array[pageNo] = line;
  } else if (lineNo == 3) {
    displayLine4Array[pageNo] = line;
  }
}

String callUrlAndGetResponse(String url) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    Serial.println("URL: " + url);
    http.begin(url);
    int httpCode = http.GET();
    String payload;
    if (httpCode > 0) {
      payload = http.getString();
    }

    http.end();   //Close connection
    Serial.println("HTTP connection closed");
    return payload;
  }
  Serial.println("Not connected to WLAN");
  return "";
}

void compareLottoZahlen() {
  int lineNo = 0;
  displayLastPageNo = displayLastPageNo + 1;

  // check 6 aus 49
  for (int tipp = 0; tipp < COUNT_LOTTO_TIPPS; tipp++) {
    int countRichtige = 0;
    for (int i = 0; i < 6; i++) {
      for (int j = 0; j < 6; j++) {
        if (LOTTO_TIPPS[tipp][i] == lotto6Aus49[j]) {
          countRichtige++;
          break;
        }
      }
    }

    if (countRichtige > 2 || (countRichtige > 1 && SUPERZAHL == superZahl)) {
      if (lineNo > 3) {
        lineNo = 0;
        displayLastPageNo = displayLastPageNo + 1;
      }
      String text = "Kasten " + String(tipp + 1) + ": " + String(countRichtige);
      if (SUPERZAHL == superZahl) {
        text = text + " + Superzahl";
      }
      storeTextForDisplay(displayLastPageNo, lineNo, text);
      lineNo++;
    }
    yield();
  }

  // check spiel77
  int countSpiel77 = 0;
  int pos = 6;
  while (pos > -1 && spiel77[pos] == SPIEL_77[pos]) {
    countSpiel77++;
    pos--;
  }
  if (countSpiel77 > 0) {
    String text = "Spiel77: " + String(countSpiel77) + " Ziffer(n)";
    storeTextForDisplay(4, lineNo, text);
    lineNo++;
  }

  if (lineNo == 0) {
    String text = "Leider kein Gewinn";
    storeTextForDisplay(4, lineNo, text);
  }
}

void getLottoZahlen() {
  if (lotto6Aus49[0] != 0) {
    return; // load lotto data only once
  }

  lcd.clear();
  Serial.println("Get lotto data...");
  printLineCut(0, "Lade Lottodaten...");

  LottoData lottoData = lotto.getLastSaturdayLotto();
  if (!lottoData.success) {
    printLineCut(0, "Lottodaten konnten");
    printLineCut(1, "nicht geladen werden");
    delay(updateDisplayIntervalMillis);
    return;
  }

  spiel77 = lottoData.spiel77;
  lotto6Aus49[0] = lottoData.z1.toInt();
  lotto6Aus49[1] = lottoData.z2.toInt();
  lotto6Aus49[2] = lottoData.z3.toInt();
  lotto6Aus49[3] = lottoData.z4.toInt();
  lotto6Aus49[4] = lottoData.z5.toInt();
  lotto6Aus49[5] = lottoData.z6.toInt();
  superZahl = lottoData.super.toInt();

  displayLastPageNo =  displayLastPageNo + 1;
  displayLine1Array[displayLastPageNo] = "Lotto " + lottoData.date;
  displayLine2Array[displayLastPageNo] = lottoData.z1 + " " + lottoData.z2 + " " + lottoData.z3 + " " + lottoData.z4 + " " + lottoData.z5 + " " + lottoData.z6;
  displayLine3Array[displayLastPageNo] = "Superzahl: " + lottoData.super;
  displayLine4Array[displayLastPageNo] = "Spiel77: " + lottoData.spiel77;
  compareLottoZahlen();
}

// shorten the weather description
String shortenWeatherDescription(String text) {
  // Serial.println("ORIGINAL:" + text);
  text.replace("\303\234berwiegend", "\303\234bw"); // Überwiegend
  text.replace("Ein paar", "Paar");
  text.replace("Leichter", "Leicht");
  // Serial.println("SHORTEN: " + text);
  return text;
}

// update the current weather text for cityIndex 0 or 1
void updateCurrentWeatherText(byte cityIndex) {
  if (cityIndex > 1) {
    return; // do nothing as index does not exists
  }
  String city = cities[cityIndex];
  if (city.length() == 0) {
    return; // do nothing as city is not set
  }
  String url = WEATHER_URL_TEMPLATE;
  url.replace("%1", city);
  url.replace("%2", OPEN_WEATHER_MAP_APP_ID);

  String payload = callUrlAndGetResponse(url);
  if (payload.length() == 0) {
    Serial.println("No weather data received");
    return; // no weather data received
  }
  StaticJsonBuffer<2000> jsonBuffer;
  JsonObject &root = jsonBuffer.parseObject(payload);
  if (!root.success()) {
    Serial.println(F("Failed to read JSON"));
    lcd.clear();
    printLineCut(0, "Fehler Wetterdaten");
    delay(2000);
    return;
  }
  Serial.println("JSON parsed");
  String degree = root["main"]["temp"];
  int degreeInt = degree.toInt();
  Serial.println("degree: " + String(degreeInt));
  String weather = root["weather"][0]["description"];
  Serial.println("weather: " + weather);
  weather = shortenWeatherDescription(weather);
  String line1 = city + ": " + String(degreeInt) + "°";
  String line2 = weather;

  displayLine1Array[0] = line1;
  displayLine2Array[0] = line2;
  // for (int page = 0; page < DISPLAY_PAGE_SIZE; page++) {
  //   if (cityIndex == 0) {
  //     displayLine1Array[page] = line1;
  //   } else {
  //     displayLine3Array[page] = line1;
  //   }
  // }
  // if (cityIndex == 0) {
  //   displayLine2Array[0] = line2;
  // } else {
  //   displayLine4Array[0] = line2;
  // }
  displayLastPageNo = 0;
}

// update the forecast weather text for cityIndex 0 or 1
void updateForecastWeatherText(byte cityIndex) {
  if (cityIndex > 1) {
    return; // do nothing as index does not exists
  }
  String city = cities[cityIndex];
  if (city.length() == 0) {
    return; // do nothing as city is not set
  }
  String url = FORECAST_URL_TEMPLATE;
  url.replace("%1", city);
  url.replace("%2", OPEN_WEATHER_MAP_APP_ID);

  Serial.println("Calling URL...");
  // delay(1000);
  String payload = callUrlAndGetResponse(url);
  // Serial.println("Payload BEGIN");
  // Serial.println(payload);
  // Serial.println("Payload END");
  if (payload.length() == 0) {
    Serial.println("No weather data received");
    return; // no weather data received
  }

  unsigned int startPos = 0;
  unsigned int index = 0;
  int foundIndex = payload.indexOf("dt", startPos);
  while (foundIndex != -1) {
    // search temp
    foundIndex = payload.indexOf("main", foundIndex);
    if (foundIndex == -1) {
      break;
    }
    foundIndex = payload.indexOf("\"temp\":", foundIndex);
    if (foundIndex == -1) {
      break;
    }
    int endIndex = payload.indexOf(",", foundIndex);
    if (endIndex == -1) {
      break;
    }
    String tempText = payload.substring(foundIndex + 7, endIndex);
    int tempInt = tempText.toInt();

    // search weather
    foundIndex = payload.indexOf("\"weather\":", foundIndex);
    if (foundIndex == -1) {
      break;
    }
    foundIndex = payload.indexOf("\"description\":", foundIndex);
    if (foundIndex == -1) {
      break;
    }
    endIndex = payload.indexOf("\"", foundIndex + 16);
    if (endIndex == -1) {
      break;
    }
    String weatherText = payload.substring(foundIndex + 15, endIndex);
    weatherText = shortenWeatherDescription(weatherText);

    // search dateTime
    foundIndex = payload.indexOf("\"dt_txt\":", foundIndex);
    if (foundIndex == -1) {
      break;
    }
    endIndex = payload.indexOf("}", foundIndex);
    if (endIndex == -1) {
      break;
    }
    String dateTimeText = payload.substring(foundIndex + 10, endIndex - 1);
    dateTimeText = dateTimeText.substring(11); // remove date
    dateTimeText.replace(":00:00", "h");
    String line = dateTimeText + " " + String(tempInt) + "° " + weatherText;
    if (index == 0) {
      displayLine3Array[0] = line;
    } else if (index == 1) {
      displayLine4Array[0] = line;
    } else {
      unsigned int pageNo = ((index - 2) / 4) + 1;
      if (pageNo > DISPLAY_MAX_PAGE_SIZE) {
        break;
      }
      unsigned int lineNo = ((index - 2) % 4);
      storeTextForDisplay(pageNo, lineNo, line);
      displayLastPageNo = max(displayLastPageNo, pageNo);
    }
    foundIndex = payload.indexOf("dt", endIndex); // search next time
    index++;
  }
}

void getWeatherData() {
  lcd.clear();
  Serial.println("Get weather data...");
  printLineCut(0, "Lade Wetterdaten...");
  updateCurrentWeatherText(0);
  updateForecastWeatherText(0);
  displayTextChanged = true;
}

void updateWeatherData() {
  // check if new update from job is needed
  unsigned long currentMillis = millis();
  unsigned long intervalSinceLastUpdate = currentMillis - lastWeatherUpdateMillis;

  if (lastWeatherUpdateMillis == 0 || intervalSinceLastUpdate > updateWeatherIntervalMillis) {
    getWeatherData();
    lastWeatherUpdateMillis = currentMillis;
  }
}

void updateDisplay() {
  // if (displayTextChanged) {
    // Serial.println("Writing to LCD...");
    for (unsigned int page = 0; page <= displayLastPageNo; page++) {
      lcd.clear();
      printLineCut(0, displayLine1Array[page]);
      printLineCut(1, displayLine2Array[page]);
      printLineCut(2, displayLine3Array[page]);
      printLineCut(3, displayLine4Array[page]);
      delay(updateDisplayIntervalMillis);
    }
    // displayTextChanged = false;
  // }
}

void setup() {
  lcd.init();
  lcd.createChar(1, Auml);
  lcd.createChar(2, Uuml);
  lcd.createChar(3, Ouml);
  lcd.backlight();

  printLineCut(0, "Verbinde mit WLAN...");

  Serial.begin(115200);
  Serial.println();
  Serial.println("Booted");
  Serial.println("Connecting to Wi-Fi");

  WiFi.mode(WIFI_STA);
  WiFi.begin (WLAN_SSID, WLAN_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("WiFi connected");
}

void loop() {
  updateWeatherData();
  getLottoZahlen();
  updateDisplay();
}
