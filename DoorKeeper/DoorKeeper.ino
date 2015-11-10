#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Servo.h> 
#include <EEPROM.h>
#include <WiFiUdp.h>
#include <Ticker.h>

const char* ssid = "binzume.kstm.org";
const char* password = "**********";
MDNSResponder mdns;
ESP8266WebServer server(80);
Servo lockServo;
Ticker ticker;
WiFiUDP udpServer;
Ticker pingTicker;

const uint8_t led = 4;

const uint8_t lockServoPos = 90;
const uint8_t unlockServoPos = 180;

const uint8_t lockServoPin = 13;
const bool lockOnPowerOn = true;
bool locked = true;

const char* apiHost = "api.example.com";
const int apiPort = 9000;
const String apiToken = "*****";

void handleRoot() {
  digitalWrite(led, 1);
  server.send(200, "text/plain", "Door keeper v0.1");
  digitalWrite(led, 0);
}

void handleNotFound(){
  digitalWrite(led, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  digitalWrite(led, 0);
}
 
void setup(void){
  pinMode(led, OUTPUT);
  digitalWrite(led, 0);
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.println("");

  if (lockOnPowerOn) {
    servo_apply();
  }

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  if (mdns.begin("esp8266", WiFi.localIP())) {
    Serial.println("MDNS responder started");
  }
  
  server.on("/", handleRoot);
  
  server.on("/door/status", [](){
    ping();
    server.send(200, "text/json", String("{\"status\":\"ok\", \"locked\": ") + (locked?"true":"false") +"}");
  });

  server.on("/door/lock", [](){
    locked = true;
    servo_apply();
    server.send(200, "text/json", "{\"status\":\"ok\", \"message\": \"locked!\"}");
  });

  server.on("/door/unlock", [](){
    locked = false;
    servo_apply();
    server.send(200, "text/json", "{\"status\":\"ok\", \"message\": \"unlocked!\"}");
  });

  server.onNotFound(handleNotFound);
  
  server.begin();
  Serial.println("HTTP server started");
  udpServer.begin(9000);
  Serial.println("UDP server started");
  ping();
  pingTicker.attach(300, ping);
}

void ping() {
  udpServer.beginPacket(apiHost, apiPort);
  udpServer.write((String("{\"type\":\"door\",\"device\":\"door01\",\"token\":\""+apiToken+"\",\"locked\": ") + (locked?"true":"false") +"}").c_str());
  udpServer.endPacket();
}

void servo_apply() {
  lockServo.attach(lockServoPin);
  lockServo.write(locked ? lockServoPos : unlockServoPos);
  ticker.attach(3, servo_off); // servo off after 3 seconds.
}
 
void servo_off() {
  lockServo.detach();
  ticker.detach();
}

void on_recv_event(String msg) {
  Serial.println(msg);
  // TODO auth
  if (msg == "token:"+apiToken+"\tcommand:lock") {
    locked = true;
    servo_apply();
  } else if (msg == "token:"+apiToken+"\tcommand:unlock") {
    locked = false;
    servo_apply();
  } else {
    Serial.println("unknown msg.");
  }
}

void loop(void){
  server.handleClient();
  int packetSize = udpServer.parsePacket();
  if (packetSize) {
    char packetBuffer[255];
    int len = udpServer.read(packetBuffer, sizeof(packetBuffer));
    if (len > 0) packetBuffer[len] = 0;
    // Serial.println(packetSize);
    on_recv_event(packetBuffer);
  }
}

