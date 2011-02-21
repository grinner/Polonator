#ifndef _HARDWARE_D4000_
#define _HARDWARE_D4000_

/* =============================================================================
// 
// Polonator G.007 Selective Release Software
//
// Church Lab, Harvard Medical School
// Written by Daniel Levner
//
// hardware_D4000.h: hardware functions for D4000 DMD array using an API that
// allows overloading support for other hardware using #ifdefs
//
// Original version -- 07-22-2009 [DL]
//
// This software may be modified and re-distributed, but this header must appear
// at the top of the file.
//
// =============================================================================
/*/



#define DEVNUM              0       /* use device 0 */
#define RHW_DISABLE_FILL    0xff    /* all-1 fill when UV is off */
//#define RHW_DISABLE_FILL    0x0    /* all-1 fill when UV is off */

#ifdef __cplusplus
extern "C"
{
#endif



int rhw_init(HardwareData_type *HardwareDescriptor);
int rhw_disable(void);
int rhw_enable(void);
int rhw_load_image(unsigned char *frame);
int rhw_light_off(void);
int rhw_light_on(void);
int rhw_float(void);
int rhw_clear_mem(void);

#ifdef __cplusplus
}
#endif

#endif
