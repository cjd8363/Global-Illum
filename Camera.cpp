/* 
 * Basic Ray Tracer Component for Hw1
 * The Camera object for the scene
 * @author Charlene DiMeglio
 * @author Jorge Leon
 */

#include "Camera.h"
  #include <string>
#include <vector>
#include <iostream>
#include <stdlib.h> 
#include <stdio.h> 

//Inits the focal length to 1 if not provided
Camera::Camera(Point* pos, Point* lookAt, Point* up)
{
    Camera(pos, lookAt, up, 1.0);
}

Camera::Camera(Point* pos, Point* lookAt, Point* up, float focalLen)
{
    this->lookAt = lookAt;
    this->position = pos;
    this->up = up;
    this->focalL = focalLen;
    this->calcViewMatrix();
}

Point* Camera::getLookAt()
{
    return this->lookAt;
}
    
Point* Camera::getPosition()
{
    return this->position;
}
   
Point* Camera::getUp()
{
    return this->up;
}

float Camera::getFocalL()
{
    return this->focalL;
}

// Calculate the view matrix with camera's current qualities
void Camera::calcViewMatrix()
{
    Vect n = Vect(this->lookAt->getX() - this->position->getX(), this->lookAt->getY() - this->position->getY(), this->lookAt->getZ() - this->position->getZ());
    Vect up = Vect(this->up->getX(), this->up->getY(), this->up->getZ());
    Vect v = n.crossProd(&up);
    Vect u = v.crossProd(&n);
    Vect nn = n.normalize();
    Vect vn = v.normalize();
    Vect un = u.normalize();
    Vect pos = Vect(this->position->getX(), this->position->getY(), this->position->getZ());
    float vArray[] = {u.getX(), u.getY(), u.getZ(), -(u.dotProduct(&pos)), v.getX(), v.getY(), v.getZ(), -(v.dotProduct(&pos)), n.getX(), n.getY(), n.getZ(), -(n.dotProduct(&pos)), 0.0, 0.0, 0.0, 1.0};
    size_t row = 4;
    size_t col = 4;
    this->viewMatrix = fMatrix(row, col, vArray);
}
    
fMatrix* Camera::getViewMatrix()
{
    return &(this->viewMatrix);
}


//Takes the world, which has been transformed into camera space,
//Spawns Rays, and shoots them into the world, recording the 
//colors returned world.spaw into the pixel array
void Camera::render(World* world)
{
    // Info for where we are in the pixel array
    float pX = 1;
    float pY = 1;
    float x = -(world->width/2)+(pX/2);
    float y = -(world->height/2)+(pX/2);
    float z = this->getFocalL();
    float startX = x;
    
    for (int i = 0; i < world->height; i++)
    {
        x = startX;
        // For every height we are adding a row of pixels to our pixel container
        vector<Pixel> rowPix;
        for (int j = 0; j < world->width; j++)
        {
            //Create ray
            Point ori = Point(0,0,0,world->bgColor);
            Vect vec = Vect(x,y,z);
            Vect nVec = vec.normalize();
            Ray ray = Ray(&ori,&nVec);
            
            //Send it out into the world
            Point* p = world->trace(&ray);
            
            // if we hit something, the current pixel gets the color of the hit point
            if (p != NULL)
            {
                Point pixPos = Point(x,y,z, p->getColor());
                rowPix.push_back(Pixel(&pixPos,p->getColor()));
            }
            // else we default to the background color
            else
            {
                Point pixPos = Point(x,y,z,world->bgColor);
                rowPix.push_back(Pixel(&pixPos,world->bgColor));
            }
            x+=pX;
        }
        world->pixels.push_back(rowPix);
        y+=pY;
    }
} 