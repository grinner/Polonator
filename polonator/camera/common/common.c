/****************************************************************************
 *
 * ACTIVE SILICON LIMITED
 *
 * File name   : common.c
 * Function    : Common routines used by the example suite
 * Project     : Phoenix
 *
 * Copyright (c) 2004 Active Silicon Ltd.
 ****************************************************************************
 * Comments:
 * --------
 * This file provides cross platform support services for the Phoenix Example Suite.
 * Whilst the API is fixed for all platforms, the implementation is OS specific.
 *
 *
 ****************************************************************************
 */


/* Header file for the support sevices
 */
#include <common.h>
#include <stdlib.h>


#ifdef _PHX_POSIX
static int              peek_character = -1;
static struct termios   initial_settings, new_settings;
#endif
#ifdef _PHX_VXWORKS
volatile int gnKbHitCount;
volatile int gnKbHitCountMax = 10000;
#endif


/*
PhxCommonDefaultConfig(ForBrief)
 * Create a default file name (including relative path) for a Config File
 * This is built up using pre-defined (OS dependent) strings for the, file name,
 * relative path, and if necessary absolute path to the current directory.
 */
static etStat PhxCommonDefaultConfig(
   char    *szProgName, /* Name of application, including full path */
   tPhxCmd *ptPhxCmd    /* Structure containing Parsed information */
)
{
   char   *pPcfFile;
   etStat  eStat = PHX_OK;

#if defined _PHX_MACOSX
   /* The default config file is in the examples directory not in the directory containing the executeable
    * This is calculated as a relative path from the application directory.
    */
   {
      char *pPcfDirName = dirname( szProgName );
      if( !pPcfDirName )
      {
         eStat = PHX_ERROR_SYSTEM_CALL_FAILED;
         goto Exit;
      }
      /* pPcfFile = malloc( strlen( pPcfDirName ) + strlen( DEFAULT_UP_DIR ) + strlen( DEFAULT_CFG_FILENAME ) + 3 ); */
      pPcfFile = malloc( strlen( pPcfDirName ) + strlen( getenv("POLONATOR_PATH") ) + strlen( DEFAULT_CFG_FILENAME ) + 3 );
      /* strcpy( pPcfFile, pPcfDirName ); */
      /* strcat( pPcfFile, DEFAULT_UP_DIR );*/
      strcpy( pPcfFile, getenv("POLONATOR_PATH") );
      strcat( pPcfFile, DEFAULT_CFG_FILENAME );
   }

#elif defined _PHX_VXWORKS
   /* Because there may not be a file system when running under VxWorks,
    * we don't attempt to set the default config file name
    * This behaviour can be changed by the user if they have access to a suitable
    * hard drive
    */
   {
   }

#else
   /* The default config file is in the examples directory not in the directory containing the executeable
    * This is calculated as a relative path from the application directory.
    */
   {
      (void) szProgName;

      //pPcfFile = (char *) malloc( strlen(DEFAULT_UP_DIR ) + strlen(DEFAULT_CFG_FILENAME ) + 2 );
      pPcfFile = (char *) malloc( strlen(getenv("POLONATOR_PATH")) + strlen(DEFAULT_CFG_FILENAME ) + 2 );
      if( !pPcfFile )
      {
         eStat = PHX_ERROR_MALLOC_FAILED;
         goto Exit;
      }
      strcpy( pPcfFile, getenv("POLONATOR_PATH") );
      /* strcpy( pPcfFile, DEFAULT_UP_DIR ); */
      strcat( pPcfFile, DEFAULT_CFG_FILENAME );
   }

#endif

   ptPhxCmd->pszConfigFileName = pPcfFile;

Exit:
   return eStat;
}


/*
PhxCommonParseCmd(ForBrief)
 * Parse the command line parameters, and place results in a common structure
 * The command line parameters take the following form:
 * AppName -b<BoardNumber> -c<ConfigFileName> -o<OutputFileName>
 * -b<BoardNumber>    is an optional parameter which specifies which board to use.
 *                    The default value is board 1.
 * -c<ConfigFileName> is an optional parameter specifying the Phoenix Config File,
 *                    The default value is an OS specific path to "default.pcf" which
 *                    is in the root directory of the example suite.
 * -O<OutputFileName> is an optional parameter specifiying the root name of an output file.
 *                    The default setting is NULL, indicating no output file.
 * Whilst all parameters may be specified, each example application only uses appropriate
 * parameters, for example "OutputFileName" will be ignored by the phxinfo example.
 */
