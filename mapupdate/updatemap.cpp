/*
	One possible solution for exercise 1. You may use it freely
  in assignment 1 (just mention you are using the provided solution
  for exercise 1).

  Your solution for assignment 1 does not need to exactly duplicate
  this cursor movement speed. As long as it is natural and varies
  with how far the joystick is pushed.
*/

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_ILI9341.h>
#include "lcd_image.h"

#define TFT_DC 9
#define TFT_CS 10
#define SD_CS 6

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// physical dimensions of the tft display (# of pixels)
#define DISPLAY_WIDTH  320
#define DISPLAY_HEIGHT 240

// dimensions of the part allocated to the map display
#define MAP_DISP_WIDTH (DISPLAY_WIDTH - 48)
#define MAP_DISP_HEIGHT DISPLAY_HEIGHT

// width and height (in pixels) of the LCD image
#define LCD_WIDTH 2048
#define LCD_HEIGHT 2048
lcd_image_t yegImage = { "yeg-big.lcd", LCD_WIDTH, LCD_HEIGHT };

#define JOY_VERT  A1 // should connect A1 to pin VRx
#define JOY_HORIZ A0 // should connect A0 to pin VRy
#define JOY_SEL   2

#define JOY_CENTER   512
#define JOY_DEADZONE 64

#define CURSOR_SIZE 9

// smaller numbers yield faster cursor movement
#define JOY_SPEED 64

// the cursor position on the display, stored as the middle pixel of the cursor
int cursorX, cursorY;
int left, right;

// upper-left coordinates in the image of the middle of the map of Edmonton
#define YEG_MIDDLE_X (LCD_WIDTH/2 - MAP_DISP_WIDTH/2)
#define YEG_MIDDLE_Y (LCD_HEIGHT/2 - MAP_DISP_HEIGHT/2)


int mapx = 888;
int mapy = 904;

// forward declaration for drawing the cursor
void redrawCursor(int newX, int newY, int oldX, int oldY);

void setup() {
  init();

  Serial.begin(9600);

  // not actually used in this exercise, but it's ok to leave it
	pinMode(JOY_SEL, INPUT_PULLUP);

	tft.begin();

	Serial.print("Initializing SD card...");
	if (!SD.begin(SD_CS)) {
		Serial.println("failed! Is it inserted properly?");
		while (true) {}
	}
	Serial.println("OK!");

	tft.setRotation(3);

  tft.fillScreen(ILI9341_BLACK);

  // draws the centre of the Edmonton map, leaving the rightmost 48 columns black
	lcd_image_draw(&yegImage, &tft, YEG_MIDDLE_X, YEG_MIDDLE_Y,
                 0, 0, MAP_DISP_WIDTH, MAP_DISP_HEIGHT);

  // initial cursor position is the middle of the screen
  cursorX = (DISPLAY_WIDTH - 48 - CURSOR_SIZE)/2;
  cursorY = (DISPLAY_HEIGHT - CURSOR_SIZE)/2;

  // draw the initial cursor
  redrawCursor(cursorX, cursorY, cursorX, cursorY);
}

// redraws the patch of edmonton over the older cursor position
// given by "oldX, oldY" and draws the cursor at its new position
// given by "newX, newY"
void redrawCursor(int newX, int newY, int oldX, int oldY) {

  // draw the patch of edmonton over the old cursor
  lcd_image_draw(&yegImage, &tft,
                 mapx + oldX, mapy + oldY,
                 oldX, oldY, CURSOR_SIZE, CURSOR_SIZE);

  // and now draw the cursor at the new position
  tft.fillRect(newX, newY, CURSOR_SIZE, CURSOR_SIZE, ILI9341_RED);
}







void screenupdate() {
  if (cursorX == 0) {
    mapx-=50;
    lcd_image_draw(&yegImage, &tft, mapx,
    mapy, 0, 0, MAP_DISP_WIDTH, MAP_DISP_HEIGHT);

    cursorX = 250;
  }
  if (cursorX == 263) {
    mapx+=50;
    lcd_image_draw(&yegImage, &tft, mapx,
    mapy, 0, 0, MAP_DISP_WIDTH, MAP_DISP_HEIGHT);

    cursorX = 10;
  }
  if (cursorY == 0) {
    mapy-=50;
    lcd_image_draw(&yegImage, &tft, mapx,
    mapy, 0, 0, MAP_DISP_WIDTH, MAP_DISP_HEIGHT);

    cursorY = 220;
  }
  if (cursorY == 231) {
    mapy+=50;
    lcd_image_draw(&yegImage, &tft, mapx,
    mapy, 0, 0, MAP_DISP_WIDTH, MAP_DISP_HEIGHT);

    cursorY = 10;
  }
  // FIND THE CUNTION WHERE IT REDRAWS THE LITTLE PART OF THE MAP AND DO
  // RIGHT*50 THERE

}




void processJoystick() {
  int xVal = analogRead(JOY_HORIZ);
  int yVal = analogRead(JOY_VERT);
  int buttonVal = digitalRead(JOY_SEL);

  // copy the cursor position (to check later if it changed)
  int oldX = cursorX;
  int oldY = cursorY;

  // move the cursor, further pushes mean faster movement
  cursorX += (JOY_CENTER - xVal) / JOY_SPEED;
  cursorY += (yVal - JOY_CENTER) / JOY_SPEED;

  // constrain so the cursor does not go off of the map display window
  cursorX = constrain(cursorX, 0, MAP_DISP_WIDTH - CURSOR_SIZE);
  cursorY = constrain(cursorY, 0, MAP_DISP_HEIGHT - CURSOR_SIZE);

  screenupdate();
  // redraw the cursor only if its position actually changed
  if (cursorX != oldX || cursorY != oldY) {

    Serial.println(cursorX);
    Serial.println(cursorY);
    redrawCursor(cursorX, cursorY, oldX, oldY);
  }

  delay(10);
}


// void screenupdate () {
//   if the cursor goes up
//     add 1 to something like int up
//     update the screen using 240 x 320 or whatever the dimensions are of the
//     next section of the map
//     do like up * 240 (coordinates) of the yeg map so for example if up = 8,
//     that means it's the 8th time going up, redraw the map 8 squares up
//     down is up--
// }



int main() {
	setup();
  Serial.println(mapx);
  Serial.println(mapy);
  while (true) {
    processJoystick();
  }

	Serial.end();
	return 0;
}
