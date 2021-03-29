/*
    CJ4 PFD/DCP Joystick Arduino firmware

    Copyright (c) 2021, Wojciech Swidzinski

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

/*
  This code has been made for a PFD/Display Mode Controler for CJ4 mod by Working Title (tested in version 0.11.3).
  It is designed to present a 26 button Joystick. This allows to use software like FSUIPC to map Joystick buttons to MSFS2020 Events.
  I am using the MobiFlight WASM module for the FSUIPC to MSFS interfacing.
  I highly recommend changing the name of the Arduino in boards.txt to something identifying this joystick.
  For example I use "CJ4 PFD_DMC Panel"

  It has been designed to be used with the Arduino Pro Micro or it's clones/derivatives using AtMega32u4 AVR.
*/


#include <CommonBusEncoders.h> //Managing encoders. Common Bus Encoders library version 
#include <Adafruit_MCP23017.h> //Managing switches via I2C Port Expander. Version 1.3.0
#include <Joystick.h> //Managing joystick functionality. Version 2.0.5

//Encoder Bus Pin Assignments
const int busAPin = 4;
const int busBPin = 5;
const int busSwPin = 6;

const int EncoderNumber = 5;
// Encoder Individual Pin Numbers:
const int Encoder_1_Pin_Number = 7;
const int Encoder_2_Pin_Number = 8;
const int Encoder_3_Pin_Number = 9;
const int Encoder_4_Pin_Number = 1;
const int Encoder_5_Pin_Number = 0;

//Initialize Encoders
CommonBusEncoders encoders(busAPin, busBPin, busSwPin, EncoderNumber);

//Initialize MCP23017 Port Expander
Adafruit_MCP23017 mcp; //only one, using default name mcp
const int MCP_Update_Interval = 10; //how often to pool the MCP for new button presses. Default 10 ms.

//Joystick settings
uint8_t buttonCount = 26; //Indicates how many buttons will be available on the joystick.
uint8_t hatSwitchCount = 0; //Indicates how many hat switches will be available on the joystick. Range: 0 - 2
bool includeXAxis = false; //Indicates if the X Axis is available on the joystick.
bool includeYAxis = false; //Indicates if the Y Axis is available on the joystick.
bool includeZAxis = false; //Indicates if the Z Axis (in some situations this is the right X Axis) is available on the joystick.
bool includeRxAxis = false; //Indicates if the X Axis Rotation (in some situations this is the right Y Axis) is available on the joystick.
bool includeRyAxis = false; //Indicates if the Y Axis Rotation is available on the joystick.
bool includeRzAxis = false; //Indicates if the Z Axis Rotation is available on the joystick.
bool includeRudder = false; //Indicates if the Rudder is available on the joystick.
bool includeThrottle = false; //Indicates if the Throttle is available on the joystick.
bool includeAccelerator = false; //Indicates if the Accelerator is available on the joystick.
bool includeBrake = false; //Indicates if the Brake is available on the joystick.
bool includeSteering = false; //Indicates if the Steering is available on the joystick.

// Initialize Joystick
Joystick_ Joy(JOYSTICK_DEFAULT_REPORT_ID,
              JOYSTICK_TYPE_JOYSTICK, buttonCount, hatSwitchCount,
              includeXAxis, includeYAxis, includeZAxis, includeRxAxis, includeRyAxis, includeRzAxis,
              includeRudder, includeThrottle, includeAccelerator, includeBrake, includeSteering);
// Arrays to store Button values
byte OldValues[] = { 0, 0, 0, 0, 0, 0,
                     0, 0, 0, 0, 0
                   };
byte NewValues[] = { 0, 0, 0, 0, 0, 0,
                     0, 0, 0, 0, 0
                   };
// 0 to 10 - physical switches

long LastTimer = 0;
const int TimerDelay = 200;

void setup() {
  // put your setup code here, to run once:
  //start-up the I2C bus
  Wire.setClock(400000); //Make sure it is running in fast mode
  Wire.begin();
  encoders.setDebounce(32); //set debounce - high (to compensate for not so good encoders
  encoders.resetChronoAfter(250); //milliseconds between switching encoders

  //Declare Encoders:
  encoders.addEncoder(1, 4, Encoder_1_Pin_Number, 1, 1100, 1300);
  encoders.addEncoder(2, 4, Encoder_2_Pin_Number, 1, 1400, 1600);
  encoders.addEncoder(3, 4, Encoder_3_Pin_Number, 1, 1700, 1900);
  encoders.addEncoder(4, 4, Encoder_4_Pin_Number, 1, 2000, 2200);
  encoders.addEncoder(5, 4, Encoder_5_Pin_Number, 1, 2300, 2500);

  //Initialize MCP23017
  mcp.begin();
  for (int i = 0; i < 11; i++) { //Iterate through first 10 ports and set them as input with the internal (50-100k) pull up activated
    mcp.pinMode(i, INPUT);
    mcp.pullUp(i, HIGH);
  }
  //Initialize Joystick
  Joy.begin();

}

void BUTTONS() {

  Serial.println("MCP-->");
  for (int i = 0; i < 11 ; i++) //iterate via all pin ports
  {
    NewValues[i] = mcp.digitalRead(i); //Read the pin
    if (NewValues[i] != OldValues[i]) { // On change of pin
      OldValues[i] = NewValues[i]; // Update pin list
      if (NewValues[i] == 1) {
        Joy.releaseButton(i); //on change to high, release button
      }
      if (NewValues[i] == 0) {
        Joy.pressButton(i); //on change to low press button
      }

    }
  }
}

void ENCODERS() {
  //Encoders
  int encoder_code = encoders.readAll();
  if (encoder_code > 0) {
    //Encoder was triggered, so I clear all previous encoders
    for (int i = 11; i < 26; i++) {
      Joy.releaseButton(i); //release all buttons
    }
    // get the button number
    // get the encoder number (encoders give a signal based on their button number):
    int encoder_no = (encoder_code / 100);
    // add the turn number (one direction = button equal to encoder, other direction = button equal to encoder + 1):
    encoder_no = encoder_no + (encoder_code % 100);
    Joy.pressButton(encoder_no); //press triggered button
    LastTimer = millis(); //Make sure that every switch gets at least some milliseconds
  }
  // Make sure to release the button after some time
  if ((LastTimer + TimerDelay) < millis()) {
    for (int i = 11; i < 26; i++) {
      Joy.releaseButton(i);
    }
    LastTimer = millis();
  }

}

void loop() {

  if (millis() % MCP_Update_Interval == 0 ) //Do a comparison every so often
  {
    BUTTONS() ;
  }
  ENCODERS() ;

}
