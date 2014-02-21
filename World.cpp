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
 
 void World::transformAllObjs(fMatrix* m)
 {
     for (int i = 0; i < this->objs.size(); i++)
     {
         this->objs[i]->transform(m);
     }
 }