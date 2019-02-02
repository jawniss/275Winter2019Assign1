
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


#define JOY_VERT  A1
#define JOY_HORIZ A0
#define JOY_SEL   2

#define JOY_CENTER   512
#define JOY_DEADZONE 64


// name testing
#define NUM_NAMES 18

const char *stringsToDisplay[] = {
	"Zac", "Omid", "Veronica", "Zach", "Jason", "Joseph", "Arseniy",
	"Daniel", "Mohammad", "Parash", "Everton", "Touqir", "Logan",
	"A Very Long Name That Should Not Wrap Around", "A","B","C","D"
	,"E","F","G" // extra names
}; uint16_t highlightedString; // indexes the above const char strings to display




const char *stringsToDisplay2[] = {
	"one", "two", "three", "four", "five", "six", "seven",
	"eight", "nine", "ten", "eleevn", "twele", "thirteen",
	"fourteen", "fitteen", "sixteen", "SEVENTEEN"
}; uint16_t String2;







void drawName(uint16_t index){
	tft.setCursor(0,index*15); // should be on the left each word is 8 bit high (its in the adafruit graphicks library) we want size 2 so 7*2 +1 which is white space 1

	if(index == highlightedString){
		tft.setTextColor(ILI9341_BLACK, ILI9341_WHITE); // black with white background
	}else {
		tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK); // black with white background
	}
	tft.println(stringsToDisplay[index]);
}


void drawName2(uint16_t index){
	tft.setCursor(0,index*15); // should be on the left each word is 8 bit high (its in the adafruit graphicks library) we want size 2 so 7*2 +1 which is white space 1

	if(index == String2){
		tft.setTextColor(ILI9341_BLACK, ILI9341_WHITE); // black with white background
	}else {
		tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK); // black with white background
	}
	tft.println(stringsToDisplay2[index]);
}






// refresh the display and dispay all names
// do not change the highlighred stirng
void displayAllNamesPage2(){
	tft.fillScreen(ILI9341_BLACK); // fill it in black

	for (uint16_t i=0; i< NUM_NAMES; i++){ // loop thorugh all names and draw them Note it can only draw 15 names on the screen

		drawName2(i);
	}
}




void displayAllNames(){
	tft.fillScreen(ILI9341_BLACK); // fill it in black

	for (uint16_t i=0; i< NUM_NAMES; i++){ // loop thorugh all names and draw them Note it can only draw 15 names on the screen

		drawName(i);
	}
}

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
}; restaurant rest;

// setup function
void setup() {
	init();
	Serial.begin(9600);

	pinMode(JOY_SEL, INPUT_PULLUP);

	tft.begin();
	tft.fillScreen(ILI9341_BLACK);
	tft.setRotation(3);

	highlightedString = 0;
	tft.setTextSize(2);
	tft.setTextWrap(false);// roll of the edge and not wrap around
	// if you put true it displays the last line over
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

	// now start reading restaurant data, let's do the first block now
	restaurant restBlock[8]; // 512 bytes in total: a block
	restaurant rest;

	// name display test

	displayAllNames();
	bool display1 = true;
	bool display2 = false;
	while (true) {
		// if statement for if its on the 16- blank names go to a different function that draws the next few names

		// reread the joystick everytime to check if a valid tilt is inputed
		// int xVal = analogRead(JOY_HORIZ);
		int yVal = analogRead(JOY_VERT);
		int buttonVal = digitalRead(JOY_SEL);
		uint16_t prevHighlight = highlightedString;


		constrain(highlightedString,0,40);
		constrain(String2,0,40);
		constrain(prevHighlight,0,40);



		if (highlightedString > 0 && highlightedString <= 15) {
			if (display1 == false && display2 == true) {
				displayAllNames();
				display1 = true;
				display2 = false;
			}
			if (yVal >= (JOY_CENTER + JOY_DEADZONE)){ // this is to move down

				highlightedString = (highlightedString+1)%NUM_NAMES; //highlighted one will be lower one since you pushed down
				drawName(prevHighlight);
				drawName(highlightedString);

			} else if (yVal <= (JOY_CENTER - JOY_DEADZONE) ){ // this is move up
				highlightedString = (highlightedString-1)%NUM_NAMES;
				drawName(prevHighlight);
				drawName(highlightedString);
			}
			// if (highlightedString >= 15) {
			// 	highlightedString = 0;
			// 	displayAllNamesPage2();
			// }
		}
		else if (highlightedString == 0) {


			if (yVal >= (JOY_CENTER + JOY_DEADZONE)){ // this is to move down

				highlightedString = (highlightedString+1)%NUM_NAMES; //highlighted one will be lower one since you pushed down
				drawName(prevHighlight);
				drawName(highlightedString);

			}


		}
		else {


			uint16_t prevHighlight2 = String2;

			constrain(prevHighlight2,0,40);
			constrain(highlightedString,0,40);
			constrain(String2,0,40);
			constrain(prevHighlight,0,40);

			// if highlightedString < 31 isn't working maybe it
			// should be if prevHighlight < 31



			// I THINK I NEED TO PUT A TOP CONSTRAINT FOR HIGHLIGH = 15 I THINK
			if (highlightedString > 15 && highlightedString < 31) {
				if (display2 == false && display1 == true) {
					String2 = 0;
					displayAllNamesPage2();

					display2 = true;
					display1 = false;
				}
				if (yVal >= (JOY_CENTER + JOY_DEADZONE)){ // this is to move down


					highlightedString = (highlightedString+1); //highlighted one will be lower one since you pushed down
					prevHighlight2 = String2;
					String2 = (String2+1)%NUM_NAMES;

					drawName2(prevHighlight2);
					drawName2(String2);

				} else if (yVal <= (JOY_CENTER - JOY_DEADZONE) ){ // this is move up
					highlightedString = (highlightedString-1);
					prevHighlight2 = String2;
					String2 = (String2-1)%NUM_NAMES;

					drawName2(prevHighlight2);
					drawName2(String2);
				}
			}
			// here if it hits this else statement that should mean that highlightedString
			// equals 31
			else {
				if (yVal <= (JOY_CENTER - JOY_DEADZONE) ){ // this is move up
					highlightedString = (highlightedString-1);
					prevHighlight2 = String2;
					String2 = (String2-1)%NUM_NAMES;

					drawName2(prevHighlight2);
					drawName2(String2);
				}


			}



		}
		Serial.print("this is the current name");
		Serial.println(highlightedString);
		delay (50);


		// Change the highlighted names with joysticks (done) (even works where if you go pass the top you go to bot and vice versa)
		// add more names than can be dipslayed on one screen, and
		// go to the " next page" of names if you select far enough down
	}





	// snippit from eclass
	/*
	tft.fillScreen (0) ;
	tft.setCursor (0 , 0) ; // where the characters will be displayed
	tft.setTextWrap (false) ;
	int selectedRest = 0; // which restaurant is selected ?
	for ( int16_t i = 0; i < 30; i ++) {
	restaurant r ;
	getRestaurant(restDist[i].index , &r) ;
	if(i!= selectedRest) { // not highlighted
	// white characters on black background
	tft . setTextColor (0xFFFF , 0x0000) ;
} else { // highlighted
// black characters on white background
tft . setTextColor (0x0000 , 0xFFFF ) ;
}
tft.print ( r.name ) ;
tft.print ("\n") ;
}
tft.print ("\n") ;
*/

Serial.end();
return 0;
}
