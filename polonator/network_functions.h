#ifndef _POL_NETWORK_FUNC_H
#define _POL_NETWORK_FUNC_H

void network_startserver(int *serv_sock, int *clnt_sock, int portnum);
int network_sendfilename(int clnt_sock, char *filename);
int network_sendfcnum(int clnt_sock, int fcnum);
int network_waitforsend(int clnt_sock);
int network_waitforDMD(int clnt_sock);
int network_sendimage(int clnt_sock, short unsigned int fcnum, short unsigned int arraynum, short unsigned int imagenum, short unsigned int *image_ptr);
void network_stopserver(int serv_sock);
void network_iboot_on(int* clnt_sock);
void network_iboot_off(int* clnt_sock);
void network_send(int clnt_sock, char* data, int bytes_to_send);
void network_shutdown(void);

void py_network_startserver(int portnum);
int py_network_sendfilename(char *filename);
int py_network_sendfcnum(int fcnum);
int py_network_waitforsend(void);
int py_network_waitforDMD(void);
int py_network_sendimage(short unsigned int fcnum, short unsigned int arraynum, short unsigned int imagenum, short unsigned int *image_ptr);
void py_network_stopserver(void);
void py_network_iboot_on(void);
void py_network_iboot_off(void);
void py_network_send(char* data, int bytes_to_send);

/* Added by NC 2/3/2010 */
void py_network_shutdown(void);

#endif

