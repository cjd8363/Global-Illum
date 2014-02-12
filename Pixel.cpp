/* 
 * Basic Ray Tracer Component for Hw1
 * Class representing one pixel, in
 * its color and position relative to camera
 * @author Charlene DiMeglio
 * @author Jorge Leon
 */
 
 
 public class Pixel
 {
     //The position of the pixel
     private Point* position;
     
     //The color of the pixel
     private Color* color;
     
     public Pixel(Point* pos, Color* col)
     {
         this.position = pos;
         this.color = col;
     }
     
     public Point* getPosition()
     {
         return this.position;
     }
     
     public Color* getColor()
     {
         return this.Color;
     }
     
     public void setPosition(Position* pos)
     {
         this.position = pos;
     }
     
     public void setColor(Color* col)
     {
         this.color = col;
     }
 };