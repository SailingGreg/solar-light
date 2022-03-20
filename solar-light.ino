/*
 * solar-light sketch 
 * 
 */

#include <Arduino.h>
#if defined (PICO)
  #include <stdio.h>
  #include "pico/stdlib.h"
#endif

#include "hardware/adc.h" // native sdk
#include <Adafruit_NeoPixel.h> 

#define PIN 6
#define NUMPIXELS 7

const bool On = true;
const bool Off = false;
const int buttonPin = 2;
bool buttonFlag = false;
int buttonDebounce = 200; // found to be 120-150 for the simple buttons
const int pirPin = 22;
bool pirFlag = false;
int unsigned pirChange = 0; // time of previous press
int lightMode = 1; // this can be 1, 2 or 3
int previousMode = 0;

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_RGBW + NEO_KHZ800);

void setup() {
  
  // put your setup code here, to run once:
  Serial.begin(115200); // do we need to wait for this to come up?
  // the following is used for the reflecting the 'state'
  pinMode(LED_BUILTIN, OUTPUT);

  // adc setup - using the Pico SDK directly
  adc_init(); // init the hardware
  adc_gpio_init(28); // disable digital funciton son ping 28 or ADC2
  adc_select_input(2); // 2 for ADC2!
  
  // initialise and turn leds off?
  pixels.begin();
  pixels.show(); // all pixels to 'off'

  // setup interrupt for button
  pinMode(buttonPin, INPUT_PULLUP);
  attachInterrupt(buttonPin, buttonInterrupt, FALLING);
  //attachInterrupt(buttonPin, buttonInterrupt, CHANGE);
  gpio_pull_up(buttonPin); // this is need to ensure the line is not floating!!
  
  // setup the PIR for low on activation
  pinMode(pirPin, INPUT_PULLUP);
  attachInterrupt(pirPin, pirInterrupt, RISING);
  gpio_pull_up(pirPin);

  //Serial.println("Starting script");
}

void loop() {
  // put your main code here, to run repeatedly:
  static unsigned int loopCnt = 0;
  static unsigned int prevChange = 0; // for illumiance
  static int lightVal;
  static int pirVal;
  static bool lightState = Off;
  static int luxCutoff = 250;  // the minimum for lights on
  static bool lightOn = false;
  static bool pixelsOn = false; // used to indicate led status

  float cfactor = (3.1 / (1 << 12)); // allow for 0.2 drop for the phototransisor
  
  lightVal = adc_read(); // check each loop
  pirVal = digitalRead(pirPin); // read to help with tracing but triggered interrupt

  // reflect status for debug and calibration
  Serial.print("Running loop: ");
  Serial.print(loopCnt);
  Serial.print(", lMode ");
  Serial.print(lightMode);
  Serial.print(", pVal ");
  Serial.print(pirVal);
  Serial.print(", pFlag ");
  Serial.print(pirFlag);
  Serial.print(", ");
  Serial.print(lightVal);
  Serial.print(", ");
  Serial.print(cfactor * 1000);
  Serial.print(", ");
  Serial.println(lightVal * cfactor);
  loopCnt = loopCnt + 1;

  // if mode changed then flag
  if (lightMode != previousMode) {
    previousMode = lightMode; // note now

    //Serial.println("buttonFlag true ..");
    // flash onboard led to reflect the new mode
    for (int i = lightMode; i > 0; i--) {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(250);
      digitalWrite(LED_BUILTIN, LOW);
      delay(250);
    }
  }

  // check light levels and flag on if below threshold
  // move to a moving average to take fluactions out of the reading
  if (lightVal < luxCutoff) { // guard for no light
    lightState = On;
  } else {
    // check that we have an interval
    if ((lightVal > (luxCutoff + 200)) && ((prevChange - millis()) > 10000)) {
      lightState = Off;
      prevChange = millis();
    }
  }

  // and if pir event flagged time to reset?
  if ( pirFlag == true ) {
    // has it been x seconds?
    if ((millis() - pirChange) > 10000) { // 10 seconds for now
      pirFlag = false; // turn off
      //pirChange = millis();
    }
  }

  // light control centralised in the loop & based on mode plus state
  // if mode 1 then turnon only if dark, lightState == On
  if (lightMode == 1 && lightState == On) {
    lightOn = true;
  // if mode 2 then turn on if motion (pirFlag == true
  } else if (lightMode == 2 && pirFlag == true) {
    lightOn = true;
  // mode 3 for is for either
  } else if (lightMode == 3 && (lightState == On || pirFlag == true)){
    lightOn = true;
  } else {
    lightOn = false;
  }
  
  if (lightOn == true) {
    if (pixelsOn == false) { // turn on if of
      pixels.fill(pixels.Color(32, 32, 32, 32), 0); // fill white
      //pixels.fill(pixels.Color(255, 255, 255, 255), 0); // fill white
      pixels.show();
      delay(500);
      pixelsOn = true;
    }
  } else {
    pixels.clear();
    pixels.show();
    pixelsOn = false;
  }

  delay (250); // just slow the loop for now
}

// we simply set the flag to be true
// and note the time in msec
void pirInterrupt()
{
  pirFlag = true;
  pirChange = millis();  
}

// button pressed - debounce
void buttonInterrupt()
{
  static unsigned int previousPress = 0;  // retain the last press

  //Serial.println("Press debounced"); // <- doing this screws up when using mbed/pico

  // the buttonDebounce should be suffixcient but buttonFlag added as an extra guard
  if((millis() - previousPress) > buttonDebounce ) {
    //if (Debug == true) Serial.println("Press debounced");
       
    previousPress = millis();
    buttonFlag = !buttonFlag; // just toggle it

    // change state
    lightMode = lightMode + 1;
    if (lightMode > 3) { // upper is 3 for now
      lightMode = 1;
    }
  }
}

// end of sketch
