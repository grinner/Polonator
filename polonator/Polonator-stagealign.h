/*#define FLATTEN_WINDOWSIZE 5*/
#define FLATTEN_WINDOWSIZE 3
#define WAIT usleep(10000)
#define DEBUG_STAGEALIGN 1


void register_image(short unsigned int *base_img,
		    short unsigned int *test_img,
		    int *x_offset,
		    int *y_offset,
		    int *score, FILE*);
		    
void flatten_image(int* orig_image,
		   int* new_image,
		   int rescale,
		   int img_size);
void stagealign(int fcnum,
		int lane_num,
		int initialize);

void snap_autoexposure(float exposure, float gain, char *color, char *filename);
