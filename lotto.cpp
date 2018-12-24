/*
  Lotto.h - Library for getting lotto information from "www.sachsenlotto.de" and "ard-text.de".
  Created by Patrick Lenger, 2018
*/
#include "Arduino.h"
#include "Lotto.h"
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

const char* URL_LOTTO_SATURDAY = "http://www.ard-text.de/mobil/581";
String hostSachsenLotto = "www.sachsenlotto.de";
String urlBaseSachsenLotto = "/portal/zahlen-quoten/gewinnzahlen/lotto-gewinnzahlen/lotto-gewinnzahlen.jsp";
int portSachsenLotto = 443;

typedef struct {
  int endIndex;
  String number;
} LottoNumberResult;

Lotto::Lotto() {

}

void sendRequest(WiFiClient &client, String host, int port, String url) {
   Serial.print("connecting to ");
   Serial.println(host);

   if (!client.connect(host, port)) {
     Serial.println("connection failed");
     return;
   }

   Serial.print("requesting URL: ");
   Serial.println(url);

   client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                "Host: " + host + "\r\n" +
                "Connection: close\r\n\r\n");

   Serial.println("request sent");
}

String extractText(WiFiClient &client, const char* firstToken, const char* startToken, char endToken) {
  while (client.connected()) {
    // Serial.println("Client is connected");
    size_t size = client.available();
    // Serial.println("Size: " + String(size));
    // get available data size
    if (size) {
     boolean found = client.find(firstToken);
     if (!found) {
       break;
     }
     // Serial.println("firstToken found");
     found = client.find(startToken);
     if (!found) {
       break;
     }
     // Serial.println("startToken found");
     String text = client.readStringUntil(endToken);
     text.trim();
     return text;
    }
  }
  // Serial.println("Nothing found");
  return ""; // not found
}

String extractSamstagDatum(WiFiClient &client, String host, int port, String url) {
  sendRequest(client, host, port, url);
  String date = extractText(client, "name=\"ziehungsDatum\"", "Sa,", '<');
  date.trim();
  return date;
}

String Lotto::_callUrlAndGetResponse(String url) {
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

LottoNumberResult getNextNumber(String &response, int startIndex) {
  LottoNumberResult result;
  result.endIndex = -1; // initialize with -1 as no number was found

  startIndex = response.indexOf("<td", startIndex);
  if (startIndex == -1) {
    return result;
  }
  startIndex = response.indexOf(">", startIndex);
  if (startIndex == -1) {
    return result;
  }
  int endIndex = response.indexOf("<", startIndex);
  if (endIndex == -1) {
    return result;
  }
  result.number = response.substring(startIndex + 1, endIndex);
  result.endIndex = endIndex;
  return result;
}

String getSpiel77(String &response, int startIndex) {
  String result = "";
  startIndex = response.indexOf("Spiel 77:", startIndex);
  if (startIndex == -1) {
    return result;
  }
  int endIndex = startIndex;
  for (int count = 0; count < 7; count++) {
    LottoNumberResult numberResult = getNextNumber(response, endIndex);
    if (numberResult.endIndex == -1) {
      return "";
    }
    result = result + numberResult.number;
    endIndex = numberResult.endIndex;
  }
  return result;
}

LottoData Lotto::getLastSaturdayLottoArd()
{
  LottoData lottoData;
  lottoData.success = false; // initialize with not success
  String response = _callUrlAndGetResponse(URL_LOTTO_SATURDAY);

  int startIndex = response.indexOf("Woche /");
  if (startIndex > -1) {
    int endIndex = response.indexOf("</p>", startIndex);
    if (endIndex > -1) {
      String date = response.substring(startIndex + 7, endIndex);
      date.trim();
      lottoData.date = date;
    }
  }

  // 6 aus 49
  startIndex = response.indexOf("6 aus 49");
  LottoNumberResult result = getNextNumber(response, startIndex);
  if (result.endIndex == -1) {
    return lottoData;
  }
  lottoData.z1 = result.number;
  result = getNextNumber(response, result.endIndex);
  if (result.endIndex == -1) {
    return lottoData;
  }
  lottoData.z2 = result.number;
  result = getNextNumber(response, result.endIndex);
  if (result.endIndex == -1) {
    return lottoData;
  }
  lottoData.z3 = result.number;
  result = getNextNumber(response, result.endIndex);
  if (result.endIndex == -1) {
    return lottoData;
  }
  lottoData.z4 = result.number;
  result = getNextNumber(response, result.endIndex);
  if (result.endIndex == -1) {
    return lottoData;
  }
  lottoData.z5 = result.number;
  result = getNextNumber(response, result.endIndex);
  if (result.endIndex == -1) {
    return lottoData;
  }
  lottoData.z6 = result.number;

  // Superzahl
  startIndex = response.indexOf("Superzahl", result.endIndex);
  if (startIndex == -1) {
    return lottoData;
  }
  startIndex = response.indexOf("<p>", startIndex);
  if (startIndex == -1) {
    return lottoData;
  }
  int endIndex = response.indexOf("</p>", startIndex);
  if (endIndex == -1) {
    return lottoData;
  }
  lottoData.super = response.substring(startIndex + 3, endIndex);

  // Spiel77
  lottoData.spiel77 = getSpiel77(response, endIndex);
  if (lottoData.spiel77 != "") {
    lottoData.success = true; // everything was extracted
  }

  return lottoData;
}

LottoData Lotto::getLastSaturdayLottoSachsenLotto() {
  LottoData lottoData;
  lottoData.success = false; // initialize with not success

  WiFiClientSecure client;
  String date = extractSamstagDatum(client, hostSachsenLotto, portSachsenLotto, urlBaseSachsenLotto);
  if (date == "") {
    return lottoData; // not successful
  }
  lottoData.date = date;
  Serial.println(date);

  sendRequest(client, hostSachsenLotto, portSachsenLotto, urlBaseSachsenLotto + "?ziehungsDatum=" + date);

  String numbers[6];
  String super;
  String spiel77;
  int indexNumbers = 0;

   // read 6 aus 49
  while (indexNumbers < 6) {
    String number = extractText(client, "sl-statistic-number-circle-container-filled", ">", '<');
    if (number == "") {
      return lottoData; // not successful
    }
    numbers[indexNumbers] = number;
    indexNumbers++;
    Serial.println(number);
  }
  lottoData.z1 = numbers[0];
  lottoData.z2 = numbers[1];
  lottoData.z3 = numbers[2];
  lottoData.z4 = numbers[3];
  lottoData.z5 = numbers[4];
  lottoData.z6 = numbers[5];

  // read superzahl
  super = extractText(client, "sl-statistic-number-circle-container-superzahl", ">", '<');
  if (super == "") {
    return lottoData; // not successful
  }
  lottoData.super = super;
  Serial.println("Super: " + super);

  // read Spiel77
  spiel77 = extractText(client, "Spiel77", "<b>", '<');
  if (spiel77 == "") {
    return lottoData; // not successful
  }
  lottoData.spiel77 = spiel77;
  Serial.println("Spiel77: " + spiel77);

  lottoData.success = true;
  return lottoData;
}

LottoData Lotto::getLastSaturdayLotto()
{
  LottoData lottoData = getLastSaturdayLottoSachsenLotto();
  if (lottoData.success) {
    Serial.println("Lotto data loaded from SachsenLotto");
    return lottoData;
  }
  lottoData = getLastSaturdayLottoArd();
  if (lottoData.success) {
    Serial.println("Lotto data loaded from Ard");
  }
  return lottoData;
}
