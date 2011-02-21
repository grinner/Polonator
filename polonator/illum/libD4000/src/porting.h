/* porting.h
Convenience definitions for symbols defined in the windows environment
By Dr. Daniel Levner, Church Lab, Harvard Medical School
Copyright Daniel Levner (c) 2009
*/

#ifndef __PORTING_H_
#define __PORTING_H_


/* Windows type shortcuts */
typedef unsigned int UINT;
typedef unsigned char UCHAR;
typedef unsigned char BYTE;
typedef  unsigned short USHORT;
typedef unsigned long DWORD;
typedef int BOOL;
typedef long LONG;
typedef const char *LPCTSTR;
typedef BOOL *HBITMAP;

#define FAR  // useless remnant from win16
#define TRUE 1
#define FALSE 0
#define MB_OK 1		// return value for message box "ok" click
#define MB_ICONSTOP 3


#define _T(some_string) some_string	// strip a _T("message") to simply "message"
#ifndef ASSERT
#define ASSERT(x) assert(x)		// Posix assert is lowercase
#endif



/* */
/* Windows/EzUSB types */
/* */

typedef struct _VENDOR_OR_CLASS_REQUEST_CONTROL
{
   // transfer direction (0=host to device, 1=device to host)
   UCHAR direction;

   // request type (1=class, 2=vendor)
   UCHAR requestType;

   // recipient (0=device,1=interface,2=endpoint,3=other)
   UCHAR recepient;
   //
   // see the USB Specification for an explanation of the
   // following paramaters.
   //
   UCHAR requestTypeReservedBits;
   UCHAR request;
   USHORT value;
   USHORT index;
} VENDOR_OR_CLASS_REQUEST_CONTROL;



/* Windows Sleep() is in ms, not typical to unix */
#ifdef __cplusplus
extern "C"
{
#endif

	void Sleep(DWORD milliseconds);

#ifdef __cplusplus
}
#endif


#endif //__PORTING_H_

