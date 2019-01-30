


/*
  A demonstration of how we can fetch restaurant data from the SD card
  by reading one block at a time.

  Also has some examples of how to use structs.
*/

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <SD.h>

#define TFT_DC 9
#define TFT_CS 10
#define SD_CS 6

#define REST_START_BLOCK 4000000
#define NUM_RESTAURANTS 1066
// These constants are for the 2048 by 2048 map .
# define MAP_WIDTH 2048
# define MAP_HEIGHT 2048
# define LAT_NORTH 5361858l
# define LAT_SOUTH 5340953l
# define LON_WEST -11368652l
# define LON_EAST -11333496l

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
uint32_t lastBlockNum = REST_START_BLOCK-1;
restaurant restBlock[8];


struct RestDist {
  uint16_t index; //index of restaurant from 0 to NUM_RESTAURANTS - 1
  uint16_t dist; // manhatten distance to cursor position
};
RestDist rest_dist[NUM_RESTAURANTS];


int latitude[1067];
int longitude[1067];

void setup() {
  init();
  Serial.begin(9600);

  // including this seems to fix some SD card readblock errors
  // (at least on the old display)
  tft.begin();
  tft.fillScreen(ILI9341_BLACK);
  // The following initialization (commented out) is
  // not needed for just raw reads from the SD card, but you should add it later
  // when you bring the map picture back into your assignment
  // (both initializations are required for the assignment)

  // initialize the SD card
  /*
  Serial.print("Initializing SD card...");
  if (!SD.begin(SD_CS)) {
    Serial.println("failed! Is the card inserted properly?");
    while (true) {}
  }
  else {
    Serial.println("OK!");
  }
  */

  // initialize SPI (serial peripheral interface)
  // communication between the Arduino and the SD controller

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

// read the restaurant at position "restIndex" from the card
// and store at the location pointed to by restPtr
void getRestaurant(int restIndex, restaurant* restPtr) {
  // calculate the block containing this restaurant
  uint32_t blockNum = REST_START_BLOCK + restIndex/8;
  restaurant restBlock[8];

   //Serial.println(blockNum);

  // fetch the block of restaurants containing the restaurant
  // with index "restIndex"
  while (!card.readBlock(blockNum, (uint8_t*) restBlock)) {
    Serial.println("Read block failed, trying again.");
  }
   //Serial.print("Loaded: ");
   //Serial.println(restBlock[0].name);
  *restPtr = restBlock[restIndex % 8];
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
    //restaurant restBlock[8];// cache
    // reset counter
    //restCounter = 0;
    // overwrite the block
    while (!card.readBlock(blockNum, (uint8_t*) restBlock)) {
      Serial.println("Read block failed, trying again.");
    }
  }
  // increment counter everytime a restaurant is read
  //restCounter ++;

  *restPtr = restBlock[restIndex % 8];
}

/*
// swap function from eclass quicksort.cpp that swaps two inputs
void swap(int* a,int* b){
	int t = *a;
	*a = *b;
	*b = t;
}

// working i sort
void isort(RestDist* dist,int len){
  int i;
  int j;
  i = 1;
  while (i < lenArray){
    j = i;
    while (( j>0 ) && (array[j-1] > array[j])){
      swap(array[j],array[j-1]);
      j = j-1;
    }
    i = i+1;
  }
}
*/


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

int main() {
  setup();

  // now start reading restaurant data, let's do the first block now
  //restaurant restBlock[8]; // 512 bytes in total: a block

  Serial.println("Now reading restaurants");

  Serial.println();
  Serial.println("Fetching all restaurants");
  restaurant rest;

  for (int i=0; i < 1065;i++){
    getRestaurantFast(i, &rest);
      Serial.print(i);
      Serial.print(" ");
      Serial.println(rest.name);
      Serial.print(i);
      Serial.print(" this is lat in y ");
      Serial.println(lat_to_y (rest.lat));
      longitude[i] = rest.lon;
      Serial.print(i);
      Serial.print(" this is lon in x  ");
      Serial.println(lon_to_x(rest.lon));

  }



  // Part 1
  /*
  for (int i = 0; i < NUM_RESTAURANTS; ++i) {
    getRestaurantFast(i, &rest);
    if (rest.rating == 10){
      Serial.print(i);
      Serial.print(" ");
      Serial.println(rest.name);
    }
  }
  */

  /*
  // Part 2
  for (int i = 0; i < NUM_RESTAURANTS; ++i) {
    getRestaurantFast(i, &rest);
    if (strstr(rest.name, "Subway")){
        Serial.print(i);
        Serial.print(" ");
        Serial.print("Latitude: ");
        Serial.print(rest.lat);
        Serial.print(", Longitude: ");
        Serial.print(rest.lon);
        Serial.print(", Rating: ");
        Serial.print(rest.rating);
        Serial.println();
        Serial.print("this is restaurant name:  ");
        Serial.println(rest.name);
      }
  }
*/


  Serial.end();

  return 0;
}
