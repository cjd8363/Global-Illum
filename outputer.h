#ifndef SAVEBMP_CPP
#define SAVEBMP_CPP
#include <vector>
#include <cstdio>
#include "Pixel.h"

#include "netpbm/ppm.h"

class outputer
{
public:
    static void savebmp(const char *filename, float wf, float hf, int dpi,  std::vector< std::vector <Pixel> >* data);
    static void saveppm(const char *filename, float wf, float hf, int dpi,  std::vector< std::vector <Pixel> >* data);
};

#endif