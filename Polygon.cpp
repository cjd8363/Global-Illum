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
    
    Point[] vertices;
    
    Vect[] norms;
    
public:
    
    // Maybe not correct
    Polygon(Point[] pts, Vect[] norms, char* material) :
        Object(material)
    {
        this->vertices = pts;
        this->norms = norms;
    }
    
    // No material provided constructor needed
    // Getters, Setters
    
    
    Point* intersect(Ray ray)
    {
        Color c = Color(0.0f, 0.0f, 0.0f);
        Point* p = new Point(0.0f, 0.0f, 0.0f, &c);
        return p;
    }
    
    void transform(Matrix matrix)
    {
    }
};