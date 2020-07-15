#include "GyverRGB.h"
#include <SoftwareSerial.h>

// Режимы работы
#define OFF 0
#define RGB_MODE 1
#define HSV_MODE 2
#define GRADIENT_MODE 3
#define STATUS_REQUEST 7

// Пины для подключения Bluetooth HC-05
#define RX 8
#define TX 7
SoftwareSerial BTSerial(RX, TX);

// Пины для подключения RGB-ленты
#define Rpin 10
#define Bpin 9
#define Gpin 11
GRGB diode(Rpin, Gpin, Bpin);

// Режим и значения при включении
byte mode = GRADIENT_MODE;
unsigned long previousMillis = 0;
int interval = 5000, changeStep = 1;
int H = 0, S = 255, V = 50;
int R = 100, G = 150, B = 200;

// Прием данных из Bluetooth соединения в виде '$int int int int;'
#define PARSE_AMOUNT 4
int intData[PARSE_AMOUNT];
boolean recievedFlag;
boolean getStarted;
byte index;
String string_convert = "";

void parsing() {
  if (BTSerial.available() > 0) {
    char incomingByte = BTSerial.read();                  // обязательно ЧИТАЕМ входящий символ
    if (getStarted) {                                     // если приняли начальный символ (парсинг разрешён)
      if (incomingByte != ' ' && incomingByte != ';') {   // если это не пробел И не конец
        string_convert += incomingByte;                   // складываем в строку
      } else {                                            // если это пробел или ; конец пакета
        intData[index] = string_convert.toInt();          // преобразуем строку в int и кладём в массив
        string_convert = "";                              // очищаем строку
        index++;                                          // переходим к парсингу следующего элемента массива
      }
    }
    if (incomingByte == '$') {                            // если это $
      getStarted = true;                                  // поднимаем флаг, что можно парсить
      index = 0;                                          // сбрасываем индекс
      string_convert = "";                                // очищаем строку
    }
    if (incomingByte == ';') {                            // если таки приняли ; - конец парсинга
      getStarted = false;                                 // сброс
      recievedFlag = true;                                // флаг на принятие
    }
  }
}

// Отправка текущего состояния параметров
void sendStatus(byte mode) {
  String package = "$" +
                   String(mode) + " " +
                   String(R) + " " + String(G) + " " + String(B) + " " +
                   String(H) + " " + String(S) + " " + String(V) + " " +
                   String(interval) + " " + String(changeStep) + ";";
  //Serial.println(package);
  BTSerial.print(package);
}


void setup() {
  diode.setHSV(H, S, V);
  Serial.begin(9600);
  BTSerial.begin(38400);
}

void loop() {
  parsing();                                          // функция парсинга
  if (recievedFlag) {                                 // если получены данные
    recievedFlag = false;
    /*for (byte i = 0; i < PARSE_AMOUNT; i++) {         // выводим элементы массива
      Serial.print(intData[i]); Serial.print(" ");
    } Serial.println();*/

    // Действия согласно полученным данным
    switch (intData[0]) {
      case RGB_MODE:
        mode = RGB_MODE;
        R = intData[1]; G = intData[2]; B = intData[3];
        diode.setRGB(R, G, B);
        break;
      case HSV_MODE:
        mode = HSV_MODE;
        H = intData[1]; S = intData[2]; V = intData[3];
        diode.setHSV(H, S, V);
        break;
      case GRADIENT_MODE:
        mode = GRADIENT_MODE;
        interval = intData[1]; changeStep = intData[2];
        break;
      case STATUS_REQUEST:
        sendStatus(mode);
        break;
      default:
        break;
    }
  }

  // Смена значения в режиме GRADIENT
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    if (mode == GRADIENT_MODE) {
      H = (H + changeStep) % 255;
      diode.setHSV(H, S, V);
    }
  }
}
