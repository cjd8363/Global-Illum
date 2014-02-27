/* 
 * Basic Ray Tracer Component for Hw1
 * Class to represent the world
 * @author Charlene DiMeglio
 * @author Jorge Leon
 */
 
#include <string>
#include <vector>
#include <iostream>
#include <stdlib.h> 
#include <stdio.h> 
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
    //cout << ray->getDirection()->getX() << " " <<  ray->getDirection()->getY() << " " << ray->getDirection()->getZ() << endl;
    //cout << "TRACE" << endl;  
    Point* lowest = NULL;
    float distanceLOW = 100000000;
    for(int k = 0; k < this->objs.size(); k++)
    {
        // Get intersection point;
        Point* p = this->objs.at(k)->intersect(ray);
        
        if (p!=NULL)
        {
           //cout << p->getColor()->getBlue() << endl; 
           // cout << "Intersect" << endl;
          //cout << ray->getDirection()->getX() << ray->getDirection()->getY() << endl;
          //cout << p->getColor()->getBlue() << endl;
            // update closest intersection point
            if (lowest!=NULL)
            {
                float distance = p->distance(Point(0,0,0, p->getColor()));
               // cout << k << endl;
               // cout << ray->getDirection()->getX() << ray->getDirection()->getY() << endl;
                
                if (distance - distanceLOW < -.001)
                {
                    //cout << distance << endl;
                    p = lowest;
                    distanceLOW = distance;
                }
            }
            else
            {
                lowest = p;
                distanceLOW = p->distance(Point(0,0,0, p->getColor()));
                //cout << k << endl;
                //cout << ray->getDirection()->getX() << ray->getDirection()->getY() << endl;
                //cout << p->distance(Point(0,0,0, p->getColor())) << endl;
            }
        }
    }
    return lowest;
 }