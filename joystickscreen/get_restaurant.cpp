/*
# ----------------------------------------------
#   Name: Ricky Au
#   ID: 1529429
#   CMPUT 275, Winter 2018
#
#   Weekly Exercise 2 : Restaurants and Pointers getRestaurantFast
# ----------------------------------------------
*/

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <SD.h>
#include <TouchScreen.h>

#define TFT_DC 9
#define TFT_CS 10
#define SD_CS 6

// physical dimensions of the tft display (# of pixels)
#define DISPLAY_WIDTH  320
#define DISPLAY_HEIGHT 240

// touch screen pins, obtained from the documentaion
#define YP A2  // must be an analog pin, use "An" notation!
#define XM A3  // must be an analog pin, use "An" notation!
#define YM  5  // can be a digital pin
#define XP  4  // can be a digital pin

// width/height of the display when rotated horizontally
#define TFT_WIDTH  320
#define TFT_HEIGHT 240

// dimensions of the part allocated to the map display
#define MAP_DISP_WIDTH (DISPLAY_WIDTH - 48)
#define MAP_DISP_HEIGHT DISPLAY_HEIGHT

#define REST_START_BLOCK 4000000
#define NUM_RESTAURANTS 1066

// calibration data for the touch screen, obtained from documentation
// the minimum/maximum possible readings from the touch point
#define TS_MINX 150
#define TS_MINY 120
#define TS_MAXX 920
#define TS_MAXY 940

// thresholds to determine if there was a touch
#define MINPRESSURE   10
#define MAXPRESSURE 1000

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// a multimeter reading says there are 300 ohms of resistance across the plate,
// so initialize with this to get more accurate readings
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

// different than SD
Sd2Card card;

// global variables for counter to know if a new block is needed
int restCounter = 8;
// global variable for restaurant block as a casche
uint8_t* restBlock;

struct restaurant {
  int32_t lat;
  int32_t lon;
  uint8_t rating; // from 0 to 10
  char name[55];
};

// touch screen initialization to have coordinates match with our convention
TSPoint touch = ts.getPoint();
int16_t screen_x = map(touch.y, TS_MINY, TS_MAXY, TFT_WIDTH-1, 0);
int16_t screen_y = map(touch.x, TS_MINX, TS_MAXX, 0, TFT_HEIGHT-1);

// setup function
void setup() {
  init();
  Serial.begin(9600);

  // tft display initialization
  tft.begin();
  // set background to black
  tft.fillScreen(ILI9341_BLACK);
  // set rotation for wider screen
  tft.setRotation(3);
  // initial screen when no button has been pressed yet
  tft.setCursor(0,0);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
  tft.setTextSize(2);
  tft.println("RECENT SLOW RUN:");
  tft.println("Not yet run");
  tft.println("");
  tft.println("SLOW RUN AVG:");
  tft.println("Not yet run");
  tft.println("");
  tft.println("RECENT FAST RUN:");
  tft.println("Not yet run");
  tft.println("");
  tft.println("FAST RUN AVG:");
  tft.println("Not yet run");

  // top rectangle button visual
  tft.drawRect(272,0,48,120, ILI9341_RED);
  // slow written vertically
  tft.setCursor(293,25);
  tft.setTextSize(2);
  tft.print("S");
  tft.setCursor(293,42);
  tft.setTextSize(2);
  tft.print("L");
  tft.setCursor(293,59);
  tft.setTextSize(2);
  tft.print("O");
  tft.setCursor(293,76);
  tft.setTextSize(2);
  tft.print("W");
  // botom rectangle button visual
  tft.drawRect(272,121,48,119, ILI9341_RED);
  // fast written vertically
  tft.setCursor(293,146);
  tft.setTextSize(2);
  tft.print("F");
  tft.setCursor(293,163);
  tft.setTextSize(2);
  tft.print("A");
  tft.setCursor(293,180);
  tft.setTextSize(2);
  tft.print("S");
  tft.setCursor(293,197);
  tft.setTextSize(2);
  tft.print("T");

  // SD card initialization for raw reads
  Serial.print("Initializing SPI communication for raw reads...");
  if (!card.init(SPI_HALF_SPEED, SD_CS)) {
    Serial.println("failed! Is the card inserted properly?");
    while (true) {}
  }
  else {
    Serial.println("OK!");
  }
}

// the implementation from class
void getRestaurant(int restIndex, restaurant* restPtr) {
  //obtain block number
  uint32_t blockNum = REST_START_BLOCK + restIndex/8;
  restaurant restBlock[8];
  // read the block everytime a restaurant is read
  while (!card.readBlock(blockNum, (uint8_t*) restBlock)) {
    Serial.println("Read block failed, trying again.");
  }
  *restPtr = restBlock[restIndex % 8];
}

