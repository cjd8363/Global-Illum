/* 
 * Basic Ray Tracer Component for Hw1
 * Class to represent a sphere
 * @author Charlene DiMeglio
 * @author Jorge Leon
 */

#include "Sphere.h"
 
Sphere::Sphere(float rad, Point* center, char* material) :
    Sphere::Object(material)
{
    this->radius = rad;
    this->center = center;
}

//Returns null if no intersection
Point* Sphere::intersect(Ray* ray)
    {
        // Create rayDirection vector from ray's direction, normalize. 
		// Since direction is normalized value A becomes 1
		Vect rayDirection = ray->getDirection()->normalize();
	//	*rayDirection.normalize();
		
		// Direction variables
		float dx = rayDirection.getX();
		float dy = rayDirection.getY();
		float dz = rayDirection.getZ(); 
		
		// Center variables
		float cX = this->center->getX();
		float cY = this->center->getY();
		float cZ = this->center->getZ();
		
		// Origin variables 
		float oX = ray->getOrigin()->getX();
		float oY = ray->getOrigin()->getY();
		float oZ = ray->getOrigin()->getZ();
		
		float B = 2*(dx*(oX-cX)+dy*(oY-cY)+dz*(oZ-cZ));
		float C = ((oX-cX)*(oX-cX)+(oY-cY)*(oY-cY)+(oZ-cZ)*(oZ-cZ))-((this->radius)*(this->radius));
		 
		// decider variable provides info about intersection of ray and sphere
		float decider = B*B - 4*C;
		
		// If decider is less than 0, no real root, no intersection point
		if (decider < 0){
			Color c = Color(0.0f, 0.0f, 0.0f);
			Point* p = new Point(0.0f, 0.0f, 0.0f, &c);
			return p;
		}
		
		// If decider is more than 0, ray goes through sphere
		else if (decider > 0){
			float root_1 = ((-B)+(float)sqrt(decider))/2;
			float root_2 = ((-B)-(float)sqrt(decider))/2;
			
			//select smallest root.
			if (root_1 < root_2){
				float w = root_1;
				// Color to be modified once the reflection functions are specified.
				Color c = Color(0.0f, 0.0f, 0.0f);
				Point* p = new Point(oX + dx*w, oY + dy*w, oZ + dz*w, &c);
				return p;
			}
			
			float w = root_2;
			// Remember to change color
			Color c = Color(0.0f, 0.0f, 0.0f);
			Point* p = new Point(oX + dx*w, oY + dy*w, oZ + dz*w, &c);
			return p;
			
		}
		
		// if neither, then decider = 0 so there's intersection and one root
		float w = -B/2;
		
		Color c = Color(0.0f, 0.0f, 0.0f);
		Point* p = new Point(oX + dx*w, oY + dy*w, oZ + dz*w, &c);
		
		return p;
    }
	
void Sphere::transform(fMatrix* matrix)
{
    this->center->transform(matrix);
}