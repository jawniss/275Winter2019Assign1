/*
# ----------------------------------------------
#   Name: Ricky Au, Johnas Wong
#   ID: 1529429, 1529241
#   CMPUT 275, Winter 2019
#
#   Assignment 1 part 2: Full version of Restaurant Finder
# ----------------------------------------------
*/

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
int xposcursor = 0;
int yposcursor = 0;

// upper-left coordinates in the image of the middle of the map of Edmonton
#define YEG_MIDDLE_X (MAP_WIDTH/2 - MAP_DISP_WIDTH/2)
#define YEG_MIDDLE_Y (MAP_HEIGHT/2 - MAP_DISP_HEIGHT/2)

// The coordinates of the top left of the screen display
int mapx = 888;
int mapy = 904;

// Global variable for rating value and number of restaurants above the rating
// The intial minimum restaurant rating is 1
int rating = 1;

// Pin definings from the lecture code for screen touching
#define YP A2  // must be an analog pin, use "An" notation!
#define XM A3  // must be an analog pin, use "An" notation!
#define YM  5  // can be a digital pin
#define XP  4  // can be a digital pin

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

#define MINPRESSURE   10
#define MAXPRESSURE 1000

// different than SD
Sd2Card card;

struct restaurant {
  int32_t lat;
  int32_t lon;
  uint8_t rating;  // from 0 to 10
  char name[55];
};
restaurant rest;
uint32_t lastBlockNum = REST_START_BLOCK-1;
restaurant restBlock[8];

struct RestDist {
  uint16_t index;  // index of restaurant from 0 to filterNum - 1
  uint16_t dist;  // manhatten distance to cursor position
};
RestDist rest_dist[NUM_RESTAURANTS]; // might need to change

int16_t position;
int16_t previousPosition;
int16_t newScreenChecker;
int16_t newScreen;
// global variable to know which index of restaurant selected
int currentRest = 0;
int longitude;
int latitude;
int mode = 0;
// drawing the list for first time
int drawMode = 0;
int lt, ln;
// changes the number of restaurants when filtered
int filterNum = NUM_RESTAURANTS;
// boolean for based on what sort to use
// set to isort initially
bool iSortVal = true;
bool qSortVal = false;
bool both = false;

// forward declaration for drawing the cursor
void redrawCursor(int newX, int newY, int oldX, int oldY);

