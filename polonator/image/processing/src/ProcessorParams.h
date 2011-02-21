// =============================================================================
// 
// Polonator G.007 Image Processing Software
//
// Church Lab, Harvard Medical School
// Written by Greg Porreca
//
// Release 1.0 -- 02-12-2008
//
// This software may be modified and re-distributed, but this header must appear
// at the top of the file.
//
// =============================================================================
//
#ifdef __cplusplus
extern "C" {
#endif 
  
#define DEBUG1 1
#define DEBUG0 1
#define SAVE_IMAGES 1 // save brightfield raw, flattened, and segmented images
#define SAVE_FL_IMAGES 1 // save all raw fluorescent images; this is ALOT of data
  //#define BCDEBUG0 1  


#define WHITE_BEADS 1 // define this if the beads are lighter than the background in darkfield mode


  //
  // processor.c, send_data.c, initialize_processor.c
#define PORTNUM 4000

  //
  // ProcessImage.c
#define IMGS_PER_ARRAY 2180
#define ARRAYS_PER_FC 8
#define FCs_PER_RUN 1
#define REG_PIXELS 30000
#define SEARCH_XCOLS 20
#define SEARCH_YROWS 20
#define OFFSET_HISTORY_LENGTH 20
#define MAX_ADDITIONAL_OFFSET 50

  
  //
  // ProcessImage_extract.c
#define NUM_XCOLS 1000
#define NUM_YROWS 1000
  
  //
  // processor.c
#define MAX_BEADS_PERFRAME 1000000
#define DEFAULT_PROCESSOR_LOGFILENAME "processor-log"
  
  //
  // find_objects.c
  // use this for stainless disc F07 flowcells w/ ring illuminator
  //#define FINDOBJ_STDMULT -14.5 

  // use this w/ ground plastic G07 w/ point illuminator
  //#define FINDOBJ_STDMULT -9.5

  // use this w/ treated titanium flowcell w/ point illuminator
  //#define FINDOBJ_STDMULT -16.5

  // use this w/ TiO2 flowcell w/ point illuminator (tune R2 so that reading @J13=2.96V)
  // when using a Ti flowcell, tune R2 so that reading @J13=2.76V
#define FINDOBJ_STDMULT -13
  //
  // img_tools.c
#define FLATTEN_WINDOWSIZE 5


  //
  // Basecaller.c
#define MAX_BEADFILES 200
#define PRIMER_THRESHOLD_FILENAME "primer_thresholds.dat"

#ifdef __cplusplus
 }
#endif 
