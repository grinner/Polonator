%module alignmentFunctions

%include "carrays.i"

%{
#include "Illuminate_common.h"
%}

/* Creates new_CameraImgArray(), delete_CameraImgArray(), CameraImgArray_getitem(), and CameraImgArray_setitem() */
/* preferably done explicitly to load data size from file */
%array_functions(unsigned short, CameraImgArray)

/* Creates new_intArray(), delete_intArray(), intArray_getitem(), and intArray_setitem() */
/* preferably done explicitly to load data size from file */
%array_functions(int, intArray);

/* these inline functions allow for us to access coordinate data structures in C from python */
%inline %{
IlluminateCoords_type * new_coordArray(int size)
{
	return calloc(size,sizeof(IlluminateCoords_type ));
}

void delete_coordArray(IlluminateCoords_type * arry)
{
	return free(arry);
}

signed short int coordArray_get_x(IlluminateCoords_type *c_xy, int i) 
{
	return c_xy[i].x;
}
signed short int coordArray_get_y(IlluminateCoords_type *c_xy, int i) 
{
	return c_xy[i].y;
}
void coordArray_set_x(IlluminateCoords_type *c_xy, int i, signed short int val) 
{
	c_xy[i].x;
}
void coordArray_set_y(IlluminateCoords_type *c_xy, int i, signed short int val) 
{
	c_xy[i].y = val;
}

IlluminateCoords_type * ptr_coordArray(IlluminateCoords_type *c_xy, int i) 
{
	return &(c_xy[i]);
}

%}

int illum_af_init(int IlluminateWidth, int IlluminateHeight, int CameraWidth, int CameraHeight);
int illum_af_load(int *al_params, int param_num);
int illum_af_load_identity(void);
int illum_af_transform_coords(IlluminateCoords_type *rel_coords, IlluminateCoords_type *cam_coords, int num_coords);
/*int illum_af_transform_mask(IlluminateMask outmask, IlluminateMask inmask);*/
int illum_af_find_spot(unsigned short *image, IlluminateCoords_type *coords);



