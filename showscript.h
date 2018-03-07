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

#ifndef _SHOWSCRIPT_H_
#define _SHOWSCRIPT_H_

#include <ArduinoJson.h>

#include "SimpleList.h"

// Define an image type
typedef struct _imageType_{
	char     *name;
	uint16_t  width;
	uint16_t  height;
	uint8_t  *pData;
}Image_t;

// We define a "show" as a series of steps.
// 
// Define the types of steps.

typedef enum{
	stepImageBounce=1,
	stepImageToAll=2,
	stepFlash=3,
	stepClear=4,
	stepImageScroll=5,
	stepWait=6,
	stepImageFlash=7
}StepType_t; 

// Just make a simple struct for our steps. This is wasteful on non-parameter steps,
// but this is just for fun. could go nuts and make a union here (or worse, a c++ class hierarchy)

typedef struct _ShowStep_ {
	StepType_t      type;
	int    			count;
	char  		  * strImage;
	int 			offset;
}ShowStep_t;

typedef SimpleList<ShowStep_t*> ShowStepList_t; 

// Public API for the show
bool initShow();
ShowStepList_t * getTheShow(void);
Image_t *getImage(String strName);

#endif