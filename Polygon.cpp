/* 
 * Basic Ray Tracer Component for Hw1
 * Class to represent general polygons
 * @author Charlene DiMeglio
 * @author Jorge Leon
 */
 
#include "Polygon.h" 
 
Polygon::Polygon(Point pts[], char* material) :
    Object(material)
{
    this->vertices = std::vector<Point>(pts, pts + sizeof(pts)/sizeof(Point));
}

// No material provided constructor needed
// Getters, Setters

// Takes in an ray and checks if that ray intersects with the polygon
// If so, the point of intersection will be returned, otherwise NULL
Point* Polygon::intersect(Ray* ray)
{
    Color c = Color(0.0f, 0.0f, 0.0f);
    Point* p = new Point(0.0f, 0.0f, 0.0f, &c);
    return p;
}

// @arg matrix = matrix to transform the polygon with 
void Polygon::transform(fMatrix* matrix)
{
    for (int i = 0; i < this->vertices.size(); i++)
    {
        this->vertices.at(i).transform(matrix);
    }
}