#include "pixutils.h"
#include "bmp/bmp.h"

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

//private methods -> make static
static pixMap* pixMap_init();
static pixMap* pixMap_copy(pixMap *p);

//plugin methods <-new for Assignment 4;
static void rotate(pixMap *p, pixMap *oldPixMap,int i, int j,void *data);
static void convolution(pixMap *p, pixMap *oldPixMap,int i, int j,void *data);
static void flipVertical(pixMap *p, pixMap *oldPixMap,int i, int j,void *data);
static void flipHorizontal(pixMap *p, pixMap *oldPixMap,int i, int j,void *data);

static pixMap* pixMap_init(){
	pixMap *p=malloc(sizeof(pixMap));
	p->pixArray_overlay=0;
	return p;
}	

void pixMap_destroy (pixMap **p){
	if(!p || !*p) return;
	pixMap *this_p=*p;
	if(this_p->pixArray_overlay)
	 free(this_p->pixArray_overlay);
	if(this_p->image)free(this_p->image);	
	free(this_p);
	this_p=0;
}
	
pixMap *pixMap_read(char *filename){
	pixMap *p=pixMap_init();
 int error;
 if((error=lodepng_decode32_file(&(p->image), &(p->imageWidth), &(p->imageHeight),filename))){
  fprintf(stderr,"error %u: %s\n", error, lodepng_error_text(error));
  return 0;
	}
	p->pixArray_overlay=malloc(p->imageHeight*sizeof(rgba*));
	p->pixArray_overlay[0]=(rgba*) p->image;
	for(int i=1;i<p->imageHeight;i++){
  p->pixArray_overlay[i]=p->pixArray_overlay[i-1]+p->imageWidth;
	}			
	return p;
}
int pixMap_write(pixMap *p,char *filename){
	int error=0;
 if(lodepng_encode32_file(filename, p->image, p->imageWidth, p->imageHeight)){
  fprintf(stderr,"error %u: %s\n", error, lodepng_error_text(error));
  return 1;
	}
	return 0;
}	 

pixMap *pixMap_copy(pixMap *p){
	pixMap *new=pixMap_init();
	*new=*p;
	new->image=malloc(new->imageHeight*new->imageWidth*sizeof(rgba));
	memcpy(new->image,p->image,p->imageHeight*p->imageWidth*sizeof(rgba));	
	new->pixArray_overlay=malloc(new->imageHeight*sizeof(void*));
	new->pixArray_overlay[0]=(rgba*) new->image;
	for(int i=1;i<new->imageHeight;i++){
  new->pixArray_overlay[i]=new->pixArray_overlay[i-1]+new->imageWidth;
	}	
	return new;
}

	
void pixMap_apply_plugin(pixMap *p,plugin *plug){
	pixMap *copy=pixMap_copy(p);
	for(int i=0;i<p->imageHeight;i++){
	 for(int j=0;j<p->imageWidth;j++){
			plug->function(p,copy,i,j,plug->data);
		}
	}
	pixMap_destroy(&copy);	 
}

int pixMap_write_bmp16(pixMap *p,char *filename){
 BMP16map *bmp16=BMP16map_init(p->imageHeight,p->imageWidth,0,5,6,5); //initialize the bmp type
 if(!bmp16) return 1;
 
	//bmp16->pixArray[i][j] is 2-d array for bmp files. It is analogous to the one for our png file pixMaps except that it is 16 bits
	
 //However pixMap and BMP16_map are "upside down" relative to each other
 //need to flip one of the the row indices when copying
	uint16_t rMask = ((1 << bmp16->Rbits) - 1) << (8 - bmp16->Rbits);
	uint16_t gMask = ((1 << bmp16->Gbits) - 1) << (8 - bmp16->Gbits);
	uint16_t bMask = ((1 << bmp16->Bbits) - 1) << (8 - bmp16->Bbits);
	for (int i = 0; i < p->imageHeight; i++) {
		for (int j = 0; j < p->imageWidth; j++) {
			uint16_t pix16 = 0;
			uint16_t r16 = p->pixArray_overlay[p->imageHeight - 1 - i][j].r;
			uint16_t g16 = p->pixArray_overlay[p->imageHeight - 1 - i][j].g;
			uint16_t b16 = p->pixArray_overlay[p->imageHeight - 1 - i][j].b;
			r16 = (r16 & rMask) << 8;
			g16 = (g16 & gMask) << (8 - bmp16->Rbits);
			b16 = (b16 & bMask) >> (8 - bmp16->Bbits);
			pix16 = r16 | g16 | b16;
			bmp16->pixArray[i][j] = pix16;
		}
	}
	
	BMP16map_write(bmp16,filename);
	BMP16map_destroy(&bmp16);
	return 0;
}	 
void plugin_destroy(plugin **plug){
 //free the allocated memory and set *plug to zero (NULL)
	if ((*plug)->data) {
		free((*plug)->data);
	}
 free(*plug);
}

