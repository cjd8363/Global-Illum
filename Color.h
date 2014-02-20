/* 
 * Basic Ray Tracer Component for Hw1
 * Class for colors used for pixels
 * @author Charlene DiMeglio
 * @author Jorge Leon
 */
#ifndef COLOR_H
#define COLOR_H

#include <cstdio>  // Library required for the savebmp fwrite, fopen functions
#include <cmath>   // Library required for the savebmp floor function 
class Color
{
private:
    
    //Red component of color
    float red;
    
    //Green component of color
    float green;
    
    //Blue component of color
    float blue;
    
public:
    
    Color (float r, float g, float b);
    
    float getRed();
    
    float getBlue();
    
    float getGreen();
    
    void setRed(float red);
    
    void setGreen(float green);
    
    void setBlue(float blue);
	
	void savebmp(const char *filename, int w, int h, int dpi, Color *data);
};
#endif