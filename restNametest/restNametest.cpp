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

// int latitude[1067];
// int longitude[1067];

void setup() {
  init();
  Serial.begin(9600);

  tft.begin();
  tft.fillScreen(ILI9341_BLACK);

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
  int currentx = 0; // comment out these two when you have postion from cursor
  int currenty = 0;
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


  Serial.end();

  return 0;
}
