#include <ESP8266WiFi.h>
#include <PZEM004Tv30.h>
#define BLYNK_PRINT Serial
#include <BlynkSimpleEsp8266.h>
#include <ThingspeakClient.h>
#include <EEPROM.h>
#include <WiFiClientSecureAxTLS.h>

#define FIRST_ADDRESS 0
#define MAX_ATTEMPTS 10
#define min(a,b) ((a)<(b)?(a):(b))

PZEM004Tv30 pzem(D1, D2); 
IPAddress ip(192, 168, 1, 1);

const char* ssid     = "Nnn";
const char* password = "0954341522";
char auth[] = "egsX4t1tFY7Prm6jOq6BZJHjBHBLE2YK";  

WiFiClient client;
ThingspeakClient thingspeak;
const char* host = "api.thingspeak.com";
const String THINGSPEAK_CHANNEL_ID = "910692";
const String THINGSPEAK_API_READ_KEY = "Y1KYNFR0ZU3KBZMR"; 
const char* api   = "KXEF11CR1BI8ZBGI";  

float v, i, p, e, b;
long milisec = 0;
float calBill(float Unit, float ft, bool over_150_Unit_per_month = false) ;
float FT = -11.6;
int firstState;

void Line_Notify(String message);
int token = 0;
String LINE_TOKEN [2]   = {"RW3WF8ND72pLaT22hl59za4gHC7q4HsPBOP9ROETdi8","ME9s64rJCh7BpLjxKzdRqut4CMgDc1r716U4WoQYQPc"}; 
unsigned long currentMillis, previousMillis, previousMillis2, previousMillis3, readMillis = 0;

String message = "%20Volt%20";  
String message2 = "%20Current%20";  
String message3 = "%20Power%20Now%20";  
String message4 = "%20Energy%20";  
String message5 = "%20Cost%20";  
String message6 = "%20Warning%20overload%20"; 
String message7 = "%20Power%20Total%20"; 

void setup() {
  Serial.begin(115200);

  Blynk.begin(auth, ssid, password);

  EEPROM.begin(512);
  firstState = EEPROM.read(FIRST_ADDRESS);
  Serial.println(firstState);

  if (firstState == 5) {
    Serial.print("use again:");
    thingspeak.getLastChannelItem(THINGSPEAK_CHANNEL_ID, THINGSPEAK_API_READ_KEY);
    String f1 = (thingspeak.getFieldValue(3));
    String f2 = (thingspeak.getFieldValue(4));
    String f3 = (thingspeak.getFieldValue(5));
    e = f1.toFloat();
    b = f2.toFloat();
    FT = f3.toFloat();
  } else {
    Serial.print("use new:");
    Serial.println("first write eeprom");
    EEPROM.write(0, 5);
    EEPROM.commit();
  }

  ReadPzem();
  SendNormal();
  currentMillis = previousMillis = previousMillis2 = previousMillis3 = readMillis = millis();
  Serial.println(FT);
  Blynk.virtualWrite(V7, FT);
  b = calBill(e, FT, true);
}

void loop() {
  currentMillis = millis();
  Blynk.run();

  if (currentMillis - readMillis >= 1000) {
    ReadPzem();
    SetBlynk();
  }

  if (currentMillis - previousMillis3 >= 30000) UpdateThingspeak();
  if (currentMillis - previousMillis >= 100000) SendNormal();
  if (i >= 2.00 && currentMillis - previousMillis2 >= 40000) SendWarning();
}

void ReadPzem() {
  readMillis = millis();
  unsigned long time;

  v = getVoltage();
  i = getCurrent();
  p = v * i;
  if (p >= 1) {
    time += 1;
    e += (p * time) / (1000 * 3600);
    b = calBill(e, FT, true);
  } else {
    time = 0;
  }

  Serial.print("Volt: "); Serial.print(v); Serial.print("V\n");
  Serial.print("Current: "); Serial.print(i); Serial.print("A\n");
  Serial.print("power Now: "); Serial.print(p); Serial.print("W\n");
  Serial.print("Energy: "); Serial.print(e, 3); Serial.print("kWh\n");
  Serial.print("Cost: "); Serial.print(b, 3); Serial.print("฿\n");
  Serial.print("FT: "); Serial.println(FT);
  Serial.println("--------------------");

}

