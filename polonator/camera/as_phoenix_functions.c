/* =============================================================================
// 
// Polonator G.007 Image Acquisition Software
//
// Church Lab, Harvard Medical School
// Written by Greg Porreca
//
// as_phoenix_functions.c: functionality for dealing with the Hammamatsu
// EMCCD C9100-02 camera and the Phoenix framegrabber
//
// Release 1.0 -- 04-15-2008
//
// This software may be modified and re-distributed, but this header must appear
// at the top of the file.
//
// =============================================================================
/*/

#include "as_phoenix_functions.h"
#include "common.h"

char log_string[500];


static void snap_callback(tHandle hCamera, ui32 dwInterruptMask, void *pvParams);
typedef struct{
  volatile int image_ready;
} tPhxCallbackInfo;
stImageBuff img_buffer;
tPhxCallbackInfo sPCI;

void py_cameraInit(int tdiflag){
  etStat eStat;
  
  p_log("STATUS:\tpy_cameraInit: Initialize internal camera handle");
  pyHandle = 0;
  if(tdiflag){
    p_log("Use 9100-50 config file");
    eStat = PHX_CameraConfigLoad(&pyHandle, "/home/polonator/G.007/G.007_acquisition/em9100-50.pcf", PHX_BOARD_AUTO | PHX_DIGITAL, NULL);
  }
  else{
    p_log_simple("Use 9100-02 config file");
    eStat = PHX_CameraConfigLoad(&pyHandle, "/home/polonator/G.007/G.007_acquisition/em9100-02.pcf",(etCamConfigLoad)(PHX_BOARD_AUTO|PHX_DIGITAL|PHX_NO_RECONFIGURE),PHX_ErrHandlerDefault);
    /*eStat = PHX_CameraConfigLoad(&pyHandle, "/home/polonator/G.007/G.007_acquisition/em9100-02.pcf", PHX_BOARD_AUTO | PHX_DIGITAL, NULL);*/
  }
  check_for_error(eStat, "py_cameraInit", "PHX_CameraConfigLoad");

  p_log_simple("STATUS:\tpy_cameraInit: Set camera to external (Maestro) triggering");
  init_camera_external_trigger(pyHandle);

  p_log_simple("STATUS:\tpy_cameraInit: Allocate image buffer");
  image_ptr = (short unsigned int*)malloc(1000000*sizeof(short unsigned int));

  /* initialize PhxCallback structure and image_ready flag*/
  memset(&sPCI, 0, sizeof(tPhxCallbackInfo));
  sPCI.image_ready = 0;
}

void py_cameraClose(){
  p_log_simple("STATUS:\tpy_cameraClose: Release internal camera handle");
  PHX_CameraRelease(&pyHandle);

  p_log_simple("STATUS:\tpy_cameraClose: Free image buffer");
  free(image_ptr);
}

void py_set_exposure(double time_inseconds){
  sprintf(log_string, "STATUS:\tpy_set_exposure: Set exposure to <%f> seconds", time_inseconds);
  p_log_simple(log_string);
  set_exposure(pyHandle, time_inseconds);
}

void py_set_gain(int gain){
  sprintf(log_string, "STATUS:\tpy_set_gain: Set EM gain to <%d>", gain);
  p_log_simple(log_string);
  set_gain(pyHandle, gain);
}

void py_setupSnap(){
  etStat eStat;
  etParamValue dw;
  ui32 PHX_buffersize = 1;

  p_log_simple("STATUS:\tpy_setupSnap: setup acquisition for snap");

  p_log_simple("STATUS:\tpy_setupSnap: configure events to recognize");
  dw = (etParamValue) (0|PHX_INTRPT_BUFFER_READY);
  eStat = PHX_ParameterSet(pyHandle, PHX_INTRPT_SET, &dw);
  check_for_error(eStat, "py_setupSnap()", "PHX_ParameterSet(PHX_INTRPT_SET)");

  p_log_simple("STATUS:\tpy_setupSnap: configure callback event context");
  eStat = PHX_ParameterSet(pyHandle, PHX_EVENT_CONTEXT, (void*) &sPCI);
  check_for_error(eStat, "py_setupSnap()", "PHX_ParameterSet(PHX_EVENT_CONTEXT)");

  p_log_simple("STATUS:\tpy_setupSnap: setup framegrabber for number of images to capture");
  eStat = PHX_ParameterSet(pyHandle, PHX_ACQ_NUM_IMAGES, (void*) &PHX_buffersize);
  check_for_error(eStat, "py_setupSnap()", "PHX_ParameterSet(PHX_ACQ_NUM_IMGS)");

  p_log_simple("STATUS:\tpy_setupSnap: start waiting for image to arrive");
  eStat = PHX_Acquire(pyHandle, PHX_START, (void*) snap_callback);
  check_for_error(eStat, "py_setupSnap()", "PHX_Acquire(PHX_START)");
}


