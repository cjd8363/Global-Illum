/* 
 * Basic Ray Tracer Component for Hw1
 * Class to represent a sphere
 * @author Charlene DiMeglio
 * @author Jorge Leon
 */

#include "Sphere.h"
 
Sphere::Sphere(float rad, Point* center, char* material) :
    Sphere::Object(material)
{
    this->radius = rad;
    this->center = center;
}

//Returns null if no intersection
Point* Sphere::Object::intersect(Ray ray)
{
    //TODO
    Color c = Color(0.0f, 0.0f, 0.0f);
    Point* p = new Point(0.0f, 0.0f, 0.0f, &c);
    return p;
} 
 
