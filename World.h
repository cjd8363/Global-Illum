/* 
 * Basic Ray Tracer Component for Hw1
 * Class to represent the world
 * @author Charlene DiMeglio
 * @author Jorge Leon
 */
 
#ifndef WORLD_H
#define WORLD_H 
 
#include <vector>
#include "Object.h"
#include "Pixel.h"
#include "Ray.h"
 
using namespace std;
typedef techsoft::matrix<float> fMatrix;
 
class World
{
private:
      
  
public:
    
     // The background color of the world;
     Color* bgColor;
    
    // Height of the view plane
    float height;
    
    // Width of the view plane
    float width;
    
    // Array of pixels in view plane
    vector< vector <Pixel> > pixels;
    
    
     // The objects in the world
     vector<Object*> objs;
     
     World(float height, float width, Color* bg);
     
     // Adds pointer to obj to objList
     void addObj(Object* obj);
     
     void transformAllObjs(fMatrix* m);
     
     Point* trace(Ray* ray);
     
 };
 
 #endif