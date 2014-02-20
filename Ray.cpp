/* 
 * Basic Ray Tracer Component for Hw1
 * Class to represent rays for ray tracer
 * @author Charlene DiMeglio 
 * @author Jorge Leon
 */

#include "Ray.h"
 
Ray::Ray(Point* pnt, Vect* dir)
{
    this->origin = pnt;
    this->direction = dir;
}

Point* Ray::getOrigin()
{
    return this->origin;
}

Vect* Ray::getDirection()
{
    return this->direction;
}

void Ray::setOrigin(Point* pnt)
{
    this->origin = pnt;
}

void Ray::setDirection(Vect* vect)
{
    this->direction = vect;
}