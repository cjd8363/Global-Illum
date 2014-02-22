/* 
 * Basic Ray Tracer Component for Hw1
 * Class to represent general polygons
 * @author Charlene DiMeglio
 * @author Jorge Leon
 */
 
#include "Polygon.h" 
 
// Maybe not correct
Polygon::Polygon(Point pts[], char* material) :
    Object(material)
{
    this->vertices = std::vector<Point>(pts, pts + sizeof(pts)/sizeof(Point));
}

// No material provided constructor needed
// Getters, Setters


Point* Polygon::intersect(Ray* ray)
{
    Color c = Color(0.0f, 0.0f, 0.0f);
    Point* p = new Point(0.0f, 0.0f, 0.0f, &c);
    return p;
}

void Polygon::transform(fMatrix* matrix)
{
    for (int i = 0; i < this->vertices.size(); i++)
    {
        this->vertices.at(i).transform(matrix);
    }
}