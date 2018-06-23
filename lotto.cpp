/*
  Lotto.h - Library for getting lotto information from "ard-text.de".
  Created by Patrick Lenger, 2018
*/
#include "Arduino.h"
#include "Lotto.h"
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

const char* URL_LOTTO_SATURDAY = "http://www.ard-text.de/mobil/581";

typedef struct {
  int endIndex;
  String number;
} LottoNumberResult;

Lotto::Lotto() {

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

LottoNumberResult getNextNumber(String response, int startIndex) {
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

String getSpiel77(String response, int startIndex) {
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

LottoData Lotto::getLastSaturdayLotto()
{
  LottoData lottoData;
  lottoData.success = false; // initialize with not success
  String response = _callUrlAndGetResponse(URL_LOTTO_SATURDAY);

  // 6 aus 49
  int startIndex = response.indexOf("6 aus 49");
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


  // // dummy
  // lottoData.date="16.06.2018";
  // lottoData.z1="1";
  // lottoData.z2="2";
  // lottoData.z3="3";
  // lottoData.z4="4";
  // lottoData.z5="5";
  // lottoData.z6="6";
  // lottoData.super="7";
  // lottoData.spiel77="7777777";

  return lottoData;
}