etStat PhxCommonParseCmd(
   int      argc,       /* Command line parameter count */
   char    *argv[],     /* Command line parameters */
   tPhxCmd *ptPhxCmd    /* Structure containing Parsed information */
)
{
   etStat eStat = PHX_OK;

   /* Initialise the PhxCmd structure with default values */
   ptPhxCmd->dwBoardNumber     = 1;
   ptPhxCmd->pszConfigFileName = NULL;
   ptPhxCmd->pszOutputFileName = NULL;
   ptPhxCmd->dwBayerOption     = 11;
   ptPhxCmd->dwGammaOption     = 100;
   ptPhxCmd->dwFrameOption     = 300;
   ptPhxCmd->dwTimeOption      = 3;
   ptPhxCmd->dwSlowOption      = 10;
   eStat = PhxCommonDefaultConfig( *argv, ptPhxCmd );
   if ( PHX_OK != eStat ) goto Error;

   /* The first argument is always the function name itself */
   printf("\n*** %s ***\n", *argv );
   argc--;
   argv++;

   /* Parse the command options */
   while ( argc > 0 ) {
      if ( '-' == **argv ) {
         switch ( (int) *(*argv+1) ) {
            /* Board number */
            case 'b' : case 'B' :
               ptPhxCmd->dwBoardNumber = atoi(*argv+2);
               break;

            /* Config File */
            case 'c' : case 'C' :
               strcpy( ptPhxCmd->bConfigFileName, *argv+2 );
               if ( 0 != strlen(ptPhxCmd->bConfigFileName) )
                  ptPhxCmd->pszConfigFileName = ptPhxCmd->bConfigFileName;
               else
                  ptPhxCmd->pszConfigFileName = NULL;
               break;

            /* Gamma correction */
            case 'g' : case 'G' :
               ptPhxCmd->dwGammaOption = atoi(*argv+2);
               break;

            /* Output File */
            case 'o' : case 'O' :
               strcpy( ptPhxCmd->bOutputFileName, *argv+2 );
               if ( 0 != strlen(ptPhxCmd->bOutputFileName) )
                  ptPhxCmd->pszOutputFileName = ptPhxCmd->bOutputFileName;
               else
                  ptPhxCmd->pszOutputFileName = NULL;
               break;

            /* baYer conversion */
            case 'y' : case 'Y' :
               ptPhxCmd->dwBayerOption = atoi(*argv+2);
               break;

            /* Frames */
            case 'f' : case 'F' :
               ptPhxCmd->dwFrameOption = atoi(*argv+2);
               break;

            /* Seconds */
            case 't' : case 'T' :
               ptPhxCmd->dwTimeOption = atoi(*argv+2);
               break;

            /* Slow motion subsample */
            case 's' : case 'S' :
               ptPhxCmd->dwSlowOption = atoi(*argv+2);
               break;

            default :
               printf("Unrecognised parameter %c - Ignoring\n", *(*argv+1) );
               break;
         }
      }
      argc--;
      argv++;
   }

   printf("Using BoardNumber = %d\n",  ptPhxCmd->dwBoardNumber );
   printf("      Config File = ");
   if ( NULL == ptPhxCmd->pszConfigFileName ) printf("<None>\n"); else printf("%s\n", ptPhxCmd->pszConfigFileName );
   printf("      Output File = ");
   if ( NULL == ptPhxCmd->pszOutputFileName ) printf("<None>\n"); else printf("%s\n", ptPhxCmd->pszOutputFileName );
   printf("\n");

   /* Create an eCamConfigLoad parameter by OR'ing the board number with the board type */
   ptPhxCmd->eCamConfigLoad = (etCamConfigLoad) ( PHX_DIGITAL | ptPhxCmd->dwBoardNumber );
Error:
   return eStat;
}


/*
PhxCommonGetBytesPerPixel()
 * Return the number of bytes per pixel for a given pixel format
 */
