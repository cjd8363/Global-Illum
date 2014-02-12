/* 
 * Basic Ray Tracer Component for Hw1
 * Point class
 * @author Charlene DiMeglio
 * @author Jorge Leon
 */
 
#include <math.h>
 
 public class Point
 {
    //X positon
    private float x;
    
    //Y position
    private float y;
    
    //Z position
    private float z;
    
    public Point(float x, float y, float z)
    {
        this.x = x;
        this.y = y;
        this.z = z;
    }
        
    // Returns distance to given point
    public float distance(Point point)
    {
      return (float)sqrt((this.x - point.getX())^2+(this.y - point.getY())^2+(this.z - point.getZ())^2); 
    }
    
    //SHELL
    public Point transform(Matrix matrix)
    {
        return new Point(0f,0f,0f);
    }
    
    public float getX()
    {
        return this.x;
    }
    
    public float getY()
    {
        return this.y;
    }
    
    public float getZ()
    {
        return this.z;
    }
    
    public void setX(float num)
    {
        this.x = num;
    }
    
    public void setY(float num)
    {
        this.y = num;
    }
    
    public void setZ(float num)
    {
        this.z = num;
    }
    
 };