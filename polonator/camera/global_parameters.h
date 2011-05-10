#ifndef __GlOBAL_PARAMETERS_
#define __GlOBAL_PARAMETERS_

/*
 * =============================================================================
 *
 * Polonator G.007 Image Acquisition Software
 *
 * Wyss Institute
 * Written by Nick Conway
 * global_parameters.h: This is a header file intended to include all
 * definitions of the all constants including path names and constants
 * pertaining to images (e.g. resolution) and machine configuration
 * (e.g flow cell numbers)
 *
 * Revision 1.0 11/24/2009
 *
 * This software may be modified and re-distributed, but this header must appear
 * at the top of the file.
 *
 * =============================================================================
 */

#define TOTAL_IMG_PER_LANE			2180	/* the total number of images per lane */
#define TOTAL_IMG_PER_X_SCAN_LINE	218		/* the total number images in the X direction of a flowcell scan */
#define TOTAL_IMG_PER_Y_SCAN_LINE	10		/* the total number images in the Y direction of a flowcell scan */
#define IMAGE_RESOLUTION_X			1000	/* number of pixels in the X direction of an image */
#define IMAGE_RESOLUTION_Y			1000	/* number of pixels in the X direction of an image */

#define INTEGRATION_IN_MS_THRESHOLD 10000

#define FOCUS_OFFSET_LIM			40 		/* focus offset limit value */

#define SLEEP_CYCLES				10000	/* number of cycles to sleep */
#define WAIT_CYCLES					1000	/* number of cycles to wait for maestro */

/* MAESTRO Buffer sizes/lengths */
#define CMD_BUF0					255		/* Buffer size for string */
#define LOG_BUF0					255		/* Buffer size for string */
#define CFG_BUF0					255		/* Buffer size for string */
#define RESPONSE_BUF0				255		/* Buffer size for string */
#define RCV_BUF0					255		/* Buffer size for string */
#define FOCUS_BUF0					255		/* Buffer size for string */
#define INPUT_BUF0					500		/* Buffer size for string */
#define LOG_STR_BUF0				500		/* Buffer size for string */

#define CMD_BUF1					100		/* Buffer size for string */
#define LOG_BUF1					255		/* Buffer size for string */
#define RESPONSE_BUF1				100		/* Buffer size for string */

#define CMD_BUF2					200		/* Buffer size for string */
#define LOG_BUF2					255		/* Buffer size for string */
#define RESPONSE_BUF2				100		/* Buffer size for string */

#define RESPONSE_BUF3				250 	/* Buffer size for string */
/* end MAESTRO BUFS */

/* the full path of the configuration file */
#define CONFIG_FILE_FULL_PATH		"/config_files/polonator-acq.cfg"
/* the full path of the log file */
#define LOG_FILE_FULL_PATH			"/polonator/G.007/acquisition/logs/py_maestro-log"


#endif /* __GlOBAL_PARAMETERS_ */
