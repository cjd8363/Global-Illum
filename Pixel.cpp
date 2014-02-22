/* 
 * Basic Ray Tracer Component for Hw1
 * Class representing one pixel, in
 * its color and position relative to camera
 * @author Charlene DiMeglio 
 * @author Jorge Leon
 */
 
#include "Pixel.h"
 
 // position = position of pixel in worldspace, color = color of pixel
 Pixel::Pixel(Point* pos, Color* col)
 {
     this->position = pos;
     this->color = col;
 }
     
 Point* Pixel::getPosition()
 {
     return this->position;
 }
 
 Color* Pixel::getColor()
 {
     return this->color;
 }
 
 void Pixel::setPosition(Point* pos)
 {
     this->position = pos;
 }
 
 void Pixel::setColor(Color* col)
 {
     this->color = col;
 }