etStat PhxCommonGetBytesPerPixel(
   ui32   dwFormat,
   ui32 *pdwBytesPerPixel
) {
   etStat eStat = PHX_OK;

   switch ( dwFormat ) {
      case PHX_DST_FORMAT_Y8: case PHX_DST_FORMAT_BAY8:
         *pdwBytesPerPixel = 1;
         break;

      case PHX_DST_FORMAT_RGB15: case PHX_DST_FORMAT_BGR15:
      case PHX_DST_FORMAT_RGB16: case PHX_DST_FORMAT_BGR16:
      case PHX_DST_FORMAT_Y10:   case PHX_DST_FORMAT_BAY10:
      case PHX_DST_FORMAT_Y12:   case PHX_DST_FORMAT_BAY12:
      case PHX_DST_FORMAT_Y14:   case PHX_DST_FORMAT_BAY14:
      case PHX_DST_FORMAT_Y16:   case PHX_DST_FORMAT_BAY16:
         *pdwBytesPerPixel = 2;
         break;

      case PHX_DST_FORMAT_RGB24: case PHX_DST_FORMAT_BGR24:
         *pdwBytesPerPixel = 3;
         break;

      case PHX_DST_FORMAT_Y32:
      case PHX_DST_FORMAT_RGBX32: case PHX_DST_FORMAT_BGRX32:
      case PHX_DST_FORMAT_XRGB32: case PHX_DST_FORMAT_XBGR32:
      case PHX_DST_FORMAT_RGB32:  case PHX_DST_FORMAT_BGR32:
         *pdwBytesPerPixel = 4;
         break;

      case PHX_DST_FORMAT_RGB48: case PHX_DST_FORMAT_BGR48:
         *pdwBytesPerPixel = 6;
         break;

      default:
         *pdwBytesPerPixel = 0;
         printf("Unrecognised buffer depth\n");
         eStat = PHX_ERROR_BAD_PARAM;
         break;
   }

   return eStat;
}


/*
PhxCommonKbHit()
 * Cross platform implementation of a keyboard input routine to terminate application
 */
int PhxCommonKbHit()
{
#if defined _PHX_POSIX
   char  ch;
   int   nread;

   if( -1 != peek_character )
      return 1;
   new_settings.c_cc[VMIN] = 0;
   tcsetattr(0, TCSANOW, &new_settings);
   nread = read(0, &ch, 1);
   new_settings.c_cc[VMIN] = 1;
   tcsetattr(0, TCSANOW, &new_settings);

   if( 1 == nread ) {
      peek_character = ch;
      return 1;
   }
   return 0;

#elif defined _PHX_VXWORKS
   return ( gnKbHitCount++ > gnKbHitCountMax );

#else
   return kbhit();
#endif
}


/*
PhxCommonKbClose()
 * Cross platform implementation of a keyboard restore routine
 */
void PhxCommonKbClose( void )
{
#ifdef _PHX_POSIX
   tcsetattr(0, TCSANOW, &initial_settings);

#elif defined _PHX_VXWORKS
   /* TODO */

#else
   /* Nothing to do */
#endif
}


/*
PhxCommonKbInit()
 * Cross platform implementation of a keyboard initialisation routine
 */
void PhxCommonKbInit( void )
{
#ifdef _PHX_POSIX
   tcgetattr(0, &initial_settings);
   new_settings = initial_settings;
   new_settings.c_lflag    &= ~ICANON;
   new_settings.c_lflag    &= ~ECHO;
   new_settings.c_lflag    &= ~ISIG;
   new_settings.c_cc[VMIN]  = 1;
   new_settings.c_cc[VTIME] = 0;
   tcsetattr(0, TCSANOW, &new_settings);
#elif defined _PHX_VXWORKS
   gnKbHitCount = 0;

#else
   /* Nothing to do */
#endif
}


/*
PhxCommonKbRead()
 * Cross platform implementation of a keyboard character read routine
 */
int PhxCommonKbRead( void )
{
#ifdef _PHX_POSIX
   char ch;

   if(peek_character != -1)
   {
      ch = peek_character;
      peek_character = -1;
      return ch;
   }
   read(0, &ch, 1);
   return ch;

#elif defined _PHX_VXWORKS
   /* TODO */

#else
   return getch();
#endif
}


#if defined _PHX_VXWORKS
/* Simple function to map from a board number to the relevant etCamConfigLoad value
 */
int PhxGeneric(
   int nBoardNumber
)
{
   return ( PHX_DIGITAL | nBoardNumber );
}

int PhxD24(
   int nBoardNumber
)
{
   return ( PHX_D24CL_PCI32 | nBoardNumber );
}

/* Function used to run our example code, but initialise the keyboard handling before and faster
 * Note: this relies on the fact that the spawn function accepts 10 parameters, although all 10
 * do not have to be specified on the command line
 */
int PhxRun ( int (*pFn)(int, int, int, int, int, int ,int, int, int, int),
             int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8, int a9, int a10
)
{
   int nStatus;

   PhxCommonKbInit();
   nStatus = pFn(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);
   PhxCommonKbClose();
}
#endif

