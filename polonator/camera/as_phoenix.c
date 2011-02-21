/* =============================================================================
// 
// Polonator G.007 Image Acquisition Software
//
// Wyss Insittute, Harvard Medical School
// Written by Nick Conway
//
// as_phoenix.c: functionality for dealing with the Hammamatsu
// EMCCD C9100-02 camera and the Phoenix framegrabber
//
// Release 1.0 -- 01-26-2010
//
// This software may be modified and re-distributed, but this header must appear
// at the top of the file.
//
// =============================================================================
*/

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include "common/common.h"
#include "global_parameters.h"
#include "as_phoenix_functions.h"
#include "maestro_functions.h"
#include "as_phoenix.h"
#include "as_phoenix_live.h"

/* 14 bit to 8 bit converter */
/* returns a shifted value of a 14 bit image at a given index */

int m_sock;
int camera_open = -1; // initialize value

unsigned char py_14to8bit(unsigned short *image, int i)
{
   unsigned char  low_fi;
   low_fi =  image[i] >> 6;  // shift only 6 bits to make a 14 bit image an 8 bit
   return low_fi;
}

void py_14to8Image(unsigned short *image, unsigned char *image_copy, int len)
{
    int i;
    for (i = 0; i < len; i++)
    {
        image_copy[i] = (unsigned char) (image[i] >> 6);  // shift only 6 bits to make a 14 bit image an 8 bit
    }
}

unsigned short * snap(float exposure, float gain, char *color, char *filename)
{

	unsigned short *image;
	FILE *outfile;

	/* open file */
	outfile = fopen(filename, "w");

	image = snapPtr(exposure, gain, color);

	/* write image to file */

	fwrite(image, sizeof(unsigned short), 1000000, outfile);

	/* close file */
	fclose(outfile);

        return image;
}

/* returns a pointer to an image copied from frame buffer.  Like snap but no output file */
unsigned short * snapPtr(float exposure, float gain, char *color)
{
	//int m_sock;
	unsigned short *image;
	int imagemean;
	int shutterflag;

	/* open hardware and file */
	if (camera_open == -1) // for 0 and -1
	{
	    py_cameraInit(0); /* use non-TDI config file */
	    py_set_gain(gain);
	    maestro_open(&m_sock);
	    camera_open = 1; 
	    py_setupSnap();
	}
        else
        {
            py_set_gain(gain);
        }

	/* configure hardware */
	//maestro_locktheta(msock);
	//maestro_setcolor(m_sock, color);
	/*maestro_darkfield_off(m_sock);*/
	

	/* determine whether or not to use the shutter */
	if(!strcmp(color, "none"))
	{
		shutterflag = 0;
		maestro_darkfield_on(m_sock);
	}
	else
	{
		shutterflag = 1;
		maestro_darkfield_off(m_sock);
	}

	/* setup the software to receive an image from the camera */
	//py_setupSnap();


	/* snap the image */
	maestro_snap(m_sock, (int)(exposure * 1000.0), shutterflag);


	/* wait for image to be received by framegrabber */
	while(!py_snapReceived()){;}


	/* get pointer to image */
	image = py_getSnapImage();


	/* calculate mean for informational purposes */
	//imagemean = py_imagemean(image);
	//fprintf(stdout, "Image mean: %d\n", imagemean);

	/* close hardware */
	maestro_darkfield_off(m_sock);
	/* TODO Do we need to close the maestro socket? */
	    //fprintf(stdout, "The color is %s\n", color);
        return image;
}

void py_snapPtr(unsigned short * raw_image, float exposure, float gain, char *color)
{
    //raw_image = snapPtr(exposure, gain, color);
    copyImage(snapPtr(exposure, gain, color), raw_image);
    //fprintf(stdout, "The color is %s\n", color);
}

void copyImage(unsigned short *from_image, unsigned short *to_image)
{
    memcpy(to_image,from_image,2*IMAGE_RESOLUTION_X*IMAGE_RESOLUTION_Y);
}

int freeImage(unsigned short * image)
{
    free(image);
    return 0;
}

void cameraClose(void)
{
    if (camera_open == 1) // not sure if we want this in case of a crash
    {
        py_cameraClose();
        camera_open = 0;
    } 
}

int cameraLive(int argc, char **argv, int tdiflag)
{
	return camera_live(argc,argv,tdiflag);
}

unsigned short getPixel(unsigned short *image, int i)
{
    return image[i];
}
