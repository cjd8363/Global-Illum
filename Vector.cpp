/* 
 * Basic Ray Tracer Component for Hw1
 * Class to represent vectors
 * @author Charlene DiMeglio
 * @author Jorge Leon
 */

#include <math.h>
 
class Vect
{    
    public:
    
        Vect(float x, float y, float z)
        {
            this->x = x;
            this->y = y;
            this->z = z;
        }

        // Default contructor, all 0
        Vect()
        {
            this->x = 0.0;
            this->y = 0.0;
            this->z = 0.0;
        }
       
    
        Vect add(Vect* v)
        {
            return Vect(this->x + v->getX(), this->y + v->getY(), this->z + v->getZ());
        }
    
        Vect sub(Vect* v)
        {
            return Vect(this->x - v->getX(), this->y - v->getY(), this->z - v->getZ());
        }

    	Vect crossProd (Vect* v)
        {
            return Vect((this->y * v->getZ()) - (this->z * v->getY()), 
					    (this->z * v->getX()) - (this->x * v->getZ()), 
					    (this->x * v->getY()) - (this->y * v->getX()));
	    }
    
        float length()
        {
            return (float)sqrt((x*x) + (y*y) + (z*z));
        }
     
        Vect normalize()
        {
            float mag = this->length();
		    return Vect(this->x/mag, this->y/mag, this->z/mag);
	    }
	
	    Vect negative()
        {
            return Vect(-(this->x), -(this->y), -(this->z));
	    }
	
	    float dotProduct(Vect* v){
            return ((this->x * v->getX()) + (this->y * v->getY()) + (this->z * v->getZ()));
	    }
	
	    Vect vectMult(float sca){
            return Vect (sca * this->x, sca * this->y, sca * this->z);
	    }
        
        void setX(float x)
        {
            this->x = x;
        }
        
        void setY(float y)
        {
            this->y = y;
        }
        
        void setZ(float Z)
        {
            this->z = z;
        }
        
        float getX()
        {
            return this->x;
        }
        
        float getY()
        {
            return this->y;
        }
        
        float getZ()
        {
            return this->z;
        }
        
    // The X, Y and Z components of the vector
    private:
        float x, y, z;
};
