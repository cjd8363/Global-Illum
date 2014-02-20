/* 
 * Basic Ray Tracer Component for Hw1
 * The Camera object for the scene
 * @author Charlene DiMeglio
 * @author Jorge Leon
 */

#include "Camera.h"

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
}

//Takes the world, which has been transformed into camera space,
//Spawns Rays, and shoots them into the world through the given pixels
void Camera::render(World* world, vector< vector<Pixel> >* pixels)
{
//  vector<Ray>* rays = world->spawn(pixels); //(makes a ray, one for each camera to pixel possibility)
   // for (int i = 0; i < rays->size(); i++)
    //{
      //  for (int j = 0; j < world->objs->size(); j++)
       // {
        //    Point* p = world->objs[j]->intersect(rays[i]);
         //   if (p != NULL)
            //{
            //pixels[pixels.size()/i][pixels.size()%i]->setColor(p->getColor());
                
                // OLD v
                //grab rgb of intersection point and apply color to pixel
                // For now, just simple flat application of sphere's color, independent of point
                //sorta
                //pixels[pixels.length/i][pixels.length%i]->setColor(obj[j]->getColor());
          //  }
       // }
   // }
}