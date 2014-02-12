/* 
 * Basic Ray Tracer Component for Hw1
 * Class to represent general polygons
 * @author Charlene DiMeglio
 * @author Jorge Leon
 */
 
public class Polygon : public Object
{
    private Point[] vertices;
    
    private Vector[] normals;
  
    // Maybe not correct
    public Polygon(Point[] pts, Vector[] norms, char* material):
        Object(material)
    {
        this.vertices = pts;
        this.norms = norms;
    }
    
    // No material provided constructor needed
    // Getters, Setters
    
    
    public Point* intersect(Ray ray)
    {
        
    }
};