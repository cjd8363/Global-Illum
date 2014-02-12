/* 
 * Basic Ray Tracer Component for Hw1
 * Class to represent the world
 * @author Charlene DiMeglio
 * @author Jorge Leon
 */
 
#include <vector>
using namespace std;
 
 public class World
 {
     // The objects in the world
     private vector<Object*> objs;
     
     // The background color of the world;
     private Color* bgColor
  
     public World(Color* bg, int numObjs)
     {
         this.objs = vector(numObjs); 
         this.bgColor = bg;
     }
     
     // Adds pointer to obj to objList
     public void addObj(Object* obj)
     {
         this.objs.push_back(obj);
     }
     
     //May return back transformed object?
     //Not sure what this function is about
     //public void transform();
     
     public void transformAllObjs(Matrix m)
     {
         for (int i = 0; i < this.objs.size(); i++)
         {
             this.objs[i]->transform(m);
         }
     }
     
     // Given the pixels, create rays from the camera to pixel
     public Ray[] spawn(Pixel[][] pixels)
     {
         //TO DO
     }
 };