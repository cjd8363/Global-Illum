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
 
class World
{
private:
     
     // The background color of the world;
     Color* bgColor;
  
public:
    
     // The objects in the world
     vector<Object*> objs;
     
     World(Color* bg, int numObjs);
     
     // Adds pointer to obj to objList
     void addObj(Object* obj);
     
     //May return back transformed object?
     //Not sure what this function is about
     //public void transform();
     
     void transformAllObjs(Matrix m);
     
     // Given the pixels, create rays from the camera to pixel
     vector<Ray> World::spawn(vector< vector<Pixel> > pixels);
     
     
 };
 
 #endif