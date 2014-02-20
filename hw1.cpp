/* 
 * Basic Ray Tracer Component for Hw1
 * @author Charlene DiMeglio
 * @author Jorge Leon
 */

#include <string>
#include <vector>
#include <iostream>
#include "Camera.h"
 
class RayTracer
{
    
private:
    
    // Height of the view plane
    float height;
    
    // Width of the view plane
    float width;
    
    // Array of pixels in view plane
    vector< vector <Pixel> > pixels;
    
public:
    
    // Takes in the height and width of the scene
    RayTracer(float height, float width)
    {
        this->height = height;
        this->width = width;
    }
    
    
    void main(int argc, char** argv)
    {
      // Height and Width of image
      // Camera Specs (position lookat and up)
      // World Attributes (background color)
      // Objects to be added with init values
   //RayTracer rayTray = new rayTray(height, width);
  // Camera cam = new Camera(&pos, &lookat, &up);
  //  World world = new World(&bgColor, numObjs);
      //Below, depending on how u do this, the sphere may already be made at this point
      //during file parsing, or the variables may be named differently.
      // also, for now, material is a string (may be a float later)
   //world.add(new Sphere(center, radius, material));
      // Need matrix still
  // world.transformAll(Matrix m);
 //  cam.render(world);
    }
};
 
 
 

