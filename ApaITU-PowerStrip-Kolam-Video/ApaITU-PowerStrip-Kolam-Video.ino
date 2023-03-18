/*
 * #ApaITU march 2023
 * video #1 Project Intro & DIY Module : https://youtu.be/KCm-y2z40Z0
 * video #2 Schematic, Assembly Instruction & First Trial : https://youtu.be/-XCEHxJLOTc
 * video #3 Code Explanation : 
 * Components :
 * used PowerStrip 3 prong
 * 1 Wemos D1 Mini
 * 2 relay module (DIY see video part 1)
 * 2 touch sensor
 * 1 Hi-link 5V 0.6A psu
 * 2 Led indicator Module (DIY see video part 1)
 * 
 * Tasks :
 * 1) PowerStrip or (PS) should work online as well offline (without Present of Access Point)
 * 2) Relay/PowerOutlet could be activate via web or touch sensor
 * 3) WiFi & Web
 * a) StaticIP
 * b) Non-blocking on void setup
 * c) synchronize relay state between led indicator & web button
 * d) information on the web page : outlet1, outlet2, ssid, rssi (signal strength)
 * e) D4-build-in led on wemos should blink if wifi connected
 * f) Over the Air Programming (OTA)
 * 
 * 
 */

// Library


#include <OneButton.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include "index.h"

#define DEBUG true  //set to true for debug output, false for no debug output
#define DEBUG_SERIAL if(DEBUG)Serial

//touch sensor on D5 & D6 - false = Active LOW
OneButton btn1(D5,false); 
OneButton btn2(D6,false);

ESP8266WebServer server(80);

IPAddress local_IP(192,168,2,12); 
IPAddress gateway(192,168,2,1); 
IPAddress subnet(255,255,255,0);
IPAddress dns(192,168,1,200); 

String page = FPSTR(MAIN_PAGE); //load page from index.h

//relay 1 & 2 state 0 = low  , 1 = high
int r1State = 0;
int r2State = 0;

const uint32_t connectTimeoutMs = 5000; //wifiMulti.run(connectionTimeoutMS)
uint32_t pMillis;   //prev Millis for WiFi state check 
uint32_t bpMillis;  //prev Millis for led Blink on D4
uint32_t bDelay;    // blink interval
bool bFlag = false; //blink Flag

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println();
  Serial.println(__FILE__);
  pinMode(D4,OUTPUT);
  digitalWrite(D4,HIGH);
  pinMode(D7,OUTPUT);
  pinMode(D8,OUTPUT);
  digitalWrite(D7,r1State);
  digitalWrite(D8,r2State);
  btn1.attachClick(btn1Click);
  btn2.attachClick(btn2Click);

  WiFi.mode(WIFI_STA);
  WiFi.config(local_IP, gateway, subnet,dns);
  WiFi.begin("ApaITU", "1234567890");
  
  /*
  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    DEBUG_SERIAL.println(".");
  }
  */

  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);

  //webserver handler
  server.on("/", handleRoot);
  server.on("/relay1State",getState1);      //check state every 1seconds if change made via touch sensor
  server.on("/relay2State",getState2);
  server.on("/update",relayUpdate);         //write relay state from web if button slided
  server.on("/getRSSI",getRSSI);
  server.on("/getSSID",getSSID);
  server.on("/reboot",[](){digitalWrite(D8,HIGH);delay(1000);digitalWrite(D8,LOW); delay(1000);digitalWrite(D8,HIGH);delay(1000);ESP.restart();});
  
  server.begin();     //Run WebServer
  
  DEBUG_SERIAL.println("HTTP server started");
  DEBUG_SERIAL.print("http://");
  DEBUG_SERIAL.print(WiFi.localIP());
  DEBUG_SERIAL.println("/");
  if(WiFi.status()==3){
    DEBUG_SERIAL.print("WiFi connected with IP : ");
    DEBUG_SERIAL.println(WiFi.localIP());
    bFlag = true;
  }else{
    DEBUG_SERIAL.println("WiFi is not Connected yet");
    bFlag = false;
  }
  
  otaSetup();     //OTA
}

//Touch Sensor btn1 & btn2
void btn1Click(){
  DEBUG_SERIAL.println("btn1 clicked  !!! " + String(r1State));
  r1State = !r1State;
  digitalWrite(D7,r1State);
}
void btn2Click(){
  DEBUG_SERIAL.println("btn2 clicked  !!! " + String(r2State));
  r2State = !r2State;
  digitalWrite(D8,r2State);
}

void handleRoot(){
  String s;
   s+=page;
  if(digitalRead(D7)){
    s.replace("@@RElAY1STATE@@","");
  }else{
    s.replace("@@RElAY1STATE@@","checked");
  }
  DEBUG_SERIAL.println(digitalRead(D7));
  
  if(digitalRead(D8)){
    s.replace("@@RElAY2STATE@@","");
  }else{
    s.replace("@@RElAY2STATE@@","checked");
  }
  DEBUG_SERIAL.println(digitalRead(D8));
  
  server.send(200,"text/html",s);
}

void getState1(){
  String s = String(digitalRead(D7));
  server.send(200,"text/plain",s);
}

void getState2(){
  String s = String(digitalRead(D8));
  server.send(200,"text/plain",s);
}

void getRSSI(){
  String s="";
  if(WiFi.status() == WL_CONNECTED){
    s += String(WiFi.RSSI());
  }else{
    s +="WiFi Not Connected";
  }
  server.send(200,"text/plain",s);

}

void getSSID(){
  String s="";
  if(WiFi.status() == WL_CONNECTED){
    s += WiFi.SSID();
  }else{
    s +="WiFi Not Connected";
  }
  server.send(200,"text/plain",s);

}

void relayUpdate(){
  String r = server.arg("relay");
  String s = server.arg("state");
  DEBUG_SERIAL.print(r);
  DEBUG_SERIAL.println(" : relayUpdate("+ s +")" );
  if(r == "relay1"){
    if(s == "on"){
      r1State = 1; 
    }else{
      r1State = 0;
    }
    digitalWrite(D7,r1State);
  }
  if(r == "relay2"){
    if(s == "on"){
      r1State = 1; 
    }else{
      r1State = 0;
    }
    digitalWrite(D8,r1State);
  }
  server.send(200,"text/html","UPDATE");
}



//blink D4 - flash led
void flashBlink(){
  static int flashState;
  if(bFlag){
    bDelay=1000;
  }else{
    bDelay=300;
  }
  if(millis()-bpMillis>bDelay){
    bpMillis=millis();
    flashState = !flashState;
    digitalWrite(D4,flashState);
  }
}

void loop() {
  flashBlink();
  btn1.tick();
  btn2.tick();
  server.handleClient();
  ArduinoOTA.handle();

  //check wifi connectivity every 10secs
  if(millis()-pMillis>10000){
    pMillis = millis();
    if (WiFi.status() == WL_CONNECTED) {
      DEBUG_SERIAL.print("WiFi connected: ");
      DEBUG_SERIAL.print(WiFi.SSID());
      DEBUG_SERIAL.print(" ");
      DEBUG_SERIAL.println(WiFi.localIP());
      DEBUG_SERIAL.print("RSSI : ");
      DEBUG_SERIAL.print(WiFi.RSSI());
      DEBUG_SERIAL.println("db");
      bFlag=true;
    } else {
      DEBUG_SERIAL.println("WiFi not connected!");
      bFlag=false;
    }
  }
}
