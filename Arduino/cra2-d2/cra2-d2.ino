#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>
#include <Fonts/TomThumb.h>
#include <Wire.h>               // For I2C communication; receive commands from Padawan360


// ToDo:
// Code for PSIs
// Code for Holoprojectors (maybe)
// Some effects
// Receive proper commands from Padawan360


// The sketch is set up for receiving I2C signas from Padawan 360 with address 10
// This is set up the same as the Teeces lights, and so it shoyuld be (somewhat) interchangeable.


/******************************************************************************************/
/************** SOME VARIABLES YOU CAN PLAY WITH/ADJUST FOR VARIOUS EFFECTS ***************/
/******************************************************************************************/


// Set loopTEST to true to keep looping displaytest infinitely. Useful for testing LEDs wile building
// NB! Will loop only the FTLD display (default pin 6)
#define loopTEST false

// Define pins used for the various display strings:
#define pinFTLD 6   // Pin for Front Top Logic Display strand
#define pinFBLD 5   // Pin for Front Bottom Logic Display strand
#define pinRLD  4   // Pin for Rear Logic Display strand
#define pinFPSI 3   // Pin for front PSI
#define pinRPSI 2   // Pin for rear  PSI

// Define the number of LEDs in each display
#define LEDcountFTLD 50
#define LEDcountFBLD 50
#define LEDcountRLD 150
#define LEDcountFPSI 12
#define LEDcountRPSI 12

// Some variables to set animation speed for the RGB test:
#define RGBtestAnimationSpeed 10        // Set to 0 to skip RGB test at startup
#define RGBtestDelayBetweenColors 250


// Define default text for scroll display. Used once during startup procedure (after RGB-test)
String displayTextFTLD = "R2-D2";
String displayTextFBLD = "          POWERING UP";
String displayTextRLD  = "BY CHRISTIAN RAMSVIK";

// Some variables to adjust misc speeds etc
byte brightness = 24;   // Overall display brightness default value
int lingerON  = 1800;    // Factor for how long to keep LED on MAX when in random mode. Increase to keep LEDs at max longer.
int lingerOFF = 500;    // Factor for how long to keep LED off when in random mode. Increase to keep LEDs off longer.
int textSpeed = 30;    // A delay-factor adjusting the scrolling speed of the text


// This is where you define the matrix for each screen. If you hooked your LED chains differently than
// what is default (look in the instruction PDF) you will need to make adjustments
// to the second line on each matrix definition

// MATRIX DECLARATION:
// Parameter 1 = width of NeoPixel matrix
// Parameter 2 = height of matrix
// Parameter 3 = pin number (most are valid)
// Parameter 4 = matrix layout flags, add together as needed:
//   NEO_MATRIX_TOP, NEO_MATRIX_BOTTOM, NEO_MATRIX_LEFT, NEO_MATRIX_RIGHT:
//     Position of the FIRST LED in the matrix; pick two, e.g.
//     NEO_MATRIX_TOP + NEO_MATRIX_LEFT for the top-left corner.
//   NEO_MATRIX_ROWS, NEO_MATRIX_COLUMNS: LEDs are arranged in horizontal
//     rows or in vertical columns, respectively; pick one or the other.
//   NEO_MATRIX_PROGRESSIVE, NEO_MATRIX_ZIGZAG: all rows/columns proceed
//     in the same order, or alternate lines reverse direction; pick one.
//   See example below for these values in action.
// Parameter 5 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)

// Define NeoMatrix strand for Front Top Logic Display
Adafruit_NeoMatrix matrixFTLD = Adafruit_NeoMatrix(10, 5, pinFTLD,
  NEO_MATRIX_TOP     + NEO_MATRIX_LEFT +
  NEO_MATRIX_ROWS    + NEO_MATRIX_PROGRESSIVE,
  NEO_RGB            + NEO_KHZ800);

// Define NeoMatrix strand for Front Bottom Logic Display
Adafruit_NeoMatrix matrixFBLD = Adafruit_NeoMatrix(10, 5, pinFBLD,
  NEO_MATRIX_TOP     + NEO_MATRIX_RIGHT +
  NEO_MATRIX_ROWS    + NEO_MATRIX_ZIGZAG,
  NEO_RGB            + NEO_KHZ800);

