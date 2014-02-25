#include "Outputer.h"



void savebmp(const char *filename, float wf, float hf, int dpi,  std::vector< std::vector <Pixel> >* data)
{
       /* Example program fragment to read a PAM or PNM image
      from stdin, add up the values of every sample in it
      (I don't know why), and write the image unchanged to
      stdout. */
   FILE* f = fopen(filename,"wb");
   pm_init(filename, 0);
   pixel** truePix = ppm_allocarray(wf,hf);
   for (int i = 0; i < wf; i++)
   {
       for (int j = 0; j < hf; j++)
       {
           PPM_ASSIGN(truePix[i][j],data->at(i).at(j).getColor()->getRed(), data->at(i).at(j).getColor()->getGreen(), data->at(i).at(j).getColor()->getBlue());
       }
   }
   ppm_writeppm(f, truePix, (int)wf, (int)hf, 256, 0); 
   ppm_freearray(truePix, (int)hf);
}