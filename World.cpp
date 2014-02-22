/* 
 * Basic Ray Tracer Component for Hw1
 * Class to represent the world
 * @author Charlene DiMeglio
 * @author Jorge Leon
 */
 
#include "World.h"

 // height and width refers to the pixel container, view pane
 // bg is a default color
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
 
 // m matrix to apply to all objects in world
 void World::transformAllObjs(fMatrix* m)
 {
     for (int i = 0; i < this->objs.size(); i++)
     {
         this->objs[i]->transform(m);
     }
 }
 
 // ray Ray to be traced, to be checked for intersections with objects
 // Returns a point, which is the closest point of intersection with an object
 // in the world. Will be null if no intersection occurs
 Point* World::trace(Ray* ray)
 {
    
    Point* lowest = NULL;
    float distanceLOW = 100000000;
    for(int k = 0; k < this->objs.size(); k++)
    {
        // Get intersection point
        Point* p = this->objs.at(k)->intersect(ray);
        if (p!=NULL)
        {
            // update closest intersection point
            if (lowest!=NULL)
            {
                float distance = p->distance(Point(0,0,0, p->getColor()));
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