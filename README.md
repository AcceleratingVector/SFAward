
OVERVIEW
======================
Setup to display images across a grid of displays. Current set to 4 oleds.

Uses an esp32 to drive the screens

The goal is to use the oleds as "screens" that make up an overall display. T

Using sparkfun oled's 64x48px

Screen Layout
--------------
     |<---------------- One Big Display --------------->|
     +--------+    +--------+    +--------+    +--------+   
     | oled 0 |    | oled 1 |    | oled 2 |    | oled 3 | - name/reference
     | pos  0 |    | pos  1 |    | pos 2  |    | pos 3  | - screen layout position
     +--------+    +--------+    +--------+    +--------+   
     0px     64px  65px    128px 129px   192px 193px   256px

Note
This takes it's general multi-oled use with a esp32 thing inspiration from Alex's OLED clock


IMAGES
------
Most of the graphics operations/displays activities is performed using images. The images
are single bit pixels (one/off) that are encoded as a byte stream. The Y axes/scan lines
being packed in bytes (8 scan lines per byte). This basically follows the format of a X bitmap
(XPM).

The software currently ignores the Y coordinate for location/height and assumes all images are
the height of the OLED display (48px). Hacky, but keeps things simple.

While most examples for the OLED displays use a staticly defined uint8 array which is defined in
a C header file, this forces  the routines that use an image to hard code it in (unless you write
a parser). To provide a dynamic method to deploy images, this system uses a simple binary format:

      [width - uint16][height - uint16][packed pixels - uint8]

These files are created from the defined static definitions in a header file using a simple C
routine on a PC/Mac.

Image files are currently created using a convoluted prodess that originanted from the instructions for the OLED display. The steps are:

*  Create a binary image of what you desire to diplay - Save as a BMP format
  * N x 48px. (any length - but 48px in height to match height of the OLED screen)
* On Windows, use LCD Assistant to convert the image to a static C array
          http://en.radzio.dxp.pl/bitmap_converter/
* In the resource directory hack writebinarr.c to use the generated header file.
  *  Include Header
  * Change input/output names (output file name pattern: <image name>.bin)
     * Compile and execute
* Copy the resultant binary file to the data/images directory of this repository

The goal is distill down these steps into a simple python program.

OPERATIONAL USE
---------------
This system is scripted, allowing dynamic "show" creation outside of firmware modification.
An award "show" is scripted using a JSON file and associated resources/images.

The script and associated resources are stored on the SPIFFS file system of the esp32. So you
need SPIFFS enabled and deployed when you flash the target esp32.

The "show" is defined by an array of "step" objects in a json formatted file called theshow.json.
Show "steps" are contained in the field "theshow" of the JSON files main ojbect. Edit this file to define the overall show for the screens.

Current show steps have the following type code:
  Type Codes:

        Image Bounce         = 1
        Image to all Screens = 2
        Flash the screens    = 3
        Clear the Screens    = 4
        Scroll an Image      = 5
        Wait  (ms)           = 6
        Image Flash          = 7

For now, see the examples on how to format the json file. Image structure and location are described above.

Once a show is defined in a json file, upload the SPIFFs directory to your target esp32 device,
flash the board with this routine and reboot the device.

DEPENDANCIES
------------
* **[SparkFun OLED Library]( https://github.com/sparkfun/SparkFun_Micro_OLED_Arduino_Library/tree/V_1.0.0SparkFun)**  
* **[Arduino Log](https://github.com/thijse/Arduino-Log)**
* **[LDC Assistant](http://en.radzio.dxp.pl/bitmap_converter/)**
* **[Arduino JSON](https://github.com/bblanchon/ArduinoJson)**
* **[ESP32 Arduino](https://github.com/espressif/arduino-esp32)**
* **[SparkFun ESP32 Thing](https://learn.sparkfun.com/tutorials/esp32-thing-hookup-guide)**
* **[Circuit Design Inspiration](https://github.com/awende/OLED_Clock)**

NOTES
-------
    Feb 2018 -    The mkspiffs command shipped with the current esp32 Arudino core fails to upload
                  the SPIFFS for the esp32. The workaround is to download the latest version of
                  mkspiffs from espressif, and replace the current Arudino version wih this one.
                      Latest mkspiffs:
                         https://github.com/igrr/mkspiffs/releases/tag/0.2.2

                      Install:
                          <Arudio Directory>/hardware/espressif/esp32/tools/mkspiffs
