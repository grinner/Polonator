%module polProcDMD
%{

%}
/* Wyss Institute polonator_proc_DMD.i SWIG interface file */
/* Version 1.0 Nick Conway 1/20/2010 */

/* This function sends an image named 'image.raw' to the */
/* processing machine and waits to receive back */
/* particular offsets */
/* takes as arguments the lane number then the image number*/
extern void py_send_DMD_register_image(int,int);

/* This determines the X image offset between the DMD and the camera */
/* needed since data is stored as two integers */
extern int py_get_X_offset(void);
/* This determines the Y image offset between the DMD and the camera */
/* needed since data is stored as two integers */
extern int py_get_Y_offset(void);

/* This starts communication with the network server */
extern void py_start_network_server(void);

/* This stops communication with the network server */
extern void py_stop_network_server(void);

extern void py_force_network_shutdown(void);
