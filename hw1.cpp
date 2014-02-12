/* 
 * Basic Ray Tracer Component for Hw1
 * @author Charlene DiMeglio
 * @author Jorge Leon
 */

#include string
#include iostream
 
public class RayTracer
{
    // Height of the view plane
    private float height;
    
    // Width of the view plane
    private float width;
    
    // Array of pixels in view plane
    private Pixel[][] pixels;
    
    // Takes in the height and width of the scene
    public RayTracer(float height, float width)
    {
        this.height = height;
        this.width = width;
    }
    
    void main(int argc, char** argv)
    {
      // Height and Width of image
      // Camera Specs (position lookat and up)
      // World Attributes (background color)
      // Objects to be added with init values
      RayTracer rayTray = new rayTray(height, width);
      Camera cam = new Camera(&pos, &lookat, &up);
      World world = new World(bgColor);
      //Below, depending on how u do this, the sphere may already be made at this point
      //during file parsing, or the variables may be named differently.
      // also, for now, material is a string (may be a float later)
      world.add(new Sphere(center, radius, material));
      // init pixels with color and position
    }
};
 
 
 