// fast version self made which resuses block if already loaded
void getRestaurantFast(int restIndex, restaurant* restPtr) {
  // obtain block number
  uint32_t blockNum = REST_START_BLOCK + restIndex/8;
  restaurant restBlock[8];
  // if condition to see if we need a new block
  // if not needed we use the casched block saved in global variable above
  if (restCounter == 8){
    // reset counter
    restCounter = 0;
    // overwrite the block
    while (!card.readBlock(blockNum, (uint8_t*) restBlock)) {
      Serial.println("Read block failed, trying again.");
    }
  }
  // increment counter everytime a restaurant is read
  restCounter ++;

  *restPtr = restBlock[restIndex % 8];
}

int main() {
  setup();
  int slowCounter = 0;
  int totalSlow = 0;
  int avgSlow = 0;
  int fastCounter = 0;
  int totalFast = 0;
  int avgFast = 0;
  unsigned long deltaSlow;
  unsigned long deltaFast;
  // now start reading restaurant data, let's do the first block now
  restaurant restBlock[8]; // 512 bytes in total: a block

  restaurant rest;

  while (true) {

    //read touch screen data when pressed
    TSPoint touch = ts.getPoint();
  	int16_t screen_x = map(touch.y, TS_MINY, TS_MAXY, TFT_WIDTH-1, 0);
  	int16_t screen_y = map(touch.x, TS_MINX, TS_MAXX, 0, TFT_HEIGHT-1);
  	delay(200);
    // if condition for the coordinates of the slow button
    if (((screen_x >= 260) and (screen_x <= 320)) and ((screen_y >= 0) and (screen_y <= 120))){
      Serial.println("Slow button pressed");
      while(true){
        Serial.print("Time: ");
        // starting time when slow getrestaurant is run
        unsigned long startTimeSlow = millis();
        for (int i = 0; i < NUM_RESTAURANTS; ++i) {
          getRestaurant(i, &rest);
        }
        // end time when slow get restaurant finished running
        unsigned long endTimeSlow = millis();
        // calculating the amount of time it takes for all restaurants to run
        unsigned long deltaSlow = endTimeSlow - startTimeSlow;
        Serial.print("relapse: ");
        Serial.println(deltaSlow);
        Serial.print("end time: ");
        Serial.println(endTimeSlow);
        Serial.print("start: ");
        Serial.println(startTimeSlow);
        // accumulate total speed every time slow is pressed
        totalSlow = totalSlow + deltaSlow;
        // how many times the slow button has been pressed
        slowCounter ++;
        // calculate average slow run
        avgSlow = totalSlow / slowCounter;
        delay(1000);
        // redraw only the texts and slow times keeping the fast times the same
        tft.setCursor(0,0);
        tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
        tft.setTextSize(2);
        tft.println("RECENT SLOW RUN:");
        tft.print(deltaSlow);
        tft.println(" ms       ");
        tft.println("");
        tft.println("SLOW RUN AVG:");
        tft.print(avgSlow);
        tft.println(" ms       ");
        tft.println("");
        tft.println("RECENT FAST RUN:");
        tft.println("");
        tft.println("");
        tft.println("FAST RUN AVG:");
        tft.println("");
        break;
      }
    // if condition for the coordinates of the fast button
    }else if(((screen_x >= 260) and (screen_x <= 320)) and ((screen_y >= 121) and (screen_y <= 240))) {
      Serial.println("Fast button pressed");
      while(true){
        Serial.print("Time: ");
        // starting time when getRestaurantFast is run
        unsigned long startTimeFast = millis();
        for (int i = 0; i < NUM_RESTAURANTS; ++i) {
          getRestaurantFast(i, &rest);
        }
        // end time when fast get restaurant finished running
        unsigned long endTimeFast = millis();
        // calculating the amount of time it takes for all restaurants to run
        unsigned long deltaFast = endTimeFast-startTimeFast;
        Serial.print("relapse: ");
        Serial.println(deltaFast);
        Serial.print("end time: ");
        Serial.println(endTimeFast);
        Serial.print("start: ");
        Serial.println(startTimeFast);
        // accumulate total speed every time fast is pressed
        totalFast = totalFast + deltaFast;
        // how many times the fast button has been pressed
        fastCounter ++;
        // calculate average fast run
        avgFast = totalFast / fastCounter;
        delay(1000);
        // redraw only the texts and fast times keeping the slow times the same
        tft.setCursor(0,0);
        tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
        tft.setTextSize(2);
        tft.println("RECENT SLOW RUN:");
        tft.println("");
        tft.println("");
        tft.println("SLOW RUN AVG:");
        tft.println("");
        tft.println("");
        tft.println("RECENT FAST RUN:");
        tft.print(deltaFast);
        tft.println(" ms       ");
        tft.println("");
        tft.println("FAST RUN AVG:");
        tft.print(avgFast);
        tft.println(" ms       ");
        break;
      }
    }
	}
  Serial.end();
  return 0;
}
