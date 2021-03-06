/* 
 * Basic Ray Tracer Component for Hw1
 * @author Charlene DiMeglio
 * @author Jorge Leon 
 */

#include <string>
#include <vector>
#include <iostream>
#include <stdlib.h> 
#include <stdio.h>  
#include "Camera.h"
#include "Reader.h"
#include "Sphere.h"
#include "Polygon.h"
#include "Outputer.h"
 
 
int main(int argc, char** argv)
{
    if (argc < 2)
    {
        printf ("Error opening file");
        exit (EXIT_FAILURE);
    }
    printf("ENTER");
    vector<string> params = getInfo(argv[1]);
    printf("INFO");
    vector< vector<float> > data;
    for (int i = 0; i < params.size(); i++)
    {
       data.push_back(dimensions(params.at(i)));
    }
    
    //BackGround Color
    cout << "BG COLOR " << data.at(4).at(0) << " " << data.at(4).at(1) << " " << data.at(4).at(2) << endl;
    Color bg = Color(data.at(4).at(0), data.at(4).at(1), data.at(4).at(2));
    
    //Create Camera
    Point camPos = Point((data.at(1).at(0)), data.at(1).at(1), data.at(1).at(2), &bg);
    Point camLook = Point(data.at(2).at(0), data.at(2).at(1), data.at(2).at(2), &bg);
    Point camUp = Point(data.at(3).at(0), data.at(3).at(1), data.at(3).at(2), &bg);
    Camera cam = Camera(&camPos, &camLook, &camUp, 25.0f);
    
    //Create World
    World world = World((data.at(0).at(0)), data.at(0).at(1), &bg);
    cout << "HIT" << endl;
    //Create Objects
    //TO DO EXTRA: Make so that this sequence is not hardcoded
    
    for (int i = 5; i < data.size(); i++)
    {
      
        if (data.at(i).at(0) == 31.0)
        {
            Color* color = new Color(data.at(i).at(5),data.at(i).at(6),data.at(i).at(7));
            cout << "SPHERE" << endl;
            cout << "COLOR " << data.at(i).at(5) << " " << data.at(i).at(6) << " " << data.at(i).at(7) << endl;
            cout << "CENTER " << data.at(i).at(2) << " " <<  data.at(i).at(3) << " " << data.at(i).at(4) << endl;
            Point* center = new Point(data.at(i).at(2),data.at(i).at(3),data.at(i).at(4), color);
            char temp = 'a';
            Object* s = new Sphere(data.at(i).at(1), center, &temp);
            world.addObj(s);
            cout << (dynamic_cast<Sphere*> (world.objs.at(0)))->getCenter()->getZ() << endl;
            cout << (dynamic_cast<Sphere*> (world.objs.at(0)))->getCenter()->getColor()->getBlue() << endl;
        }
        if (data.at(i).at(0) == 21.0)
        {
            Color* color = new Color(data.at(i).at(10),data.at(i).at(11),data.at(i).at(12));
            vector<Point*>* pts= new vector<Point*>();
            Point* v1 = new Point(data.at(i).at(1),data.at(i).at(2),data.at(i).at(3), color);
            Point* v2 = new Point(data.at(i).at(4),data.at(i).at(5),data.at(i).at(6), color);
            Point* v3 = new Point(data.at(i).at(7),data.at(i).at(8),data.at(i).at(9), color);
            pts->push_back(v1);
            pts->push_back(v2);
            pts->push_back(v3);
            //Point* pts[3] = {v1,v2,v3};
            char temp = 'a'; 
            Object* p = new Polygon(pts, &temp);
            world.addObj(p);
            cout << "POLY" << endl;
        }
    }
    printf("ADD ");
    world.transformAllObjs(cam.getViewMatrix());
    printf("TRANS ");
    cam.render(&world);
    printf("RENDER ");
    cout << world.pixels.size() << endl;
    cout << world.pixels.at(0).size() << endl;
    cout << world.height << endl;
    cout << world.width << endl;
    
    
    outputer::saveppm(argv[2], world.width, world.height, 72, &world.pixels);
    printf("DONE ");
    
        
    //Output to file
    
    
  //Below, depending on how u do this, the sphere may already be made at this point
  //during file parsing, or the variables may be named differently.
  // also, for now, material is a string (may be a float later)
//world.add(new Sphere(center, radius, material));
  // Need matrix still
// world.transformAll(Matrix m);
//  cam.render(world);
    return 0;
}


