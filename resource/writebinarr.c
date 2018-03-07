/*
 * Simple routine to dump out a xbm file array (loaded via a header) to 
 * a binary file. 
 *
 * These files are then read in using arudio binary routines
 *
 * YES - you have to edit this file for every file processed.
 * YES - I'm lazy - not up for some vararg work today. :)
 */

#import <stdlib.h>
#import <sys/types.h>
#import <stdio.h>

typedef unsigned char uint8_t;

#import "../px_10year.h"

short width = width10Year;
short height = 48;

int main(){

	FILE *fpW;

	fpW = (FILE*)fopen("10year.bin", "wb");
	printf("Size of Short %d, width: %d, height: %d\n", (int)sizeof(short), width, height);
	fwrite(&width, sizeof(short), 1, fpW);
	fwrite(&height, sizeof(short), 1, fpW);
	fwrite(img10Year, sizeof(uint8_t), sizeof(img10Year)/sizeof(uint8_t), fpW);

//	printf("Image Array Size: %d", sizeof(i)/sizeof(uint8_t));
	fclose(fpW);

	exit(0);
}