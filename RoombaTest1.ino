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

#define CLOSE_BY 4

float lastDistance = 10000;

Roomba roomba(&Serial1, Roomba::Baud19200);
int ledPin =  13;
char *desiredMAC;
long turnDegrees;
char* buff;
int count = 0;  // for flickering LED on Roomba

char ssid[] = "Joe"; //  your network SSID (name) 
char pass[] = "elenapuleo";    // your network password (use for WPA, or use as key for WEP)
char server[] = "ancient-gorge-84645.herokuapp.com"; // WEB API

RestClient client(server, ssid, pass);

std::vector<char*> pathVector;
std::vector<int> angleVector;

bool closeby = false;
boolean backwards = false;
boolean takingBackwardsPath = false;
int currBeaconIndex = 0;

void roombaHomeCheck() {
  if(currBeaconIndex == -1) {
    //send home, end program
    roomba.dock();
    endProgram();
  }
}

void endProgram() {
  takingBackwardsPath = false;
  while(1) {}
}

char* convertFromString(String s) {
  int str_len = s.length() + 1;
  buff = new char[str_len];
  s.toCharArray(buff, str_len);
  return buff;
}

void turnRoomba(int desiredDegree) {
  if(desiredDegree == 0) {
    return;
  }
  
   uint8_t currDegree[2];

   int16_t temp = 0;

   boolean sensorsRead = roomba.getSensors((uint8_t)20, currDegree, (uint8_t)2);

   if (sensorsRead) {
    roomba.drive(500, Roomba::DriveInPlaceClockwise);
   }

   while(roomba.getSensors((uint8_t)20, currDegree, (uint8_t)2)) {

    delay(30);
    
    temp += -1 * (0 | (((int16_t)currDegree[0]) << 8) | (int16_t)(currDegree[1]));
      
    if(temp >= desiredDegree - 13) //constant
      break;
   }

   roomba.drive(0,0);
}

void driveRoomba(aJsonObject* jsonObj) {
  char* id = aJson.getObjectItem(jsonObj, "id")->valuestring; // This is the Mac Address from Eddystone
    
    if(strcmp(desiredMAC, id) == 0) {
      float distanceFromBeacon = aJson.getObjectItem(jsonObj, "distance")->valuefloat;

      // if found, check the distance and if less than a certain amount, turn by degrees sepcified
      if(distanceFromBeacon < CLOSE_BY) {
        closeby = true;
        backwards = false;
        lastDistance = 10000;
      } else {
        
        if(distanceFromBeacon > lastDistance) {
          backwards = !backwards;
        } 

        driveRoomba(distanceFromBeacon);
        
        lastDistance = distanceFromBeacon;   
      }
    }
}

void driveRoomba(int distanceFromBeacon) {
  //distance is in meters, roomba drives in 100 mm/s, multiply by 1000 to convert to ms
  int drivingDelay = distanceFromBeacon / (0.5) * 1000 / 10; //10 is arbitrary
  
  if(!backwards) {
    roomba.drive(500, Roomba::DriveStraight);
    delay(drivingDelay);
    roomba.drive(0, 0);
  } else {
    roomba.drive(-500, Roomba::DriveStraight);
    delay(drivingDelay);
    roomba.drive(0, 0);
  }
}

void turnRoombaForBeaconIndex() {
  if(currBeaconIndex == angleVector.size()) {
      turnRoomba(180);
      takingBackwardsPath = true;
      currBeaconIndex -= 2;
      desiredMAC = pathVector[currBeaconIndex];
      roomba.leds(ROOMBA_MASK_LED_ADVANCE, 127, 255);
  } else {
    if(!takingBackwardsPath) {
      desiredMAC = pathVector[currBeaconIndex];
      turnRoomba(angleVector.at(currBeaconIndex++));
    } else {
      turnRoomba(360 - angleVector[currBeaconIndex+1]);
      desiredMAC = currBeaconIndex <= 0 ? desiredMAC : pathVector[currBeaconIndex-1];
      --currBeaconIndex;
    }
    if(count % 2 == 0) {
      roomba.leds(ROOMBA_MASK_LED_ADVANCE, 255, 255);
    } else {
      roomba.leds(ROOMBA_MASK_LED_ADVANCE, 0, 255);
    }
    count++;
  }
}

void setup() {    

  Serial.begin(9600);
  
  pinMode(ledPin, OUTPUT);  
  digitalWrite(13, HIGH);
  
  roomba.start();
  roomba.fullMode();
  
  roomba.drive(0,0);
  roomba.leds(ROOMBA_MASK_LED_ADVANCE, 255, 255);

  // 2 bytes per note
  uint8_t song[] = {62, 12, 66, 12, 69, 12, 74, 36};
  roomba.song(0, song, sizeof(song));
   
  desiredMAC = "e8b0c6217436"; // ufl

  client.connect();

  delay(5000);

  digitalWrite(13, LOW);
  
  String response = "";

  int statusCode = client.get("/api/paths", &response);
  
  char* buff = convertFromString(response);

  aJsonObject* pathObject = aJson.getArrayItem(aJson.parse(buff), 0);

  delete buff;

  aJsonObject* pathIDList = aJson.getObjectItem(pathObject, "beacons");

  int numIDs = aJson.getArraySize(pathIDList);

  for(int i = 0; i < numIDs; ++i) {
    pathVector.push_back(aJson.getObjectItem(aJson.getArrayItem(pathIDList, i), "macAddress")->valuestring);
  }

  aJsonObject* pathAnglesList = aJson.getObjectItem(pathObject, "angles");
  int numAngles = aJson.getArraySize(pathAnglesList);
  
  for(int i = 0; i < numAngles; ++i) {
    angleVector.push_back(aJson.getArrayItem(pathAnglesList, i)->valueint);
  }
 
  turnRoombaForBeaconIndex(); //turn in direction of first beacon from base station and set desired mac
}

void loop() {

  roombaHomeCheck();
  
  system("rfkill unblock bluetooth");
  system("hciconfig hci0 down");
  system("hciconfig hci0 up");

  system("node /home/root/basic.js > /home/root/bleout.txt");
 
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

  aJsonObject* foundBeaconsList = aJson.parse(nodeBuff);

  delete nodeBuff;
  
  closeby = false;
  if(aJson.getArraySize(foundBeaconsList) != 0) {
    aJsonObject* crawl = aJson.getArrayItem(foundBeaconsList, 0);
      
    while(crawl != NULL) {
          
      driveRoomba(crawl);
      
      crawl = crawl->next;
    } 
  
    if(closeby) {
      roomba.playSong(0);
      turnRoombaForBeaconIndex();
    }  
  }
}
