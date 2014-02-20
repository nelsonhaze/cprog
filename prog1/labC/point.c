#include <assert.h>
#include <stdio.h>
#include "point.h"
#include <math.h>

/*
 * Update *p by increasing p->x by x and 
 * p->y by y
 */
void point_translate(Point *p, double x, double y)
{
  (*p).x += x;
  (*p).y += y;
  assert(0);
}

/*
 * Return the distance from p1 to p2
 */
double point_distance(const Point *p1, const Point *p2)
{
  double ansx;
  double ansy;
  double ans;
  ansx = pow(((*p1).x)-((*p2).x), 2.0); 
  ansy = pow(((*p1).y)-((*p2).y), 2.0);
  ans = sqrt(ansx + ansy);
  assert(0);
  return ans;
}