// initialization
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

  // draws the centre of the Edmonton map, leaving the rightmost 48 columns
  // black
  lcd_image_draw(&yegImage, &tft, YEG_MIDDLE_X, YEG_MIDDLE_Y,
    0, 0, MAP_DISP_WIDTH, MAP_DISP_HEIGHT);

    // initial cursor position is the middle of the screen
    cursorX = (DISPLAY_WIDTH - 48 - CURSOR_SIZE)/2;
    cursorY = (DISPLAY_HEIGHT - CURSOR_SIZE)/2;

    // draw the initial cursor
    redrawCursor(cursorX, cursorY, cursorX, cursorY);

    // initial draw when the program is run
    tft.fillRect(272,121,48,119, ILI9341_WHITE);
    tft.drawRect(272,121,48,119, ILI9341_GREEN);
    tft.fillRect(272,0,48,119, ILI9341_WHITE);
    tft.drawRect(272,0,48,119, ILI9341_RED);
    // Top button
    tft.setTextColor(ILI9341_BLACK, ILI9341_WHITE);
    tft.setCursor(293, 52);
    tft.setTextSize(2);
    tft.print(rating);
    // isort written vertically
    tft.setTextColor(ILI9341_BLACK, ILI9341_WHITE);
    tft.setCursor(293,141);
    tft.setTextSize(2);
    tft.print("I");
    tft.setCursor(293,158);
    tft.setTextSize(2);
    tft.print("S");
    tft.setCursor(293,175);
    tft.setTextSize(2);
    tft.print("O");
    tft.setCursor(293,192);
    tft.setTextSize(2);
    tft.print("R");
    tft.setCursor(293,209);
    tft.setTextSize(2);
    tft.print("T");

    tft.setTextSize(1);
    tft.setTextWrap(false);  // roll of the edge and not wrap around
    // if you put true it displays the last line over

    Serial.print("Initializing SPI communication for raw reads...");
    // SPI Speed can be SPI_FULL_SPEED, SPI_HALF_SPEED or SPI_QUARTER_SPEED.
    if (!card.init(SPI_HALF_SPEED, SD_CS)) {
      Serial.println("failed! Is the card inserted properly?");
      while (true) {}
    } else {
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

// If the cursor reaches an edge of the screen, update the screen to show the
// next respective section of the map
void screenupdate() {
  // Initially there is no reason to refresh the screen
  bool refreshscreen = false;
  mapx = constrain(mapx, 0, 1776);
  mapy = constrain(mapy, 0, 1808);
  // If the screen is already at an edge of the map, no need to
  // refresh
  if (mapx == 0 || mapx == 1776 || mapy == 0 || mapy == 1808) {
    refreshscreen = false;
  }
  // If the cursor touches the left side, draw the next left patch of map
  if (cursorX == 0) {
    if (mapx - 272 >= 0) {
      mapx-=272;
      cursorX = 262;
      refreshscreen = true;
    } else if (mapx != 0){
      mapx = 0;
      cursorX = 262;
      refreshscreen = true;
    }
  }
  // If the cursor touches the right, draw the next right patch of map
  if (cursorX == 263) {
    if (mapx + 272 <= 1776) {
      mapx+=272;
      cursorX = 1;
      refreshscreen = true;
    } else if (mapx != 1776){
      mapx = 1776;
      cursorX = 1;
      refreshscreen = true;
    }
  }
  // If the cursor touches the top of the screen relative to the rotation
  // orientation, move a screen up
  if (cursorY == 0) {
    if (mapy - 240 >= 0) {
      mapy-=240;
      cursorY = 229;
      refreshscreen = true;
    } else if (mapy != 0){
      mapy = 0;
      cursorY = 229;
      refreshscreen = true;
    }
  }
  // If the cursor touches the bottom of the screen relative to the rotation
  // orientation, move a screen down
  if (cursorY == 231) {
    if (mapy + 240 <= 1808) {
      mapy+=240;
      cursorY = 1;
      refreshscreen = true;
    } else if (mapy != 1808){
      mapy = 1808;
      cursorY = 1;
      refreshscreen = true;
    }
  }
  // If the conditions to call a screen redraw are met, redraw the screen
  if (refreshscreen == true) {
    lcd_image_draw(&yegImage, &tft, mapx,
      mapy, 0, 0, MAP_DISP_WIDTH, MAP_DISP_HEIGHT);
    refreshscreen = false;
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
    redrawCursor(cursorX, cursorY, oldX, oldY);
  }
  delay(10);
}

// fast version self made which reuses block if already loaded
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

// function to calculate manhatten distance
int manhatten(int currentx, int restx, int currenty, int resty){
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

// insertionsort function that sorts the distances in accending order
// comparing the manhatten distances but swapping the index along with distance
void isort(RestDist dist[], int len) {
  int i;
  int j;
  i = 1;
  while (i < len) {
    j = i;
    // need .dist to compare just the distances
    while ((j > 0) && (dist[j-1].dist > dist[j].dist)) {
      // but swap the whole struct to keep index lined up
      swap(dist[j], dist[j-1]);
      j = j-1;
    }
    i = i+1;
  }
}


// function that draws all the names
void drawName(uint16_t selectedRest, uint16_t cycle) {
  tft.setCursor(0, 0);
  tft.setTextSize(1);
  // if already drawn don't redraw whole thing
  if (drawMode!= 0) {
    // if you moved in list mode
    if (previousPosition != selectedRest) {
      for (int16_t i = (0 + cycle); i < (30 + cycle); i++) {
        if (i == selectedRest) {
          // highlight new selected restaurant
          getRestaurantFast(rest_dist[selectedRest].index , &rest);
          tft.setTextColor(ILI9341_BLACK, ILI9341_WHITE);
          tft.println(rest.name);
        } else if (i == previousPosition) {
          // unhighlight
          getRestaurantFast(rest_dist[previousPosition].index , &rest);
          tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
          tft.println(rest.name);
        } else {
          // keep restaurants the same if you didn't move towards it
          tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
          tft.println("");
        }
      }
    }
  // case for when you entered the list from map
  // draw all names that are close
  } else {
    // draw the screen all black first
    tft.fillScreen(ILI9341_BLACK);
    for (int16_t i = (0 + cycle); i < (30 + cycle); i ++) {
      getRestaurantFast(rest_dist[i].index , &rest);
      // case for if loops further than the number of restaurants
      if (i > (filterNum - 1)){
        tft.setTextColor(ILI9341_BLACK, ILI9341_BLACK);
        tft.println("               ");
      }
      else{
        if (i == selectedRest) {  // highlight
          // black characters on white background
          tft.setTextColor(ILI9341_BLACK, ILI9341_WHITE);
        } else {  // not highlighted
          // white characters on black background
          tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
        }
        tft.println(rest.name);
      }
      drawMode = 1;
    }
  }
}

// taken from eclass quicksort.cpp
/* This function takes last element as pivot, places
the pivot element at its correct position in sorted
	array, and places all smaller (smaller than pivot)
to left of pivot and all greater elements to right
of pivot */
int partition (RestDist dist[], int low, int high)
{
	int pivot = dist[high].dist; // pivot
	int i = (low - 1); // Index of smaller element

	for (int j = low; j <= high- 1; j++)
	{
		// If current element is smaller than or
		// equal to pivot
		if (dist[j].dist <= pivot)
		{
			i++; // increment index of smaller element
			swap(dist[i], dist[j]);
		}
	}
	swap(dist[i + 1], dist[high]);
	return (i + 1);
}

// taken from eclass quicksort.cpp
/* The main function that implements QuickSort
arr[] --> Array to be sorted,
low --> Starting index,
high --> Ending index */
void quickSort(RestDist dist[], int low, int high)
{
	if (low < high)
	{
		/* pi is partitioning index, arr[p] is now
		at right place */
		int pi = partition(dist, low, high);

		// Separately sort elements before
		// partition and after partition
		quickSort(dist, low, pi - 1);
		quickSort(dist, pi + 1, high);
	}
}

// function that if the sorting button on bottom left is pressed uses
void sortButton() {
  TSPoint touch = ts.getPoint();
  int16_t screen_x = map(touch.y, MINPRESSURE, MAXPRESSURE, DISPLAY_WIDTH-1, 0);
  int16_t screen_y = map(touch.x, MINPRESSURE, MAXPRESSURE, 0, DISPLAY_HEIGHT-1);
  // condition to trigger only if the bottom right button is pressed
  if(((screen_x >= 260) and (screen_x <= 320)) and ((screen_y >= 121) and (screen_y <= 240))) {
    if (touch.z < MINPRESSURE || touch.z > MAXPRESSURE) {
      return;
    }
    // delay so the button doesn't take in unintended button presses
    delay(500);
    // case to switch to qsort if pressed and currently on isort
    if (iSortVal == true){
      qSortVal = true;
      iSortVal = false;
      // bottom rectangle button visual
      tft.fillRect(272,121,48,119, ILI9341_WHITE);
      tft.drawRect(272,121,48,119, ILI9341_GREEN);
      // qsort written vertically
      tft.setTextColor(ILI9341_BLACK, ILI9341_WHITE);
      tft.setCursor(293,141);
      tft.setTextSize(2);
      tft.print("Q");
      tft.setCursor(293,158);
      tft.setTextSize(2);
      tft.print("S");
      tft.setCursor(293,175);
      tft.setTextSize(2);
      tft.print("O");
      tft.setCursor(293,192);
      tft.setTextSize(2);
      tft.print("R");
      tft.setCursor(293,209);
      tft.setTextSize(2);
      tft.print("T");
    }
    // case to switch to both if pressed and currently on qsort
    else if (qSortVal == true){
      both = true;
      qSortVal = false;
      // bottom rectangle button visual
      tft.fillRect(272,121,48,119, ILI9341_WHITE);
      tft.drawRect(272,121,48,119, ILI9341_GREEN);
      // both written vertically
      tft.setTextColor(ILI9341_BLACK, ILI9341_WHITE);
      tft.setCursor(293,146);
      tft.setTextSize(2);
      tft.print("B");
      tft.setCursor(293,163);
      tft.setTextSize(2);
      tft.print("O");
      tft.setCursor(293,180);
      tft.setTextSize(2);
      tft.print("T");
      tft.setCursor(293,197);
      tft.setTextSize(2);
      tft.print("H");
    }
    // case to switch to isort if pressed and currently on both
    else if (both == true){
      iSortVal = true;
      both = false;
      // bottom rectangle button visual
      tft.fillRect(272,121,48,119, ILI9341_WHITE);
      tft.drawRect(272,121,48,119, ILI9341_GREEN);
      // isort written vertically
      tft.setTextColor(ILI9341_BLACK, ILI9341_WHITE);
      tft.setCursor(293,141);
      tft.setTextSize(2);
      tft.print("I");
      tft.setCursor(293,158);
      tft.setTextSize(2);
      tft.print("S");
      tft.setCursor(293,175);
      tft.setTextSize(2);
      tft.print("O");
      tft.setCursor(293,192);
      tft.setTextSize(2);
      tft.print("R");
      tft.setCursor(293,209);
      tft.setTextSize(2);
      tft.print("T");
    }
  }
}

// When the screen is touched, display the locations of restaurants on screen
void screentap() {
  TSPoint touch = ts.getPoint();
  int16_t screen_x = map(touch.y, MINPRESSURE, MAXPRESSURE, DISPLAY_WIDTH-1, 0);
  // condition to trigger only if non black side of screen is pressed
  if (screen_x <= 260) {
    if (touch.z < MINPRESSURE || touch.z > MAXPRESSURE) {
      return;
    }
    // Go through the restaurants and if their coordinates are within the
    // current screen, draw a circle on the location
    for (int i = 0; i < 1067; i++) {
      getRestaurantFast(i, &rest);
      int scale10rating = rest.rating;
      int newscalerating = max(((scale10rating + 1)/2), 1);
      if (newscalerating >= rating) {
        longitude = lon_to_x(rest.lon);
        latitude = lat_to_y(rest.lat);
        // Check if the restaurant is within the screen display
        if (longitude >= mapx && longitude <= mapx + 263 && latitude >= mapy
        && latitude <= mapy + 231) {
          tft.fillCircle(longitude - mapx - 1, latitude - mapy - 1,
            4, ILI9341_BLUE);
        }
      }
    }
  }
}

// Update the rating button if it's pressed
void ratingselector() {
  TSPoint touch = ts.getPoint();
  int16_t screen_x = map(touch.y, MINPRESSURE, MAXPRESSURE, DISPLAY_WIDTH-1, 0);
  int16_t screen_y = map(touch.x, MINPRESSURE, MAXPRESSURE, 0, DISPLAY_HEIGHT-1);
  if (((screen_x >= 260) and (screen_x <= 320)) and ((screen_y >= 0) and (screen_y <= 120))) {
    if (touch.z < MINPRESSURE || touch.z > MAXPRESSURE) {
      return;
    }
    delay(500);
    // resetting number of restaurants that meet the rating
    filterNum = 0;
    if (rating >= 1 && rating <= 4) {
      rating++;
    }
    else if (rating >= 5) {
      rating = 1;
    }
    tft.setCursor(293, 52);
    tft.setTextSize(2);
    tft.print(rating);
  }
}

// Converts the 0-10 scale ratings of the restaurants to the new 1-5 scale
// and only stores the restaurants that meet the rating into the rest_dist
// array
void restsaboverating() {
  // Making sure to reset the temp varaibles in case the rating value was
  // changed
  filterNum = 0;
  int restdistindex = 0;
  int distance;
  for (int i = 0; i < 1067; i++) {
    getRestaurantFast(i, &rest);
    int scale10rating = rest.rating;
    int newscalerating = max(((scale10rating + 1)/2), 1);
    if (newscalerating >= rating) {
      filterNum++;
      rest_dist[restdistindex].index = i;
      ln = lon_to_x(rest.lon);
      lt = lat_to_y(rest.lat);
      distance = manhatten(xposcursor, ln, yposcursor, lt);
      rest_dist[restdistindex].dist = distance;
      restdistindex++;
    }
  }
}

int main() {
  setup();
  restaurant rest;
  int restaurantCounter;
  int checkButton;
  int swapToScreen = 0;
  while (true){
    ratingselector();
    screentap();
    sortButton();
    cursorlocation();
    processJoystick();
    checkButton = digitalRead(JOY_SEL);
    // swap to screen if condition
    if (swapToScreen != 0){
      swapToScreen = 0;
      // draw the screen all black first
      tft.fillScreen(ILI9341_BLACK);
      // call the selected restaurant
      getRestaurantFast(rest_dist[position].index, &rest);
      // obtain longitude and latitude of selected restaurant
      longitude = lon_to_x(rest.lon);
      latitude = lat_to_y(rest.lat);
      // the below are not the relative x and y coordinates
      // to the screen - they're the x and y's on the whole map
      mapx = constrain(mapx, 0, 1776);
      mapy = constrain(mapy, 0, 1808);
      lcd_image_draw(&yegImage, &tft, mapx, mapy, 0, 0,
        MAP_DISP_WIDTH, MAP_DISP_HEIGHT);
        // cases to draw the button that it is returning to
        if (iSortVal == true){
          // Redraw the 'rating' button with whatever the selected rating value
          // was
          tft.fillRect(272,0,48,119, ILI9341_WHITE);
          tft.drawRect(272,0,48,119, ILI9341_RED);
          tft.setTextColor(ILI9341_BLACK, ILI9341_WHITE);
          tft.setCursor(293, 52);
          tft.setTextSize(2);
          tft.print(rating);
          // bottom rectangle button visual
          tft.fillRect(272,121,48,119, ILI9341_WHITE);
          tft.drawRect(272,121,48,119, ILI9341_GREEN);
          // isort written vertically
          tft.setTextColor(ILI9341_BLACK, ILI9341_WHITE);
          tft.setCursor(293,141);
          tft.setTextSize(2);
          tft.print("I");
          tft.setCursor(293,158);
          tft.setTextSize(2);
          tft.print("S");
          tft.setCursor(293,175);
          tft.setTextSize(2);
          tft.print("O");
          tft.setCursor(293,192);
          tft.setTextSize(2);
          tft.print("R");
          tft.setCursor(293,209);
          tft.setTextSize(2);
          tft.print("T");
        }
        else if (qSortVal == true){
          // Redraw the 'rating' button with whatever the selected rating value
          // was
          tft.fillRect(272,0,48,119, ILI9341_WHITE);
          tft.drawRect(272,0,48,119, ILI9341_RED);
          tft.setTextColor(ILI9341_BLACK, ILI9341_WHITE);
          tft.setCursor(293, 52);
          tft.setTextSize(2);
          tft.print(rating);
          // qsort rectangle button visual
          tft.fillRect(272,121,48,119, ILI9341_WHITE);
          tft.drawRect(272,121,48,119, ILI9341_GREEN);
          // fast written vertically
          tft.setTextColor(ILI9341_BLACK, ILI9341_WHITE);
          tft.setCursor(293,141);
          tft.setTextSize(2);
          tft.print("Q");
          tft.setCursor(293,158);
          tft.setTextSize(2);
          tft.print("S");
          tft.setCursor(293,175);
          tft.setTextSize(2);
          tft.print("O");
          tft.setCursor(293,192);
          tft.setTextSize(2);
          tft.print("R");
          tft.setCursor(293,209);
          tft.setTextSize(2);
          tft.print("T");
        }
        else if (both == true){
          // Redraw the 'rating' button with whatever the selected rating value
          // was
          tft.fillRect(272,0,48,119, ILI9341_WHITE);
          tft.drawRect(272,0,48,119, ILI9341_RED);
          tft.setTextColor(ILI9341_BLACK, ILI9341_WHITE);
          tft.setCursor(293, 52);
          tft.setTextSize(2);
          tft.print(rating);
          // bottom rectangle button visual
          tft.fillRect(272,121,48,119, ILI9341_WHITE);
          tft.drawRect(272,121,48,119, ILI9341_GREEN);
          // both written vertically
          tft.setTextColor(ILI9341_BLACK, ILI9341_WHITE);
          tft.setCursor(293,146);
          tft.setTextSize(2);
          tft.print("B");
          tft.setCursor(293,163);
          tft.setTextSize(2);
          tft.print("O");
          tft.setCursor(293,180);
          tft.setTextSize(2);
          tft.print("T");
          tft.setCursor(293,197);
          tft.setTextSize(2);
          tft.print("H");
        }

      // If the screen is currently on the edge of the map, instead of
      // centreing the cursor on middle of screen draw the cursor on the
      // restaurant
      if (mapx == 0 || mapx == 1776 || mapy == 0 || mapy == 1808) {
        if (longitude - mapx - 5 <= 263) {
          cursorX = longitude - mapx - 5;
        } else {
          cursorX = 263;
        }
        cursorY = latitude - mapy - 5;
      } else {
      // If the screen isn't a map edge have cursor in middle of screen
      cursorX = (DISPLAY_WIDTH - 48 - CURSOR_SIZE)/2;
      cursorY = (DISPLAY_HEIGHT - CURSOR_SIZE)/2;
      }
        // draw the initial cursor
        redrawCursor(cursorX, cursorY, cursorX, cursorY);
      }
      // button pushed
      if (checkButton == LOW){
        // conditions to use the sort values set from map screen
        if (iSortVal == true) {
          restsaboverating();
          unsigned long startTimeISort = millis();
          isort(rest_dist, filterNum);
          unsigned long endTimeISort = millis();
          unsigned long deltaIsort = endTimeISort-startTimeISort;
          Serial.print("isort ");
          Serial.print(filterNum);
          Serial.print(" restaurants: ");
          Serial.print(deltaIsort);
          Serial.println(" ms");
        } else if (qSortVal == true) {
          restsaboverating();
          unsigned long startTimeQSort = millis();
          quickSort(rest_dist, 0, filterNum-1);
          unsigned long endTimeQSort = millis();
          unsigned long deltaQsort = endTimeQSort - startTimeQSort;
          Serial.print("qsort ");
          Serial.print(filterNum);
          Serial.print(" restaurants: ");
          Serial.print(deltaQsort);
          Serial.println(" ms");
        } else if (both == true) {
          restsaboverating();
          unsigned long startTimeISort = millis();
          isort(rest_dist, filterNum);
          unsigned long endTimeISort = millis();
          unsigned long deltaIsort = endTimeISort - startTimeISort;
          Serial.print("isort ");
          Serial.print(filterNum);
          Serial.print(" restaurants: ");
          Serial.print(deltaIsort);
          Serial.println(" ms");
          restsaboverating();
          unsigned long startTimeQSort = millis();
          quickSort(rest_dist, 0, filterNum-1);
          unsigned long endTimeQSort = millis();
          unsigned long deltaQsort = endTimeQSort - startTimeQSort;
          Serial.print("qsort ");
          Serial.print(filterNum);
          Serial.print(" restaurants: ");
          Serial.print(deltaQsort);
          Serial.println(" ms");
        }

        // draw the screen all black first
        tft.fillScreen(ILI9341_BLACK);
        // always start with first restaurant
        position = 0;
        newScreenChecker = 0;
        newScreen = 0;
        mode = 1;
        drawMode = 0;
        previousPosition = position;
        drawName(position,newScreen);
        while(mode!=0){
          // if statement for if its on the 16- blank names go to a different
          // function that draws the next few names
          // reread the joystick everytime to check if a valid tilt is inputed
          int yVal = analogRead(JOY_VERT);
          int buttonVal = digitalRead(JOY_SEL);
          // this is to move down
          if (yVal >= (JOY_CENTER + JOY_DEADZONE)){
            position++;
            newScreenChecker++;
            //clamp the very last position
            if (position > (filterNum - 1)){
              position = (filterNum - 1);
              newScreenChecker = (filterNum % 30); //16 in the case of all since extra 16 restaurant
            }
            // case for last page that is less than 30
            if ((position > ((filterNum - 1)-(filterNum % 30))) && (newScreenChecker> 29)){
              newScreen = newScreen + 30;
              newScreenChecker = 0;
              drawMode = 0;
            }
            // conditions for moving to next screen
            if ((newScreenChecker > 29) && (position < ((filterNum - 1)-(filterNum % 30)))){
              newScreenChecker = 0;
              newScreen = newScreen + 30;
              drawMode = 0;
            }
            drawName(position,newScreen);
            previousPosition = position;
            // this is move up
          } else if (yVal <= (JOY_CENTER - JOY_DEADZONE) ){
            position--;
            newScreenChecker--;
            if (position < 0){
              position = 0;
              newScreenChecker = 0;
            }
            if ((newScreenChecker < 0) && (position >= 29)){
              newScreenChecker = 29;
              newScreen = newScreen - 30;
              drawMode = 0;
            }
            drawName(position,newScreen);
            previousPosition = position;
          }

          checkButton = digitalRead(JOY_SEL);
          // case for if button pressed while in list
          if (checkButton == LOW){
            mode = 0;
            drawMode = 0;
            swapToScreen = 1;
            getRestaurantFast(rest_dist[position].index, &rest);
            longitude = lon_to_x(rest.lon);
            latitude = lat_to_y(rest.lat);
            // Constraints for the restaurant locations - if the location is
            // out of the map bounds, draw the cursor on the closest edge
            if (longitude - 136 > 0) {
              mapx = longitude - 136;
            }
            else if (longitude - 136 <= 0) {
              mapx = 0;
              cursorX = 0;
            }
            else if (longitude - 136 >= 1776) {
              mapx = 1776;
              cursorX = 263;
            }

            if (latitude - 120 > 0) {
              mapy = latitude - 120;
            }
            else if (latitude - 120 <= 0 ) {
              mapy = 0;
              cursorY = 0;
            }
             else if (latitude - 120 >= 1808) {
               mapy = 1808;
               cursorY = 231;
             }
            delay(950);
          }
          delay (50);
        }
      }
    }

    Serial.end();

    return 0;
  }
