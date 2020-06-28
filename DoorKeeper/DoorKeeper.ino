#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266mDNS.h>
#include <Servo.h> 
#include <EEPROM.h>
#include <WiFiUdp.h>
#include <Ticker.h>
#include "config.h"

#define LOG Serial

// Wio Node
const uint8_t PORT0A = 1;
const uint8_t PORT0B = 3;
const uint8_t PORT1A = 4;
const uint8_t PORT1B = 5;
const uint8_t PORT_POWER = 15;
const uint8_t FUNC_BTN = 0;
const uint8_t BLUE_LED = 2;

// servo params.
const uint8_t lockServoPin = PORT1A;
const uint8_t waitServoPos = 90;
const uint8_t lockServoPos = 5;
const uint8_t unlockServoPos = 175;
const bool lockOnPowerOn = false;

MDNSResponder mdns;
Servo lockServo;
Ticker ticker;
WiFiUDP udpServer;
Ticker pingTicker;

bool locked = lockOnPowerOn;

void setup(void){
  pinMode(BLUE_LED, OUTPUT);
  pinMode(PORT_POWER, OUTPUT);
  digitalWrite(BLUE_LED, 1);

  pinMode(lockServoPin, OUTPUT);
  digitalWrite(lockServoPin, 0);

  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  LOG.println("");

  if (lockOnPowerOn) {
    servo_apply();
  }

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    LOG.print(".");
  }
  LOG.println("");
  LOG.print("Connected to ");
  LOG.println(ssid);
  LOG.print("IP address: ");
  LOG.println(WiFi.localIP());
  
  if (mdns.begin("door1", WiFi.localIP())) {
    LOG.println("MDNS responder started");
  }

  LOG.println("HTTP server started");
  udpServer.begin(apiPort);
  LOG.println("UDP server started");
  ping();
  pingTicker.attach(300, ping);

  wifi_set_sleep_type(LIGHT_SLEEP_T);
}

void ping() {
  LOG.println("ping");
  udpServer.beginPacket(apiHost, apiPort);
  udpServer.write(
      (String() + "type:door\tdevice:" + device + "\ttoken:" + apiToken+ "\tlocked:" + (locked?"true":"false"))
          .c_str());
  udpServer.endPacket();
}

void servo_apply() {
  wifi_set_sleep_type(MODEM_SLEEP_T);
  digitalWrite(lockServoPin, 0);
  digitalWrite(PORT_POWER, 1);

  lockServo.attach(lockServoPin);
  lockServo.write(locked ? lockServoPos : unlockServoPos);

  ticker.attach(1, servo_newtral); // servo off after 3 seconds.
}

void servo_newtral() {
  ticker.detach();
  lockServo.write(waitServoPos);
  ticker.attach(1, servo_off); // servo off.
}

void servo_off() {
  lockServo.detach();
  ticker.detach();
  digitalWrite(PORT_POWER, 0);
  digitalWrite(lockServoPin, 0);
  wifi_set_sleep_type(LIGHT_SLEEP_T);
}

void on_recv_event(String msg) {
  LOG.println(msg);
  if (msg == "token:"+apiToken+"\tcommand:lock") {
    locked = true;
    servo_apply();
  } else if (msg == "token:"+apiToken+"\tcommand:unlock") {
    locked = false;
    servo_apply();
  } else {
    LOG.println("unknown msg.");
  }
}

void loop(void){
  int packetSize = udpServer.parsePacket();
  if (packetSize) {
    char packetBuffer[255];
    int len = udpServer.read(packetBuffer, sizeof(packetBuffer));
    if (len > 0) packetBuffer[len] = 0;
    // LOG.println(packetSize);
    udpServer.flush();
    on_recv_event(packetBuffer);
  } else {
    delay(500);
  }
}
