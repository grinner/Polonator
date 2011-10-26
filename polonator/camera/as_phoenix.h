
/* =============================================================================
//
// Polonator G.007 Selective Illuminate Software
//
// Wyss Insitute
// Written by Nick Conway
//
// as_phoenix.h:
//
// Update and commented version -- 01-04-2010 [NC]
//
// This software may be modified and re-distributed, but this header must appear
// at the top of the file.
//
// =============================================================================
/*/


#ifndef _AS_PHOENIX_H
#define _AS_PHOENIX_H

unsigned char py_14to8bit(unsigned short *image, int i);
void py_14to8Image(unsigned short *image, unsigned char * image_copy, int len);

unsigned short * snap(float exposure, float gain, char *color, char *filename);
unsigned short * snapPtr(float exposure, float gain, char *color);
int cameraLive(float time, int gain);
int freeImage(unsigned short * image);
void copyImage(unsigned short *from_image, unsigned short *to_image);
unsigned short getPixel(unsigned short *image, int i);
void cameraClose(void);
void py_snapPtr(unsigned short * raw_image, float exposure, float gain, char *color);

#endif /* _AS_PHOENIX_H */
