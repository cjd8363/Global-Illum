/* 
 * Basic Ray Tracer Component for Hw1
 * The Camera object for the scene
 * @author Charlene DiMeglio
 * @author Jorge Leon
 */
 
 
public class Camera
{
    private Point* lookAt;
    private Point* position;
    private Point* up;
    
    public Camera(Point* pos, Point* lookAt, Point* up)
    {
        this.lookAt = lookAt;
        this.position = pos;
        this.up = up;
    }
    
    //Takes the world, which has been transformed into camera space,
    //Spawns Rays, and shoots them into the world through the given pixels
    public void render(World world, Pixel[] pixels)
    {
        
    };
};