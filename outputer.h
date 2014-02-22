#ifndef SAVEBMP_CPP
#define SAVEBMP_CPP
#include <vector>
#include <cstdio>
#include "Pixel.h"

class outputer
{
public:
    static void savebmp(const char *filename, float wf, float hf, int dpi,  std::vector< std::vector <Pixel> >* data);
};

#endif