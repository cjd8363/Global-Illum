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
 
 Point* World::trace(Ray* ray)
 {
    
    Point* lowest = NULL;
    float distanceLOW = 100000000;
    for(int k = 0; k < this->objs.size(); k++)
    {
        Point* p = this->objs.at(k)->intersect(ray);
        if (p!=NULL)
        {
            if (lowest!=NULL)
            {
                float distance = (float)sqrt((float)pow(p->getX(),2)+(float)pow(p->getY(),2)+(float)pow(p->getZ(),2));
                if (distance < distanceLOW)
                {
                    p = lowest;
                    distanceLOW = distance;
                }
            }
            else
            {
                p = lowest;
            }
        }
    }
    return lowest;
 }