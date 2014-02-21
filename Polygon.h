/* 
 * Basic Ray Tracer Component for Hw1
 * Class to represent general polygons
 * @author Charlene DiMeglio
 * @author Jorge Leon
 */
 
#include "Object.h" 
 
class Polygon : public Object
{
private:
    
    Point* vertices;
    
    Vect norm;
    
public:
    
    // Maybe not correct
    Polygon(Point* pts, char* material);
    
    // No material provided constructor needed
    // Getters, Setters
    
    
    Point* intersect(Ray ray);
    
    void transform(fMatrix* matrix);
};