int py_snapReceived(){
  return sPCI.image_ready;
}


short unsigned int* py_getSnapImage(){
  stImageBuff stBuffer;
  etStat eStat;
  int i;

  p_log_simple("STATUS:\tpy_getSnapImage(): get pointer to framegrabber image buffer");
  eStat = PHX_Acquire(pyHandle, PHX_BUFFER_GET, &stBuffer);
  check_for_error(eStat, "py_getSnapImage()", "PHX_Acquire(PHX_BUFFER_GET)");
  
  p_log_simple("STATUS:\tpy_getSnapImage(): copy image into user memory space");
  for(i=0; i<1000000; i++){
    *(image_ptr + i) = *((short unsigned int*)(stBuffer.pvAddress) + i);
  }

  p_log_simple("STATUS:\tpy_getSnapImage(): release framegrabber buffer");
  eStat = PHX_Acquire(pyHandle, PHX_BUFFER_RELEASE, &stBuffer);
  check_for_error(eStat, "py_getSnapImage()", "PHX_Acquire(PHX_BUFFER_RELEASE)");

  p_log_simple("STATUS:\tpy_getSnapImage(): return pointer to image in user memory");
  sPCI.image_ready = 0;
  return image_ptr;
}



/* returns the mean of the 1mpix image pointed to by int* img.
   mean is cast as an integer
*/
int py_imagemean(short unsigned int* img){
  int i;
  double sum = 0;
  int mean = 0;
  long long int sigma=0;
  double std=0;

  for(i=0; i<1000000; i++){
    sum += *(img + i);
  }

  mean = (int)(sum / 1000000);
  for(i=0; i<1000000; i++){
    sigma+=(long long int)(pow((*(img+i)-mean),2));
  }
  
  std = sqrt((1.0/((double)1000000-1)) * sigma);

  sprintf(log_string, "STATUS:\tpy_imagemean: Mean pixel value is %d, Stdev is %d", (int)(sum / 1000000),(int)std);
  p_log_simple(log_string);

  return (int)(sum / 1000000);
}


/* This mode is used when the Polonator controller is controlling
   image acquisition; it sets the trigger line high for the duration
   of the integration period.  We therefore don't need to set 'exposure
   time' in software.  This is called 'level triggering' (as opposed to
   'edge triggering', where the trigger edge signals the start of 
   integration, and the duration is set in software w/ the AET command.
   We use the AET command consistently anyway, since it must be used when
   triggering the camera in software (e.g. single-shot mode during 
   autoexposure).
*/
void init_camera_external_trigger(tHandle hCamera){
  p_log("initialize camera for external trigger area mode");
  /*phxser(hCamera, "INI");*/ /* initialize the camera*/
  /*phxser(hCamera, "RES Y");*/ /* return commands as part of response*/
  phxser(hCamera, "AMD E"); /* set to external trigger mode (from Maestro TTL)*/
  phxser(hCamera, "EMD L"); /* set to level triggering (trigger rise/fall starts/ends exposure)*/
  /*  phxser(hCamera, "EMD E");*/ /* set to edge triggering (trigger edge starts exposure)*/
  phxser(hCamera, "ESC M"); /* set to trigger source "multi-pin"*/
  phxser(hCamera, "ATP P"); /* set to positive polarity trigger*/  
  /*phxser(hCamera, "ACN 0");*/ /* acquire cycling forever */
  /*phxser(hCamera, "CEO 0");*/ /* set 'contrast enhancement' to maximum */

}


