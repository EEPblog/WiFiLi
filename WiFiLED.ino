//MIT License
//
//Copyright (c) 2017 Angelo Pescarini (aka EEPblog)
//
//Permission is hereby granted, free of charge, to any person obtaining a copy
//of this software and associated documentation files (the "Software"), to deal
//in the Software without restriction, including without limitation the rights
//to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//copies of the Software, and to permit persons to whom the Software is
//furnished to do so, subject to the following conditions:
//
//The above copyright notice and this permission notice shall be included in all
//copies or substantial portions of the Software.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//SOFTWARE.

#include <ESP8266WiFi.h>                   //include the ESP8266 library
#define PWMpin 15                          //define the GPIO where the MOSFET is connected to. Refer to schematics!
#define StatLed 13                         //status led shows whether we are connected to WiFi.

const char* ssid = "YourWiFiName";         //enter the SSID of the WiFi to connect to
const char* password = "YourPassword";     //enter the password of the WiFi you're trying to connect to
int thisval = 100;                         //integer to store the current brightness
int lastval = 100;                         //integer to store the brightness from previous loop

WiFiServer server(80);                     //start the WiFi server

//=========================================================================== SETUP
void setup() {
  Serial.begin(115200);                    //start the Serial port - used for debugging
  delay(10);

  pinMode(PWMpin, OUTPUT);                 //set up the GPIOs that we'll be using
  pinMode(StatLed, OUTPUT);
   
  Serial.print("Connecting to ");          //print some info about the network to the console
  Serial.println(ssid);

  WiFi.begin(ssid, password);              //start the WiFi on the ESP8266

  while (WiFi.status() != WL_CONNECTED) {  //wait till the connection is estabilished
    digitalWrite(StatLed, 0);
    delay(250);
    Serial.print(".");
    digitalWrite(StatLed, 1);
    delay(250);
  }
  
  Serial.println("");                       //print some more debug info to the console
  Serial.println("WiFi connected");
  digitalWrite(StatLed, 0);

  server.begin();                           //initialize a server on the ESP8266
  Serial.println("Server started");

  // Print the IP address
  Serial.println(WiFi.localIP());           //some more debug into the console
}
//============================================================================= LOOP
void loop() {

  while (WiFi.status() != WL_CONNECTED) {   //in case the WiFi is disconnected for some
    digitalWrite(StatLed, 0);               //reason, the Status LED will blink
    delay(250);
    Serial.print(".");
    digitalWrite(StatLed, 1);
    delay(250);
  }
  //-------------------------------------------------------------- Handling the incoming data and UI
  
  WiFiClient client = server.available();   //check if a client has connected
  if (!client) {
    return;
  }

  Serial.println("new client");
  while (!client.available()) {              //wait until the client sends some data
    delay(1);
  }

  String req = client.readStringUntil('\r');  //Read the incoming request from the client
  Serial.println(req);
  client.flush();

  if (req.indexOf("/bright/0") != -1)         //create a value accoring to the request
    thisval = 0;                              // Completely OFF - 0% brightness
  else if (req.indexOf("/bright/25") != -1)
    thisval = 25;                             //25% brightness
  else if (req.indexOf("/bright/50") != -1)
    thisval = 50;                             //50% brightness
  else if (req.indexOf("/bright/75") != -1)
    thisval = 75;                             //75% brightness
  else if (req.indexOf("/bright/100") != -1)
    thisval = 100;                            //Full ON - 100% brightness
  else {                                      //otherwise, if there was an unexpected response, reply with the page below:
    Serial.println("invalid request");
    String s2 = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\n<head><title>My Light</title></head>Current Brightness: ";     //HTML intro stuff
    switch (thisval) {                        //let the user know the current state of the light
      case 0:
        s2 += "OFF";
        break;
      case 25:
        s2 += "25%";
        break;
      case 50:
        s2 += "50%";
        break;
      case 75:
        s2 += "75%";
        break;
      case 100:
        s2 += "Full ON";
        break;
      default:
        s2 += "Ooops, something went wrong :/";
        break;
    }
    s2 += "<br> <form action=\"";
    s2 += "/bright/100\"><input type=\"submit\" value=\"ON\" style=\"height:500px; width:1500px; font-size:250px;\" /></form>";     //draw the buttons

    s2 += "<form action=\"";
    s2 += "/bright/25\"><input type=\"submit\" value=\"25%\" style=\"height:250px; width:1500px; font-size:250px;\" /></form>";

    s2 += "<form action=\"";
    s2 += "/bright/50\"><input type=\"submit\" value=\"50%\" style=\"height:250px; width:1500px; font-size:250px;\" /></form>";

    s2 += "<form action=\"";
    s2 += "/bright/75\"><input type=\"submit\" value=\"75%\" style=\"height:250px; width:1500px; font-size:250px;\" /></form>";

    s2 += "<form action=\"";
    s2 += "/bright/0\"><input type=\"submit\" value=\"OFF\" style=\"height:500px; width:1500px; font-size:250px;\" /></form>";

    s2 += "</html>\n";
    client.print(s2); //send the response to the client
    return;
  }
//------------------------------------------------------------- Dimming animation between brightnesses

  if (thisval > lastval) {                          //decide if the last set brightness was bigger or smaller
    int PWMmax = map(thisval, 0, 100, 1, 1024);     //this will remap the brightness percentage to 10bit PWM values
    int PWMmin = map(lastval, 0, 100, 1, 1024);
    for (float fadeValue = PWMmax; fadeValue >= PWMmin; fadeValue = fadeValue / 1.1) { //this is where the magic takes place
      analogWrite(PWMpin, map(floor(PWMmax - fadeValue + PWMmin), 0, 1024, 1024, 0));  //we have to invert the value because
      Serial.println(PWMmax - fadeValue + PWMmin);                                     //of the transistor driver
      delay(10);
    }
    analogWrite(PWMpin, map(PWMmax, 0, 1024, 1024, 0));  //this is the final line that sets the desired brightness
  }
  if (thisval < lastval) {
    int PWMmax = map(lastval, 0, 100, 1, 1024);           //this will remap the brightness percentage to 10bit PWM values
    int PWMmin = map(thisval, 0, 100, 1, 1024);
    for (float fadeValue = PWMmin; fadeValue <= PWMmax; fadeValue = fadeValue * 1.1) { //this is where the magic takes place
      analogWrite(PWMpin, map(floor(PWMmax - fadeValue + PWMmin), 0, 1024, 1024, 0));  //we have to invert the value because
      Serial.println(PWMmax - fadeValue + PWMmin);                                     //of the transistor driver
      delay(10);
    }
    analogWrite(PWMpin, map(PWMmin, 0, 1024, 1024, 0));  //this is the final line that sets the desired brightness
  }
  //----------------------------------------------------------- Ending the client connection
  
  client.flush();            //we flush the client now to prevent the client from interrupting the animation
  
                             //Now we prepare the HTML response for the client
  String s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\n<head><title>My Light</title></head>WiFI enabled LED strip! Current Brightness: ";  //HTML intro stuff
  switch (thisval) {         //let the user know the current state of the light
    case 0:
      s += "OFF";
      break;
    case 25:
      s += "25%";
      break;
    case 50:
      s += "50%";
      break;
    case 75:
      s += "75%";
      break;
    case 100:
      s += "Full ON";
      break;
    default:
      s += "Ooops, something went wrong :/";
      break;
  }
  s += "<br> <form action=\"";  //Draw the buttons
  s += "/bright/100\"><input type=\"submit\" value=\"ON\" style=\"height:500px; width:1500px; font-size:250px;\" /></form>";

  s += "<form action=\"";
  s += "/bright/25\"><input type=\"submit\" value=\"25%\" style=\"height:250px; width:1500px; font-size:250px;\" /></form>";

  s += "<form action=\"";
  s += "/bright/50\"><input type=\"submit\" value=\"50%\" style=\"height:250px; width:1500px; font-size:250px;\" /></form>";

  s += "<form action=\"";
  s += "/bright/75\"><input type=\"submit\" value=\"75%\" style=\"height:250px; width:1500px; font-size:250px;\" /></form>";

  s += "<form action=\"";
  s += "/bright/0\"><input type=\"submit\" value=\"OFF\" style=\"height:500px; width:1500px; font-size:250px;\" /></form>";

  s += "</html>\n";

  client.print(s);          //Send the constructed HTML response
  delay(1);
  Serial.println("Client disonnected");
  Serial.println(thisval);  //some more debugging
  lastval = thisval;        //lastly store the current brightness level as "old"

  //NOTE: The client will actually be disconnected
  //      when the function returns and 'client' object is detroyed
}

