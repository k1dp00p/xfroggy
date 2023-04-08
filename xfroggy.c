#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include <X11/Xlib.h>
#include <X11/xpm.h>
#include <X11/extensions/shape.h>

//            ///
//           (0<0)
//           (   )
//  _   __    "_"       ___   ___
// | | /_ |   | |      / _ \ / _ \
// | | _| | __| |_ __ | | | | | | |_ __
// | |/ / |/ _` | '_ \| | | | | | | '_ \
// |   <| | (_| | |_) | |_| | |_| | |_) |
// |_|\_\_|\__,_| .__/ \___/ \___/| .__/
//              | |               | |
//              |_|               |_|

// compiling:
//          $ gcc -lX11 -lXpm -lXext xfroggy.c -o xfroggy
// usage:
//          $ ./xfroggy

#define DIRECTIONS 8
#define MOTIONS 3
#define BUBBLES 12
#define WAIT_TIME 200000 // [us]

// source of default images:
// big thanks to 尾羽つばさ-san (@obane153)
// 尾羽の小屋
// http://obane.blog.shinobi.jp/kitei/kitei
char frog_file[DIRECTIONS][MOTIONS][256]
= {{"img/kaeru_otoko/0_l.xpm", "img/kaeru_otoko/0_r.xpm", "img/kaeru_otoko/0_s.xpm"},
   {"img/kaeru_otoko/1_l.xpm", "img/kaeru_otoko/1_r.xpm", "img/kaeru_otoko/1_s.xpm"},
   {"img/kaeru_otoko/2_l.xpm", "img/kaeru_otoko/2_r.xpm", "img/kaeru_otoko/2_s.xpm"},
   {"img/kaeru_otoko/3_l.xpm", "img/kaeru_otoko/3_r.xpm", "img/kaeru_otoko/3_s.xpm"},
   {"img/kaeru_otoko/4_l.xpm", "img/kaeru_otoko/4_r.xpm", "img/kaeru_otoko/4_s.xpm"},
   {"img/kaeru_otoko/5_l.xpm", "img/kaeru_otoko/5_r.xpm", "img/kaeru_otoko/5_s.xpm"},
   {"img/kaeru_otoko/6_l.xpm", "img/kaeru_otoko/6_r.xpm", "img/kaeru_otoko/6_s.xpm"},
   {"img/kaeru_otoko/7_l.xpm", "img/kaeru_otoko/7_r.xpm", "img/kaeru_otoko/7_s.xpm"}};

// directions:
//                north
//     northwest 7  0  1 northeast
// west          6     2          east
//     southwest 5  4  3 southeast
//                south
//
// use of indexes to mean more than numbers is undesirable ...
//
// motions:
//          l - left foot forward
//          r - right foot forward
//          s - standstill
Pixmap frog_img[DIRECTIONS][MOTIONS], frog_msk[DIRECTIONS][MOTIONS];
// use XpmAttributes only to get the size of the images
// and assume that all images are of equal size
// therefore, declare only one
XpmAttributes frog_attr;

char bubb_file[BUBBLES][256]
= {"img/fukidashi/a.xpm",
   "img/fukidashi/b.xpm",
   "img/fukidashi/c.xpm",
   "img/fukidashi/d.xpm",
   "img/fukidashi/e.xpm",
   "img/fukidashi/f.xpm",
   "img/fukidashi/g.xpm",
   "img/fukidashi/h.xpm",
   "img/fukidashi/i.xpm",
   "img/fukidashi/j.xpm",
   "img/fukidashi/k.xpm",
   "img/fukidashi/l.xpm"};

Pixmap bubb_img[BUBBLES], bubb_msk[BUBBLES];
XpmAttributes bubb_attr;

Display *dpy; // display
int dpy_width;
int dpy_height;
int scr; // screen
Window frog_win; // window for frog
Window bubb_win; // window for speech bubble
XSetWindowAttributes attr; // use the same for both frog and bubb

// for frog
int direction;
int motion;
int x, y; // current position of the frog
int step_size_x, step_size_y; // size of a step in x- or y-direction

void draw_frog(int direction, int motion)
{
   XShapeCombineMask(dpy, frog_win, ShapeBounding, 0, 0, frog_msk[direction][motion], ShapeSet);
   XCopyArea(dpy, frog_img[direction][motion], frog_win, DefaultGC(dpy, scr), 0, 0, frog_attr.width, frog_attr.height, 0, 0);

   XFlush(dpy);
}

int get_random(int min, int max)
{
   // get a random int between min and max
   return (min + (int)(rand() * (max - min + 1.0) / (1.0 + RAND_MAX)));
}

