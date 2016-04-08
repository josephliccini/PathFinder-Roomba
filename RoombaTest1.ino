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
#include <vector>


Roomba roomba(&Serial1, Roomba::Baud19200);
int ledPin =  13;
char *desiredMAC;
long turnDegrees;

char ssid[] = "sahir"; //  your network SSID (name) 
char pass[] = "abcd1234";    // your network password (use for WPA, or use as key for WEP)
char server[] = "ancient-gorge-84645.herokuapp.com";
char* buff;

RestClient client(server, ssid, pass);


char* convertFromString(String s) {
  int str_len = s.length() + 1;
  buff = new char[str_len];
  s.toCharArray(buff, str_len);
  return buff;
}

void turnRoomba(int desiredDegree) {

  Serial.println("About to drive");
  
  roomba.drive(100, Roomba::DriveInPlaceClockwise);
  Serial.println("Started to drive");
  
   uint8_t currDegree[2];

   int16_t temp = 0;
   
   while(roomba.Roomba::getSensors((uint8_t)20, currDegree, (uint8_t)2) && currDegree[0] != desiredDegree) {

    delay(35);
    
    temp += -1 * (0 | (((int16_t)currDegree[0]) << 8) | (int16_t)(currDegree[1]));
    
    Serial.println(temp);
    
    if(temp >= desiredDegree)
      break;
   }

   roomba.drive(0,0);

//   Serial.print("Curr: ");
  // Serial.println(currDegree);
   Serial.print("Desired: ");
   Serial.println(desiredDegree);
}

void setup()
{    
  Serial.begin(9600);

  roomba.start();
  roomba.fullMode();
  roomba.leds(ROOMBA_MASK_LED_ADVANCE, 255, 255);
  
  pinMode(ledPin, OUTPUT);  
      
  desiredMAC = "e8b0c6217436"; // ufl

  client.connect();

  delay(5000);
  
  String response = "";

  int statusCode = client.get("/api/paths", &response);
  
  Serial.println();

  char* buff = convertFromString(response);

  aJsonObject* pathObject = aJson.parse(buff);
  pathObject = aJson.getArrayItem(pathObject, 0);

  aJsonObject* pathIDList = aJson.getObjectItem(pathObject, "beaconIds");

  int numIDs = aJson.getArraySize(pathIDList);

  Serial.println("-------------------------------------------------------------------------------------------------------------");
  Serial.println("Visiting in order:");

  std::vector<char*> pathVector;

  for(int i = 1; i < numIDs; ++i) {
    pathVector.push_back(aJson.getArrayItem(pathIDList, i)->valuestring);
  }

  Serial.println(pathVector.size());
   
  if(buff) 
    free(buff);
}

void loop() {
  
  // Serial.println(macStr.buffer);

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

  Serial.println("Node Buff: where we're parsing from");
  Serial.println(nodeBuff);

  aJsonObject* foundBeaconsList = aJson.parse(nodeBuff);

  bool found = false;
  bool closeby = false;
    
  aJsonObject* crawl = aJson.getArrayItem(foundBeaconsList, 0);

  float lastDistance = 10000;
  boolean backwards = false;
  int angle = 0;
    
  while(crawl != NULL) {
    
    // aJsonObject* beacon = crawl;
    
    char* id = aJson.getObjectItem(crawl, "id")->valuestring; // This is the Mac Address from Eddystone

    Serial.println(id);
    
    if(strcmp(desiredMAC, id) == 0) {
      // Serial.println("FOUND");
      found = true;

      float distanceFromBeacon = aJson.getObjectItem(crawl, "distance")->valuefloat;

      // if found, check the distance and if less than a certain amount, turn by degrees sepcified
      if(distanceFromBeacon < 10.0) {
       
        // turn clockwise in place
        // roomba.drive(100, Roomba::DriveInPlaceClockwise);

        closeby = true;

        Serial.println("I'm close to the desired one");
        break;
        
        // roomba.drive(0, 0);
        //update next direction and mac address
        // char *temp = desiredMAC;
        // desiredMAC = nextMAC;
        // nextMAC = temp;
      } else {
         
        if(distanceFromBeacon > lastDistance) {
         backwards = true;
        } 
        
        lastDistance = distanceFromBeacon;
        //distance is in meters, roomba drives in 100 mm/s
        int drivingDelay = distanceFromBeacon / (0.1);
        if(backwards) {
          roomba.drive(100, Roomba::DriveStraight);
          delay(drivingDelay);
        } else {
          roomba.drive(-100, Roomba::DriveStraight);
          delay(drivingDelay);
        }
      }
    } else {
      Serial.println("This is not the desried beacon MAC address");
      // Serial.println(idString);
      // digitalWrite(13, LOW);
    }
    
    crawl = crawl->next;
  } 

  if(found) {
    Serial.println("Found the beacon");
  }
  if(closeby) {
    Serial.println("Desired beacon is within 10");
    turnRoomba(90);
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