// Define NeoMatrix strand for Rear Logic Display
Adafruit_NeoMatrix matrixRLD = Adafruit_NeoMatrix(30, 5, pinRLD,
  NEO_MATRIX_TOP     + NEO_MATRIX_LEFT +
  NEO_MATRIX_ROWS    + NEO_MATRIX_PROGRESSIVE,
  NEO_RGB            + NEO_KHZ800);

// Define NeoMatrix strand for Front PSI
Adafruit_NeoMatrix matrixFPSI = Adafruit_NeoMatrix(2, 6, pinFPSI,
  NEO_MATRIX_TOP     + NEO_MATRIX_LEFT +
  NEO_MATRIX_ROWS    + NEO_MATRIX_PROGRESSIVE,
  NEO_RGB            + NEO_KHZ800);


// Define NeoMatrix strand for Rear PSI
Adafruit_NeoMatrix matrixRPSI = Adafruit_NeoMatrix(2, 6, pinRPSI,
  NEO_MATRIX_TOP     + NEO_MATRIX_LEFT +
  NEO_MATRIX_ROWS    + NEO_MATRIX_PROGRESSIVE,
  NEO_RGB            + NEO_KHZ800);


/******************************************************************************************/
/******* DON'T CHANGE ANYTHING BELOW THIS POINT UNLESS YOU KNOW WHAT YOU ARE DOING ********/
/******************************************************************************************/


// Define action names for cleaner code:
#define doRANDOM 1
#define doSCROLL 2
#define doGRAPHS 3


// Define the variables that keeps track of what each display is doing and set default value:
byte actionFTLD = 1;
byte actionFBLD = 1;
byte actionRLD  = 1;


// Define an array of bytes to keep info about each LED. One array pr logic display with total number of LEDs
// Stores info about color, intensity and direction of fade (up/down)
byte rndFTLD[LEDcountFTLD] = {};
byte rndFBLD[LEDcountFBLD] = {};
byte rndRLD[LEDcountRLD]   = {};
byte rndFPSI[LEDcountFPSI]  = {};
byte rndRPSI[LEDcountRPSI]  = {};

byte barGraphRANDOMIZER[30]={2,0,2,1,1,3,0,1,3,3,2,1,3,0,1,3,2,1,3,0,1,2,2,0,1,3,3,3,0,1};


// This is what the bits are currently used for in ieach byte:
// BIT  PURPOSE
// 0    Gradient
// 1    Gradient
// 2    Gradient
// 3    Gradient
// 4    Linger    Linger is handled in separate random function. Use for more fade levels instead?  
// 5    Linger    Linger is handled in separate random function. Use for more colors instead?
// 6    Color (0 blue, 1 white)
// 7    0 falling, 1 rising

// Variables for keeping track of milliseconds. Relates to timing.
unsigned long lastScroll = 0;
unsigned long lastRandom = 0;
unsigned long currentMillis; // running clock reference



// Variables for text scrolling:
int FTLDX = matrixFTLD.width();
int FBLDX = matrixFBLD.width();
int RLDX  = matrixRLD.width();




#define colorBLUE  0
#define colorWHITE 1
#define colorRED   2
#define colorAMBER 3
#define colorGREEN 4


// Color scheme for random display. Default blue and white
// Defined as separate bytes in order to calculate intensity for fade-effect.
// Need separate function for rear display to have 3 colors.
const byte predefinedColorList[][3] = {{0,0,255},     // Blue   0
                                      {255,255,255},  // White  1
                                      {255,0,0},      // Red    2
                                      {255,191,0},    // Amber  3
                                      {0,179,30},     // Green  4
                                     };  



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


