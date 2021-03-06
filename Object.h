/* 
 * Basic Ray Tracer Component for Hw1
 * Class to represent a general CG object
 * @author Charlene DiMeglio 
 * @author Jorge Leon
 */

 
#ifndef OBJECT_H
#define OBJECT_H
 
#include "Ray.h"
typedef techsoft::matrix<float> fMatrix;

class Object
{
protected:
    // The material of the object, for now a string
    char* material;
    
public:
    
    Object(char* material)
    {
        this->material = material;
    }

    // Virtual function for the intersection of the object with given ray
    virtual Point* intersect(Ray* ray)
    {
        Color c = Color(0,0,0);
        return new Point(0,0,0, &c);
    }
    
    virtual void transform(fMatrix* matrix)
    {
        
    }
        
};

#endif
