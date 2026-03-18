#ifndef Triangle_h
#define Triangle_h 1

#include <string>
#include <iostream>
#include <vector>
#include <cmath>
#include "Global.hh"

class Triangle
{
public:
  Triangle(double vtx[3][3]);
  virtual ~Triangle();
  double X(int n){return vertex[n][0];}
  double Y(int n){return vertex[n][1];}
  double Z(int n){return vertex[n][2];}
  double A(){return a;}
  double B(){return b;}
  double C(){return c;}
  double D(){return d;}
  double GetArea(){return area;}
  bool Collision(const Position& s, const Position& t, Position& p);
  Position GetSurfPoint(double r1, double r2);
  Direction GetNormal();
  Position BBMin(){ return bbmin; }
  Position BBMax(){ return bbmax; }
  Position Centroid(){ return centroid; }
  
private:
  double vertex[3][3];
  Direction normal;
  double a,b,c,d;// Plane equation: ax+by+cz+d=0
  double area;// Surface area
  Position bbmin;
  Position bbmax;
  Position centroid;
};

#endif
