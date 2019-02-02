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

// dimensions of the part allocated to the map display
#define MAP_DISP_WIDTH (DISPLAY_WIDTH - 48)
#define MAP_DISP_HEIGHT DISPLAY_HEIGHT

#define REST_START_BLOCK 4000000
#define NUM_RESTAURANTS 1066
// These constants are for the 2048 by 2048 map .
# define MAP_WIDTH 2048
# define MAP_HEIGHT 2048
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

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// different than SD
Sd2Card card;

// global variables for counter to know if a new block is needed
//uint32_t lastBlockNum = REST_START_BLOCK-1;
// global variable for restaurant block as a casche
//uint8_t* restBlock;

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



void setup() {
  init();
  Serial.begin(9600);

  pinMode(JOY_SEL, INPUT_PULLUP);

	tft.begin();
	tft.fillScreen(ILI9341_BLACK);
	tft.setRotation(3);

  tft.setTextSize(2);
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


// void manhatten(current location x, all restaurantx, current location y, all restauranty){
int manhatten(int restx, int resty){
  int currentx = 2048; // comment out these two when you have postion from cursor
  int currenty = 2048;
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

/*
void drawName(uint16_t index){
  tft.setCursor(0,index*15); // should be on the left each word is 8 bit high (its in the adafruit graphicks library) we want size 2 so 7*2 +1 which is white space 1
  // fetches the restaurant at that index
  highlightedString = rest_dist;
  getRestaurantFast(rest_dist[index].index, &rest);

  Serial.print("this is rest_dist[index]");
  Serial.println(rest_dist);

  if(index == rest_dist){ // if it matchs do this
    tft.setTextColor(ILI9341_BLACK, ILI9341_WHITE); // black with white background
  }else { // else show not selected
    tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK); // black with white background
  }
  // print restaurant name
  tft.println(rest.name);
}
*/
// first 15
void drawName(uint16_t selectedRest){
  //selectedRest = 0; // which restaurant is selected ?
  for ( int16_t i = 0; i < 15; i ++) {

    getRestaurantFast(rest_dist[i].index , &rest) ;
    Serial.print(" this is i: ");
    Serial.println(i);
    Serial.print(" this is selectedRest: ");
    Serial.println(selectedRest);
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
  tft.print ("\n") ;
}

/*
void displayFirsthalf(){
  tft.fillScreen(ILI9341_BLACK); // fill it in black

  for (uint16_t i=0; i < 15; i++){ // loop thorugh all names and draw them Note it can only draw 15 names on the screen

    drawName(i);
  }
}
*/


int main() {
  setup();

  restaurant rest;
  int lt, ln;
  int restaurantCounter;

  //manhatten(longitude, latitude);
  for (int i=0; i < 1067;i++){
    getRestaurantFast(i, &rest); // first value tells you what restaurant that number is and allows you to look for it directly
      // what you do is find the manhatten dist of closest one

      ln = lon_to_x(rest.lon) ;
      lt = lat_to_y(rest.lat);
      rest_dist[i].dist = manhatten(ln,lt);
      rest_dist[i].index = i;
      /*
      Serial.print(" this is index: ");
      Serial.print(rest_dist[i].index);
      Serial.print("    this is longitude: ");
      Serial.println(ln);
      Serial.print("    this is longitude: ");
      Serial.print(lt);
      Serial.print("    this is rest_dist: ");
      Serial.println(rest_dist[i].dist);
      */
  }

  isort(rest_dist,NUM_RESTAURANTS);
  // maybe only need to show to 30
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
  position = 0;// always start with first restaurant
  //displayFirsthalf();
  Serial.println(" before while loop ");
  drawName(position);
  while(true){
    // if statement for if its on the 16- blank names go to a different function that draws the next few names
    // reread the joystick everytime to check if a valid tilt is inputed
    int yVal = analogRead(JOY_VERT);
    int buttonVal = digitalRead(JOY_SEL);
    //uint16_t prevHighlight = highlightedString;
    Serial.println(" inside while loop ");// entered while loop
    if (yVal >= (JOY_CENTER + JOY_DEADZONE)){ // this is to move down
      position++;
      //Serial.println(" pushed down ");
      //Serial.println(position);
      //highlightedString = (highlightedString+1)%NUM_NAMES_PER_PAGE; //highlighted one will be lower one since you pushed down
      drawName(position);
      /*
      drawName(prevHighlight);
      drawName(highlightedString);
      */
    } else if (yVal <= (JOY_CENTER - JOY_DEADZONE) ){ // this is move up
      //highlightedString = (highlightedString-1)%NUM_NAMES_PER_PAGE;
      position--;
      //Serial.println(" pushed up ");
      //Serial.println(position);
      drawName(position);
      /*
      drawName(prevHighlight);
      drawName(highlightedString);
      */
    }
    /*
		if (highlightedString >= 15) {
			highlightedString = 0;
			displayAllNamesPage2();
		}
    Serial.print("this is the current name");
    Serial.println(highlightedString);
    */
    delay (500);


    // Change the highlighted names with joysticks (done) (even works where if you go pass the top you go to bot and vice versa)
    // add more names than can be dipslayed on one screen, and
    // go to the " next page" of names if you select far enough down
  }






  Serial.end();

  return 0;
}
