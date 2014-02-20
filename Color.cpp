/* 
 * Basic Ray Tracer Component for Hw1
 * Class for colors used for pixels
 * @author Charlene DiMeglio
 * @author Jorge Leon
 */

#include "Color.h"
 
    
Color::Color(float r, float g, float b)
{
    this->red = r;
    this->green = g;
    this->blue = b;
}

float Color::getRed()
{
    return this->red;
}

float Color::getBlue()
{
    return this->blue;
}

float Color::getGreen()
{
    return this->green;
}

void Color::setRed(float red)
{
    this->red = red;
}

void Color::setGreen(float green)
{
    this->green = green;
}

void Color::setBlue(float blue)
{
    this->blue = blue;
}

/*
	void savebmp
	Function that will save a .bmp format file of the ray tracer image
	const char *filename:	name of image file that will be saved
	int w:					width of image file
	int h:					height of image file
	int dpi:				resolution in dots per inch of file
	Color *data:			array of Color objects for image
*/
void savebmp(const char *filename, int w, int h, int dpi, Color *data){
	FILE *f;
	int k = w*h;
	int s = 4*k;
	int filesize = 54 + s;
	
	float factor = 39.375f;
	int m = static_cast <int> (factor);
	
	int ppm = dpi*m;
	
	unsigned char bmpfileheader[14] = {'B','M', 0,0,0,0, 0,0,0,0, 54,0,0,0};
	unsigned char bmpinfoheader[40] = {40,0,0,0, 0,0,0,0, 0,0,0,0, 1,0,24,0};
	
	bmpfileheader[2] = (unsigned char)(filesize);
	bmpfileheader[3] = (unsigned char)(filesize >> 8);
	bmpfileheader[4] = (unsigned char)(filesize >> 16);
	bmpfileheader[5] = (unsigned char)(filesize >> 24);
	
	bmpinfoheader[4] = (unsigned char)(w);
	bmpinfoheader[5] = (unsigned char)(w >> 8);
	bmpinfoheader[6] = (unsigned char)(w >> 16);
	bmpinfoheader[7] = (unsigned char)(w >> 24);
	
	bmpinfoheader[8] = (unsigned char)(h);
	bmpinfoheader[9] = (unsigned char)(h >> 8);
	bmpinfoheader[10] = (unsigned char)(h >> 16);
	bmpinfoheader[11] = (unsigned char)(h >> 24);
	
	bmpinfoheader[21] = (unsigned char)(s);
	bmpinfoheader[22] = (unsigned char)(s >> 8);
	bmpinfoheader[23] = (unsigned char)(s >> 16);
	bmpinfoheader[24] = (unsigned char)(s >> 24);
	
	bmpinfoheader[25] = (unsigned char)(ppm);
	bmpinfoheader[26] = (unsigned char)(ppm >> 8);
	bmpinfoheader[27] = (unsigned char)(ppm >> 16);
	bmpinfoheader[28] = (unsigned char)(ppm >> 24);
	
	bmpinfoheader[29] = (unsigned char)(ppm);
	bmpinfoheader[30] = (unsigned char)(ppm >> 8);
	bmpinfoheader[31] = (unsigned char)(ppm >> 16);
	bmpinfoheader[32] = (unsigned char)(ppm >> 24);
	
	f = fopen(filename,"wb");
	
	fwrite(bmpfileheader,1,14,f);
	fwrite(bmpinfoheader,1,40,f);
	
	for (int i = 0; i <k; i++){
		Color rgb = data[i];
		
		float red   =  	(data[i].getRed())*255;
		float green =  	(data[i].getGreen())*255;
		float blue  =   (data[i].getBlue())*255;

		unsigned char color[3] = {(int)floor(blue),(int)floor(green),(int)floor(red)};
		
		fwrite(color,1,3,f);		
	}
	fclose(f);
	
}