int spin_frog(int direction)
{
   // create a new window for a speech bubble
   bubb_win = XCreateWindow(dpy, DefaultRootWindow(dpy), x + 7, y - 12, bubb_attr.width, bubb_attr.height, 0, CopyFromParent, CopyFromParent, CopyFromParent, CWOverrideRedirect, &attr);
   XMapWindow(dpy, bubb_win);

   // and draw in it
   int bubb_num = get_random(0, BUBBLES - 1);
   XShapeCombineMask(dpy, bubb_win, ShapeBounding, 0, 0, bubb_msk[bubb_num], ShapeSet);
   XCopyArea(dpy, bubb_img[bubb_num], bubb_win, DefaultGC(dpy, scr), 0, 0, bubb_attr.width, bubb_attr.height, 0, 0);

   // rotate the frog once in its place
   for (int i = 0; i < DIRECTIONS; i++)
   {
      // use only standing motions
      draw_frog((direction + i) % DIRECTIONS, MOTIONS - 1);
      usleep((int)WAIT_TIME / 2);
   }

   // destroy the speech bubble
   XDestroyWindow(dpy, bubb_win);

   return ((direction + DIRECTIONS - 1) % DIRECTIONS);
}


int get_direction(int x, int y, int x_dst, int y_dst)
{
   // determine the direction in which the frog should head based on its current location and destination
   // only 4-directional judgments are made
   // should be expanded to 8 directions ...
   int direction;

   if (x - x_dst > 0)
   {
      if (y - y_dst > 0)
      {
         // northwest
         direction = 7;
      }
      else
      {
         // southwest
         direction = 5;
      }
   }
   else
   {
      if (y - y_dst > 0)
      {
         // northeast
         direction = 1;
      }
      else
      {
         // southeast
         direction = 3;
      }
   }

   return direction;
}

void walk_a_step(int *x, int *y, int direction)
{
   // let the frog take a step toward the direction
   switch(direction)
   {
      case 1: // northeast
         *x += step_size_x;
         *y -= step_size_y;
         break;
      case 3: // southeast
         *x += step_size_x;
         *y += step_size_y;
         break;
      case 5: // southwest
         *x -= step_size_x;
         *y += step_size_y;
         break;
      case 7: // northwest
         *x -= step_size_x;
         *y -= step_size_y;
         break;
   }

   XMoveWindow(dpy, frog_win, *x, *y);
}

void main(void)
{
   srand(time(NULL)); // random seed based on current time

   dpy = XOpenDisplay(NULL);

   // read the .xpm files
   for (int i = 0; i < DIRECTIONS; i++)
   {
      for (int j = 0; j < MOTIONS; j++)
      {
         int ret = XpmReadFileToPixmap(dpy, DefaultRootWindow(dpy), frog_file[i][j], &frog_img[i][j], &frog_msk[i][j], &frog_attr);
         if (ret != XpmSuccess)
         {
            fprintf(stderr, "[E] oops! couldn't read the .xpm files for the frog ...\n");
            exit(1);
         }
      }
   }

   for (int i = 0; i < BUBBLES; i++)
   {
      int ret = XpmReadFileToPixmap(dpy, DefaultRootWindow(dpy), bubb_file[i], &bubb_img[i], &bubb_msk[i], &bubb_attr);
      if (ret != XpmSuccess)
      {
         fprintf(stderr, "[E] oops! couldn't read the .xpm files for the speech bubbles ...\n");
         exit(1);
      }
   }


   attr.override_redirect = True; // ignore WM

   scr = DefaultScreen(dpy);
   dpy_width = DisplayWidth(dpy, scr);
   dpy_height = DisplayHeight(dpy, scr);
   x = get_random(0, dpy_width - frog_attr.width);
   y = get_random(0, dpy_height - (frog_attr.height + bubb_attr.height));

   frog_win = XCreateWindow(dpy, DefaultRootWindow(dpy), x, y, frog_attr.width, frog_attr.height, 0, CopyFromParent, CopyFromParent, CopyFromParent, CWOverrideRedirect, &attr);

   XMapWindow(dpy, frog_win);

   fprintf(stdout, "[I] to exit, press Ctrl+C ...\n");
   while(1)
   {
      // destination for the frog
      int x_dst = get_random(0, dpy_width - frog_attr.width);
      int y_dst = get_random(frog_attr.height + bubb_attr.height, dpy_height);

      // rotate the frog in the direction it should go
      // maybe this process should be put together in spin_frog()
      for (int i = 1; i < ((DIRECTIONS + (get_direction(x, y, x_dst, y_dst) - direction)) % DIRECTIONS + 1); i++)
      {
         draw_frog((direction + i) % DIRECTIONS, MOTIONS - 1);
         usleep((int)WAIT_TIME / 4);
      }

      direction = get_direction(x, y, x_dst, y_dst); // always odd

      step_size_x = get_random(3, 10);
      step_size_y = get_random(3, 10);
      int steps;
      if(abs(x - x_dst) > abs(y - y_dst))
      {
         steps = (int) abs(y - y_dst) / step_size_y;
      }
      else
      {
         steps = (int) abs(x - x_dst) / step_size_x;
      }

      for (int i = 0; i < steps; i++)
      {
         motion = i % (MOTIONS - 1); // use only walking motions
         draw_frog(direction, motion);
         usleep(WAIT_TIME);

         walk_a_step(&x, &y, direction);
      }

      direction = spin_frog(direction); // always even
   }
}