/* This mode is used when this software is controlling image acquisition.  The
   software fires a 'software trigger', which tells the camera to start integrating.
   Integration ends when 'exposure time' has elapsed (set w/ AET).
*/
void init_camera_internal_trigger(tHandle hCamera){
  p_log("initialize camera for internal trigger area mode");
  phxser(hCamera, "AMD N"); /* set to internal ("normal") trigger mode from Phoenix*/
  phxser(hCamera, "ACT I"); /* only image when internally triggere */
  /*  phxser(hCamera, "AMD I");*/ /* set to external trigger mode (from Maestro TTL)*/
  /*phxser(hCamera, "ACN 0");*/ /* acquire cycling forever */
  /*phxser(hCamera, "CEO 0");*/ /* set 'contrast enhancement' to maximum */
}


/* This mode is used when running TDI in normal TDI mode, triggering by controller
*/
void init_camera_external_trigger_TDI(tHandle hCamera){
  p_log("initialize camera for external trigger TDI mode");
  /*phxser(hCamera, "INI");*/ /* initialize the camera*/
  /*phxser(hCamera, "RES Y");*/ /* return commands as part of response*/
  phxser(hCamera, "AMD T"); /* set to TDI 'normal' mode*/
  phxser(hCamera, "TMD E"); /* set to external TDI mode*/
  phxser(hCamera, "ESC M"); /* set to trigger source "multi-pin"*/
  phxser(hCamera, "ATP P"); /* set to positive polarity trigger*/  
  /*phxser(hCamera, "ACN 0");*/ /* acquire one image */
  /*phxser(hCamera, "CEO 0");*/ /* set 'contrast enhancement' to maximum */
}


/* This is used when running in TDI with internal triggering (for live debugging
   purposes only
*/
void init_camera_internal_trigger_TDI(tHandle hCamera){
  p_log("initialize camera for internal trigger TDI mode");
  phxser(hCamera, "INI"); /* initialize the camera*/
  phxser(hCamera, "RES Y"); /* return commands as part of response*/
  phxser(hCamera, "AMD T"); /* set to internal ("normal") trigger mode from Phoenix*/
  phxser(hCamera, "TMD I"); /* set to internal TDI mode*/
  phxser(hCamera, "ESC M"); /* set to trigger source "multi-pin"*/
  phxser(hCamera, "ATP P"); /* set to positive polarity trigger*/  
  phxser(hCamera, "ACN 0"); /* acquire cycling forever */
  phxser(hCamera, "CEO 0"); /* set 'contrast enhancement' to maximum */
}



/* input must be exposure time in seconds;
 0.001 <= time <= 9.999;
 time must have <=3 decimal places (e.g. 0.0010 is not allowed)
*/
void set_exposure(tHandle hCamera, double time_inseconds){
  char exposure_command[14];
  if(time_inseconds < 0.001){
    sprintf(log_string, "ERROR: set_exposure: time %f too small; must be >= 0.001 seconds", time_inseconds);
    p_log(log_string);
  }
  else if(time_inseconds > 9.999){
    sprintf(log_string, "ERROR: set_exposure: time %f too large; must be < 10.0 seconds", time_inseconds);
    p_log(log_string);
  }
  else{
    sprintf(exposure_command, "AET %0.03f", time_inseconds);
    phxser(hCamera, exposure_command);
  }
}


/* input must be EM gain setting between 0 and 255 inclusive
*/
void set_gain(tHandle hCamera, int gain){
  char gain_command[14];

  if(gain < 0){
    sprintf(log_string, "ERROR: set_gain: gain %d too small; must be >= 0\n", gain);
    p_log(log_string);

  }
  else if(gain > 255){
    sprintf(log_string, "ERROR: set_gain: gain %d too large; must be <= 255\n", gain);
    p_log(log_string);
  }
  else{
    sprintf(gain_command, "EMG %d", gain);
    phxser(hCamera, gain_command);
  }
}



/* check for a camera-specific error; if any, pass to p_log for 
   timestamping and file output
*/
void check_for_error(etStat eStat, char *fn_name, char *error_call){
  char error[255];
  if ( PHX_OK != eStat ){
    PHX_ErrCodeDecode(error, eStat);
    sprintf(log_string, "ERROR: %s: %s: %d--%s\n", fn_name, error_call, eStat, error);
    p_log_simple(log_string);
    exit(1);
  }
}

static void snap_callback(tHandle hCamera,
			  ui32 dwInterruptMask,
			  void *pvParams){

  tPhxCallbackInfo *sPCI = (tPhxCallbackInfo*) pvParams;
  (void) hCamera;

  /* event PHX_INTRPT_BUFFER_READY evaluates true when the framegrabber receives a
     full image from the camera; at this point, we can get a pointer to the image
     in the framegrabber's memory pool and copy it out for our own use
  */
  if(PHX_INTRPT_BUFFER_READY & dwInterruptMask){
    sPCI->image_ready = 1;
    p_log_simple("PHX CALLBACK: IMAGE READY");

  }
}


