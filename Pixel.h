/* 
 * Basic Ray Tracer Component for Hw1
 * Class representing one pixel, in 
 * its color and position relative to camera
 * @author Charlene DiMeglio
 * @author Jorge Leon
 */
 
#include "Point.h"
 
class Pixel
{
private:
     
     //The position of the pixel
     Point* position;
     
     //The color of the pixel
     Color* color;
     
public:
     
     Pixel(Point* pos, Color* col);
     
     Point* getPosition();
     
     Color* getColor();
     
     void setPosition(Point* pos);
     
     void setColor(Color* col);
 };