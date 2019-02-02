#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <SPI.h>
#include <SD.h>
#include <TouchScreen.h>
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

#define REST_START_BLOCK 4000000
#define NUM_RESTAURANTS 1066
// These constants are for the 2048 by 2048 map .
# define MAP_WIDTH 2048
# define MAP_HEIGHT 2048
lcd_image_t yegImage = { "yeg-big.lcd", MAP_WIDTH, MAP_HEIGHT };
# define LAT_NORTH 5361858l
# define LAT_SOUTH 5340953l
# define LON_WEST -11368652l
# define LON_EAST -11333496l

#define JOY_VERT  A1
#define JOY_HORIZ A0
#define JOY_SEL   2

#define JOY_CENTER   512
#define JOY_DEADZONE 64

#define NUM_NAMES_PER_PAGE 15;

#define CURSOR_SIZE 9
// smaller numbers yield faster cursor movement
#define JOY_SPEED 64

// the cursor position on the display, stored as the middle pixel of the cursor
int cursorX, cursorY;
int left, right;
int xposcursor = 0;
int yposcursor = 0;

// upper-left coordinates in the image of the middle of the map of Edmonton
#define YEG_MIDDLE_X (MAP_WIDTH/2 - MAP_DISP_WIDTH/2)
#define YEG_MIDDLE_Y (MAP_HEIGHT/2 - MAP_DISP_HEIGHT/2)

int mapx = 888;
int mapy = 904;

// different than SD
Sd2Card card;

struct restaurant {
  int32_t lat;
  int32_t lon;
  uint8_t rating; // from 0 to 10
  char name[55];
};
restaurant rest;
uint32_t lastBlockNum = REST_START_BLOCK-1;
restaurant restBlock[8];

struct RestDist {
  uint16_t index; //index of restaurant from 0 to NUM_RESTAURANTS - 1
  uint16_t dist; // manhatten distance to cursor position
};
RestDist rest_dist[NUM_RESTAURANTS];

int16_t position;

/*
int xVal;
int yVal;
int buttonVal;
int oldX;
int oldY;
*/

