/* 
 * Basic Ray Tracer Component for Hw1
 * Class to represent vectors
 * @author Charlene DiMeglio
 * @author Jorge Leon
 */
 
 // vector is taken by the library vector.h, we can't use that name for this
public class vect_or
{
    // The X, Y and Z components of the vector
    private:
        float x, y, z;
    
    
    public:
    
    vect_or(float x, float y, float z)
    {
        this.x = x;
        this.y = y;
        this.z = z;
    }
    
    // Syntax corrections
    
    vect_or add(vect_or v)
    {
        return vect_or(this.x + v.getX(), this.y + v.getY(), this.z + v.getZ());
    }
    
    vect_or sub(vect_or v)
    {
        return vect_or(this.x - v.getX(), this.y - v.getY(), this.z - v.getZ());
    }
    // need cross, dot, length, transform
    // Took care of the other methods
    	vect_or crossProd (vect_or v){
		      return Vect ( y*v.getZ() - z*v.getY(), 
					                 z*v.getX() - x*v.getZ(), 
					                 x*v.getY() - y*v.getX());
	    }
    
     double length(){
        return sqrt(x*x + y*y + z*z);
     }
     
     vect_or normalize(){
		      double magnitude = sqrt((x*x)+(y*y)+(z*z));
		      return vect_or (x/magnitude, y/magnitude, z/magnitude);
	    }
	
	    vect_or negative(){
		        return vect_or (-x, -y, -z);
	     }
	
	    double dotProduct (Vect v){
		        return (x*v.getVectX()+ y*v.getVectY() + z*v.getVectZ());
	    }
	
	    vect_or vectMult (double sca){
	        	return Vect (sca*x, sca*y, sca*z);
	    }
    
    
};
