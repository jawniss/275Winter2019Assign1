/*
Demonstrating cursor movement over the map of Edmonton. You will improve over
what we do in the next weekly exercise.
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

#define DISPLAY_WIDTH  320
#define DISPLAY_HEIGHT 240
#define YEG_SIZE 2048

lcd_image_t yegImage = { "yeg-big.lcd", YEG_SIZE, YEG_SIZE };

#define JOY_VERT  A1  // should connect A1 to pin VRx
#define JOY_HORIZ A0  // should connect A0 to pin VRy
#define JOY_SEL   2

#define JOY_CENTER   512
#define JOY_DEADZONE 64

#define CURSOR_SIZE 9

// the cursor position on the display
int cursorX, cursorY;
int yegMiddleX = YEG_SIZE/2 - (DISPLAY_WIDTH - 48)/2;
int yegMiddleY = YEG_SIZE/2 - DISPLAY_HEIGHT/2;
int turning;

// forward declaration for redrawing the cursor
void redrawCursor(uint16_t colour);

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

  tft.setRotation(3);

  tft.fillScreen(ILI9341_BLACK);

  // draws the centre of the Edmonton map, leaving the rightmost 48 columns
  // black
  int yegMiddleX = YEG_SIZE/2 - (DISPLAY_WIDTH - 48)/2;
  int yegMiddleY = YEG_SIZE/2 - DISPLAY_HEIGHT/2;
  lcd_image_draw(&yegImage, &tft, yegMiddleX, yegMiddleY,
    0, 0, DISPLAY_WIDTH - 48, DISPLAY_HEIGHT);

    // initial cursor position is the middle of the screen
    cursorX = (DISPLAY_WIDTH - 48)/2;
    cursorY = DISPLAY_HEIGHT/2;
    redrawCursor(ILI9341_RED);
  }


  void redrawCursor(uint16_t colour) {
    tft.fillRect(cursorX - CURSOR_SIZE/2, cursorY - CURSOR_SIZE/2,
      CURSOR_SIZE, CURSOR_SIZE, colour);
    }


    void processJoystick() {
      int xVal = analogRead(JOY_HORIZ);
      int yVal = analogRead(JOY_VERT);
      int buttonVal = digitalRead(JOY_SEL);
      // If the joystick is stationary, do not do anything
      if (455 < yVal && 555 > yVal && xVal < 567 && 467 < xVal) {
        return;
      }
      // Redraw the portion of the background map that was drawn over in the
      // previous frame by the cursor
      lcd_image_draw(&yegImage, &tft, yegMiddleX + cursorX - 4,
        yegMiddleY + cursorY - 4, cursorX - 4, cursorY - 4,
        CURSOR_SIZE, CURSOR_SIZE);


        // Moving the cursor:
        // Check if the cursor is within the edge boundaries, if so all joystick
        // directions are allowed
        if (cursorY > 4 && cursorY < 236) {
          // First and slowest cursor speed
          if (300 < yVal && yVal < 448) {
            cursorY -= 1;  // decrease the y coordinate of the cursor
          } else if (800 > yVal && yVal > 576) {
            cursorY += 1;
          }
          // Second and medium cursor speed
          if (0 < yVal && yVal < 300) {
            // If for the 4-pixel speed the cursor is closer than 4 pixels to
            // the edge, set cursor position to the edge to prevent exitting
            // boundaries, I did the same for the 10-pixel speed
            if (cursorY <= 8) {
              cursorY = 4;
            } else {
              cursorY -= 4;
            }
          } else if (1000 > yVal && yVal > 800) {
            if (cursorY >= 232) {
              cursorY = 236;
            } else {
              cursorY += 4;
            }
          }
          // Third and fastest cursor speed
          if (0 == yVal) {
            if (cursorY <= 14) {
              cursorY = 4;
            } else {
              cursorY -= 10;
            }
          } else if (yVal >= 1000) {
            if (cursorY >= 226) {
              // Note: I set the cursor location to 236 instead of 240 as at 240
              // half of the cursor is off screen. Setting to 236 has the whole
              // cursor showing on screen. I apply the same thinking to the
              // other boundary edges
              cursorY = 236;
            } else {
              cursorY += 10;
            }
          }
        } else if (cursorY == 4) {
          if (800 > yVal && yVal > 576) {
            cursorY += 1;
          } else if (1000 > yVal && yVal > 800) {
            if (cursorY >= 232) {
              cursorY = 236;
            } else {
              cursorY += 4;
            }
          } else if (yVal >= 1000) {
            if (cursorY >= 226) {
              cursorY = 236;
            } else {
              cursorY += 10;
            }
          }
        } else if (cursorY == 236) {
          if (300 < yVal && yVal < 448) {
            cursorY -= 1;
          }
          if (0 < yVal && yVal < 300) {
            if (cursorY <= 8) {
              cursorY = 4;
            } else {
              cursorY -= 4;
            }
          }
          if (0 == yVal) {
            if (cursorY <= 14) {
              cursorY = 4;
            } else {
              cursorY -= 10;
            }
          }
        }
        // Same proccess as above for cursorY and yVal, but below is for
        // cursorX and xVal
        if (cursorX > 4 && cursorX < 267) {
          if (800 > xVal && xVal > 576) {
            cursorX -= 1;
          } else if (300 < xVal && xVal < 448) {
            cursorX += 1;
          }
          if (1000 > xVal && xVal > 800) {
            if (cursorX <= 8) {
              cursorX = 4;
            } else {
              cursorX -= 4;
            }
          } else if (0 < xVal && xVal < 300) {
            if (cursorX >= 264) {
              cursorX = 267;
            } else {
              cursorX += 4;
            }
          }
          if (0 == xVal) {
            if (cursorX >= 258) {
              cursorX = 267;
            } else {
              cursorX += 10;
            }
          } else if (xVal >= 1000) {
            if (cursorX <= 14) {
              cursorX = 4;
            } else {
              cursorX -= 10;
            }
          }
        } else if (cursorX == 4) {
          if (300 < xVal && xVal < 448) {
            cursorX += 1;
          } else if (0 < xVal && xVal < 300) {
            if (cursorX >= 264) {
              cursorX = 267;
            } else {
              cursorX += 4;
            }
          }
          if (0 == xVal) {
            if (cursorX >= 258) {
              cursorX = 267;
            } else {
              cursorX += 10;
            }
          }
        } else if (cursorX == 267) {
          if (800 > xVal && xVal > 576) {
            cursorX -= 1;
          }
          if (1000 > xVal && xVal > 800) {
            if (cursorX <= 8) {
              cursorX = 4;
            } else {
              cursorX -= 4;
            }
          } else if (xVal >= 1000) {
            if (cursorX <= 14) {
              cursorX = 4;
            } else {
              cursorX -= 10;
            }
          }
        }
        redrawCursor(ILI9341_RED);
        delay(20);
      }



      int main() {
        setup();
        while (true) {
          processJoystick();
        }
        Serial.end();
        return 0;
      }
