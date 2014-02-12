/* 
 * Basic Ray Tracer Component for Hw1
 * The Camera object for the scene
 * @author Charlene DiMeglio
 * @author Jorge Leon
 */
 
 
public class Camera
{
    // The point which the camera is looking at
    private Point* lookAt;
    
    // The position of the camera in world space
    private Point* position;
    
    // The up direction the camera. Which way is up?
    private Point* up;
    
    // The focal length of the camera
    private float focalL;
    
    //Inits the focal length to 1 if not provided
    public Camera(Point* pos, Point* lookAt, Point* up)
    {
        Camera(pos, lookAt, up, 1f);
    }
    
    public Camera((Point* pos, Point* lookAt, Point* up, Float focalLen)
    {
        this.lookAt = lookAt;
        this.position = pos;
        this.up = up;
        this.focalL = focalLen;
    }
    
    //Takes the world, which has been transformed into camera space,
    //Spawns Rays, and shoots them into the world through the given pixels
    public void render(World world, Pixel[] pixels)
    {
        vector<Ray> rays = world.spawn(pixels); //(makes a ray, one for each camera to pixel possibility)
        for (int i = 0; i < rays.size(); i++)
        {
            for (int j = 0; j < world.objs.length; j++)
            {
                if (world.objs[j].intersect(rays[i]) != null)
                {
                    //grab rgb of intersection point and apply color to pixel
                    // For now, just simple flat application of sphere's color, independent of point
                }
            }
        }
    };
};