#ifndef _FINDOBJ_H
#define _FINDOBJ_H

int assign(int, int);
unsigned short find_objects( \
          int len_w, \
          int len_h, \
          unsigned short *raw_image, \
          unsigned short *obj_xcol, \
          unsigned short *obj_yrow, \
          unsigned short *segmask_image);


#endif
