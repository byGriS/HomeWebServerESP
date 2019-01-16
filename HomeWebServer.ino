#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <SoftwareSerial.h>

#define countRoom 6

const char* ssid = "ssid home wifi";
const char* password = "pass home wifi";
const char* ssidServer = "ssid wifi esp";
const char* passwordserver = "pass wifi esp";
ESP8266WebServer server(80);
SoftwareSerial mySerial(14, 12);

String roomTitle[countRoom] = {"kitchen", "bedroom1", "bedroom2", "living", "hall", "bathroom"};
String roomTitleRu[countRoom] = {"Кухння", "Спальня1", "Спальня2", "Зал", "Коридор", "Ванная"};
bool water[countRoom];
int temp[countRoom];

int tm = 0;
int ringTimer = 0;
int ringCounter = 0;

void setup() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ssidServer, passwordserver);
  server.on("/", handleRoot);
  server.on("/room", handleRoom);
  server.on("/archive", handleArchive);
  server.on("/ring", handleRing);
  server.on("/ringReset", handleRingReset);
  server.begin();
  Serial.begin(115200);
  mySerial.begin(115200);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(ssid, password);
    if (WiFi.waitForConnectResult() != WL_CONNECTED) return;
  } else {
    refreshData();
  }
  server.handleClient();
  if (ringCounter > 0) {
    Call();
  }
  delay(1);
}

void refreshData() {
  if (tm <= 0) {
    for (int i = 0; i < countRoom; i++) {
      HTTPClient http;
      String host = "http://host-your-web-server/push.php?room=";
      host += roomTitle[i];
      host += "&water=";
      host += String(water[i]);
      host += "&temp=";
      host += String(temp[i]);
      http.begin(host);
      http.setUserAgent("Mozilla/5.0");
      http.GET();
      http.end();
    }
    tm = 60000;
  }
  tm--;
}

void handleRoot() {
  String result = "<html>\
  <head>\
    <title>HomeServer</title>\
    <meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\">\
    <style>td, a, p, button{font-size: 26pt; margin: 0;}\
    table{text-align: center;}\
    </style>\
  </head>\
  <body>\
    <table>\
      <tr>\
        <td>Комната</td>\
        <td>Температура</td>\
        <td>Протечка</td>\
      </tr>";
  for (int i = 0; i < countRoom; i++) {
    result += "<tr><td> <a href = \"/archive?room=";
    result += roomTitle[i];
    result += "&page=1\">";
    result += roomTitleRu[i];
    result += "</a></td><td>";
    result += temp[i];
    result += "</td><td>";
    result += water[i] ? "1" : "0";
    result += "</td></tr>";
  }
  result += "</table>";
  result += "<a href=\"/ring\"><button>Позвонить</button></a>";
  if (ringCounter > 0) {
    result += "<p>Будет совершен звонок</p>";
    result += " <a href=\"/ringReset\"><button>Сброс сирены</button></a>";
  }
  result += "</body></html>";
  server.send ( 200, "text/html", result );
}

void handleRoom() {
  for (int i = 0; i < countRoom; i++) {
    if (server.arg("room") == roomTitle[i]) {
      bool curWater = server.arg("water") == "1";
      if (curWater && curWater != water[i]) {
        ringCounter = 3;
        ringTimer = 0;
      }
      if (!curWater)
        ringCounter = 0;
      water[i] = curWater;
      temp[i] = server.arg("temp").toInt();
    }
  }
  server.send ( 200, "text/html", "complete" );
  Serial.println(ringCounter);
  Serial.println(ringTimer);
}

void Call() {
  if (ringTimer <= 0) {
    mySerial.println("ATD+phone-number;");
    ringTimer = 60000;
    ringCounter--;
  }
  ringTimer--;
  //delay(1);
}

void handleArchive() {
  String room = server.arg("room");
  int page = server.arg("page").toInt();
  String host = "http://host-your-web-server/pull.php?room=" + room;
  host += "&page=";
  host += String(page);
  HTTPClient http;
  http.begin(host);
  http.setUserAgent("Mozilla/5.0");
  http.GET();
  String payload = "<html>\
              <head>\
<title>HomeServer</title>\
<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\">\
<style>td, a, p, button{font-size: 26pt; margin: 0;}</style>\
</head>\
<body>\
<a href=\"/\">Назад</a><br/>";
  payload += http.getString();
  page--;
  payload += "<a href=\"/archive?room=";
  payload += room;
  payload += "&page=";
  payload += String(page);
  payload += "\">Пред.</a>&nbsp;&nbsp;&nbsp;&nbsp;<a href=\"/archive?room=";
  page += 2;
  payload += room;
  payload += "&page=";
  payload += String(page);
  payload += "\">След.</a>";
  payload += "</body></html>";
  http.end();
  server.send ( 200, "text/html", payload );
}

void handleRing() {
  mySerial.println("ATD+phone-number;");
  String payload = "<html>\
              <head>\
<title>HomeServer</title>\
<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\">\
<style>td, a, p, button{font-size: 26pt; margin: 0;}</style>\
</head>\
<body>\
<p>Идет звонок...</p>\
<a href=\"/\">Назад</a><br/>";
  payload += "</body></html>";
  server.send ( 200, "text/html", payload );
}

void handleRingReset() {
  ringCounter = 0;
  String payload = "<html>\
              <head>\
<title>HomeServer</title>\
<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\">\
<style>td, a, p, button{font-size: 26pt; margin: 0;}</style>\
</head>\
<body>\
<p>Сброс произведен</p>\
<a href=\"/\">Назад</a><br/>";
  payload += "</body></html>";
  server.send ( 200, "text/html", payload );
}

