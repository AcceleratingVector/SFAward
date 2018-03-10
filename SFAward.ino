/*
 *---------------------------------------------------------------------------------
 *  Copyright (c) 2018, Kirk Benell
 *  All rights reserved. 
 *---------------------------------------------------------------------------------
 * 
 * Licensed under the MIT License <http://opensource.org/licenses/MIT>.
 *
 * Permission is hereby  granted, free of charge, to any  person obtaining a copy
 * of this software and associated  documentation files (the "Software"), to deal
 * in the Software  without restriction, including without  limitation the rights
 * to  use, copy,  modify, merge,  publish, distribute,  sublicense, and/or  sell
 * copies  of  the Software,  and  to  permit persons  to  whom  the Software  is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE  IS PROVIDED "AS  IS", WITHOUT WARRANTY  OF ANY KIND,  EXPRESS OR
 * IMPLIED,  INCLUDING BUT  NOT  LIMITED TO  THE  WARRANTIES OF  MERCHANTABILITY,
 * FITNESS FOR  A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN NO EVENT  SHALL THE
 * AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY  CLAIM,  DAMAGES OR  OTHER
 * LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include <SPI.h>  
#include <SFE_MicroOLED.h>  // Include the SFE_MicroOLED library

#include "SFAward.h"


// Include our "show scripting" processor API
#include "showscript.h"

//====================================================================================================
// VERSION:
//
//     1.0    05 March 18   Initial deploy for the first award
//
//====================================================================================================
// OVERVIEW
//====================================================================================================
// Setup to display images across a grid of displays. Current set to 4 oleds.
// 
// Uses an esp32 to drive the screens
//
// The goal is to use the oleds as "screens" that make up an overall display. T
//
// Using sparkfun oled's 64x48px 
//
// Note: Screen Layout
// 
//     |<---------------- One Big Display --------------->| 
//     +--------+    +--------+    +--------+    +--------+   
//     | oled 0 |    | oled 1 |    | oled 2 |    | oled 3 | - name/reference
//     | pos  0 |    | pos  1 |    | pos 2  |    | pos 3  | - screen layout position
//     +--------+    +--------+    +--------+    +--------+   
//     0px     64px  65px    128px 129px   192px 193px   256px
//
// Note:
//   This takes it's general multi-oled use with a esp32 thing inspiration from Alex's OLED clock
//
//====================================================================================================
// IMAGES
// Most of the graphics operations/displays activities is performed using images. The images
// are single bit pixels (one/off) that are encoded as a byte stream. The Y axes/scan lines
// being packed in bytes (8 scan lines per byte). This basically follows the format of a X bitmap 
// (XPM).
//
// The software currently ignores the Y coordinate for location/height and assumes all images are 
// the height of the OLED display (48px). Hacky, but keeps things simple. 
//
// While most examples for the OLED displays use a staticly defined uint8 array which is defined in
// a C header file, this forces  the routines that use an image to hard code it in (unless you write
// a parser). To provide a dynamic method to deploy images, this system uses a simple binary format:
//
//   [width - uint16][height - uint16][packed pixels - uint8]
//
// These files are created from the defined static definitions in a header file using a simple C
// routine on a PC/Mac. 
//
// Image files are currently created using a convoluted prodess that originanted from the instructions
// for the OLED display. The steps are:
//
//      - Create a binary image of what you desire to diplay - Save as a BMP format
//           - N x 48px. (any length - but 48px in height to match height of the OLED screen)
//      - On Windows, use LCD Assistant to convert the image to a static C array 
//          http://en.radzio.dxp.pl/bitmap_converter/
//      - In the resource directory hack writebinarr.c to use the generated header file.
//              - Include Header
//              - Change input/output names (output file name pattern: <image name>.bin)
//              - Compile and execute
//      - Copy the resultant binary file to the data/images directory of this repository
//
// * The goal is distill down these steps into a simple python program.
//
//====================================================================================================
// OPERATIONAL USE
//  
// This system is scripted, allowing dynamic "show" creation outside of firmware modification.
// An award "show" is scripted using a JSON file and associated resources/images. 
//
// The script and associated resources are stored on the SPIFFS file system of the esp32. So you
// need SPIFFS enabled and deployed when you flash the target esp32.
//
// The "show" is defined by an array of "step" objects in a json formatted file called theshow.json. 
// Edit this file to define the overall show for the screens. 
//
// Current show steps have the following typ code:
//  Type Codes:
//        Image Bounce         = 1
//        Image to all Screens = 2
//        Flash the screens    = 3
//        Clear the Screens    = 4
//        Scroll an Image      = 5
//        Wait  (ms)           = 6
//        Image Flash          = 7
//
// For now, see the examples on how to format the json file. Image structure and location are described
// above.
//
// Once a show is defined in a json file, upload the SPIFFs directory to your target esp32 device, 
// flash the board with this routine and reboot the device. 
//
//====================================================================================================
// NOTE
//    Feb 2018 -    The mkspiffs command shipped with the current esp32 Arudino core fails to upload
//                  the SPIFFS for the esp32. The workaround is to download the latest version of 
//                  mkspiffs from espressif, and replace the current Arudino version wih this one.
//                      Latest mkspiffs:
//                         https://github.com/igrr/mkspiffs/releases/tag/0.2.2
//
//                      Install:
//                          <Arudio Directory>/hardware/espressif/esp32/tools/mkspiffs
//
//====================================================================================================
//IO Pin Constants for the screens

#define PIN_DC 22

//Screen 0
#define PIN_RESET_0 2
#define PIN_CS_0    15

//Screen 1
#define PIN_RESET_1 4
#define PIN_CS_1    0

//Screen 2
#define PIN_RESET_2 17
#define PIN_CS_2    16

//Screen 3
#define PIN_RESET_3 12
#define PIN_CS_3    13

// Create an array of OLED screens. Makes it easlier for the code to adapt as screens
// are added/removed

MicroOLED oledScreens[] = {
    MicroOLED(PIN_RESET_0, PIN_DC, PIN_CS_0),
    MicroOLED(PIN_RESET_1, PIN_DC, PIN_CS_1),
    MicroOLED(PIN_RESET_2, PIN_DC, PIN_CS_2),
    MicroOLED(PIN_RESET_3, PIN_DC, PIN_CS_3)    
};

int N_SCREENS = sizeof(oledScreens)/sizeof(MicroOLED);

int SCREEN_WIDTH = oledScreens[0].getLCDWidth(); // 64px
int SCREEN_HEIGHT = oledScreens[0].getLCDHeight(); // 48px 

#define DISPLAY_WIDTH (SCREEN_WIDTH * N_SCREENS)

#define ANIMATION_DELAY 10

//====================================================================================================
// Screen Level Routies
//====================================================================================================
// drawRectToScreen()
//
//   Draws a rect provided in display coords to the input screen

// KDB NOTE (March 2018):
//   Could not get this to work across multiple screens. Appears to be cross talk or 
//   I'm probablly missing something

void drawRectToScreen(MicroOLED &oled, int oledNumber, uint8_t x, uint8_t y, uint8_t width, uint8_t height){

    int x0Oled = oledNumber * SCREEN_WIDTH;
  
    // is this line on this screen?
    if(x >= x0Oled + SCREEN_WIDTH  || x + width < x0Oled) {
      oled.clear(PAGE);
      oled.display();
      return;
  }

  int xScr;
  if(x <= x0Oled)
    xScr = 0;
  else
    xScr = x - x0Oled;

  int widthScr = x+ width - x0Oled - xScr; 
  if(widthScr >  SCREEN_WIDTH){
     widthScr = SCREEN_WIDTH- xScr;
  }
  Log.verbose("[drawRectToScreen] Screen: %d, X: %d \t Width: %d" CR, oledNumber, xScr, widthScr);

  oled.rectFill(xScr, y, widthScr, height);
  oled.display();
  delay(10);

}
//====================================================================================================
// drawImageToScreen()
//
// Draws an image at the give location (assumes y =0) on the input screen. Location is in display coords.
//
// Taking into account scroll x location and the screen number, copy the correct bytes over from
// the source image to the given displays screen buffer.
//
void drawImageToScreen(MicroOLED &oled, int oledNumber, uint8_t *pImage, int imWidth, int xOffset){

    int x0Oled = oledNumber * SCREEN_WIDTH;

    // if the image is not on this creen, we just clear the display at this point and leave
     if(xOffset >= x0Oled + SCREEN_WIDTH  || xOffset + imWidth < x0Oled) {
        oled.clear(PAGE);
        oled.display();
        return;
    }

    //Find the x0 point on the screen (dest) 
    int xDst0 = ( xOffset < x0Oled ? 0 : xOffset - x0Oled);
  
    // okay, what is the x0 point of our source
    int xSrc0 =  (xOffset < x0Oled ?  x0Oled - xOffset : 0);

    // How many bytes to copy over? Start by finding  how many X pixels to right of dest X0
    // x0 of image is >= Screen origin?
    int nXCopy;
    if(xOffset > x0Oled){
        // number of pixels on screen
        nXCopy = SCREEN_WIDTH - xOffset + x0Oled; 
    }else{
        // x0Img is less than x0 for the screen - find out how much of the image is on the screen
        nXCopy =  x0Oled - xOffset + imWidth;
    }
    if(nXCopy > SCREEN_WIDTH) // if image  width is larger than screen 
        nXCopy = SCREEN_WIDTH;

    if(nXCopy > imWidth - xSrc0) // don't exceed the image width for the copy
        nXCopy = imWidth - xSrc0;
  
    // clear out the screen buffer if image doesnt fill up screen
    uint8_t * pScreenBuffer = oled.getScreenBuffer();
    if(nXCopy != SCREEN_WIDTH || xDst0 > 0)
        memset((void*)pScreenBuffer, 0, sizeof(uint8_t)*SCREEN_HEIGHT*SCREEN_WIDTH/8);

    // copy over the source image bytes, place in screen buffer - buffer line by line 
    // NOTE: Assuming Y is the full screen (or 6 buffer lines. )
    for(int i=0; i < 6; i++){
        memcpy(pScreenBuffer + (i * SCREEN_WIDTH + xDst0)*sizeof(uint8_t),  // offset to "scan line" + Dest X offset
                    pImage + (i * imWidth + xSrc0)*sizeof(uint8_t),         // Select scan line of source image + source offset.
                    sizeof(uint8_t)*nXCopy);                                // Number of bytes to copy from src-to-dest
    }
    // Display the result
    oled.display();

    // Debug Summary - comment out 
    //Log.verbose( "[drawImageToScreen] Screen: %d, x0Src: %d, x0Dest: %d, nCopy: %d, xOffset: %d" CR, 
    //       oledNumber, xSrc0, xDst0, nXCopy, xOffset);

}

//****************************************************************************************************
// displayImage
//
// Displays the image at an offset across our grid of displays
//
void displayImage(Image_t *pImage, int x0){

    for(int i=0; i < N_SCREENS; i++){
      drawImageToScreen(oledScreens[i], N_SCREENS-1-i, pImage->pData, pImage->width, x0 );
    }

}
//****************************************************************************************************
// drawRect
void drawRect(uint8_t x, uint8_t y, uint8_t width, uint8_t height){

    Log.verbose( "[drawRect] (%d, %d, %d, %d) " CR, x, y, width, height);

    for(int i=0; i < N_SCREENS; i++){
      drawRectToScreen(oledScreens[i], N_SCREENS-1-i, x, y, width, height);
    }

}
//****************************************************************************************************
// imageToAllScreens()
//
// This puts the same image on each screen.
//
// Assumes (for now) the image is the size of the screen
void imageToAllScreens(Image_t * pImage){

    for(int i=0; i < N_SCREENS; i++){
      oledScreens[i].drawBitmap(pImage->pData);
      oledScreens[i].display();
    }

}
//****************************************************************************************************
// clearDisplay()
//
// Erase/clear the screens in the display.

void clearDisplay(){

    for(int i=0; i < N_SCREENS; i++) {  
        oledScreens[i].clear(ALL);
        oledScreens[i].display();
    }        

}
//****************************************************************************************************
// invert display
void invertDisplay(bool bInvert){

    for(int i=0; i < N_SCREENS; i++) {  
        oledScreens[i].invert( bInvert);  // mod is mod
        oledScreens[i].display();
    }
}

//****************************************************************************************************
// flashDisplay()
//
// Invert the display n times - causing a "flashy" graphic
void flashDisplay(int nTimes){

    for(nTimes = nTimes*2; nTimes > 0; nTimes--){
        invertDisplay( (nTimes+1) % 2);  // mod is mod
        delay(180);    

    }
}
//****************************************************************************************************
// flashImageDisplay()
//
// Flashes the screen with the dispalyed image.
//
// Implemented this b/c just inverting the screen caused issues in this setup (same image rect on each screen)

void flashImageDisplay(int nTimes, Image_t *pImage, int xOffset){
    
    for(nTimes = nTimes*2  ; nTimes > 0 ; nTimes--){
        invertDisplay((nTimes+1) % 2);
        displayImage(pImage, xOffset);
        delay(200);
    }
}
//****************************************************************************************************
// scrollBanner()
//
// Scroll a banner image across virutal display

void scrollBanner(Image_t *pImage, bool doInvert){


    unsigned long ticktock = millis();
    bool invert=doInvert;

    // Scroll until the image rolls off the left edge of display
    for(int xScroll = DISPLAY_WIDTH; xScroll + pImage->width >= 0; xScroll = xScroll-2) {

        displayImage(pImage, xScroll);


        if(doInvert && millis() - ticktock > 240){
            invert = !invert;
            invertDisplay(invert);
            ticktock = millis();

        }
        delay(ANIMATION_DELAY/3);        
    }
    invertDisplay(false);
}

//****************************************************************************************************
// bounceImage()
//
// Scroll an image across virutal display - back and forth n times
//
// For now, this assumes the image width is less than the display width. 

void bounceImage(Image_t *pImage, int nBounce){


    int iBounce;
    int xScroll = DISPLAY_WIDTH;

    for(int iBounce=0; iBounce < nBounce; iBounce++){

        for(; xScroll >= 0; xScroll = xScroll-2){
            displayImage(pImage, xScroll);
            delay(ANIMATION_DELAY/4);
        }

        for(; xScroll < DISPLAY_WIDTH-pImage->width; xScroll = xScroll+2 ){
            displayImage(pImage, xScroll);
            delay(ANIMATION_DELAY/4);
        }


    }

}
//****************************************************************************************************
// fillDisplay with a linear pattern - simple 
void fillDisplayLinearXY(){

    int inc=2;

    uint8_t x, y, width, height;

    x = SCREEN_WIDTH/2 - inc;
    width = 0;

    y=SCREEN_HEIGHT/2-inc;
    height = inc*2;

    for(; x > 0; x=x-inc){
        width = width + inc*2;

        height = height + inc*2;
        if(height > SCREEN_HEIGHT){
            height = SCREEN_HEIGHT;
        }
        
        y = y - (inc*2);
        if(y < 0) y =0;


        for(int i=0; i < N_SCREENS; i++) {  
            oledScreens[i].clear(PAGE);
            oledScreens[i].rectFill(x, y, width, height);
            oledScreens[i].display();
        }
        delay(100);

    }


}
//****************************************************************************************************
// LIFE CYCLE
//****************************************************************************************************
// Arduino setup

void setup() {

    // Serial setup
    Serial.begin(115200);
    while(!Serial && !Serial.available()){}
    Serial.println(""); // start fresh

    // Log setup
    Log.begin(LOG_LEVEL_VERBOSE, &Serial);

    // Start our log
    Log.notice("+------------------------------------------+" CR);
    Log.notice("|          ESP32 Award Display             |" CR);
    Log.notice("+------------------------------------------+" CR);    


    // setup our screens
    for(int i=0; i < N_SCREENS; i++){
        oledScreens[i].begin();
        oledScreens[i].clear(PAGE);
    }

    if(!initShow() ){
        Log.error("Unable to access the award show description file.");
        Log.fatal("Unable to continue.");
        abort();
    }
}
//****************************************************************************************************
// Arduino Loop()
//
//   
void loop() {

    // Get the list of steps that define the show and process them.

    ShowStepList_t *ptheShow = getTheShow();
    ShowStep_t *pStep;
    Image_t *pImage;    

    // Process the steps in the show!
    for(int i=0; i < ptheShow->count();  i++){

        pStep = ptheShow->get(i);

        // do we have an image?
        if(pStep->strImage != NULL){
            pImage = getImage(pStep->strImage);
            if(!pImage){
                Log.error("[LOOP] Unable to retrieve a step image %s" CR, pStep->strImage);
                Log.error("    Was the image %s.bin deployed in the data/images directory of this device?" CR,
                    pStep->strImage);
                Log.error("Skipping this step" CR);

                continue;
            }
        }
        switch(pStep->type){

            case stepImageBounce:
                bounceImage(pImage, pStep->count);
                break;
            case stepImageToAll:
                imageToAllScreens(pImage);
                break;            
            case stepFlash:
                flashDisplay(pStep->count);
                break;
            case stepWait:
                delay(pStep->count);
                break;
            case stepClear:
                clearDisplay();
                break;
            case stepImageFlash:
                flashImageDisplay(pStep->count, pImage, pStep->offset);
                break;
            case stepImageScroll:
                scrollBanner(pImage, false); 
                break;                        
            default:
                Log.warning("[LOOP] - Unknown step type: %d" CR, pStep->type);
                break;
        }//switch
    } // for

    delay(5000);    

}



