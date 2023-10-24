//24/10/23 - 1:03pm
//TODO
//[] Score bar fills up
//[] User score is represented in green
//[] Slider doesnt fail user when enting main zone
//[x] zones with led and slider position match
//[] clear prompt to add step

#include <Button.h>
#include <Adafruit_NeoPixel.h>
//using pitches header from arduino toneMelody example
#include "pitches.h"


//LED Pins
int led1 = 3;
int led2 = 5;
int led3 = 6;
int led4 = 9;

int ledBrightness;

//Slide Potentiometer
int potPin = 5;
bool sliderInMotion = false;
int sliderMaxVal = 15;
int sliderPos;
int prevSliderPos;


int sliderArray[256];
int sliderPosCount;

//Buttons
int btn1Pin = 4;
int btn2Pin = 8;
int btn3Pin = 7;
int btn4Pin = 10;


Button btn1(btn1Pin);
Button btn2(btn2Pin);
Button btn3(btn3Pin);
Button btn4(btn4Pin);

//Neopixel LED Stick, 16 LEDs
int neopixelPin = 13;
int scorePixelPin = 12;
int pixelCount = 12;
//maximum value of analogue pin on uno is 1023 so we can divide by number of pixels
//to get specific LED number
int pixelSize = 1025 / (pixelCount - 2.5);
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(16, neopixelPin, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel scorePixel = Adafruit_NeoPixel(16, scorePixelPin, NEO_GRB + NEO_KHZ800);

int colourGreen = pixels.Color(0, 255, 0);
int colourRed = pixels.Color(200, 50, 50);

int colorPurple = pixels.Color(86, 10, 173);
int colorBlue = pixels.Color(8, 114, 207);
int colorPink = pixels.Color(212, 32, 191);
int colourWhite = pixels.Color(100, 100, 100);

int colourBlue;
int wipePos;
bool wipeFlag = false;
enum pixelSM { base,
               successLight,
               failLight,
               wipe };

pixelSM pixelState = base;


int buzzer = A4;
enum stripState { clear,
                  pass,
                  fail };

enum state { gameSetup,
             gameStart,
             firstStep,
             passGame,
             addStep,
             playSequence,
             success,
             noSuccess };
state smState = gameSetup;

void setup() {
  //Arcade Pushbuttons
  btn1.begin();
  btn2.begin();
  btn3.begin();
  btn4.begin();
  //Enable Pullups
  pinMode(btn1Pin, INPUT_PULLUP);
  pinMode(btn2Pin, INPUT_PULLUP);
  pinMode(btn3Pin, INPUT_PULLUP);
  pinMode(btn4Pin, INPUT_PULLUP);

  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  pinMode(led3, OUTPUT);
  pinMode(led4, OUTPUT);


  //Enable LED strip
  pixels.begin();
  pixels.setBrightness(30);
  scorePixel.setBrightness(100);
  //Enable Peizo Buzzer
  //Enable serial communication for debugging
  Serial.begin(9600);
  scorePixel.begin();

  prevSliderPos = 0;
}
//MEMORY GAME ZONE
//Variables to store previous steps
int stepArray[255];
//int stepArray[] = { 1, 2, 3, 4, 1, 2, 3, 4 };

int stepCount;
int currentStep;
int sequenceLength;
int sliderGoalPos;
int successLoopCounter;
int goalPos;


//THANK YOU FOR VISITING THE MEMORY GAME ZONE
//main loop

void loop() {
  //Serial.println(currentStep);
  setPixelsPos();

  if (smState == gameSetup) {
    ledBrightness = 255;
    stepCount = 0;
    sequenceLength = 0;
    currentStep = 0;
    //stepArray[0] = 1;
    Serial.println("setup");
    led(5, 255);
    smState = addStep;
  }

  if (smState == playSequence) {
    setPixelsPos();
    int saveButton = getBtn();
    int selectedAction;
    int selectedZone;
    if (getBtn() != 0){
      selectedAction = saveButton;
    } else if (getSliderPos() != 0) {
      while (getSliderPos() != 0) {
        sliderPeak();
        setPixelsPos();
      }
      selectedAction = 5;
      selectedZone = getSliderZone(sliderMaxVal);
      sliderMaxVal = 15;
    } 
    int sliderPositionStore = getSliderPos();

    if (currentStep == sequenceLength) {

      playSuccess();
      smState = success;

    } else if (selectedAction != 0 && selectedAction != 5) {

      if (stepArray[currentStep] != selectedAction) {
        Serial.println("fail");
        playFail();
        //smState = noSuccess;
      } else {
        btnPush(selectedAction);
        currentStep += 1;
      }

    } else if (selectedAction == 5) {

      if (sliderArray[currentStep] == selectedZone && stepArray[currentStep] == 5) {
        //smState = success;
        currentStep += 1;
      } else {
        //fail
        playFail();
      }
    }
  }

  if (smState == success) {
    pixelState = successLight;
    setPixelsPos();
    delay(1000);
    pixelState = base;
    currentStep = 0;
    smState = addStep;
  }

  if (smState == noSuccess) {
  }

  if (smState == addStep) {
    //Serial.println("addSte");
    int cBtn = getBtn();
    int cSlider = getSliderPos();
    int newStep;
    if (cBtn != 0) {
      newStep = cBtn;
      btnPush(newStep);
      stepArray[sequenceLength] = newStep;
      //Serial.println("step input is ");
      //Serial.println(newStep);
      sequenceLength += 1;
      smState = playSequence;
    } else if (cSlider != 0) {

      if (getSliderPos() != 0) {
        //block until slider is at apex (still refreshes pixels, maybe have buzzer tone?)
        while (getSliderPos() != 0) {
          setPixelsPos();
          sliderPeak();
        }
        //Serial.println("position locked in");
        newStep = getSliderZone(sliderMaxVal);
        sliderMaxVal = 15;
        stepArray[sequenceLength] = 5;
        sliderArray[sequenceLength] = newStep;
        Serial.println("this has now run");
        sequenceLength += 1;
        currentStep = 0;
        smState = passGame;
      }
    }
  }

  if (smState == passGame) {
    Serial.println("heyhey");
    smState = playSequence;
  }
}


//OUTPUT ZONE

void btnPush(int selectedAction) {
  led(selectedAction, 255);
  playTone(selectedAction);
  delay(300);
  noTone(buzzer);
  led(0, 255);
}

//When given a number of an led, it will be lit (via shift register)
void led(int ledNum, int brightness) {
  digitalWrite(led1, LOW);
  digitalWrite(led2, LOW);
  digitalWrite(led3, LOW);
  digitalWrite(led4, LOW);
  switch (ledNum) {
    case 0:
      break;
    case 1:
      analogWrite(led1, brightness);

      break;
    case 2:
      analogWrite(led2, brightness);
      break;
    case 3:
      analogWrite(led3, brightness);
      break;
    case 4:
      analogWrite(led4, brightness);
      break;
    case 5:
      digitalWrite(led1, HIGH);
      digitalWrite(led2, HIGH);
      digitalWrite(led3, HIGH);
      digitalWrite(led4, HIGH);
      break;
  }
}

void playTone(int frequency) {
  int duration = 100;
  switch (frequency) {
    case 1:
      tone(buzzer, NOTE_C4);
      break;
    case 2:
      tone(buzzer, NOTE_D4);
      break;
    case 3:
      tone(buzzer, NOTE_E4);
      break;
    case 4:
      tone(buzzer, NOTE_F4);
      break;
  }
}

void playFail() {
  tone(buzzer, NOTE_D1);
  pixelState = failLight;
  led(5, 255);
  clearPixels();
  pixels.fill(pixels.Color(200, 0, 0), 0, 16);
  scorePixel.fill(pixels.Color(200, 0, 0), 0, 16);
  showPixels();
  delay(400);
  noTone(buzzer);
  led(0, 255);
  pixelState = base;
  smState = gameSetup;
}

void playSuccess() {
  tone(buzzer, NOTE_G5);
  clearPixels();
  pixels.fill(colourGreen);
  scorePixel.fill(colourGreen);
  showPixels();
  delay(400);
  noTone(buzzer);
  smState = addStep;
}

//Neopixel Zone
void clearPixels() {
  pixels.clear();
  scorePixel.clear();
}
void showPixels() {
  pixels.show();
  scorePixel.show();
}

//Update pattern on LED stick
void setPixelsPos() {
  //score pixels
  scorePixel.clear();

  scorePixel.setPixelColor(currentStep, colourWhite);

  //dont show goal pixel until that "page" is present
  if (sequenceLength > 16) {
    if (currentStep > 16) {
    }
  } else {
    scorePixel.setPixelColor(sequenceLength, colourGreen);
  }
  scorePixel.show();



  int potValue = analogRead(potPin);
  int pixnum = floor(potValue / (pixelSize));
  //sliderpos = pixnum;

  if (pixnum != 13) {
    sliderInMotion = true;
    if (prevSliderPos > pixnum) {
      //Slider inflection point
    }
  }
  pixels.clear();
  updateSliderZone();
  //light led that represents the space the user is currently in
  pixels.setPixelColor(pixnum, pixels.Color(255, 255, 255));
  //display pixels
  pixels.show();




  //   case wipe:
  //     led(0);
  //     if (wipePos >= 20 && (wipePos < 220)) {
  //       digitalWrite(led1, HIGH);
  //     }
  //     if (wipePos >= 35 && (wipePos < 235)) {
  //       digitalWrite(led3, HIGH);
  //     }
  //     if (wipePos > 65 && (wipePos < 265)) {
  //       int pixelProgress = floor(((wipePos - 65) / 16));
  //       scorePixel.fill(colourGreen, 0, pixelProgress);
  //     }
  //     if (wipePos >= 100 && (wipePos < 300)) {
  //       digitalWrite(led2, HIGH);
  //     }
  //     if (wipePos >= 140 && (wipePos < 340)) {
  //       digitalWrite(led4, HIGH);
  //     }
  //     if (wipePos >= 210 && (wipePos < 410)) {
  //       digitalWrite(led4, HIGH);
  //       pixels.fill(colourGreen, 0, 16);
  //     }
  //     wipePos += 1;
  //     if (wipePos > 411) {
  //       wipePos = 0;
  //     }
  //     break;
  // }
}

//INPUT ZONE
//returns action inclusive and slider

int getSliderPos() {
  if (analogRead(5) < 25) {
    return 0;
  } else {
    return analogRead(5);
  }
}

void sliderPeak() {
  sliderPos = getSliderPos();
  if (sliderPos > sliderMaxVal) {
    Serial.println("new max");
    Serial.println(sliderMaxVal);
    sliderMaxVal = sliderPos;
  }
}

//zone 1 starts two pixels in
int z1Start = 2;
int z1End = z1Start + 4;
int z1StartPos = getPosFromPix(z1Start);
int z1EndPos = getPosFromPix(z1End);
int z2Start = z1End + 1;
int z2End = z2Start + 4;
int z2StartPos = getPosFromPix(z2Start);
int z2EndPos = getPosFromPix(z2End);

int getSliderZone(int sPos) {

  if (sPos > z1StartPos && sPos < z1EndPos) {
    Serial.println("zone 1");
    return 1;
  }
  if (sPos > z2StartPos && sPos < z2EndPos) {
    Serial.println("zone 2");
    return 2;
  }
}

//sets fill coords to as set above, doesnt use clear and show commands as it will be called in main pix block
void updateSliderZone() {
  pixels.fill(colourGreen, z1Start, z1End);
  pixels.fill(colourRed, z2Start, z2End);
}

//returns startpos * pixelsize
int getPosFromPix(int startPos) {
  return startPos * pixelSize;
}

//Returns value of button pressed, or 0 if no button has been pushed
int getBtn() {
  if (btn1.pressed()) {
    //Serial.println("button 1 pushed");
    return 1;
  } else if (btn2.pressed()) {
    //Serial.println("button 2 pushed");
    return 2;
  } else if (btn3.pressed()) {
    //Serial.println("button 3 pushed");
    return 3;
  } else if (btn4.pressed()) {
    //Serial.println("button 4 pushed");
    return 4;
  } else {
    return 0;
  }
}
