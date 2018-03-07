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

#include <FS.h>
#include <SPIFFS.h>
#include <string.h>

#include "esp32Award.h"
#include "showscript.h"




#define SHOW_FILE "/theshow.json"


// For the image files database...
#define IMAGE_DIRECTORY "/images"
#define IMAGE_SUFFIX ".bin"


// Just using a simple linked list to store our images. We shouldn't have a large number
typedef SimpleList<Image_t*> ImageList_t; 

static ImageList_t  s_availableImages;

static ShowStepList_t s_theShow;

//==========================================================================
// getImage()
//
// Return the image associated with a given name.

Image_t *getImage(String strName){

	Image_t *pImage=NULL;

	for( int nImages = s_availableImages.count(); nImages >0; nImages--){

		pImage = s_availableImages.get(nImages-1);

		if(!strcmp(strName.c_str(), pImage->name)){
			break;
		}
	}
    // No Image Found
    if( pImage == NULL){
        Log.verbose("[getImage] Image search miss. Term: %s" CR, strName);
    }
	return pImage;

}
//==========================================================================
// Load the image files from our image directory and put them in a list
//
// Images are simple binary files. Format: [width -> int16][height -> int16][data - uint8]

void loadImageFiles(void){

    File dir = SPIFFS.open(IMAGE_DIRECTORY);
    File fImage;

    Image_t  *pCurrImage;

    while( fImage = dir.openNextFile() ){

    	pCurrImage = new Image_t;
    	memset(pCurrImage, '\0', sizeof(Image_t));


    	String fname = String(fImage.name());
    	String fBasename = fname.substring(strlen(IMAGE_DIRECTORY) + 1, fname.length()-strlen(IMAGE_SUFFIX));
    	

    	pCurrImage->name = new char[fBasename.length()+1];
    	strcpy(pCurrImage->name, fBasename.c_str());


        fImage.readBytes((char*)&pCurrImage->width, sizeof(pCurrImage->width));
        fImage.readBytes((char*)&pCurrImage->height, sizeof(pCurrImage->height));

        // Need to calcuate the image array size. Image Y values are packed into bytes ( 8 scan lines / byte);

        int nPacked =  ceil((float)pCurrImage->height/8.0);

        Log.verbose("[loadImageFiles] Image: %s \t Width: %d, Height: %d, Packed Lines: %d, Array Len: %d" CR, 
                     pCurrImage->name, pCurrImage->width, pCurrImage->height, nPacked, pCurrImage->width * nPacked);

        pCurrImage->pData = new uint8_t[pCurrImage->width * nPacked];

        fImage.read(pCurrImage->pData, pCurrImage->width * nPacked );

        fImage.close();

        // stash the image
        s_availableImages.add(pCurrImage);
    }
    dir.close();
}
//==========================================================================
//
// Assumes the file is there and SPIFFS is spiffy
//
// Loads our JSON file that contains an array of steps.
// Processes the steps
// creates our internal list of show steps.

void loadTheShow(){

	File fShow = SPIFFS.open(SHOW_FILE, "r");

	if(!fShow){
        Log.error("[loadTheShow] Unable to open/mount the SPIFFS file system." CR);
		return;
	}
	size_t fileSize = fShow.size();

    // allocate buffer to load file contents
    std::unique_ptr<char[]> buf(new char[fileSize]);
    fShow.readBytes(buf.get(), fileSize);
    DynamicJsonBuffer jsonBuffer;

    // Load the JSON file
    JsonObject& json = jsonBuffer.parseObject(buf.get());
    if(json.success()){

        // get the step array in the JSON object
        JsonArray& showSteps = json.get<JsonArray>("theshow");

        // process the steps in the json array and create our list of steps
        ShowStep_t *pStep;

        int nSteps = showSteps.size();

        // Since the linked list we use just addes new things to the list head,
        // we start processing from the end of the provided list.

        for(int i=nSteps; i > 0; i--){

       		JsonObject &oStep = showSteps[i-1];

            pStep = new ShowStep_t;
            memset(pStep, '\0', sizeof(ShowStep_t));

            pStep->type = (StepType_t)oStep.get<int>("type");
       	  
            switch(pStep->type){

                case stepImageBounce:
                    pStep->count = oStep["times"];
                    // fall through
                case stepImageToAll:
                    pStep->strImage = strdup(oStep["image"]);
                    break;            
                case stepFlash:
                case stepWait:
                    pStep->count = oStep["times"];
                    break;
                case stepClear:
                    break;
                case stepImageFlash:
                    pStep->count = oStep["times"];
                    pStep->offset = oStep["offset"];    
                    // fall through                     
                case stepImageScroll:
                    pStep->strImage = strdup(oStep["image"]);
                    break;                        
                default:
                    Log.error("[loadTheShow] Error parsing the show json file. Invalid step type." CR);
                    break;
            }
            s_theShow.add(pStep);	
        }
        return;// showSteps;
    }else{
        Log.error("[loadTheShow] Error parsing the show json file. Invalid file format." CR);        
    }

    return;
}

//==========================================================================
// Acccessor to get the show -- list of steps for the show.
ShowStepList_t * getTheShow(void){

  return &s_theShow;
}
//==========================================================================
// Init routine for the system.

bool initShow(){

	if( SPIFFS.begin() ){
		if(!SPIFFS.exists(SHOW_FILE)){
            Log.error("[initShow] The show definition file %s, doesn't exist on this device." CR, SHOW_FILE);
			return false;
		}
		loadTheShow();
		loadImageFiles();
		return true;
	}else{
        Log.error("[initShow] Failure to load the device file system. SPIFFS.begin failed." CR);
		return false;
	}
	return false;
}