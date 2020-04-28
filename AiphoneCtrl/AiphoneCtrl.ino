#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>
#include <WiFiUdp.h>
#include <Ticker.h>
#include "config.h"

// fqbn:
// esp8266:esp8266:d1_mini:CpuFrequency=80,FlashSize=4M,UploadSpeed=512000

const uint8_t PORT_POWER = 15; // (common with RED_LED)
const uint8_t FUNC_BTN = 0;
const uint8_t BLUE_LED = 2;

const char *device = "aiphone";

// #define DEBUG

#ifdef DEBUG
#define LOG Serial
#else
#define LOG Serial1
#endif

MDNSResponder mdns;
ESP8266WebServer server(80);
WiFiUDP udpServer;
Ticker pingTicker;

const uint8_t ledPin = BLUE_LED;

void aiphone_start()
{
  Serial.print("@SSS");
}

void aiphone_stop()
{
  Serial.print("@sss");
}

void aiphone_open()
{
  digitalWrite(ledPin, 0);
  Serial.print("@OOO");
  delay(5);
  digitalWrite(ledPin, 1);
}

void setup(void)
{
  pinMode(PORT_POWER, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  digitalWrite(PORT_POWER, 1);
  digitalWrite(BLUE_LED, 1);

#ifndef DEBUG
  Serial.begin(1200);
#endif

  LOG.begin(115200);
  WiFi.begin(ssid, password);
  LOG.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    LOG.print(".");
  }
  LOG.println("");
  LOG.print("Connected to ");
  LOG.println(ssid);
  LOG.print("IP address: ");
  LOG.println(WiFi.localIP());

  if (mdns.begin(device, WiFi.localIP()))
  {
    LOG.println("MDNS responder started");
  }

  server.on("/", []() {
    server.send(200, "text/plain", "Aiphone controller v0.1");
  });

  server.on("/aiphone/status", []() {
    server.send(200, "text/json", "{\"status\":\"ok\"}");
  });

  server.on("/door/open", []() {
    aiphone_open();
    server.send(200, "text/json", "{\"status\":\"ok\", \"message\": \"open!\"}");
  });

  server.on("/aiphone/start", []() {
    aiphone_start();
    server.send(200, "text/json", "{\"status\":\"ok\", \"message\": \"start!\"}");
  });

  server.onNotFound([]() {
    server.send(404, "text/plain", "Not Found");
  });

  server.begin();
  LOG.println("HTTP server started");
  udpServer.begin(9000);
  LOG.println("UDP server started");
  ping();
  pingTicker.attach(300, ping);
}

void ping()
{
  udpServer.beginPacket(apiHost, apiPort);
  udpServer.write((String() + "type:door\tdevice:" + device + "\ttoken:" + apiToken).c_str());
  udpServer.endPacket();
}

void on_recv_event(String msg)
{
  LOG.println(msg);
  // TODO auth
  if (msg == "token:" + apiToken + "\tcommand:open")
  {
    aiphone_open();
    ping();
  }
  else if (msg == "token:" + apiToken + "\tcommand:start")
  {
    aiphone_start();
    ping();
  }
  else if (msg == "token:" + apiToken + "\tcommand:stop")
  {
    aiphone_start();
    ping();
  }
  else
  {
    LOG.println("unknown msg.");
  }
}

char sendBuffer[128];
int sendBufferPos = 0;
unsigned long lastBufferTime;

void send(const char buf[], int len)
{
  udpServer.beginPacket(apiHost, apiPort);
  String msg = String() + "type:door\tdevice:" + device + "\ttoken:" + apiToken + "\tdata:";
  char hex[4] = "XX ";
  for (int p = 0; p < len; p++)
  {
    uint8_t b = (buf[p] >> 4) & 0x0f;
    hex[0] = b < 10 ? b + '0' : b - 10 + 'A';
    b = buf[p] & 0x0f;
    hex[1] = b < 10 ? b + '0' : b - 10 + 'A';
    msg += hex;
  }
  udpServer.write(msg.c_str());
  udpServer.endPacket();
}

void loop(void)
{
  server.handleClient();

  int packetSize = udpServer.parsePacket();
  if (packetSize)
  {
    char packetBuffer[255];
    int len = udpServer.read(packetBuffer, sizeof(packetBuffer));
    if (len > 0)
      packetBuffer[len] = 0;
    // LOG.println(packetSize);
    on_recv_event(packetBuffer);
  }

  if (Serial.available() > 0)
  {
    if (sendBufferPos < sizeof(sendBuffer))
    {
      sendBuffer[sendBufferPos++] = Serial.read();
      lastBufferTime = millis();
    }
  }
  else if (sendBufferPos > 0 && millis() - lastBufferTime > 250)
  {
    send(sendBuffer, sendBufferPos);
    sendBufferPos = 0;
  }
}
