#include "Outputer.h"
#include <iostream>
#include <stdlib.h> 
#include <stdio.h>  
using namespace std;

// Type FILE from library
// Function fopen, fwrite, fclose from library cstdio

void outputer::savebmp(const char *filename, float wf, float hf, int dpi,  std::vector< std::vector <Pixel> >* data){
	FILE *f;
    int w = (int)wf;
    int h = (int)hf;
	int k = w * h;
	int s = 12*k;
	int filesize = 54 + s;
	
	float factor = 39.375;
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
	
	for (int i = 0; i < data->size(); i++){
        for (int j = 0; j < data->at(i).size(); j++)
        {
            float red =  	(data->at(i).at(j).getColor()->getRed());
            float green =  (data->at(i).at(j).getColor()->getGreen());
            float blue =   (data->at(i).at(j).getColor()->getBlue());
            //cout << red*255 << " " << blue*255 << " "  << green*255 << endl;
            //unsigned char color[3] = {(int)floor(red*255),(int)floor(green*255),(int)floor(blue*255)};
            int color[3] = {(int)floor(red*255),(int)floor(green*255),(int)floor(blue*255)};
            //int color[3] = {(int)floor(255),(int)floor(0),(int)floor(0)};
            
            fwrite(color,sizeof(int),sizeof(color),f);	
        }
	}
    printf("BEST");
	fclose(f);
}