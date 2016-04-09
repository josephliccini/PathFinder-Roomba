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

char ssid[] = "Joe"; //  your network SSID (name) 
char pass[] = "elenapuleo";    // your network password (use for WPA, or use as key for WEP)
char server[] = "ancient-gorge-84645.herokuapp.com";
char* buff;
char* macs[2] = { "e8b0c6217436", "f05e48d363a8" };
int count = 0;
RestClient client(server, ssid, pass);

std::vector<char*> pathIdStack;
boolean backwards = false;

char* convertFromString(String s) {
  int str_len = s.length() + 1;
  buff = new char[str_len];
  s.toCharArray(buff, str_len);
  return buff;
}

void turnRoomba(int desiredDegree) {

  //Serial.println("About to drive");
  
  roomba.drive(500, Roomba::DriveInPlaceClockwise);
  
  //Serial.println("Started to drive");
  
   uint8_t currDegree[2];

   int16_t temp = 0;
   
   while(roomba.getSensors((uint8_t)20, currDegree, (uint8_t)2) && currDegree[0] != desiredDegree) {

    delay(30);
    
    temp += -1 * (0 | (((int16_t)currDegree[0]) << 8) | (int16_t)(currDegree[1]));
    
    //Serial.println(temp);
    
    if(temp >= desiredDegree)
      break;
   }

   roomba.drive(0,0);

   //Serial.print("Desired: ");
   //Serial.println(desiredDegree);
}

void setup()
{    
  //Serial.begin(9600);
 pinMode(ledPin, OUTPUT);  
  digitalWrite(13, HIGH);

  roomba.start();
  roomba.fullMode();
  roomba.leds(ROOMBA_MASK_LED_ADVANCE, 255, 255);
  
 
      
  desiredMAC = "e8b0c6217436"; // ufl

  client.connect();

  delay(5000);

  digitalWrite(13, LOW);
  
  String response = "";

  int statusCode = client.get("/api/paths", &response);
  
  //Serial.println();

  char* buff = convertFromString(response);

  aJsonObject* pathObject = aJson.parse(buff);

  delete buff;
  
  pathObject = aJson.getArrayItem(pathObject, 0);

  aJsonObject* pathIDList = aJson.getObjectItem(pathObject, "beaconIds");

  int numIDs = aJson.getArraySize(pathIDList);

  //Serial.println("-------------------------------------------------------------------------------------------------------------");
  //Serial.println("Visiting in order:");

  std::vector<char*> pathVector;

  for(int i = 0; i < numIDs; ++i) {
    pathVector.push_back(aJson.getArrayItem(pathIDList, i)->valuestring);
  }

  //starting at base station
  
  std::vector<int> angleVector;
  aJsonObject* pathAnglesList = aJson.getObjectItem(pathObject, "angles");
  int numAngles = aJson.getArraySize(pathAnglesList);
  
  for(int i = 0; i < numAngles; ++i) {
    angleVector.push_back(aJson.getArrayItem(pathAnglesList, i)->valueint);
  }
 
  for(int i = 0; i < numAngles; ++i) {
    //Serial.print("Go to ");
    //Serial.print(pathVector[i+1]);
    //Serial.print(" with angle ");
    //Serial.println(angleVector[i]);
  }
}

void loop() {
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

  //Serial.println("Node Buff: where we're parsing from");
  //Serial.println(nodeBuff);

  aJsonObject* foundBeaconsList = aJson.parse(nodeBuff);

  delete nodeBuff;
  
  bool found = false;
  bool closeby = false;
    
  aJsonObject* crawl = aJson.getArrayItem(foundBeaconsList, 0);

  float lastDistance = 10000;
  
  int angle = 0;
    
  while(crawl != NULL) {
        
    char* id = aJson.getObjectItem(crawl, "id")->valuestring; // This is the Mac Address from Eddystone
    
    if(strcmp(desiredMAC, id) == 0) {
      found = true;

      float distanceFromBeacon = aJson.getObjectItem(crawl, "distance")->valuefloat;

      // if found, check the distance and if less than a certain amount, turn by degrees sepcified
      if(distanceFromBeacon < 5.0) {
       
        // turn clockwise in place
        // roomba.drive(100, Roomba::DriveInPlaceClockwise);

        closeby = true;

        //Serial.println("I'm close to the desired one");
        break;
        
        // roomba.drive(0, 0);
        //update next direction and mac address
        // char *temp = desiredMAC;
        // desiredMAC = nextMAC;
        // nextMAC = temp;
      } else {
         
        if(distanceFromBeacon > lastDistance) {
         backwards = !backwards;
        } 
        
        lastDistance = distanceFromBeacon;
        //Serial.println("Last Distance");
        //Serial.println(lastDistance);
        //Serial.println("Distance From Beacon");
        //Serial.println(distanceFromBeacon);
        
        //distance is in meters, roomba drives in 100 mm/s
        int drivingDelay = distanceFromBeacon / (0.01);
        if(!backwards) {
          roomba.drive(100, Roomba::DriveStraight);
          delay(drivingDelay);
          roomba.drive(0, 0);
        } else {
          roomba.drive(-100, Roomba::DriveStraight);
          delay(drivingDelay);
          roomba.drive(0, 0);
        }
      }
    } else {
      //Serial.println("This is not the desired beacon MAC address");
    }
    
    crawl = crawl->next;
  } 

  if(found && closeby) {
    //Serial.println("Let's move to the next beacon");
    turnRoomba(180);
    count++;
    count = count % 2;
    desiredMAC = macs[count];
  }  
}
