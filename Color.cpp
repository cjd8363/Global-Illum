/* 
 * Basic Ray Tracer Component for Hw1
 * Color Class for pixels
 * @author Charlene DiMeglio
 * @author Jorge Leon
 */
 
public class Color
{
    private float red;
    private float green;
    private float blue;
    
    public Color(float r, float g, float b)
    {
        this.red = r;
        this.green = g;
        this.blue = b;
    }
    
    public float getRed()
    {
        return this.red;
    }
    
    public float getBlue()
    {
        return this.blue;
    }
    
    public float getGreen()
    {
        return this.green;
    }
    
    public void setRed(float red);
    {
        this.red = red;
    }
    
    public void setGreen(float green);
    {
        this.green = green;
    }
    
    public void setBlue(float blue);
    {
        this.blue = blue;
    }
};