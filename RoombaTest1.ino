#include <aJSON.h>
#include <avr/pgmspace.h>
#include <Roomba.h>
#include <SPI.h>
#include <WiFi.h>
#include <SPI.h>
#include "RestClient.h"
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>


Roomba roomba(&Serial1, Roomba::Baud19200);
int ledPin =  13;
char *desiredMAC;
long turnDegrees;

char ssid[] = "Joe"; //  your network SSID (name) 
char pass[] = "elenapuleo";    // your network password (use for WPA, or use as key for WEP)
char server[] = "ancient-gorge-84645.herokuapp.com";

RestClient client(server, ssid, pass);

void setup()
{    
    Serial.begin(9600);
    pinMode(ledPin, OUTPUT);  
        
    desiredMAC = "e8b0c6217436"; // ufl
}

char* convertFromString(String s) {
  int str_len = s.length() + 1;
  char* buff = new char[str_len];
  s.toCharArray(buff, str_len);
  return buff;
}

void loop()
{
  client.connect();

  delay(10000);
  
  String response = "";

  int statusCode = client.get("/api/beacons", &response);
  
  Serial.println();

  char* buff = convertFromString(response);

  aJsonObject* jsonObject = aJson.parse(buff);

  aJsonObject* beacon = aJson.getArrayItem(jsonObject, 1);

  aJsonObject* MACAddr = aJson.getObjectItem(beacon, "macAddress");
  String macStr = MACAddr -> valuestring;

  Serial.println(macStr.buffer);

  system("rfkill unblock bluetooth");
  system("hciconfig hci0 down");
  system("hciconfig hci0 up");

  system("node /home/root/basic.js > /home/root/bleout.txt");
  
  digitalWrite(ledPin, LOW);

  String nodeOutput = "";
  size_t length = 0;
  ssize_t read;
  char* line = NULL;
  
  FILE* f = fopen ("/home/root/bleout.txt", "r");
  
  if (f) {
    while ((read = getline(&line, &length, f)) != -1) {
      nodeOutput.concat(line);
    }
    fclose (f);
    if (line) {
      free(line);
    }
  }
  
  char* nodeBuff = convertFromString(nodeOutput);

  aJsonObject* foundBeaconsArray = aJson.parse(nodeBuff);

  bool found = false;
  aJsonObject* crawl = aJson.getArrayItem(foundBeaconsArray, 0);
  while (crawl != NULL) {
    digitalWrite(13, LOW);
    aJsonObject* beacon = crawl;
    aJsonObject* id = aJson.getObjectItem(beacon, "id"); // This is the Mac Address from Eddystone
    char* idString = id->valuestring;
    if(strcmp(desiredMAC, idString) == 0) {
        Serial.println("FOUND");
        found = true;
        // if found, check the distance and if less than a certain amount, turn by degrees sepcified
        float distanceFromBeacon = aJson.getObjectItem(beacon, "distance")->valuefloat;
        if(distanceFromBeacon < 10.0) {
          // turn clockwise in place
          // roomba.drive(100, Roomba::DriveInPlaceClockwise);
          digitalWrite(13, HIGH);
          // roomba.drive(0, 0);
          //update next direction and mac address
          // char *temp = desiredMAC;
          // desiredMAC = nextMAC;
          // nextMAC = temp;
        }
      } else {
        Serial.println("NOT FOUND");
        Serial.println(idString);
        digitalWrite(13, LOW);
      }
      crawl = crawl->next;
  }
  
  
  //nodeOutput is separated by newlines in order: url, id, distance
  //read every 3 lines into variables and compare id's until the desired next id is found, then break out of loop
  
//  if(nodeOutput) {
//    int i = 0;
//    char *p = strtok(nodeOutput, "\n");
//    char *arr[9];
//    Serial.println("NEXT ITERATION");
//  
//    while( p != NULL) {
//      if(i < 9) {
//        arr[i++] = p;
//        p = strtok (NULL, "\n");
//      }
//    }
//    
//    //i should be multiple of 3
//    while((i % 3) != 0) {
//      i--;
//    }
//    //if couldn't find desired device, wait for next scan and stop driving until distance known
//
//    //loop through and check if desiredMAC was found
//    
//    boolean found = false;
//    for(int j = 1; j < i; j += 3) {
//      if(strcmp(desiredMAC, arr[j]) == 0) {
//        Serial.println("FOUND");
//        found = true;
//        //if found, check the distance and if less than a certain amount, turn by degrees sepcified
////        char *distanceFromBeacon = arr[j+1];
////        if(atol(distanceFromBeacon) < 1.0) {
////          //turn clockwise in place
////          roomba.drive(100, Roomba::DriveInPlaceClockwise);
////          delay(1000);
////          roomba.drive(0, 0);
////          //update next direction and mac address
////          char *temp = desiredMAC;
////          desiredMAC = nextMAC;
////          nextMAC = temp;
////        }
//      } else {
//        Serial.println("NOT FOUND");
//      }
//    }
//
//    Serial.println("Before Arr");
//    for(int j = 0; j < i; j++) {
//      Serial.println(arr[j]);
//    }
//   Serial.println("After Arr");
//  }
// 
// Serial.println("After LOOP");
//  // jsonstr = strhelp;
//  if(jsonstr) {
//    free(jsonstr);
//  }
//
//  
//  Serial.println("AFTER JSONSTR");
//  if(nodeOutput) {
//      free(nodeOutput);
//   }
//   
//   Serial.println("After nodOutput");
//
//  // roomba.drive(100, 0);
//  //Serial.print("What's Up Bitches");

  // delay(700);
  
  // roomba.leds(ROOMBA_MASK_LED_ADVANCE, 0, 255);

  // roomba.drive(-100, 0);

}
