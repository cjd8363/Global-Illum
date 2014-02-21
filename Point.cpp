/* 
 * Basic Ray Tracer Component for Hw1
 * Point class
 * @author Charlene DiMeglio
 * @author Jorge Leon
 */
 
#include "Point.h"
 
Point::Point(float x, float y, float z, Color* c) 
{
    this->x = x;
    this->y = y;
    this->z = z;
    this->color = c;
}
    
// Returns distance to given point
float Point::distance(Point point)
{
  return (float)sqrt((float)pow((this->x - point.getX()),2)+(float)pow((this->y - point.getY()),2)+(float)pow((this->z - point.getZ()),2)); 
}

void Point::transform(fMatrix* matrix)
{
    float old[] = {this->x, this->y, this->z, 0};
    Vector vOld(old, 4);
    Vector newer = *matrix * vOld; 
    this->setX(newer[0]);
    
    this->setY(newer[1]);
    
    this->setZ(newer[1]);
}

float Point::getX()
{
    return this->x;
}

float Point::getY()
{
    return this->y;
}

float Point::getZ()
{
    return this->z;
}

void Point::setX(float num)
{
    this->x = num;
}

void Point::setY(float num)
{
    this->y = num;
}

void Point::setZ(float num)
{
    this->z = num;
}

Color* Point::getColor()
{
    return this->color;
}

void Point::setColor(Color* c)
{
    this->color = c;
}