// Calculates the next state of each pixel stored in an array of bytes
void randomizePixelBYTE(byte &b, byte colorBits){
  // ToDo: parameter for colors to use. (rear LD needs 3 colors)
  // ColorBits is a flag-byte allowing up to 8 colors.
 // Serial.print(":");
//Check endpoints:
// .1..1111 = At max. Set to falling
// .0..0000 = At min. Set to rising (Randomize color)
// Check if LED is at MAX:
if((b&143)==143){   // LED is rising and has reached MAX
  // Add randomizer to create delay. Spend linger-value first
  if(random(0,lingerON)==0){bitClear(b,7);}   // If rare number 0 met, flip bit to start fading out
  return;
}
// Check if LED is at ZERO and ready for recycle:
if(b&143){
  //Serial.println("LED at 0 intensity. Recycle");
}else{
  ///CRASerial.print(" AT0 ");
    if(random(0,lingerOFF)==0){   // If rare number 0 is hit restart  this LED
      b = (random(0,2)*16) + 128;// + (random(0,1)*32) + (random(0,1)*16) ;//Skip setting random linger. Handled outside byte (LED has no stored linger)
    }else{
      return; // Random number 0 was NOT hit. Exit; LED stays off until next iteration.
    }
}

  // Increase or decrease INTENSITY based on bit 7
  bitRead(b,7)?b++:b--;
//  printBinary(b);
//  Serial.println("-");
}



/* ************************************************************************************************
 * Takes a bytelist (array of pixel definitions) and displays it on the selected display
 * 
 * LEDdefinition[]             An array of bytes. Each byte represents a LED, and each of the bytes bits defines the LED status (color, intensity, rising/falling). Used to set color in each pixel
 * nBytesInArray          The number of bytes (LEDs) in array bytelist[]. Cannot be calculated inside function.
 * matrixToUpdate         The NeoMatrix object. Used for calling functions for updating and displaying the actual LEDs
 * predefinedColorLists   An list (array) of colors, each defined by three bytes (RGB)
 * columnsX               The number of columns in (the width of) the display.
 */
void updateDisplay(byte LEDdefinition[], byte nBytesInArray, Adafruit_NeoMatrix &matrixToUpdate, byte columnsX){
   // Loop through each element in the array
//   Serial.println(predefinedColorList[bitRead(LEDdefinition[1],6)][2]/15)*(LEDdefinition[1] & 15);
  for (int i=0; i<nBytesInArray; i++) {
    //Serial.println(predefinedColorList[bitRead(LEDdefinition[i],6)][0]/15)*(LEDdefinition[i] & 15 );
//     printBinary(LEDdefinition[i]);
//     Serial.print(" - ");
//     printBinary(LEDdefinition[i]>>4);
//     Serial.print(" - ");
//     Serial.println(getColorFromByte(LEDdefinition[i]));
    // Write each pixel to the matrix  
    matrixToUpdate.writePixel((i%columnsX), (i/columnsX),  
              // TODO: Better color handling than just two colors (currently using only one bit)
                              matrixFTLD.Color(
                                (predefinedColorList[getColorFromByte(LEDdefinition[i])][0]/15)*(LEDdefinition[i] & 15 ), 
                                (predefinedColorList[getColorFromByte(LEDdefinition[i])][1]/15)*(LEDdefinition[i] & 15 ), 
                                (predefinedColorList[getColorFromByte(LEDdefinition[i])][2]/15)*(LEDdefinition[i] & 15 )
                                ));
//                              matrixFTLD.Color(
//                                (predefinedColorList[bitRead(LEDdefinition[i],6)][0]/15)*(LEDdefinition[i] & 15 ), 
//                                (predefinedColorList[bitRead(LEDdefinition[i],6)][1]/15)*(LEDdefinition[i] & 15 ), 
//                                (predefinedColorList[bitRead(LEDdefinition[i],6)][2]/15)*(LEDdefinition[i] & 15 )
//                                ));
//  predefinedColorList[bitRead(bytelist[i],6)]);
//  matrixToUpdate.writePixel((i%10), (i/10),  predefinedColorList[bitRead(bytelist[i],6)]);
  }
  // Refresh the display with the newly updated matrix values (push the matrix to the display):
  matrixToUpdate.show();
/**/
}

/* ************************************************************************************************
 * Helper function. Returns matrix.Color based on separate bytes (for calculations)
 */
uint16_t byteColor(byte predefinedColorLists[][3], byte colorNumber){
  return matrixFTLD.Color(predefinedColorList[colorNumber][0], predefinedColorList[colorNumber][1], predefinedColorList[colorNumber][2]);
}

