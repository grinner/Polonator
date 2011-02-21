#ifndef _IMGTOOLS_H
#define _IMGTOOLS_H

void flatten_image(int len_w, int len_h, \
           unsigned short* orig_image, \
           unsigned short* new_image, \
           int rescale);
int stdev(int len_w, int len_h, unsigned short *data);
int stdev2(int len_w, int len_h, unsigned short *data);
int mean(int len_w, int len_h, unsigned short *data);
 
#endif
