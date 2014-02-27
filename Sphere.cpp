/* 
 * Basic Ray Tracer Component for Hw1
 * Class to represent a sphere
 * @author Charlene DiMeglio
 * @author Jorge Leon
 */
#include <string>
#include <vector>
#include <iostream>
#include <stdlib.h> 
#include <stdio.h> 
#include "Sphere.h"

using namespace std;

Sphere::Sphere(float rad, Point* center, char* material) :
    Sphere::Object(material)
{
    this->radius = rad;
    this->center = center;
}

/*
	intersect(Ray* ray) 
	Returns a point of intersection depending on the direction of the ray and the location of the sphere:
		-	Null if there's no intersection. P in (0, 0, 0)
		-	Intersection and ray goes through sphere returns smallest root.
		-	Intersection and ray does not go through then returns 1 root.
	Ray* ray			Ray pointer that will have direction and origin information
	
*/
Point* Sphere::intersect(Ray* ray)
    {
        // Create rayDirection vector from ray's direction, normalize. 
		// Since direction is normalized value A becomes 1
		
        Vect rayDirection = ray->getDirection()->normalize();
        //Vect rayDirection = *ray->getDirection(); //->normalize();
        
        
		// Direction variables
		float dx = rayDirection.getX();
		float dy = rayDirection.getY();
		float dz = rayDirection.getZ(); 
	   // cout << "DIRECTION: " << dx << " " <<  dy << " " << dz << endl;
        
		// Center variables
		float cX = this->center->getX();
		float cY = this->center->getY();
		float cZ = this->center->getZ();
		//cout << "CENTER: " << cX << " " << cY  << " " << cZ << endl;
        
		// Origin variables 
		float oX = ray->getOrigin()->getX();
		float oY = ray->getOrigin()->getY();
		float oZ = ray->getOrigin()->getZ();
		//cout << "ORIGIN: " << oX << " " <<  oY << " " << oZ << endl;
        
		// B = 2*(dx*(OriginX - CenterX) + dy*(OriginY - CenterY)+ dz*(OriginZ - CenterZ))
		float B = 2*(dx*(oX-cX)+dy*(oY-cY)+dz*(oZ-cZ));
		// C = (OriginX - CenterX)^2+(OriginY - CenterY)^2+(OriginZ - CenterZ)^2+(radius)^2
		float C = (((oX-cX)*(oX-cX))+((oY-cY)*(oY-cY))+((oZ-cZ)*(oZ-cZ)))-((this->radius)*(this->radius));
		 
		// decider variable provides info about intersection of ray and sphere
		float decider = (B*B) - (4*C);
		// If decider is less than 0, no real root, no intersection point
		if (decider - 0.0f < -.0000001f){
           //printf("Neg");
           //cout << (decider - 0.0f) << endl;
           //printf("%d \n", decider);
		   return NULL;
		}
		
		// If decider is more than 0, ray goes through sphere
		else if (decider - 0.0f > .0000001f){
            //cout << decider << endl;
            //printf("POSITIVE");
            //printf("%d \n", decider);
			float root_1 = ((-B)+(float)sqrt(decider))/2;
			float root_2 = ((-B)-(float)sqrt(decider))/2;
			
			//select smallest root.
			if (root_1 < root_2){
				float w = root_1;
				// Color to be modified once the reflection functions are specified.
				Point* p = new Point(oX + dx*w, oY + dy*w, oZ + dz*w, this->center->getColor());
                cout << p->getColor()->getBlue() << endl;
				return p;
			}
			//cout << root_1 << endl;
            //cout << root_2 << endl;
			float w = root_2;
			// Remember to change color
			Point* p = new Point(oX + dx*w, oY + dy*w, oZ + dz*w, this->center->getColor());
            //cout << p->getColor()->getBlue() << endl;
			return p;
			
		}
		
		// if neither, then decider = 0 so there's intersection and one root
		float w = -B/2;
		//printf("ZERO");
        //printf("%d \n", decider);
		Point* p = new Point(oX + dx*w, oY + dy*w, oZ + dz*w, this->center->getColor());
		
		return p;
    }
	
void Sphere::transform(fMatrix* matrix)
{
    float cX = this->center->getX();
    float cY = this->center->getY();
    float cZ = this->center->getZ();
    cout << "CENTER: " << cX << " " << cY  << " " << cZ << endl;   
    //this->center->transform(matrix);
    float cX2 = this->center->getX();
    float cY2 = this->center->getY();
    float cZ2 = this->center->getZ();
    cout << "CENTER AFTER: " << cX2 << " " << cY2  << " " << cZ2 << endl;
}