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
    // Create rayDirection vector from ray's direction, normalize. 
	// Since direction is normalized value A becomes 1
	Vect rayDirection = ray->getDirection()->normalize();
	
	// Ray direction variables
	float dx = rayDirection.getX();
	float dy = rayDirection.getY();
	float dz = rayDirection.getZ();
	
	// Ray origin variables 
	float oX = ray->getOrigin()->getX();
	float oY = ray->getOrigin()->getY();
	float oZ = ray->getOrigin()->getZ();
	
	// Select three points from the vertices vector to calculate normal
	Point* first = &vertices.at(0);
	Point* secnd = &vertices.at(1);
	Point* third = &vertices.at(2);
	
	// Create two vectors based on point first-sec and first-third;
	Vect* first_sec = new Vect((secnd->getX()-first->getZ()), (secnd->getY()-first->getY()),(secnd->getZ()- first->getZ()));
	Vect* first_third = new Vect((third->getX()-first->getZ()), (third->getY()-first->getY()),(third->getZ()- first->getZ()));
	
	// Get plane information: vectors and normals
	// Obtain normal by getting the cross product of both vectors
	Vect normal = first_sec->crossProd(first_third);
	
	// From the normal get variables A, B and C
	float A = normal.getX();
	float B = normal.getY();
	float C = normal.getZ();
	
	// Obtain variable F from equation of the plane Ax+By+Cz = F
	float F = (A*first->getX()) + (B*first->getY()) + (C*first->getZ());
	
	// Consider the equation of the plane, the denominator will provide information
	// on if the the ray intersects the plane or not.
	int denominator = (int)(A*dx+B*dy+C*dz);
	
	// if the denominator is 0 then there's no intersection (parallel)
	// also, if denominator is < 0 then ray is behind the origin so ignore
	if (denominator <= 0){
		return NULL;
	}
	
	float w = -(A*oX + B*oY + C*oZ + F)/(float)denominator;
	
	// Otherwise the ray intersects with the plane, get the point of intersection
	Color c = Color(0.0f, 0.0f, 0.0f);
    Point* pPlane = new Point((oX+dx*w),(oY+dy*w), (oZ+dz*w), &c);
    
	// Check if the point is within the polygon now that it's on the plane of the triangle
	return pPlane;
}

// @arg matrix = matrix to transform the polygon with 
void Polygon::transform(fMatrix* matrix)
{
    for (int i = 0; i < this->vertices.size(); i++)
    {
        this->vertices.at(i).transform(matrix);
    }
}