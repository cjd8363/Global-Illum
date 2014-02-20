/* 
 * Basic Ray Tracer Component for Hw1
 * Class to represent a general CG object
 * @author Charlene DiMeglio
 * @author Jorge Leon
 */

 
#ifndef OBJECT_H
#define OBJECT_H
 
#include "Ray.h"
 
class Object
{
protected:
    // The material of the object, for now a string
    char* material;
    
public:
    
    Object(char* material);
    
    // Virtual function for the intersection of the object with given ray
    virtual Point* intersect(Ray ray) = 0;
    
    virtual void transform(Matrix matrix);
};

#endif