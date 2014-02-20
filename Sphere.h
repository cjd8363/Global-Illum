/* 
 * Basic Ray Tracer Component for Hw1
 * Class to represent a sphere
 * @author Charlene DiMeglio
 * @author Jorge Leon
 */
 
#include "Object.h"
 
class Sphere : public Object 
{
private:
    //radius of the sphere
    float radius;
    
    //center of sphere
    Point* center;
    
public:
    
    Sphere(float rad, Point* center, char* material);
    
    //Returns null if no intersection
    Point* intersect(Ray ray);
    
};
 