void ScrollText(Adafruit_NeoMatrix &displayToScroll, String textToScroll, byte colorIndex, int &positionCounter, bool loopDisplay = true ){
  displayToScroll.fillScreen(0);    //Turn off all the LEDs
displayToScroll.setTextColor(byteColor(predefinedColorList, colorIndex));
  displayToScroll.setCursor(positionCounter, 5);
  displayToScroll.print(textToScroll);
  if(currentMillis - lastScroll >= textSpeed){
  if( --positionCounter < int(textToScroll.length() * -4)) { // Text outside display. Loop around. Each char is 4 wide (spaces are smaller)
    // TODO: Return to doing random lights if scrolltext should not loop. Need to know whicl display to "reset".
    (loopDisplay)?positionCounter = displayToScroll.width():false;
  }
  lastScroll = millis();

  }
  displayToScroll.show();
}

/* ************************************************************************************************
 * Wrapper function that randomizes each led and adjust intensity for fade in/out effect then calls updateDisplay
 * 
 * LEDdefinition[]             An array of bytes. Each byte represents a LED, and each of the bytes bits defines the LED status (color, intensity, rising/falling). Used to set color in each pixel
 * nBytesInArray          The number of bytes (LEDs) in array bytelist[]. Cannot be calculated inside function.
 * matrixToUpdate         The NeoMatrix object. Used for calling functions for updating and displaying the actual LEDs
 * predefinedColorLists   An list (array) of colors, each defined by three bytes (RGB)
 * columnsX               The number of columns in (the width of) the display.
 */
void randomDisplay(byte LEDdefinition[], byte nBytesInArray, Adafruit_NeoMatrix &matrixToUpdate, byte columnsX, byte colorListForPixel=3){
// wrapper som gjør randomizing.
//Serial.println("...");
//Serial.println(nBytesInArray);
  for (int i=0; i<nBytesInArray; i++) {
    randomizePixelBYTE(LEDdefinition[i], 3);
  }
//  Serial.println("Call UD");
  updateDisplay( LEDdefinition, nBytesInArray, matrixToUpdate, columnsX);
//  Serial.println("UD returned");
}

void barGraph(Adafruit_NeoMatrix &matrixToUpdate, byte columns, byte LEDdefinition[]){
  // Define height each column
  for(byte i=0;i<columns;i++){
    // Randomize height
    barGraphRANDOMIZER[i]+=random(-1,2);
    (barGraphRANDOMIZER[i]<0)?barGraphRANDOMIZER[i]=0:false;
    (barGraphRANDOMIZER[i]>5)?barGraphRANDOMIZER[i]=5:false;
//  Serial.print(column[i]);
  //delay(100);
  }
 // Serial.println();
  // Generate rows:
  for(int x=0;x<columns;x++){
    for(int y=0;y<5;y++){
//      LEDdefinition[(y*10)+x]&(column[x]/y);
      //LEDdefinition[(y*10)+x]=LEDdefinition[(y*10)+x]|15;
      // The (15-y) part makes the higher LEDs slightly dimmer than the lower. Bitshift twice to clear rightmost bits
      (y>=barGraphRANDOMIZER[x])?LEDdefinition[(y*10)+x]=LEDdefinition[(y*10)+x]|(15-y) : LEDdefinition[(y*10)+x]=LEDdefinition[(y*10)+x]>>4<<4;
      // Set top LED to different color than the lower ones:
      (y==barGraphRANDOMIZER[x])?bitSet(LEDdefinition[(y*10)+x],4):bitClear(LEDdefinition[(y*10)+x],4);
    }
  }
  // Bars are recalculated. Update display.
  ////delay(13);
  updateDisplay(LEDdefinition, 5*columns, matrixToUpdate, columns);
}

void receiveEvent(int howMany)
{
  Serial.print("Event received");
  while(1 < Wire.available()) // loop through all but the last
  {
    char c = Wire.read(); // receive byte as a character
    Serial.print(c);         // print the character
  }
  int x = Wire.read();    // receive byte as an integer
  Serial.println(x);         // print the integer
  // Switch for different event codes to change stuff around
  switch( x ){
    case 0:
      // Random
      break;
    case 1:
      // Alarm
      break;
    case 4:
      // ???
      break;
    case 5:
      // Leia msg
      break;
    case 6:
      // System failure
      break;
    case 11:
      // Alarm 2
      displayTextFTLD="cra event";
    break;
    case 10:
      displayTextFTLD="TEST1";
      break;
    case 21:
      // Lowest speed drive
      break;
    case 22:
      // Medium speed drive
      break;
    case 23:
      // Max speed drive
      break;
    case 24:
      // Brightness down
      break;
    case 25:
      // Brightness up
      break;
  }
}




