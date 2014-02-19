/* 
 * Basic Ray Tracer Component for Hw1
 * Class to represent rays for ray tracer
 * @author Charlene DiMeglio
 * @author Jorge Leon
 */
 
#ifndef RAY_H
#define RAY_H
 
#include "Point.h"
#include "Vect.h"
 
class Ray
{
private:
    
    // The point of origin
    Point* origin;
    
    // The direction of the array
    Vect* direction;
    
public:
    
    Ray(Point* pnt, Vect* dir);
    
    Point* getOrigin();
    
    Vect* getDirection();
    
    void setOrigin(Point* pnt);
    
    void setDirection(Vect* vect);
};

#endif