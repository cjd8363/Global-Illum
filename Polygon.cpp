/* 
 * Basic Ray Tracer Component for Hw1
 * Class to represent general polygons
 * @author Charlene DiMeglio
 * @author Jorge Leon
 */
 #include <string>
#include <vector>
#include <iostream>
#include <stdlib.h> 
#include <stdio.h> 
#include "Polygon.h" 
 
Polygon::Polygon(Point* pts[], char* material) :
    Object(material)
{
    this->vertices = std::vector<Point*>(pts, pts + sizeof(pts)/sizeof(Point));
}

bool sumofAll(Point* p, std::vector<Point> vertices){
	// let size be the size of the container
	int size = vertices.size();
	
	// let angle be the resulting sum of all angles
	float angle = 0.0f;
	
	// let firstLine be the line between the first vertex and the point on the plane
	Vect* firstLine = new Vect(	(p->getX()- vertices.at(0).getX()), 
								(p->getY()- vertices.at(0).getY()),
								(p->getZ()- vertices.at(0).getZ()));
	// let lastLine be the line between the last vertex and the point on the plane
	Vect* lastLine = new Vect(	(p->getX()- vertices.at(size-1).getX()), 
								(p->getY()- vertices.at(size-1).getY()),
								(p->getZ()- vertices.at(size-1).getZ()));
	Vect* tempLine = firstLine;
	int i = 0;
	
	// for all vertices,
	while (!(i > size)){
	// if this is the last vertex, calculate angle and add to the sum of angles
		if (i == size){
			angle += ceil(lastLine->getAngle(firstLine));
			// if the angle is less than 360, then return false
			if (angle < 360.0f){
				return false;
			}
			// otherwise return true
			else 
				return true;
		}
		Vect *tempLine2 = new Vect((p->getX()- vertices.at(i).getX()), 
								(p->getY()- vertices.at(i).getY()),
								(p->getZ()- vertices.at(i).getZ()));
		angle += ceil(tempLine->getAngle(tempLine2));
		i++;
		// Free memory
		delete tempLine2;
	}
	// Free memory
	delete firstLine;
	delete lastLine;
	delete tempLine;
}


// No material provided constructor needed
// Getters, Setters

// Takes in an ray and checks if that ray intersects with the polygon
// If so, the point of intersection will be returned, otherwise NULL
Point* Polygon::intersect(Ray* ray)
{
printf("YIP");
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
	Point* first = vertices.at(0);
	Point* secnd = vertices.at(1);
	Point* third = vertices.at(2);
	
	// Create two vectors based on point first-sec and first-third;
	Vect* first_sec = new Vect((secnd->getX()-first->getX()), (secnd->getY()-first->getY()),(secnd->getZ()- first->getZ()));
	Vect* first_third = new Vect((third->getX()-first->getX()), (third->getY()-first->getY()),(third->getZ()- first->getZ()));
	
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
	// Select the color of any vertex (variable Point first in this case)
    Point* pPlane = new Point((oX+dx*w),(oY+dy*w), (oZ+dz*w), first->getColor());
	
    // Verify if the point is within the polygon, if false return NULL 
	if (!sumofAll(pPlane, vertices)){
		return NULL;
	}
	
	// Else, return the point
	return pPlane;
}

// @arg matrix = matrix to transform the polygon with 
void Polygon::transform(fMatrix* matrix)
{
    for (int i = 0; i < this->vertices.size(); i++)
    {
        this->vertices.at(i)->transform(matrix);
    }
}