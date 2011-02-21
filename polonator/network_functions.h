#ifndef _POL_NETWORK_FUNC_H
#define _POL_NETWORK_FUNC_H

void network_startserver(int *serv_sock, int *clnt_sock, int portnum);
int network_sendfilename(int clnt_sock, char *filename);
int network_sendfcnum(int clnt_sock, int fcnum);
int network_waitforsend(int clnt_sock);
int network_waitforDMD(int clnt_sock);
int network_sendimage(int clnt_sock, short unsigned int fcnum, short unsigned int arraynum, short unsigned int imagenum, short unsigned int *image_ptr);
void network_stopserver(int serv_sock);
void network_iboot_on(int*);
void network_iboot_off(int*);
void network_send(int, char*, int);

/* Added by NC 2/3/2010 */
void network_shutdown(void);

#endif
