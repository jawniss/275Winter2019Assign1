/*
# ----------------------------------------------
#   Name: Ricky Au
#   ID: 1529429
#   CMPUT 275, Winter 2018
#
#   Weekly Exercise 1 : Display and Joystick
# ----------------------------------------------
*/

// include header files
#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_ILI9341.h>
#include "lcd_image.h"

#define TFT_DC 9
#define TFT_CS 10
#define SD_CS 6

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

#define DISPLAY_WIDTH  320 // width of display
#define DISPLAY_HEIGHT 240 // height of display
#define YEG_SIZE 2048

lcd_image_t yegImage = { "yeg-big.lcd", YEG_SIZE, YEG_SIZE };

#define JOY_VERT  A1 // should connect A1 to pin VRx
#define JOY_HORIZ A0 // should connect A0 to pin VRy
#define JOY_SEL   2

#define JOY_CENTER   512 // roughly what the value of joysticks center
#define JOY_DEADZONE 64 // default deadzone for slow speed of travel
#define JOY_MEDIUM_TILTZONE 224 // medium tilt for medium speed of travel
#define JOY_HIGH_TILTZONE 444 // high tilt for max speed of travel

#define CURSOR_SIZE 9 // size of cursor

// the cursor position on the display
int cursorX, cursorY;

// forward declaration for redrawing the cursor
void redrawCursor(uint16_t colour);

// setups for before you can move
void setup() {
  init();

  Serial.begin(9600);

	pinMode(JOY_SEL, INPUT_PULLUP);

	tft.begin();

	Serial.print("Initializing SD card...");
	if (!SD.begin(SD_CS)) {
		Serial.println("failed! Is it inserted properly?");
		while (true) {}
	}
	Serial.println("OK!");

  // setrotation to 3 for a wider display
	tft.setRotation(3);

  // fill background to be black
  tft.fillScreen(ILI9341_BLACK);

  // draws the centre of the Edmonton map, leaving the rightmost 48 columns black
	int yegMiddleX = YEG_SIZE/2 - (DISPLAY_WIDTH - 48)/2;
	int yegMiddleY = YEG_SIZE/2 - DISPLAY_HEIGHT/2;
	lcd_image_draw(&yegImage, &tft, yegMiddleX, yegMiddleY,
                 0, 0, DISPLAY_WIDTH - 48, DISPLAY_HEIGHT);

  // initial cursor position is the middle of the screen
  cursorX = (DISPLAY_WIDTH - 48)/2;
  cursorY = DISPLAY_HEIGHT/2;

  // draw the initial cursor in the middle
  redrawCursor(ILI9341_RED);
}

// function that is called when ever a new cursor needs to be drawn
void redrawCursor(uint16_t colour) {
  // constrains to not allow cursor to move outside mapped region on the screen
  cursorX = constrain(cursorX,4,320-53);
  cursorY = constrain(cursorY,4,235);
  tft.fillRect(cursorX - CURSOR_SIZE/2, cursorY - CURSOR_SIZE/2,
               CURSOR_SIZE, CURSOR_SIZE, colour);
}

// function that will handle all movement of the joystick and use its inputs
void processJoystick() {

  int xVal = analogRead(JOY_HORIZ);
  int yVal = analogRead(JOY_VERT);
  int buttonVal = digitalRead(JOY_SEL);

  int yegMiddleX = YEG_SIZE/2 - (DISPLAY_WIDTH - 48)/2;
  int yegMiddleY = YEG_SIZE/2 - DISPLAY_HEIGHT/2;

  // loop that stops the code from passing through unless a valid tilt is read
  // from the joystick
  while (true){
    // reread the joystick everytime to check if a valid tilt is inputed
    int xVal = analogRead(JOY_HORIZ);
    int yVal = analogRead(JOY_VERT);
    int buttonVal = digitalRead(JOY_SEL);
    // valid tilt is when x or y value read from joystick is greater then center location add deadzone or smaller then center location minus deadzone
    if (((xVal < (JOY_CENTER - JOY_DEADZONE)) || (xVal > (JOY_CENTER + JOY_DEADZONE)) || (yVal < (JOY_CENTER - JOY_DEADZONE)) || (yVal > (JOY_CENTER + JOY_DEADZONE)))){
      break;
    }
  }

  // redraws only a small part of the map where the cursor was previously
  lcd_image_draw(&yegImage, &tft, yegMiddleX + cursorX - 4, yegMiddleY + cursorY - 4,
                 cursorX - 4, cursorY - 4, CURSOR_SIZE, CURSOR_SIZE);

  // list the high tilt settings first so they are checked first
  // if condition is any of these 4 conditions are met the movement will be 5 pixels from previous location
  if (yVal < JOY_CENTER - JOY_HIGH_TILTZONE) {
    cursorY -= 5;
  }
  else if (yVal > JOY_CENTER + JOY_HIGH_TILTZONE) {
    cursorY += 5;
  }
  if (xVal > JOY_CENTER + JOY_HIGH_TILTZONE) {
    cursorX -= 5;
  }
  else if (xVal < JOY_CENTER - JOY_HIGH_TILTZONE) {
    cursorX += 5;
  }
  // list the medium tilt settings second so they are checked if tilt is lower then high tilt setting but higher then slow speed
  // if condition is any of these 4 conditions are met the movement will be 3 pixels from previous location
  if (yVal < JOY_CENTER - JOY_MEDIUM_TILTZONE) {
    cursorY -= 3;
  }
  else if (yVal > JOY_CENTER + JOY_MEDIUM_TILTZONE) {
    cursorY += 3;
  }
  if (xVal > JOY_CENTER + JOY_MEDIUM_TILTZONE) {
    cursorX -= 3;
  }
  else if (xVal < JOY_CENTER - JOY_MEDIUM_TILTZONE) {
    cursorX += 3;
  }
  // list of slow speed tilt settings
  // if condition is any of these 4 conditions are met the movement will be 1 pixel from previous location
  if (yVal < JOY_CENTER - JOY_DEADZONE) {
    cursorY -= 1;
  }
  else if (yVal > JOY_CENTER + JOY_DEADZONE) {
    cursorY += 1;
  }
  if (xVal > JOY_CENTER + JOY_DEADZONE) {
    cursorX -= 1;
  }
  else if (xVal < JOY_CENTER - JOY_DEADZONE) {
    cursorX += 1;
  }

  // draw the new cursor at moved location
  redrawCursor(ILI9341_RED);
  delay(20);
}

int main() {
	setup();
  // while loop to allow never ending movement
  while (true) {
    processJoystick();
  }

	Serial.end();
	return 0;
}
