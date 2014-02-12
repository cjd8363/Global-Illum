/* 
 * Basic Ray Tracer Component for Hw1
 * Class to represent a sphere
 * @author Charlene DiMeglio
 * @author Jorge Leon
 */
 
class Sphere : public Object 
{
    //radius of the sphere
    private float raidus;
    
    //center of sphere
    private Point* center;
    
    public Sphere(float rad, Point* center)
    {
        //May be incorrect
        Sphere(rad, center, "");
    }
    
    public Sphere(float rad, Point* center, char* material) :
        Object(material)
    {
        this.radius = rad;
        this.center = center;
    }
    
    //Returns null if no intersection
    public Point intersect(Ray ray)
    {
        
    }
    
};
 
