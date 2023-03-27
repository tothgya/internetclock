/**************************************************************************************
 
 WiFi Internet clock (NTP) with ESP8266 NodeMCU (ESP-12E) and SSD1306 OLED display
 This is a free software with NO WARRANTY.
 http://simple-circuit.com/
 
***************************************************************************************/
 
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>               // include NTPClient library
#include <TimeLib.h>                 // include Arduino time library
#include <TM1637Display.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

// Module connection pins (Digital Pins)
#define CLK D3
#define DIO D4
 
const long defaultTimezoneOffset = 1 * 60 * 60; // ? hours * 60 * 60
const long updateInterval = 1 * 1000 * 60 * 60 * 24; // 24 hours
 
WiFiUDP ntpUDP;
// 'time.nist.gov' is used (default server) with +1 hour offset (3600 seconds) 60 seconds (60000 milliseconds) update interval
NTPClient timeClient(ntpUDP, "time.nist.gov", defaultTimezoneOffset, updateInterval);
TM1637Display display(CLK, DIO);

IPAddress ip(192, 168, 0, 9);   
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns(192, 168, 0, 1);
IPAddress googledns(8, 8, 8, 8);

uint8_t segments[] = {0, 0, 0, 0};
 
void setup(void)
{
 
  Serial.begin(9600);
  display.setBrightness(0x01);
  delay(2000);

  wifisetup();
 
  timeClient.setTimeOffset(getTimeZoneOffset());
  timeClient.begin();

}

void wifisetup() {
  int rt = 0;
  int s = 0;
//  WiFi.config(ip, gateway, subnet, dns, googledns);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  WiFi.begin(WiFi.SSID().c_str(), WiFi.psk().c_str());
  segments[0] = SEG_C + SEG_D + SEG_E + SEG_F + SEG_G; // b
  segments[1] = SEG_G + SEG_C + SEG_D + SEG_E; // o
  segments[2] = SEG_G + SEG_C + SEG_D + SEG_E;// o
  segments[3] = SEG_F + SEG_G + SEG_E + SEG_D; // t
  display.setSegments(segments);
  Serial.println("\nWaiting for connection ");
  while (WiFi.status() != WL_CONNECTED && rt++ < 50) {
    delay(200);
    Serial.print(WiFi.status());
  }

  if(WiFi.status() != WL_CONNECTED) {
    segments[0] = SEG_C + SEG_D + SEG_E + SEG_F; // W
    segments[1] = SEG_B + SEG_C + SEG_D + SEG_E; // W
    segments[2] = SEG_A + SEG_B + SEG_G + SEG_F + SEG_E; // P
    segments[3] = SEG_A + SEG_F + SEG_G + SEG_C + SEG_D; // S
    display.setSegments(segments);
    Serial.println("\nStart WPS ");
    if(WiFi.beginWPSConfig()) {
      segments[0] = 0;
      segments[1] = 0;
      segments[2] = SEG_A + SEG_B + SEG_C + SEG_D + SEG_E + SEG_F; // O
      segments[3] = SEG_F + SEG_B + SEG_G + SEG_E + SEG_C; // K
      display.setSegments(segments);
    } else {
      segments[0] = SEG_A + SEG_G + SEG_E + SEG_F; // F
      segments[1] = SEG_G + SEG_C + SEG_D + SEG_E; // a
      segments[2] = SEG_E;// i
      segments[3] = SEG_F + SEG_E + SEG_D; // L
      display.setSegments(segments);
    }
    while ((s = WiFi.status()) != WL_CONNECTED)
    {
      delay(500);
      Serial.print(s);
    }
  }  
  segments[0] = 0;
  segments[1] = 0;
  segments[2] = SEG_A + SEG_B + SEG_C + SEG_D + SEG_E + SEG_F; // O
  segments[3] = SEG_F + SEG_B + SEG_G + SEG_E + SEG_C; // K
  display.setSegments(segments);
  Serial.println("\nConnected.");

}

byte second_, minute_, hour_, wday, day_, month_, year_;
byte last_second, last_minute, last_hour;
int t;
 
void loop()
{
  timeClient.update();
  unsigned long unix_epoch = timeClient.getEpochTime();   // get UNIX Epoch time
 
  second_ = second(unix_epoch);        // get seconds from the UNIX Epoch time
  minute_ = minute(unix_epoch);      // get minutes (0 - 59)
  if (last_hour != hour_ && (hour_ == 3 || hour_ == 2)) {
    timeClient.setTimeOffset(getTimeZoneOffset());
  }
  if (last_minute != minute_)
  {
    hour_   = hour(unix_epoch);        // get hours   (0 - 23)
    wday    = weekday(unix_epoch);     // get minutes (1 - 7 with Sunday is day 1)
    day_    = day(unix_epoch);         // get month day (1 - 31, depends on month)
    month_  = month(unix_epoch);       // get month (1 - 12 with Jan is month 1)
    year_   = year(unix_epoch) - 2000; // get year with 4 digits - 2000 results 2 digits year (ex: 2018 --> 18)

    t = minute_ + 100 * hour_;
    draw_text(t);  
    last_minute = minute_;
    last_hour = hour_;
  }
 
  delay(200);
 
}
 
void draw_text(int t)
{
  display.showNumberDecEx(t, 0b01000000, true);
}

int getTimeZoneOffset() {

    WiFiClient client;
    HTTPClient http;
    int retryCount = 3;
    while(retryCount > 0) {
      http.begin(client, "http://worldtimeapi.org/api/timezone/Europe/Budapest");
      int httpCode = http.GET();
      if(httpCode > 0) {       
        StaticJsonDocument<768> doc;
        DeserializationError error = deserializeJson(doc, http.getString());
        
        if (error) {
          Serial.print(F("deserializeJson() failed: "));
          Serial.println(error.f_str());
        } else {
          int raw_offset = doc["raw_offset"];
          int dst_offset = doc["dst_offset"];
          Serial.println("Timezone offset is:");
          Serial.println(dst_offset + raw_offset);
          http.end();
          return dst_offset + raw_offset; // 3600
        }
      }
      http.end();
      segments[0] = SEG_A + SEG_E +SEG_F + SEG_D; // C
      segments[1] = 0;
      segments[2] = 0;
      segments[3] = display.encodeDigit(retryCount);
      display.setSegments(segments);
      retryCount--;
      delay(1000);
    }
    return defaultTimezoneOffset;
}
// End of code.