/****************************************************************************
 *
 * ACTIVE SILICON LIMITED
 *
 * File name   : phxser.c
 * Function    : Example serial I/O application
 * Project     : Phoenix
 *
 * Copyright (c) 2004-2006 Active Silicon Ltd.
 ****************************************************************************
 * Comments:
 * --------
 * This file is a simple terminal program for serial I/O. It allows
 * characters strings to be sent to the camera with either CR and/or LF
 * appended to the transmit string, by the use of "\r" and "\n" keys. The
 * application then waits for 1sec before displaying all received characters.
 * To quit the application, type "Exit" at the prompt.
 *
 ****************************************************************************
 */
/*
Modified 02-13-2008 to transmit a message to an open handle and exit
by Greg Porreca
Church Lab, Harvard University
*/
/*
Call with the handle to the camera, plus a null-terminated string
to be transmitted to the camera.  The string length must be <= 256 chars
*/
int phxser(tHandle hCamera, char *szCmdBuff){

  etStat  eStat   = PHX_OK;
  etParamValue eParamValue;
  char szTxLineBuff[_PHX_LINE_SIZE];
  char cRxLineBuff[_PHX_LINE_SIZE];
  char error[10000];
  int counter = 0;
  sprintf(log_string, "STATUS:\tphxser: sending command %s", szCmdBuff);
  p_log_simple(log_string);

  /* Setup for serial communication */
  eParamValue = PHX_COMMS_DATA_8;
  eStat = PHX_ParameterSet( hCamera, PHX_COMMS_DATA, &eParamValue );
  if ( PHX_OK != eStat ){
    PHX_ErrCodeDecode(error, eStat); 
    sprintf(log_string, "ERROR:\tphxser: PHX_COMMS_DATA %s", error);
    p_log_errorno(log_string);
  }
  eParamValue = PHX_COMMS_STOP_1;
  eStat = PHX_ParameterSet(hCamera, PHX_COMMS_STOP, &eParamValue );
  if ( PHX_OK != eStat ){
    PHX_ErrCodeDecode(error, eStat); 
    sprintf(log_string, "ERROR:\tphxser: PHX_COMMS_STOP %s", error);
    p_log_errorno(log_string);
  }
  
  eParamValue = PHX_COMMS_PARITY_NONE;
  eStat = PHX_ParameterSet(hCamera, PHX_COMMS_PARITY, &eParamValue );
  if ( PHX_OK != eStat ){
    PHX_ErrCodeDecode(error, eStat); 
    sprintf(log_string, "ERROR:\tphxser: PHX_COMMS_PARITY %s", error);
    p_log_errorno(log_string);
  }
  
  eParamValue = (etParamValue) 9600;
  eStat = PHX_ParameterSet(hCamera, PHX_COMMS_SPEED, &eParamValue );
  if ( PHX_OK != eStat ){
    PHX_ErrCodeDecode(error, eStat); 
    sprintf(log_string, "ERROR:\tphxser: PHX_COMMS_SPEED %s", error);
    p_log_errorno(log_string);
  }
  
  eParamValue = PHX_COMMS_FLOW_NONE;
  eStat = PHX_ParameterSet(hCamera, (etParam)(PHX_COMMS_FLOW|PHX_CACHE_FLUSH), &eParamValue );
  if ( PHX_OK != eStat ){
    PHX_ErrCodeDecode(error, eStat); 
    sprintf(log_string, "ERROR:\tphxser: PHX_CACHE_FLUSH %s", error);
    p_log_errorno(log_string);
  }
  
  ui32  dwTxMssgLen;
  ui32  dwRxMssgLen;
  char  *pcCmdLineBuff;
  char  *pcTxLineBuff;
  char  *tempbuff;
  tFlag  fAppend = TRUE;
  
  
  /* Copy the Cmd string into the TxLineBuff.
   */
  pcCmdLineBuff = szCmdBuff;
  pcTxLineBuff  = szTxLineBuff;
  dwTxMssgLen = 0;
  dwRxMssgLen = 0;
  do{
    *(szTxLineBuff + dwTxMssgLen) = (char) tolower(*(pcCmdLineBuff + dwTxMssgLen));
    dwTxMssgLen++;
  } while (*(pcCmdLineBuff+dwTxMssgLen) != '\0');
    *(szTxLineBuff+dwTxMssgLen) = '\r';
  dwTxMssgLen++;
  *(szTxLineBuff+dwTxMssgLen) = '\0';
  
  printf("szTxLineBuff = %s length = %d, dwTxMssgLen = %d\n",szTxLineBuff, strlen(szTxLineBuff), dwTxMssgLen);


  /* Transmit the serial data */
  eParamValue = (etParamValue)dwTxMssgLen;
  eStat = PHX_CommsTransmit( hCamera, (ui8*) szTxLineBuff, (ui32*) &eParamValue, 500 );
  if ( PHX_OK != eStat ){ p_log_errorno("ERROR:\tphxser: Failed to transmit all characters");}
  if ( (ui32) eParamValue != dwTxMssgLen ) {
    p_log_errorno("ERROR:\tphxser: Failed to transmit all characters");
  }

  _PHX_SleepMs(200);

  
  /* Check how many characters are waiting to be read */
  eStat = PHX_ParameterGet( hCamera, PHX_COMMS_INCOMING, &dwRxMssgLen );
  if ( PHX_OK != eStat ){
    PHX_ErrCodeDecode(error, eStat); 
    sprintf(log_string, "ERROR:\tphxser: PHX_COMMS_INCOMING %s", error);
    p_log_errorno(log_string);
  }

  while(dwRxMssgLen < dwTxMssgLen){
	sprintf(log_string, "STATUS:\tphxser: Waiting in the while loop to receive all the characters, dwRxMssgLen = %d, dwTxMssgLen = %d", dwRxMssgLen,dwTxMssgLen);
        p_log(log_string);
	_PHX_SleepMs(200);
        eStat = PHX_ParameterGet( hCamera, PHX_COMMS_INCOMING, &dwRxMssgLen );
	sprintf(log_string, "STATUS:\tphxser: Waiting in the while loop to receive all the characters, dwRxMssgLen = %d, dwTxMssgLen = %d", dwRxMssgLen,dwTxMssgLen);
        p_log_errorno(log_string);
        counter++;
        if(counter > 60)
	{
		 sprintf(log_string, "STATUS:\tphxser: Waiting in the while loop to receive all the characters, dwRxMssgLen = %d, dwTxMssgLen = %d", dwRxMssgLen,dwTxMssgLen);
  	         p_log_errorno(log_string);
		 break;
	}
  }

/* Wait 200ms for a receive message; it should have all arrived by then */
  *szTxLineBuff = tolower(*szTxLineBuff);

  /* If any characters are waiting to be read, go get them*/ 
  if ( 0 != dwRxMssgLen ) {
    eParamValue = (etParamValue)dwRxMssgLen;
    eStat = PHX_CommsReceive( hCamera, (ui8*) cRxLineBuff, (ui32*) &eParamValue, 500 );
    /* ADDED ON 101309
    if(strncmp(cRxLineBuff,szTxLineBuff,3) != 0){
    _PHX_SleepMs(600);
    eParamValue = (etParamValue)dwRxMssgLen;
    eStat = PHX_CommsReceive( hCamera, (ui8*) cRxLineBuff, (ui32*) &eParamValue, 500 );
    sprintf(log_string, "ERROR:\tphxser: return command does not match, szTxLineBuff = %s, cRxLineBuff = %s", szTxLineBuff, cRxLineBuff);
    p_log(log_string); 
    }
    ADDED ON 101309 FINISHED*/
	
    if ( (ui32) eParamValue != dwRxMssgLen ) {
      p_log("ERROR:\tphxser: Failed to read all pending characters");
    }
    
    /* The receive buffer, is NOT a NULL terminated string.
     * Therefore add the termination
     */
    cRxLineBuff[dwRxMssgLen] = '\0'; /* camera sends CR & LF; strip last char and insert string terminator */
  }

  /*else {
    sprintf(log_string, "ERROR:\tphxser: No characters received; re-calling with <%s>", szCmdBuff);
    p_log(log_string);
    phxser(hCamera,szCmdBuff); 
  }*/
  sprintf(log_string, "STATUS:\tphxser: Received %s", cRxLineBuff);
  p_log_simple(log_string);
 
 Finish:
  return 0;
}