void setup() {

// Prepare for debug output
Serial.begin(115200);
///CRASerial.println("Starting scrolltest1");
//Serial.println("Started");
  Wire.begin(10);                // join i2c bus with address #10. This is default for lights in Padawan 360
  Wire.onReceive(receiveEvent); // register event
 
// Front Top Logic Display setup
  matrixFTLD.begin();
  matrixFTLD.setTextWrap(false);
  matrixFTLD.setBrightness(brightness);
  matrixFTLD.setTextColor(byteColor(predefinedColorList, colorWHITE));
// Front Bottom Logic Display setup
  matrixFBLD.begin();
  matrixFBLD.setTextWrap(false);
  matrixFBLD.setBrightness(brightness);
  matrixFBLD.setTextColor(byteColor(predefinedColorList, colorBLUE));
// Rear Logic Display setup
  matrixRLD.begin();
  matrixRLD.setTextWrap(false);
  matrixRLD.setBrightness(brightness);
  matrixRLD.setTextColor(byteColor(predefinedColorList, colorAMBER));;

// Front PSI setup
  matrixFPSI.begin();
  //matrixFPSI.setTextWrap(false);
  matrixFPSI.setBrightness(brightness);
  matrixFPSI.setTextColor(byteColor(predefinedColorList, colorAMBER));;

// Rear PSI setup
  matrixRPSI.begin();
  //matrixRPSI.setTextWrap(false);
  matrixRPSI.setBrightness(brightness);
  matrixRPSI.setTextColor(byteColor(predefinedColorList, colorAMBER));;

  matrixFTLD.setFont(&TomThumb);
  matrixFBLD.setFont(&TomThumb);
  matrixRLD.setFont(&TomThumb);

// Set startup actions:
actionFTLD = doRANDOM;
actionFBLD = doRANDOM;
actionRLD  = doGRAPHS;

// Run colortest on all displays (skip is set up for testing):
  if(!loopTEST){
//    testRGBdisplay(LEDcountFTLD, matrixFTLD);
//    testRGBdisplay(LEDcountFBLD, matrixFBLD);
//    testRGBdisplay(LEDcountRLD, matrixRLD);
    testRGBdisplay(LEDcountFPSI, matrixFPSI);
    testRGBdisplay(LEDcountRPSI, matrixRPSI);
  }

  // Run initial scroll text once 
} //END setup()



