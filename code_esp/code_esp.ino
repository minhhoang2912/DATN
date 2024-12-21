#define BLYNK_TEMPLATE_ID "TMPL6Bmj6HSRY"
#define BLYNK_TEMPLATE_NAME "FGLG"
#define BLYNK_AUTH_TOKEN "ZD19DLIsc6lDRr4otwqPJ-Zw9ghLytqq"


#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#define ON_Board_LED 2
char kytu;
const char ssid[] = "Phòng 701+2";
const char pass[] = "abcd1234";
const char* host = "script.google.com";
const int httpsPort = 443;
WiFiClientSecure client;
BlynkTimer timer;
String GAS_ID = "AKfycby5hucIw4AiOiDl9Iy9YK8wKaGl24uFnn3rO19Uh0_xkbwe2ZMajr1GyXIeUg-VhMNknw";


BLYNK_WRITE(V0){
  int p = param.asInt();
  
}
BLYNK_WRITE(V1){
  int p = param.asInt();
}

void sendName() {
  Serial.println("==========");
  Serial.print("connecting to ");
  Serial.println(host);
  
  if (!client.connect(host, httpsPort)) {
   Serial.println("connection failed");
   return;
  }
  String string_temperature =  "DongMinhHoang";
  String url = "/macros/s/" + GAS_ID + "/exec?temperature=" + string_temperature;
  Serial.print("requesting URL: ");
  Serial.println(url);
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
    "Host: " + host + "\r\n" +
    "User-Agent: BuildFailureDetectorESP8266\r\n" +
    "Connection: close\r\n\r\n");
  Serial.println("request sent");

  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("headers received");
      break;
    }
  }
  String line = client.readStringUntil('\n');
  if (line.startsWith("{\"state\":\"success\"")) {
    Serial.println("esp8266/Arduino CI successfull!");
  } else {
    Serial.println("esp8266/Arduino CI has failed");
  }
  Serial.print("reply was : ");
  Serial.println(line);
  Serial.println("closing connection");
  Serial.println("==========");
  Serial.println();
}
void sendpass() {
  Serial.println("==========");
  Serial.print("connecting to ");
  Serial.println(host);
  if (!client.connect(host, httpsPort)) {
    Serial.println("connection failed");
    return;
  }
  String string_temperature =  "password";
  String url = "/macros/s/" + GAS_ID + "/exec?temperature=" + string_temperature;
  Serial.print("requesting URL: ");
  Serial.println(url);

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
    "Host: " + host + "\r\n" +
    "User-Agent: BuildFailureDetectorESP8266\r\n" +
    "Connection: close\r\n\r\n");
  Serial.println("request sent");

  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("headers received");
      break;
    }
  }
  String line = client.readStringUntil('\n');
  if (line.startsWith("{\"state\":\"success\"")) {
    Serial.println("esp8266/Arduino CI successfull!");
  } else {
    Serial.println("esp8266/Arduino CI has failed");
  }
  Serial.print("reply was : ");
  Serial.println(line);
  Serial.println("closing connection");
  Serial.println("==========");
  Serial.println();
} 


void setup() {
  Serial.begin(9600);
  delay(500);
  WiFi.begin(ssid, pass);
  Serial.println("");
    
  pinMode(ON_Board_LED,OUTPUT);
  digitalWrite(ON_Board_LED, HIGH);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    digitalWrite(ON_Board_LED, LOW);
    delay(250);
    digitalWrite(ON_Board_LED, HIGH);
    delay(250);
  }
  digitalWrite(ON_Board_LED, HIGH);
  Serial.println("");
  Serial.print("Successfully connected to : ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  client.setInsecure();  
  sendName();
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
}
void loop() {
  Blynk.run();
  if (Serial.available() > 0) {
    kytu = Serial.read();
    Serial.print("Received: ");
    Serial.println(kytu);
    if (kytu == 'A') 
    {
      sendName();
      Serial.println("nhan thanh cong. mo bang khuon mat");
    } 
    else if (kytu == 'e')
    {
      sendpass();
      Serial.println("nhan thanh cong. mo bang mat khau");
    }
    else
    {
      Serial.println("nhan khong thanh cong");
    }
  }
}