plugin *plugin_parse(char *argv[] ,int *iptr){
	//malloc new plugin
	plugin *new=malloc(sizeof(plugin));
	new->function=0;
	new->data=0;
	
	int i=*iptr;
	if(!strcmp(argv[i]+2,"rotate")){
  new->function=rotate;
  new->data=malloc(2 *sizeof(float));
  float *sc=(float*) new->data;
  int i=*iptr;
  float theta=atof(argv[i+1]);
  sc[0]=sin(degreesToRadians(-theta));
	 sc[1]=cos(degreesToRadians(-theta));
		//code goes here
		*iptr=i+2;
		return new;	
	}	
	if(!strcmp(argv[i]+2,"convolution")){
				//code goes here
		new->function = convolution;
		new->data = malloc(9 * sizeof(float));
		float *kernelMatrix = (float*) new->data;
		int k = *iptr;
		for (int j = 1; j < 10; j++) {
			kernelMatrix[j - 1] = atof(argv[k + j]);
		}
		*iptr=i+10;	
		return new;
	}
	if(!strcmp(argv[i]+2,"flipHorizontal")){
			//code goes here	
		new->function = flipHorizontal;	
		*iptr=i+1;
		return new;
	}
	if(!strcmp(argv[i]+2,"flipVertical")){
		//code goes here
		new->function = flipVertical;
		*iptr=i+1;
		return new;
	}		
	return(0);
}	

//define plugin functions

static void rotate(pixMap *p, pixMap *oldPixMap,int i, int j,void *data){
 float *sc=(float*) data;
 const float ox=p->imageWidth/2.0f;
 const float oy=p->imageHeight/2.0f;
 const float s=sc[0];
	const float c=sc[1];
	const int y=i;
	const int x=j;
	float rotx = c*(x-ox) - s * (oy-y) + ox;
 float roty = -(s*(x-ox) + c * (oy-y) - oy);
 int rotj=rotx+.5;
	int roti=roty+.5; 
	if(roti >=0 && roti < oldPixMap->imageHeight && rotj >=0 && rotj < oldPixMap->imageWidth){
   memcpy(p->pixArray_overlay[y]+x,oldPixMap->pixArray_overlay[roti]+rotj,sizeof(rgba));			 
		}
		else{
   memset(p->pixArray_overlay[y]+x,0,sizeof(rgba));		
		}		
}

static void convolution(pixMap *p, pixMap *oldPixMap,int i, int j,void *data){
	//implement algorithm givne in https://en.wikipedia.org/wiki/Kernel_(image_processing)
	//assume that the kernel is a 3x3 matrix of integers
	//don't forget to normalize by dividing by the sum of all the elements in the matrix
	double red = 0;
	double green = 0;
	double blue = 0;
	float sum = 0;
	
	for (int y = 0; y < 3; y++) {
		for (int x = 0; x < 3; x++) {
			int corrY = i + (y - 1);
			int corrX = j + (x - 1);
			if (corrY < 0) corrY = 0;
			if (corrX < 0) corrX = 0;
			if (corrY >= oldPixMap->imageHeight) corrY = oldPixMap->imageHeight - 1;
			if (corrX >= oldPixMap->imageWidth) corrX = oldPixMap->imageWidth - 1;
			rgba *workedPix = oldPixMap->pixArray_overlay[corrY] + corrX;
			float kernelElement = ((float*)data)[(2 * 3 - y * 3) + (2 - x)];
			red += workedPix->r * kernelElement;
			green += workedPix->g * kernelElement;
			blue += workedPix->b * kernelElement;
			sum += ((float*)data)[(2 * 3 - y * 3) + (2 - x)];
		}
	}
	p->pixArray_overlay[i][j].r = MIN(MAX((red / sum), 0), 255);
	p->pixArray_overlay[i][j].g = MIN(MAX((green / sum), 0), 255);
	p->pixArray_overlay[i][j].b = MIN(MAX((blue / sum), 0), 255);
}

//very simple functions - does not use the data pointer - good place to start
 
static void flipVertical(pixMap *p, pixMap *oldPixMap,int i, int j,void *data){
 //reverse the pixels vertically - can be done in one line		
 memcpy(p->pixArray_overlay[p->imageHeight - 1 - i] + j, oldPixMap->pixArray_overlay[i] + j, sizeof(rgba));
}	 
static void flipHorizontal(pixMap *p, pixMap *oldPixMap,int i, int j,void *data){
 //reverse the pixels horizontally - can be done in one line
 memcpy(p->pixArray_overlay[i] + p->imageWidth - 1 - j, oldPixMap->pixArray_overlay[i] + j, sizeof(rgba));
}
