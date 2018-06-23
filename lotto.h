/*
  Lotto.h - Library for getting lotto information from "ard-text.de".
  Created by Patrick Lenger, 2018
*/
#ifndef Lotto_h
#define Lotto_h

#include "Arduino.h"

typedef struct {
  String date;
  String z1;
  String z2;
  String z3;
  String z4;
  String z5;
  String z6;
  String super;
  String spiel77;
  bool success;
} LottoData;

class Lotto
{
  public:
    Lotto();
    LottoData getLastSaturdayLotto();
  private:
    String _callUrlAndGetResponse(String url);
};

#endif
