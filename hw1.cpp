/* 
 * Basic Ray Tracer Component for Hw1
 * @author Charlene DiMeglio
 * @author Jorge Leon
 */

#include string
#include iostream
 
public class RayTracer
{
    
    private float height;
    private float width;
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
      // init pixels with color and position
    }
};
 
 
 