void SetBlynk() {
  Blynk.virtualWrite(V0, v);
  Blynk.virtualWrite(V1, i);
  Blynk.virtualWrite(V2, p);
  Blynk.virtualWrite(V4, e);
  Blynk.virtualWrite(V5, b);
  Blynk.virtualWrite(V6, FT);
}

void UpdateThingspeak() {
  previousMillis3 = millis();
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }

  String etemp = String(e, 3);
  String btemp = String(b, 3);


  String url = "/update?api_key=";
  url += api;
  url += "&field1=";
  url += v;
  url += "&field2=";
  url += i;
  url += "&field3=";
  url += p;
  url += "&field4=";
  url += etemp;
  url += "&field5=";
  url += btemp;
  url += "&field6=";
  url += FT;

  Serial.print("Requesting URL: ");
  Serial.println(url);

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");
  delay(10);


  while (client.available()) {
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }

  Serial.println();
  Serial.println("closing connection");
}

void SendNormal() {
  previousMillis = millis();
  token = 0;
  Line_Notify(message + v + "V" + message2 + i + "A" + message3
              + p + "W" + message4 + e + "Wh" + message5 + b + "฿");
}

void SendWarning() {
  previousMillis2 = millis();
  token = 1;
  Line_Notify(message6 + i + "A");
}

BLYNK_WRITE(V7) {
  FT = param[0].asFloat();
}

BLYNK_WRITE(V8) {
  p = 0.0;
  e = 0.0;
  b = 0.0;
}

float calBill(float Unit, float ft, bool over_150_Unit_per_month) {
  float Service = over_150_Unit_per_month ? 38.22 : 8.19;

  float total = 0;

  if (!over_150_Unit_per_month) {
    float Rate15 = 2.3488;
    float Rate25 = 2.9882;
    float Rate35 = 3.2405;
    float Rate100 = 3.6237;
    float Rate150 = 3.7171;
    float Rate400 = 4.2218;
    float RateMore400 = 4.4217;

    if (Unit >= 6) total += min(Unit, 15) * Rate15;
    if (Unit >= 16) total += min(Unit - 15, 10) * Rate25;
    if (Unit >= 26) total += min(Unit - 25, 10) * Rate35;
    if (Unit >= 36) total += min(Unit - 35, 65) * Rate100;
    if (Unit >= 101) total += min(Unit - 100, 50) * Rate150;
    if (Unit >= 151) total += min(Unit - 150, 250) * Rate400;
    if (Unit >= 401) total += (Unit - 400) * RateMore400;
  } else {
    float Rate150 = 3.2484;
    float Rate400 = 4.2218;
    float RateMore400 = 4.4217;

    total += min(Unit, 150) * Rate150;
    if (Unit >= 151) total += min(Unit - 150, 250) * Rate400;
    if (Unit >= 401) total += (Unit - 400) * RateMore400;
  }

  total += Service;
  total += Unit * (ft / 100);
  total += total * 7 / 100;

  return total;
}

void Line_Notify(String message) {
  axTLS::WiFiClientSecure client;

  if (!client.connect("notify-api.line.me", 443)) {
    Serial.println("connection failed");
    return;
  }

  String req = "";
  req += "POST /api/notify HTTP/1.1\r\n";
  req += "Host: notify-api.line.me\r\n";
  req += "Authorization: Bearer " + String(LINE_TOKEN[token]) + "\r\n";
  req += "Cache-Control: no-cache\r\n";
  req += "User-Agent: ESP8266\r\n";
  req += "Content-Type: application/x-www-form-urlencoded\r\n";
  req += "Content-Length: " + String(String("message=" + message).length()) + "\r\n";
  req += "\r\n";
  req += "message=" + message;

  client.print(req);
  delay(30);

  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      break;
    }
  }
}

float getVoltage() {
  int i = 0;
  float r = -1.0;
  do {
    r = pzem.voltage();
    wdt_reset();
    i++;
  } while ( i < MAX_ATTEMPTS && r < 0.0);
  return r;
}

float getCurrent() {
  int i = 0;
  float r = -1.0;
  do {
    r = pzem.current();
    wdt_reset();
    i++;
  } while ( i < MAX_ATTEMPTS && r < 0.0);
  return r;
}
