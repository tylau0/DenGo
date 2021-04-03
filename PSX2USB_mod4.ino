/*
 * Copyright (C) 2020, 2021 Eddie
 * 
 * This file is modified from the PsxNewLib example at
 * https://github.com/SukkoPera/PsxNewLib/blob/master/examples/PSX2USB/PSX2USB.ino
 * 
 * It is using the Psx library at 
 * https://playground.arduino.cc/Main/PSXLibrary/
 * and the ArduinoJoystick library at
 * https://github.com/MHeironimus/ArduinoJoystickLibrary.
 * 
 *  ---------------------
 * \ 1 2 3 4 5 6 7 8 9 /
 *  -------------------
 *
 * Connection to Arduino Leonardo
 * PlayStation 1 controller pin 1 (DATA) --> Arduino Leonardo digital port 8
 * PlayStation 1 controller pin 2 (CMD) --> Arduino Leonardo digital port 9
 * PlayStation 1 controller pin 4 (GND) --> Arduino Leonardo port GND
 * PlayStation 1 controller pin 5 (VCC) --> Arduino Leonardo port 3.3V
 * PlayStation 1 controller pin 6 (ATT) --> Arduino Leonardo digital port 11
 * PlayStation 1 controller pin 7 (CLK) --> Arduino Leonardo digital port 10
 * 
 * Arduino Leonardo digital port 12 is for the optional pedal.
 * This one adds support of 
 * 1) an additional pedal for horn
 * Have pin 12 being 5V if the pedal is not pressed, 0V otherwise.
 * Connect the pin to 5V if no pedal is used.
 * 2) an additional ultrasound sensor for finger pointing confirmation
 * Have pin 4 cabled to the trigger output of the ultrasound sensor.
 * Have pin 5 cabled to the echo output of the ultrasound sensor.
 * Connect the pins to ground if no ultrasound sensor is used.
 *
 */

#include <Psx.h>
#include <Joystick.h>

#define ultrasoundTriggerPin 4
#define ultrasoundEchoPin 5
#define dataPin 8
#define cmndPin 9
#define attPin 11
#define clockPin 10
#define pedalPin 12

#define ANALOG_MIN_VALUE 0U
#define ANALOG_MAX_VALUE 255U
#define ANALOG_IDLE_VALUE 128U

const unsigned long POLLING_INTERVAL = 1000U / 50U;

// Send debug messages to serial port
//#define ENABLE_SERIAL_DEBUG

Psx Psx;

unsigned int data = 0;  // data stores the controller response

Joystick_ usbStick (
  JOYSTICK_DEFAULT_REPORT_ID,
  JOYSTICK_TYPE_GAMEPAD, 
  24,     // buttonCount  // my change
  0,      // hatSwitchCount (0-2) // my change
  true,   // includeXAxis
  true,   // includeYAxis
  false,    // includeZAxis
  true,   // includeRxAxis
  true,   // includeRyAxis
  false,    // includeRzAxis
  false,    // includeRudder
  false,    // includeThrottle
  false,    // includeAccelerator
  false,    // includeBrake
  false   // includeSteering
);


#ifdef ENABLE_SERIAL_DEBUG
  #define dstart(spd) do {Serial.begin (spd); while (!Serial) {digitalWrite (LED_BUILTIN, (millis () / 500) % 2);}} while (0);
  #define debug(...) Serial.print (__VA_ARGS__)
  #define debugln(...) Serial.println (__VA_ARGS__)
#else
  #define dstart(...)
  #define debug(...)
  #define debugln(...)
#endif

boolean haveController = false;


#define toDegrees(rad) (rad * 180.0 / PI)

#define deadify(var, thres) (abs (var) > thres ? (var) : 0)


void setup () {
  // Lit the builtin led whenever buttons are pressed
  pinMode (LED_BUILTIN, OUTPUT);

  // Init Joystick library
  usbStick.begin (false);   // We'll call sendState() manually to minimize lag

  // Init Psx library
  Psx.setupPins(dataPin, cmndPin, attPin, clockPin, 10);  // Defines what each pin is used
                                                          // (Data Pin #, Cmnd Pin #, Att Pin #, Clk Pin #, Delay)
                                                          // Delay measures how long the clock remains at each state,
                                                          // measured in microseconds.
                                                          // too small delay may not work (under 5)

  // This way we can output the same range of values we get from the PSX controller
  usbStick.setXAxisRange (ANALOG_MIN_VALUE, ANALOG_MAX_VALUE);
  usbStick.setYAxisRange (ANALOG_MIN_VALUE, ANALOG_MAX_VALUE);
  usbStick.setRxAxisRange (ANALOG_MIN_VALUE, ANALOG_MAX_VALUE);
  usbStick.setRyAxisRange (ANALOG_MIN_VALUE, ANALOG_MAX_VALUE);

  // Init ultrasound
  pinMode(ultrasoundTriggerPin, OUTPUT);
  pinMode(ultrasoundEchoPin, INPUT);
  digitalWrite(ultrasoundTriggerPin, LOW);  

  dstart (115200);

  debugln (F("Ready!"));
}

void loop () {
  static unsigned long last = 0;
 
  if (millis () - last >= POLLING_INTERVAL) {
    last = millis ();
    
    data = Psx.read();

    // Work on the ultrasound sensor first
    // Duration will be the input pulse width and distance will be the distance to the obstacle in centimeters
    float duration = 0, distance = 0;
    // Output pulse with 1ms width on trigPin
    digitalWrite(ultrasoundTriggerPin, LOW);
    delayMicroseconds(2);
    digitalWrite(ultrasoundTriggerPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(ultrasoundTriggerPin, LOW);
    // Measure the pulse input in echo pin
    duration = pulseIn(ultrasoundEchoPin, HIGH);
    // Distance is half the duration devided by 29.1 (from datasheet)
    distance = (duration/2) / 29.1;
    debugln (F(distance));
    
    usbStick.setButton (0, data & psxSqu ? 1 : 0);
    usbStick.setButton (1, data & psxX ? 1 : 0);
    if (data & psxO || digitalRead(pedalPin) == 0) {
        usbStick.setButton(2, 1);
    }
    else {
        usbStick.setButton(2, 0);
    }
    usbStick.setButton (3, data & psxTri ? 1 : 0);
    usbStick.setButton (4, data & psxL1 ? 1 : 0);
    usbStick.setButton (5, data & psxR1 ? 1 : 0);
    usbStick.setButton (13, data & psxL2 ? 1 : 0);
    usbStick.setButton (14, data & psxR2 ? 1 : 0);
    if (data & psxSlct || distance >= 5 && distance <= 50) {
        usbStick.setButton (8, 1);
    }
    else {
        usbStick.setButton (8, 0);
    }
    usbStick.setButton (9, data & psxStrt ? 1 : 0);
    usbStick.setButton (10, 0);
    usbStick.setButton (11, 0);
    
    usbStick.setButton (17, data & psxUp ? 1 : 0);
    usbStick.setButton (18, data & psxDown ? 1 : 0);
    usbStick.setButton (19, data & psxLeft ? 1 : 0);
    usbStick.setButton (20, data & psxRight ? 1 : 0);

    usbStick.setXAxis(ANALOG_IDLE_VALUE);
    usbStick.setYAxis(ANALOG_IDLE_VALUE);
    usbStick.setRxAxis(ANALOG_IDLE_VALUE);
    usbStick.setRyAxis(ANALOG_IDLE_VALUE);
    
    // All done, send data for real!
    usbStick.sendState ();
    
  }
} 
