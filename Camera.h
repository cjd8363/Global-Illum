/* 
 * Basic Ray Tracer Component for Hw1
 * The Camera object for the scene
 * @author Charlene DiMeglio
 * @author Jorge Leon 
 */

#include "World.h"
#include <vector>
 
class Camera
{
private: 
    // The point which the camera is looking at
    Point* lookAt;
    
    // The position of the camera in world space
    Point* position;
    
    // The up direction the camera. Which way is up?
    Point* up;
    
    // The focal length of the camera
    float focalL;
    
    fMatrix viewMatrix;
    
public:
    //Inits the focal length to 1 if not provided
    Camera(Point* pos, Point* lookAt, Point* up);
    
    Camera(Point* pos, Point* lookAt, Point* up, float focalLen);
    
    Point* getUp();
    
    Point* getLookAt();
    
    Point* getPosition();
    
    // Calculate the view matrix with the current camera attributes
    void calcViewMatrix();
    
    fMatrix* getViewMatrix();
    
    float getFocalL();
    
    //Takes the world, which has been transformed into camera space,
    //Spawns Rays, and shoots them into the world
    void render(World* world);
};