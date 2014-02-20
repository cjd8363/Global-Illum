/* 
 * Basic Ray Tracer Component for Hw1
 * Class to represent the world
 * @author Charlene DiMeglio
 * @author Jorge Leon
 */
 
#include "World.h"
 
 World::World(float height, float width, Color* bg) 
 {
     this->height = height;
     this->width = width;
     this->bgColor = bg;
 }
 
 // Adds pointer to obj to objList
 void World::addObj(Object* obj)
 {
     this->objs.push_back(obj);
 }
 
 //May return back transformed object?
 //Not sure what this function is about
 //public void transform();
 
 void World::transformAllObjs(Matrix m)
 {
     for (int i = 0; i < this->objs.size(); i++)
     {
         this->objs[i]->transform(m);
     }
 }
 
 // Given the pixels, create rays from the camera to pixel
 vector<Ray> World::spawn(vector< vector<Pixel> > pixels)
 {
     //TO DO
 }