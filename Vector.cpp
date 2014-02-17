/* 
 * Basic Ray Tracer Component for Hw1
 * Class to represent vectors
 * @author Charlene DiMeglio
 * @author Jorge Leon
 */
 
public class Vector
{
    // The X component of the vector
    private float x;
    
    // The Y component of the vector
    private float y;
    
    // The Z component of the vector
    private float z;
    
    public Vector(float x, float y, float z)
    {
        this.x = x;
        this.y = y;
        this.z = z;
    }
    
    
    public Vector add(Vector v)
    {
        return Vector(this.x + v.getX(), this.y + v.getY(), this.z + v.getZ());
    }
    
    public Vector sub(Vector v)
    {
        return Vector(this.x - v.getX(), this.y - v.getY(), this.z - v.getZ());
    }
    // need cross, dot, length, transform
};