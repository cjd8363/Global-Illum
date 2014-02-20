/* 
 * Basic Ray Tracer Component for Hw1
 * Point class
 * @author Charlene DiMeglio
 * @author Jorge Leon
 * NOT FINISHED
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

//SHELL
//Given a matrix, tranform the given point 
// THOUGHT: I think this should just be a vector.
//          We don't rotate or scale points.
Point* Point::transform(Matrix matrix)
{
    Color c = Color(0.0f, 0.0f, 0.0f);
    Point* p = new Point(0.0f, 0.0f, 0.0f, &c);
    return p;
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