// forward declaration for drawing the cursor
void redrawCursor(int newX, int newY, int oldX, int oldY);

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

  tft.fillScreen(ILI9341_BLACK);
  tft.setRotation(3);

  // draws the centre of the Edmonton map, leaving the rightmost 48 columns black
  lcd_image_draw(&yegImage, &tft, YEG_MIDDLE_X, YEG_MIDDLE_Y,
    0, 0, MAP_DISP_WIDTH, MAP_DISP_HEIGHT);

    // initial cursor position is the middle of the screen
    cursorX = (DISPLAY_WIDTH - 48 - CURSOR_SIZE)/2;
    cursorY = (DISPLAY_HEIGHT - CURSOR_SIZE)/2;

    // draw the initial cursor
    redrawCursor(cursorX, cursorY, cursorX, cursorY);

    tft.setTextSize(1);
    tft.setTextWrap(false);// roll of the edge and not wrap around
    // if you put true it displays the last line over


    Serial.print("Initializing SPI communication for raw reads...");
    // SPI Speed can be SPI_FULL_SPEED, SPI_HALF_SPEED or SPI_QUARTER_SPEED.
    if (!card.init(SPI_HALF_SPEED, SD_CS)) {
      Serial.println("failed! Is the card inserted properly?");
      while (true) {}
    }
    else {
      Serial.println("OK!");
    }
  }

  void redrawCursor(int newX, int newY, int oldX, int oldY) {

    // draw the patch of edmonton over the old cursor
    lcd_image_draw(&yegImage, &tft,
      mapx + oldX, mapy + oldY,
      oldX, oldY, CURSOR_SIZE, CURSOR_SIZE);

      // and now draw the cursor at the new position
      tft.fillRect(newX, newY, CURSOR_SIZE, CURSOR_SIZE, ILI9341_RED);
    }

    void cursorlocation() {
      // right now the cursorx and y is counting from the edge of the
      // cursor, not the right edge though
      xposcursor = mapx + cursorX;
      yposcursor = mapy + cursorY;
      xposcursor = constrain(xposcursor, 0, MAP_WIDTH);
      yposcursor = constrain(yposcursor, 0, MAP_HEIGHT);
    }


    void screenupdate() {
      mapx = constrain(mapx, 0, MAP_WIDTH);
      mapy = constrain(mapy, 0, MAP_HEIGHT);
      if (cursorX == 0) {
        if (mapx - 272 >= 0) {
          mapx-=272;
          lcd_image_draw(&yegImage, &tft, mapx,
            mapy, 0, 0, MAP_DISP_WIDTH, MAP_DISP_HEIGHT);
            cursorX = 250;
          } else {
            mapx = 0;
          }
        }
        if (cursorX == 263) {
          if (mapx + 272 <= 1776) {
            mapx+=272;
            lcd_image_draw(&yegImage, &tft, mapx,
              mapy, 0, 0, MAP_DISP_WIDTH, MAP_DISP_HEIGHT);
              cursorX = 10;
            } else {
              mapx = 1776;
            }
          }
          if (cursorY == 0) {
            if (mapy - 240 >= 0) {
              mapy-=240;
              lcd_image_draw(&yegImage, &tft, mapx,
                mapy, 0, 0, MAP_DISP_WIDTH, MAP_DISP_HEIGHT);
                cursorY = 220;
              } else {
                mapy = 0;
              }
            }
            if (cursorY == 231) {
              if (mapy + 240 <= 1808) {
                mapy+=240;
                lcd_image_draw(&yegImage, &tft, mapx,
                  mapy, 0, 0, MAP_DISP_WIDTH, MAP_DISP_HEIGHT);
                  cursorY = 10;
                } else {
                  mapy = 1808;
                }
              }
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

                // Serial.println(cursorX);
                // Serial.println(cursorY);
                redrawCursor(cursorX, cursorY, oldX, oldY);
                // Serial.print("X coords: ");
                // Serial.println(xposcursor);
                // Serial.print("Y coords: ");
                // Serial.println(yposcursor);



                Serial.print("X map, cursor, location: ");
                Serial.print(mapx);
                Serial.print(", ");
                Serial.print(cursorX);
                Serial.print(", ");
                Serial.println(xposcursor);
                Serial.print("Y map, cursor, location: ");
                Serial.print(mapy);
                Serial.print(", ");
                Serial.print(cursorY);
                Serial.print(", ");
                Serial.println(yposcursor);
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


            // fast version self made which resuses block if already loaded
            void getRestaurantFast(int restIndex, restaurant* restPtr) {
              uint32_t lastBlockNum = REST_START_BLOCK-1;
              // obtain block number
              uint32_t blockNum = REST_START_BLOCK + restIndex/8;
              // if condition to see if we need a new block
              // if not needed we use the casched block saved in global variable above

              if (lastBlockNum != blockNum){
                lastBlockNum = blockNum;
                // overwrite the block
                while (!card.readBlock(blockNum, (uint8_t*) restBlock)) {
                  Serial.println("Read block failed, trying again.");
                }
              }

              *restPtr = restBlock[restIndex % 8];
            }


            int manhatten(int currentx, int restx, int currenty, int resty){
              //int manhatten(int restx, int resty){
              //int currentx = 2048; // comment out these two when you have postion from cursor
              //int currenty = 2048;
              int distance;
              distance = abs(currentx - restx) + abs(currenty - resty);
              return distance;
            }


            // These functions convert between x/y map position and lat /lon
            // (and vice versa .)

            int32_t x_to_lon(int16_t x) {
              return map(x , 0, MAP_WIDTH, LON_WEST, LON_EAST) ;
            }
            int32_t y_to_lat(int16_t y) {
              return map(y , 0, MAP_HEIGHT, LAT_NORTH, LAT_SOUTH) ;
            }
            int16_t lon_to_x (int32_t lon) {
              return map( lon, LON_WEST, LON_EAST, 0, MAP_WIDTH) ;
            }
            int16_t lat_to_y (int32_t lat) {
              return map( lat, LAT_NORTH, LAT_SOUTH, 0, MAP_HEIGHT) ;
            }


            // swap function similar to eclass quicksort.cpp that swaps two inputs
            void swap(RestDist& dist,RestDist& dist2){
              RestDist temp = dist;
              dist = dist2;
              dist2 = temp;
            }

            // working i sort
            void isort(RestDist dist[],int len){
              int i;
              int j;
              i = 1;
              while (i < len){
                j = i;
                // need .dist to compare just the distances
                while (( j>0 ) && (dist[j-1].dist > dist[j].dist)){
                  // but swap the whole struct to keep index lined up
                  swap(dist[j],dist[j-1]);
                  j = j-1;
                }
                i = i+1;
              }
            }


            // first 15
            void drawName(uint16_t selectedRest){
              //selectedRest = 0; // which restaurant is selected ?
              tft.setCursor(0,0);
              //tft.setTextColor(ILI9341_BLACK, ILI9341_BLACK);
              tft.setTextSize(1);
              for ( int16_t i = 0; i < 30; i ++) {
                getRestaurantFast(rest_dist[i].index , &rest) ;
                if(i == selectedRest){ // highlight
                  // white characters on black background
                  tft.setTextColor(ILI9341_BLACK, ILI9341_WHITE);
                }
                else { // not highlighted
                  // black characters on white background
                  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
                }
                tft.println(rest.name);
              }
            }


            int main() {
              setup();

              restaurant rest;
              int lt, ln;
              int restaurantCounter;

              Serial.println(mapx);
              Serial.println(mapy);
              while (true){
                cursorlocation();
                processJoystick();
                int checkButton = digitalRead(JOY_SEL);
                if (checkButton == 0){// button pushed

                  for (int i=0; i < 1067;i++){
                    getRestaurantFast(i, &rest); // first value tells you what restaurant that number is and allows you to look for it directly
                    // what you do is find the manhatten dist of closest one
                    ln = lon_to_x(rest.lon) ;
                    lt = lat_to_y(rest.lat);
                    rest_dist[i].dist = manhatten(xposcursor,ln,yposcursor,lt);
                    rest_dist[i].index = i;
                  }
                  isort(rest_dist,NUM_RESTAURANTS);

                  for (int i=0; i < 30;i++){
                    Serial.print(" this is index: ");
                    Serial.print(rest_dist[i].index);
                    Serial.print("    this is rest_dist: ");
                    Serial.println(rest_dist[i].dist);
                  }


                  for (int i=0; i < 30;i++){
                    getRestaurantFast(rest_dist[i].index, &rest);
                    Serial.print("this is index: ");
                    Serial.print(rest_dist[i].index);
                    Serial.print("  ");
                    Serial.print(i);
                    Serial.print(" ");
                    Serial.println(rest.name);
                  }

                  tft.fillScreen(ILI9341_BLACK);// draw the screen all black first
                  position = 0;// always start with first restaurant
                  drawName(position);
                  while(true){
                    // if statement for if its on the 16- blank names go to a different function that draws the next few names
                    // reread the joystick everytime to check if a valid tilt is inputed
                    int yVal = analogRead(JOY_VERT);
                    int buttonVal = digitalRead(JOY_SEL);
                    if (yVal >= (JOY_CENTER + JOY_DEADZONE)){ // this is to move down
                      position++;
                      if (position > 29){
                        position = 0;
                      }
                      drawName(position);
                    } else if (yVal <= (JOY_CENTER - JOY_DEADZONE) ){ // this is move up
                      position--;
                      if (position < 0){
                        position = 29;
                      }
                      drawName(position);
                    }
                    delay (50);
                  }
                }
              }

              Serial.end();

              return 0;
            }