void loop() {

if(loopTEST){
  // if loopTEST is true then loop the RGB test for FTLD display for testing
    testRGBdisplay(LEDcountFTLD, matrixFTLD);
  }else{
  
  currentMillis = millis();

  if(actionFTLD == doRANDOM) randomDisplay(rndFTLD, sizeof(rndFTLD) / sizeof(rndFTLD[0]), matrixFTLD, 10);
  if(actionFTLD == doSCROLL) ScrollText(matrixFTLD, displayTextFTLD, colorAMBER, FTLDX);
  if(actionFTLD == doGRAPHS) barGraph(matrixFTLD, 10, rndFTLD);

  if(actionFBLD == doRANDOM) randomDisplay(rndFBLD, sizeof(rndFBLD) / sizeof(rndFBLD[0]), matrixFBLD, 10);
  if(actionFBLD == doSCROLL) ScrollText(matrixFBLD, displayTextFBLD, colorAMBER, FBLDX);
  if(actionFBLD == doGRAPHS) barGraph(matrixFBLD, 10, rndFBLD);

  if(actionRLD==doRANDOM) randomDisplay(rndRLD, sizeof(rndRLD) / sizeof(rndRLD[0]), matrixRLD, 30);
  if(actionRLD==doSCROLL) ScrollText(matrixRLD, displayTextRLD, colorAMBER, RLDX);
  if(actionRLD==doGRAPHS) barGraph(matrixRLD, 30, rndRLD);

// Testing FPSI:
//randomDisplay(rndFPSI, sizeof(rndFPSI) / sizeof(rndFPSI[0]), matrixFPSI, 6);
//setPSI(rndFPSI, matrixFPSI);

//Serial.print(actionFTLD);
  
/*
Serial.print("ACTIONS   FTLD / FBLD / RLD    ");
Serial.print(actionFTLD);
Serial.print(" / ");
Serial.print(actionFBLD);
Serial.print(" / ");
Serial.println(actionRLD);
*/


//randomDisplay(rndFTLD, sizeof(rndFTLD) / sizeof(rndFTLD[0]), matrixFTLD, 10);
//randomDisplay(rndFBLD, sizeof(rndFBLD) / sizeof(rndFBLD[0]), matrixFBLD, 10);
////randomDisplay(rndRLD, sizeof(rndRLD) / sizeof(rndRLD[0]), matrixRLD, 30);

////barGraph(matrixFBLD, 10, rndFBLD);

/*
    matrixFBLD.fillScreen(LED_RED_MEDIUM);
    matrixFBLD.show();
    delay(1000);
    matrixFBLD.clear();
*/

//testVAL = testVAL + 1;
//Serial.println(testVAL);
//Serial.println("m");
 //ScrollText(matrixFTLD, displayTextFTLD, colorAMBER, FTLDX);
// ScrollText(matrixFBLD, "FUNCTION TEST", colorGREEN, FBLDX);
// ScrollText(matrixRLD, "FUNCTION test",  colorWHITE, RLDX);

  }// End loopTEST false condition (do everything else, not just the RGB-test)
}//END loop()


// Helper function used for serial output during testing
void printBinary(byte inByte)
{
  for (int b = 7; b >= 0; b--)
  {
    Serial.print(bitRead(inByte, b));
  }
}/**/



void testRGBdisplay(byte nBytesInArray, Adafruit_NeoMatrix &matrixToUpdate){
  // ToDo: Get number of rows/columns from somewhere isntead of assuming 5 rows. Not true for PSI displays
  // Run all LEDs RED
  if(RGBtestAnimationSpeed==0)return; // Speed set to 0, don't do animation at all
  for(int i=0;i<nBytesInArray;i++){
    // Set color of each LED and update display to make animated effect
      matrixToUpdate.writePixel((i%(nBytesInArray/5)), (i/(nBytesInArray/5)), matrixFTLD.Color(255, 0, 0));
      delay(RGBtestAnimationSpeed);
      matrixToUpdate.show(); 
  }
  delay(RGBtestDelayBetweenColors);
  // Run all LEDs GREEN
  for(int i=0;i<nBytesInArray;i++){
    // Set color of each LED and update display to make animated effect
      matrixToUpdate.writePixel((i%(nBytesInArray/5)), (i/(nBytesInArray/5)), matrixFTLD.Color(0, 255, 0));
      delay(RGBtestAnimationSpeed);
      matrixToUpdate.show(); 
  }
  delay(RGBtestDelayBetweenColors);
  // Run all LEDs BLUE
  for(int i=0;i<nBytesInArray;i++){
    // Set color of each LED and update display to make animated effect
      matrixToUpdate.writePixel((i%(nBytesInArray/5)), (i/(nBytesInArray/5)), matrixFTLD.Color(0, 0, 255));
      delay(RGBtestAnimationSpeed);
      matrixToUpdate.show(); 
  }
  delay(RGBtestDelayBetweenColors);
    // Run all LEDs OFF
  for(int i=0;i<nBytesInArray;i++){
    // Set color of each LED and update display to make animated effect
      matrixToUpdate.writePixel((i%(nBytesInArray/5)), (i/(nBytesInArray/5)), matrixFTLD.Color(0, 0, 0));
      delay(RGBtestAnimationSpeed);
      matrixToUpdate.show(); 
  }
  delay(RGBtestDelayBetweenColors);
}

void setPSI(byte LEDdefinition[], Adafruit_NeoMatrix &matrixToUpdate){
  for(int i=0;i<12;i++){
    LEDdefinition[i]=79;
  }
  updateDisplay(LEDdefinition,  12, matrixToUpdate,  2);
}

byte getColorFromByte(byte LEDdefinition){
  //11110000
  
  return (LEDdefinition>>4)&7;
}
