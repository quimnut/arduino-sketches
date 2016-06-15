#include <ESP8266WiFi.h>
#include <WiFiClient.h>

#include "SPI.h"
#include "UTFT.h"

// wire it up like this
// https://i.ytimg.com/vi/z5QxTDmKr9Y/maxresdefault.jpg
// esp board for arduino ide, v2, might be in boards manager too if not here it is
// https://github.com/esp8266/Arduino
// UTFT ported to the esp board, needs 1.6.5~6 to work, i'm using 1.6.5
// https://github.com/gnulabis/UTFT-ESP8266
// thanks to this good citizen
// http://www.avrfreaks.net/forum/display-raw-bitmap-image-tft
// utft online converter for your images, awesome.
// http://www.rinkydinkelectronics.com/t_imageconverter565.php

// 1. assemble hardware
// 2. install esp8266 board for arduino ide (after installing arduino ide)
// 3. try a utft demo sketch out, use pins below

// This will make a http connection on port 80 to the server defined below
// It will load ident-1.raw and then ident-2.raw until it receives a 404, when it will then load 1 the next loop
// YMMV I used apache2 so if your headers are oddly different then fix that

UTFT myGLCD ( ILI9341_S5P, 13, 14, 5, 3, 2 );

extern uint8_t SmallFont[];

char ssid[] = "yourSSIDhere";  
char pass[] = "supersecretpassword"; 
int status = WL_IDLE_STATUS;

char server[] = "192.168.1.1"; // http server
char ident[] = "tap1"; // this is the prefix to my raw files

WiFiClient client;

void printWifiStatus() {
  // print SSID 
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  myGLCD.print("Connected to WiFi", LEFT, 18);

  // print IP address
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

int counter = 1;
void connectServer() {
  if (client.connect(server, 80)) {
    Serial.print("OK TCP session to ");
//    myGLCD.print("Connecting to webserver", LEFT, 1);
    Serial.println(server);
    client.print("GET /");
    client.print(ident);
    client.print("-");
    client.print(counter);
    client.print(".raw");
    client.println(" HTTP/1.1");
    client.print("Host: ");
    client.println(server);
    client.println("Connection: close");
    client.println();
  }
}


void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(9600);

  myGLCD.InitLCD();
  myGLCD.InitLCD(PORTRAIT);
  myGLCD.clrScr();
  myGLCD.setFont(SmallFont);
  myGLCD.setColor(255, 255, 255);
 
 // attempt to connect to Wifi network:
  while ( status != WL_CONNECTED) {
  
    Serial.print("INFO Connecting to SSID: ");
    Serial.println(ssid);
    myGLCD.print("Connecting to WiFi", LEFT, 1);
    
    // Connect to WPA/WPA2 network:    
    status = WiFi.begin(ssid, pass);

    // wait for connection:
    delay(3500);
  }

  Serial.println("OK Connected to WiFi");
  
  printWifiStatus();

}

void loop() {
//  delay(10000);
  Serial.println("OK Starting loop");
  
  connectServer();
  delay(1000);

  int breaks = 0; 
  while (client.available()) {

    if (breaks < 2) {
      String c = client.readStringUntil('\n');  
      if (c == "\r") {
        breaks++;
      } else if (c == "Connection: close\r") {
        breaks++;
        breaks++;
      } else if (c == "HTTP/1.1 404 Not Found\r") {
        counter = 0;
        breaks++;
        breaks++;
      } else {
        Serial.print("HEADER: ");
        Serial.println(c); 
      }
    } else {
    uint16_t buf[240];

    cbi(myGLCD.P_CS, myGLCD.B_CS);
    
    for (int y = 1; y < 320-1 && client.available(); y++) {
        myGLCD.setXY(1, y, 239 , y);
        for (int x = 0; x < 240; x++) {
            byte l = client.read();
            byte h = client.read();
            buf[x] = l * 256 + h; // store one line of bits
        }
        
     //   for (int x = 320; x > 0; x--) { // reverse the bits for landscape - 320.
     for (int x = 0; x < 240; x++) { // portrait - 240.
           myGLCD.LCD_Write_DATA((buf[x] >> 8),buf[x]);
          delay(0);
        }
        
    }
    sbi(myGLCD.P_CS, myGLCD.B_CS);
          
    }

  // if the server's disconnected, stop the client:
    if (!client.connected()) {
      Serial.println("");
      Serial.println("OK Disconnecting from server.");
      client.stop();
    }
  }

    delay(15000);
    Serial.println("OK Ended loop");
    counter++;
}



