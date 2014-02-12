/* 
 * Basic Ray Tracer Component for Hw1
 * Class to represent rays for ray tracer
 * @author Charlene DiMeglio
 * @author Jorge Leon
 */
 
public class Ray
{
    // The point of origin
    private Point* origin;
    
    // The direction of the array
    private Vector* direction;
    
    public Ray(Point* pnt, Vector* dir)
    {
        this.origin = pnt;
        this.direction = dir;
    }
    
    public Point* getOrigin()
    {
        return this.origin;
    }
    
    public Vector* getDirection()
    {
        return this.direction;
    }
    
    public void setOrigin(Point* pnt)
    {
        this.origin = pnt;
    }
    
    public void setDirection(Vector* vect)
    {
        this.direction = vect;
    }
};