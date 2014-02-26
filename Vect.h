/* 
 * Basic Ray Tracer Component for Hw1 
 * Class to represent vectors
 * @author Charlene DiMeglio
 * @author Jorge Leon
 */

#include <math.h>
 
class Vect
{    
    
private:
    // The X, Y and Z components of the vector
    float x, y, z;

public:
      
    Vect(float x, float y, float z);
    
    Vect();
    
    Vect add(Vect* v);
    
    Vect sub(Vect* v);

    Vect crossProd (Vect* v);
    
    float length();
 
    Vect normalize();
	
    Vect negative();
	
    float dotProduct(Vect* v);
	
    Vect vectMult(float sca);
	
	float getAngle(Vect* v, Vect* w);
    
    void setX(float x);
    
    void setY(float y);
    
    void setZ(float Z);
    
    float getX();
    
    float getY();
        
    float getZ();
};
