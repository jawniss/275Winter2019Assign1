/*
	Display a scrollable list of names on the screen.
*/

#include <Arduino.h>
#include <Adafruit_ILI9341.h>

#define TFT_DC 9
#define TFT_CS 10
#define SD_CS 6

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

#define DISPLAY_WIDTH  320
#define DISPLAY_HEIGHT 240

#define JOY_VERT  A1
#define JOY_HORIZ A0
#define JOY_SEL   2

#define JOY_CENTER   512
#define JOY_DEADZONE 64

#define NUM_NAMES 18

const char *stringsToDisplay[] = {
	"Zac", "Omid", "Veronica", "Zach", "Jason", "Joseph", "Arseniy",
	"Daniel", "Mohammad", "Parash", "Everton", "Touqir", "Logan",
	"A Very Long Name That Should Not Wrap Around", "A","B","C","D"
  ,"E","F","G" // extra names
};

uint16_t highlightedString; // indexes the above const char strings to display

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

// draws the name at the given index to the display
// asssumes the etext size is already 2 and that texts//
// is not wrapping and 0<= index < number of names in the list
void drawName(uint16_t index){
  tft.setCursor(0,index*15); // should be on the left each word is 8 bit high (its in the adafruit graphicks library) we want size 2 so 7*2 +1 which is white space 1

  if(index == highlightedString){
    tft.setTextColor(ILI9341_BLACK, ILI9341_WHITE); // black with white background
  }else {
    tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK); // black with white background
  }
  tft.println(stringsToDisplay[index]);
}
// refresh the display and dispay all names
// do not change the highlighred stirng
void displayAllNames(){
  tft.fillScreen(ILI9341_BLACK); // fill it in black

  for (uint16_t i=0; i< NUM_NAMES; i++){ // loop thorugh all names and draw them Note it can only draw 15 names on the screen

    drawName(i);
  }
}

// // function that will handle all movement of the joystick and use its inputs
// void processJoystick() {
//
//   //int xVal = analogRead(JOY_HORIZ);
//   int yVal = analogRead(JOY_VERT);
//   int buttonVal = digitalRead(JOY_SEL);
//
//   // loop that stops the code from passing through unless a valid tilt is read
//   // from the joystick
//   while (true){
//     // reread the joystick everytime to check if a valid tilt is inputed
//     //int xVal = analogRead(JOY_HORIZ);
//     int yVal = analogRead(JOY_VERT);
//     int buttonVal = digitalRead(JOY_SEL);
//     }
//   }


int main() {
	setup();

  displayAllNames();
  // drawName(0);
  // drawName(1);
  //
  // drawName(3);
  // highlightedString = 6;
  // drawName(6); //highlight the 6th one and draw them too
  //
  // delay(3000);
  //
  // drawName(0);

  // loop forever and cycle through the name
  while (true) {
    // if statement for if its on the 16- blank names go to a different function that draws the next few names

    // reread the joystick everytime to check if a valid tilt is inputed
    // int xVal = analogRead(JOY_HORIZ);
    int yVal = analogRead(JOY_VERT);
    int buttonVal = digitalRead(JOY_SEL);
    uint16_t prevHighlight = highlightedString;
    if (yVal >= (JOY_CENTER + JOY_DEADZONE)){ // this is to move down

      highlightedString = (highlightedString+1)%NUM_NAMES; //highlighted one will be lower one since you pushed down
      drawName(prevHighlight);
      drawName(highlightedString);

    } else if (yVal <= (JOY_CENTER - JOY_DEADZONE) ){ // this is move up
      highlightedString = (highlightedString-1)%NUM_NAMES;
      drawName(prevHighlight);
      drawName(highlightedString);
    }
    Serial.print("this is the current name");
    Serial.println(highlightedString);
    delay (50);


    // Change the highlighted names with joysticks (done) (even works where if you go pass the top you go to bot and vice versa)
    // add more names than can be dipslayed on one screen, and
    // go to the " next page" of names if you select far enough down
  }
	Serial.end();

	return 0;
}
