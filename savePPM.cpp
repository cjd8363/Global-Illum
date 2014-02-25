#include "Outputer.h"
#include <iostream>
#include <stdlib.h> 
#include <stdio.h>  
using namespace std;

// Type FILE from library
// Function fopen, fwrite, fclose from library cstdio

void outputer::saveppm(const char *filename, float wf, float hf, int dpi,  std::vector< std::vector <Pixel> >* data){
	FILE *f;
    f = fopen(filename,"wb");
    char fileType[] = {'P','3','\n'}; 
    //fwrite(fileType,sizeof(char),sizeof(fileType),f);
    fprintf(f, "P3 \n");
    char dimen[] = {(char)wf, ' ',(char)hf,'\n'}; 
    //fwrite(dimen,sizeof(char),sizeof(dimen),f);
    fprintf(f, "%d %d \n", (int)wf, (int)hf);
    char max[] = {'2', '5', '5', '\n'};
    //fwrite(max,sizeof(char),sizeof(max),f);
    fprintf(f, "255 \n");
    
	for (int i = 0; i < data->size(); i++){
        for (int j = 0; j < data->at(i).size(); j++)
        {
            float red =  	(data->at(i).at(j).getColor()->getRed());
            float green =  (data->at(i).at(j).getColor()->getGreen());
            float blue =   (data->at(i).at(j).getColor()->getBlue());
            
            int color[] = {(int)floor(red*255), ' ',(int)floor(green*255), ' ',(int)floor(blue*255), ' '};
            
            //fwrite(color,sizeof(int),sizeof(color),f);
            fprintf(f, "%d %d %d ", (int)(red), (int)(green), (int)(blue));
        }
        fprintf(f,"\n");
	}
    char end[] = {'\n'};
    //fwrite(end,sizeof(char),sizeof(end),f);
	fclose(f);
}