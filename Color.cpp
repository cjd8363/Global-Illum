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