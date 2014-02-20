/* 
 * Basic Ray Tracer Component for Hw1
 * Point class
 * @author Charlene DiMeglio
 * @author Jorge Leon
 * NOT FINISHED
 */

#ifndef POINT_H
#define POINT_H
 
#include <math.h>
#include "Color.h"
#include "cmatrix.h"

typedef techsoft::matrix<double> Matrix;

typedef std::complex<double> dComplex;
typedef techsoft::matrix<dComplex> CMatrix;
 
class Point
{
private:
    
    //The color value of the Point
    Color* color;
     
    //X positon
    float x;
    
    //Y position
    float y;
    
    //Z position
    float z;
    
public: 
        
    Point(float x, float y, float z, Color* c);
    
    // Returns distance to given point
    float distance(Point point);
    
    //SHELL
    //Given a matrix, tranform the given point 
    // THOUGHT: I think this should just be a vector.
    //          We don't rotate or scale points.
    Point* transform(Matrix matrix);
    
    float getX();
    
    float getY();
    
    float getZ();
    
    void setX(float num);
    
    void setY(float num);
    
    void setZ(float num);
    
    Color* getColor();
    
    void setColor(Color* c);
    
 };
 
 #endif