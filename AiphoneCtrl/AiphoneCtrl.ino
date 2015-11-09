#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>
#include <Ticker.h>

const char* ssid = "public";
const char* password = "********";
MDNSResponder mdns;
ESP8266WebServer server(80);

const uint8_t ledPin = 4;

// to aiphone
const uint8_t startBtnPin = 13;
const uint8_t openBtnPin = 12;

// from aiphone
const uint8_t startLedPin = 14;

void handleRoot() {
  digitalWrite(ledPin, 1);
  server.send(200, "text/plain", "Aiphone controller v0.1");
  digitalWrite(ledPin, 0);
}

void handleNotFound(){
  digitalWrite(ledPin, 1);
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
  digitalWrite(ledPin, 0);
}
 
void setup(void){
  pinMode(ledPin, OUTPUT);
  pinMode(openBtnPin, OUTPUT);
  pinMode(startBtnPin, OUTPUT);
  pinMode(startLedPin, INPUT);
  digitalWrite(ledPin, 0);
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.println("");


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
  
  server.on("/aiphone/status", [](){
    server.send(200, "text/json", "{\"status\":\"ok\"}");
  });

  server.on("/door/open", [](){
    digitalWrite(openBtnPin, 1);
    delay(200);
    digitalWrite(openBtnPin, 0);
    server.send(200, "text/json", "{\"status\":\"ok\", \"message\": \"open!\"}");
  });

  server.on("/aiphone/start", [](){
    digitalWrite(startBtnPin, 1);
    delay(200);
    digitalWrite(startBtnPin, 0);
    server.send(200, "text/json", "{\"status\":\"ok\", \"message\": \"start!\"}");
  });

  server.onNotFound(handleNotFound);
  
  server.begin();
  Serial.println("HTTP server started");
}

void loop(void){
  server.handleClient();
}

