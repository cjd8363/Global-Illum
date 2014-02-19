/* 
 * Basic Ray Tracer Component for Hw1
 * Class to represent vectors
 * @author Charlene DiMeglio
 * @author Jorge Leon
 */

#include "Vect.h"
 
Vect::Vect(float x, float y, float z)
{
    this->x = x;
    this->y = y;
    this->z = z;
}

// Default contructor, all 0
Vect::Vect()
{
    this->x = 0.0;
    this->y = 0.0;
    this->z = 0.0;
}
  
Vect Vect::add(Vect* v)
{
    return Vect(this->x + v->getX(), this->y + v->getY(), this->z + v->getZ());
}

Vect Vect::sub(Vect* v)
{
    return Vect(this->x - v->getX(), this->y - v->getY(), this->z - v->getZ());
}

Vect Vect::crossProd (Vect* v)
{
    return Vect((this->y * v->getZ()) - (this->z * v->getY()), 
                (this->z * v->getX()) - (this->x * v->getZ()), 
                (this->x * v->getY()) - (this->y * v->getX()));
}

float Vect::length()
{
    return (float)sqrt((x*x) + (y*y) + (z*z));
}

Vect Vect::normalize()
{
    float mag = this->length();
    return Vect(this->x/mag, this->y/mag, this->z/mag);
}

Vect Vect::negative()
{
    return Vect(-(this->x), -(this->y), -(this->z));
}

float Vect::dotProduct(Vect* v){
    return ((this->x * v->getX()) + (this->y * v->getY()) + (this->z * v->getZ()));
}

Vect Vect::vectMult(float sca){
    return Vect (sca * this->x, sca * this->y, sca * this->z);
}

void Vect::setX(float x)
{
    this->x = x;
}

void Vect::setY(float y)
{
    this->y = y;
}

void Vect::setZ(float Z)
{
    this->z = z;
}

float Vect::getX()
{
    return this->x;
}

float Vect::getY()
{
    return this->y;
}

float Vect::getZ()
{
    return this->z;
}

