/* 
 * Basic Ray Tracer Component for Hw1
 * Class to represent a general CG object
 * @author Charlene DiMeglio
 * @author Jorge Leon
 */
 
public class Object
{
    // The material of the object, for now a string
    protected char* material;
    
    // Virtual function for the intersection of the object with given ray
    public virtual Point intersect(Ray ray) = 0;
};