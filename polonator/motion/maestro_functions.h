#ifndef _POL_MAESTRO
#define _POL_MAESTRO


/* python interfaces; socket is kept as an internal global */ 
void py_maestro_open(char*, int);
void py_maestro_setupimaging(int, int, int, int, int, int, int);
void py_maestro_shutter_open(void);
void py_maestro_shutter_close(void);
void py_maestro_setcolor(char*);
void py_maestro_unlock(void);
void py_maestro_lock(void);
void py_maestro_darkfield_on(void);
void py_maestro_darkfield_off(void);
/*added by NC 11/22/09*/
void py_maestro_goto_image(int flowcell, int lane, int image_number);
/*added by NC 9/15/10*/
void py_maestro_hometheta(void);
void py_maestro_unlocktheta(void);
void py_maestro_locktheta(void);
void py_maestro_snap(int integration_inmsec, int shutterflag);
void py_maestro_setflag(void);
void py_maestro_stop(void);
void py_maestro_getstatus(void);
void py_maestro_resetflag(void);

void maestro_open(int *m_sock);
void consume_bytes(int m_sock, int num_bytes);
void maestro_setflag(int m_sock);
void maestro_resetflag(int m_sock);
int maestro_getflag(int m_sock);
void maestro_setupimaging(int, int, int, int, int, int, int, int);
void maestro_setcolor(int, char*);
int maestro_thetadone(int);
int maestro_thetahoming(int);
int maestro_stageinmotion(int);
void maestro_unlock(int);
void maestro_lock(int);
void maestro_autofocus_on(int);
void maestro_shutteropen(int);
void maestro_shutterclose(int);
void maestro_readresponse(int m_sock, char *response, int *response_length);
void maestro_readresponse2(int m_sock, char *response, int *response_length);
void maestro_stop(int);
int maestro_get_scan_status(int);
void maestro_darkfield_on(int);
void maestro_darkfield_off(int);
void maestro_hometheta(int);
void maestro_unlocktheta(int);
void maestro_locktheta(int m_sock);
void maestro_getstatus(int);
void maestro_setTDI(int, int);
void maestro_debug(int);
void maestro_setWL(int);
void maestro_setfocus(int, int);
int maestro_getfocus(int);
int maestro_homing(int);
void maestro_writefocus(int);
void maestro_gotostagealign_position(int, int, int);
void maestro_snap(int, int, int);
/*added by NC 11/22/09*/
void maestro_goto_image(int m_sock, int flowcell, int lane, int image_number);
/*added by NC 01/25/10*/
void maestro_reset(int m_sock);

#endif
