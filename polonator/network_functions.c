/* =============================================================================
// 
// Polonator G.007 Image Acquisition Software
//
// Church Lab, Harvard Medical School
// Written by Greg Porreca
//
// Polonator_networkfunctions.c: functionality for sending images and other data
// to the processing computer
//
// Release 1.0 -- 04-15-2008
//
// This software may be modified and re-distributed, but this header must appear
// at the top of the file.
//
// =============================================================================
/*/

#include <sys/time.h>
#include <time.h>

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "logger.h"
#include "network_functions.h"
#include "get_sock.h"


#define MAXPENDING 5

char error_string[500];
char log_string[500];

int py_serv_sock;
int py_clnt_sock

void py_network_startserver(int port)
{
    network_startserver(&py_serv_sock, &py_clnt_sock, port);
}


/*
// function to start transmit server; listens on port portnum, and returns
// when it receives a connection (assigning the client socket to clnt_sock);
// must call stop_server(serv_sock) when transaction with client is finished
*/
void network_startserver(int *serv_sock, int *clnt_sock, int portnum)
{
    struct sockaddr_in servAddr; /* server address structure */
    struct sockaddr_in clntAddr; /* client address structure */
    unsigned int clntLen; /* length of client socket structure */

    fprintf(stdout, "NetworkFunctions starting...\n");
    /* get server socket */
    if((*serv_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        perror(NULL);
        fprintf(stdout, "ERROR: Polonator_networkfunctions: start_server(): socket() failed\n");
        exit(1);
    }

    /*fcntl(*serv_sock, F_SETFL, O_NONBLOCK);*/

    /* set address params */
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(portnum);
    if(setsockopt(*serv_sock, SOL_SOCKET, SO_REUSEADDR, "TRUE", 4)!=0){
        p_log_errorno("ERROR: Polonator_networkfunctions: start_server(): setsockopt() failed");
        exit(1);
    }
    fprintf(stdout, "NetworkFunctions set address params...\n");
    /* bind socket to address */
    if(bind(*serv_sock, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0)
    {
        fprintf(stdout, "The socket address is % d\n", *serv_sock);
        fprintf(stdout,"PF_INET = %d, SOCK_STREAM = %d, IPPROTO_TCP = %d\n", PF_INET, SOCK_STREAM, IPPROTO_TCP);
        p_log_errorno("ERROR: Polonator_networkfunctions: start_server(): bind() failed");
        exit(1);
    }
    fprintf(stdout, "NetworkFunctions bound socket...\n");
    /* listen for connections */
    if(listen(*serv_sock, MAXPENDING) < 0){
        p_log_errorno("ERROR: Polonator_networkfunctions: start_server(): listen() failed");
        exit(1);
    }
    fprintf(stdout, "NetworkFunctions listen success!\n");
    /* get client socket for current connection */
    clntLen = sizeof(clntAddr);  
    if ((*clnt_sock = accept(*serv_sock, (struct sockaddr *) &clntAddr, &clntLen)) < 0){
        p_log_errorno("ERROR: Polonator_networkfunctions: start_server(): accept() failed");
        fprintf(stdout, "servsock = %d\n", *serv_sock);
        fprintf(stdout, "clntAddr = %d\n", clntAddr);
        fprintf(stdout, "clntLen = %d\n", clntLen);
        exit(1);
    }
    fprintf(stdout, "NetworkFunctions accepted!\n");
}


void py_network_sendfilename(char *filename)
{
    network_sendfilename(py_clnt_sock, filename);
}
/* 
// function sends filename over network to processing computer; assumes
// processor already connected and is present on clnt_sock (by call to
// network_startserver)
// returns 1 if filename was requested, 0 of something else was requested 
*/
int network_sendfilename(int clnt_sock, char *filename){
    char input[1];
    int bytes_sent=0;
    if(recv(clnt_sock, input, 1, 0) < 0)
    {
        sprintf(error_string, "ERROR: Polonator_networkfunctions: network_sendfilename(%s): recv()", filename);
        p_log_errorno(error_string);
        exit(1);
    }

    if(*input == '2') { /* FILENAME REQUESTED */
        sprintf(error_string, "length of filename transmitted is %d", strlen(filename)+1);
        p_log(error_string);
        network_send(clnt_sock, filename, strlen(filename)+1);
        return 1;
    }
    else{ /* SOMETHING ELSE REQUESTED; ERROR */
        fprintf(stdout, "ERROR: Polonator_networkfunctions: network_sendfilename(%s): processor request was '%c'(%d), expecting '2' for filename request\n", filename, *input, *input);
        return 0;
    }
}


int py_network_sendfcnum(int fcnum)
{
    network_sendfcnum(py_clnt_sock, fcnum)
}
/* 
// function sends flowcell number over network to processing computer; assumes
// processor already connected and is present on clnt_sock (by call to
// network_startserver)
// returns 1 if fcnum was requested, 0 of something else was requested 
*/
int network_sendfcnum(int clnt_sock, int fcnum)
{
    char input[1];
    char output = (char)fcnum;
    int bytes_sent=0;
    char log_string[255];

    p_log("waiting for request from processor");
    if(recv(clnt_sock, input, 1, 0)<0){
        sprintf(error_string, "ERROR: Polonator_networkfunctions: network_sendfcnum(%d): recv()", fcnum);
        p_log_errorno(error_string);
        exit(1);
    }

    if(*input == '3'){ /* FLOWCELL NUMBER REQUESTED */
        sprintf(log_string, "received %c %d from processor, sending fcnum\n", *input, *input);
        p_log(log_string);
        network_send(clnt_sock, &output, 1);
        return 1;
    }
    else{ /* SOMETHING ELSE REQUESTED; ERROR */
        sprintf(log_string, "ERROR: Polonator_networkfunctions: network_sendfcnum(%d): processor request was '%c'(%d), expecting '3' for fcnum request\n", fcnum, *input, *input);
        p_log_errorno(log_string);
        return 0;
    }
}
  
int py_network_waitforsend(void)
{
    network_waitforsend(py_clnt_sock);
}

/* function blocks until request for next image is received from processor */
int network_waitforsend(int clnt_sock)
{
    char input[2];
    char log_string[255];
    int bytes_recvd=0;
    struct timeval tv1; 
    struct timeval tv2; 
    int wait_time;
    int timeout = 10;

    p_log("waiting to receive request from processing computer");

    bytes_recvd += recv(clnt_sock, input, 2, 0);

    sprintf(log_string, "network_waitforsend() received %d bytes of data from processor", bytes_recvd);
    p_log(log_string);


    if(bytes_recvd > 0)
    {
        if(bytes_recvd == 1)
        {
            sprintf(log_string, "received %c:%d", input[0], input[0]);
            p_log(log_string);
        }
        else
        {
            sprintf(log_string, "received %c:%d %c:%d", input[0], input[0], input[1], input[1]);
            p_log(log_string);
        }
    }
  

    if(bytes_recvd > 0)
    {
        if((*input == '1')||(*input == '8')){ /* IMAGE REQUESTED */
            p_log("image requested");
            return 1;
        } 

        else if(((int)input[0]) > 10){ /* IMAGE REQUESTED */
            p_log("autoexp gain received");
            return ((int)input[0]);
        } 

        else { /* ERROR: SOMETHING ELSE REQUESTED */
            sprintf(error_string, "ERROR: Polonator_networkfunctions: network_sendimage(): processor request was '%c'(%d), expecting '1' for image request", *input, *input);
            p_log(error_string);
            p_log_errorno("");
            return 0;
        }    
    }

    else if(bytes_recvd == 0){
        p_log_errorno("STATUS: Polonator_networkfunctions: network_waitforsend: recv() returned 0 bytes");
    }

    else
    {
        p_log_errorno("ERROR: Polonator_networkfunctions: network_waitforsend: recv() returned < 0");
        exit(1);
    }

  return -1;
}

int py_network_waitforDMD() {
    network_waitforDMD(py_clnt_sock);
}   

int network_waitforDMD(int clnt_sock)
{
    char input[2];
    char log_string[255];
    int bytes_recvd=0;
    struct timeval tv1; 
    struct timeval tv2; 
    int wait_time;
    int timeout = 10;

    p_log("waiting to receive DMD register offset from processing computer");

    bytes_recvd += recv(clnt_sock, input, 1, 0);

    sprintf(log_string, "network_waitforDMD() received %d bytes of data from processor", bytes_recvd);
    p_log(log_string);


    if(bytes_recvd > 0){
        if(bytes_recvd == 1){
            sprintf(log_string, "received %c:%d", input[0], input[0]);
            p_log(log_string);
        }
        else{
            sprintf(log_string, "received %c:%d %c:%d", input[0], input[0], input[1], input[1]);
            p_log(log_string);
        }
    }


    if(bytes_recvd > 0){
        if((bytes_recvd==1) && ((int)input[0]>8)){
            p_log("DMD register offset received");
            return ((int)input[0]);     
        }
    }
    else if(bytes_recvd == 0){
        /*p_log_errorno("STATUS: Polonator_networkfunctions: network_waitforsend: recv() returned 0 bytes");*/
    }
    else{
        p_log_errorno("ERROR: Polonator_networkfunctions: network_waitforsend: recv() returned < 0");
        exit(1);
    }

    return -1;
}    

/* do not call this function until network_waitforsend() has returned true */
/* signalling the processor is ready to receive the data */
int py_network_sendimage(   short unsigned int fcnum,
                            short unsigned int arraynum,
                            short unsigned int imagenum,
                            short unsigned int *image_ptr)
{
    network_sendimage(clnt_sock,fcnum, arraynum, imagenum,image_ptr);
}
int network_sendimage(int clnt_sock, 
              short unsigned int fcnum,
              short unsigned int arraynum,
              short unsigned int imagenum,
              short unsigned int *image_ptr)
{

    short unsigned int image_info[4];

    image_info[0] = fcnum;
    image_info[1] = arraynum;
    image_info[2] = imagenum;
    image_info[3] = 0;


    network_send(clnt_sock, (char*)image_info, 8);
    network_send(clnt_sock, (char*)image_ptr, 2000000);
    return 1;
}
  
void py_network_send(char *data, int bytes_to_send)
{
    network_send(clnt_sock, data, bytes_to_send);
}

void network_send(int clnt_sock, char *data, int bytes_to_send)
{
    int bytes_sent;
    int total_bytes_sent;
    char log_string[255];

    bytes_sent = 0;
    total_bytes_sent = 0;

    while(total_bytes_sent < bytes_to_send)
    {
        if((bytes_sent=send(clnt_sock, data + total_bytes_sent, bytes_to_send-total_bytes_sent, 0))<0){
            sprintf(error_string, "ERROR: Polonator_networkfunctions: network_send(): send() failed, sent %d bytes of %d", total_bytes_sent, bytes_to_send);
            p_log_errorno(error_string);
            perror(NULL);
            exit(1);
        }
        sprintf(log_string, "STATUS:\tPolonator_networkfunctions: sent %d bytes", bytes_sent);
        p_log(log_string);
        total_bytes_sent += bytes_sent;
    }
}

void py_network_iboot_on(void)
{
    network_iboot_on(&clnt_sock);
}

void network_iboot_on(int *clnt_sock)
{
    char command[255];
    sprintf(command, "%cPASS%cn\r", 27, 27);

    *clnt_sock = GetSock("iboot", 80);
    network_send(*clnt_sock, command, strlen(command)); 
    sprintf(log_string, "STATUS:\tcamera on, %d", 1);
    p_log_simple(log_string);
}

void py_network_iboot_off(void)
{
    network_iboot_off(&clnt_sock);
}

void network_iboot_off(int *clnt_sock)
{
    char command[255];
    sprintf(command, "%cPASS%cf\r", 27, 27);

    *clnt_sock = GetSock("iboot", 80);
    network_send(*clnt_sock, command, strlen(command)); 
    sprintf(log_string, "STATUS:\tcamera off, %d", 0);
    p_log_simple(log_string);
}

void py_network_stopserver(void)
{
    network_stopserver(serv_sock);
}

void network_stopserver(int serv_sock)
{
    if(shutdown(serv_sock,2) < 0)
    {
        p_log_errorno("ERROR: Polonator_networkfunctions: stop_server(): shutdown() failed");
        exit(1);
    }
}

void py_network_shutdown(void)
{
    network_shutdown();
}

void network_shutdown(void){
    int serv_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    network_stopserver(serv_sock);
}

/*
int main(int argc, char *argv[]){;}
*/
