/* 
 * Basic Ray Tracer Component for Hw1
 * Class to represent general polygons
 * @author Charlene DiMeglio
 * @author Jorge Leon
 */
 
#include "Object.h" 
#include "Vect.h"
#include <vector>

class Polygon : public Object
{
private:
    
    // The verticies of this triangle
    std::vector<Point> vertices;
    
    // The normal for this triangle
    Vect norm;
    
public:
    
    // Maybe not correct
    Polygon(Point pts[], char* material);
    
    // Checks if ray intersects with this
    Point* intersect(Ray* ray);
    
    // Moves this according to the given polygon
    void transform(fMatrix* matrix);
	
	// Runs sum of all angles algorithm
	bool sumofAll(Point* p, std::vector<Point> vertices);
};