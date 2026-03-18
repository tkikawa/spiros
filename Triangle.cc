#include "Triangle.hh"

Triangle::Triangle(double vtx[3][3])
{
  for(int i=0;i<3;i++){
    for(int j=0;j<3;j++){
      vertex[i][j]=vtx[i][j];
    }
  }
  a = (vertex[1][1]-vertex[0][1])*(vertex[2][2]-vertex[0][2])-(vertex[2][1]-vertex[0][1])*(vertex[1][2]-vertex[0][2]);
  b = (vertex[1][2]-vertex[0][2])*(vertex[2][0]-vertex[0][0])-(vertex[2][2]-vertex[0][2])*(vertex[1][0]-vertex[0][0]);
  c = (vertex[1][0]-vertex[0][0])*(vertex[2][1]-vertex[0][1])-(vertex[2][0]-vertex[0][0])*(vertex[1][1]-vertex[0][1]);
  d = -(a*vertex[0][0]+b*vertex[0][1]+c*vertex[0][2]);

  area=sqrt(a*a+b*b+c*c);
  normal[0]=a/area; normal[1]=b/area; normal[2]=c/area;
  area=area/2;

  for(int i=0;i<3;i++){
    bbmin[i] = vertex[0][i];
    bbmax[i] = vertex[0][i];
  }

  for(int i=1;i<3;i++){
    for(int j=0;j<3;j++){
      if(vertex[i][j] < bbmin[j]) bbmin[j] = vertex[i][j];
      if(vertex[i][j] > bbmax[j]) bbmax[j] = vertex[i][j];
    }
  }

  for(int i=0;i<3;i++){
    centroid[i] = (vertex[0][i] + vertex[1][i] + vertex[2][i]) / 3.0;
  }  
}
Triangle::~Triangle()
{
}
bool Triangle::Collision(const Position& s, const Position& t, Position& p){//Check if a line between two points crosses the triangle or not.
  const double ds = a*s[0] + b*s[1] + c*s[2] + d;
  const double dt = a*t[0] + b*t[1] + c*t[2] + d;
  if (ds * dt >= 0.0) return false;//Check if the two points are in difference sides of the triangle plane
  const double tx = t[0] - s[0];
  const double ty = t[1] - s[1];
  const double tz = t[2] - s[2];
  const double denom = a*tx + b*ty + c*tz;
  if (denom == 0.0) return false;// Line is parallel to plane
  double k = -ds / denom;
  p[0] = s[0] + k * tx;
  p[1] = s[1] + k * ty;
  p[2] = s[2] + k * tz;
  double cp1,cp2,cp3;// Cross products
  
  if(a!=0){
    cp1=(p[1]-vertex[0][1])*(vertex[1][2]-vertex[0][2])-(p[2]-vertex[0][2])*(vertex[1][1]-vertex[0][1]);
    cp2=(p[1]-vertex[1][1])*(vertex[2][2]-vertex[1][2])-(p[2]-vertex[1][2])*(vertex[2][1]-vertex[1][1]);
    if (cp1 * cp2 <= 0.0) return false;
    cp3=(p[1]-vertex[2][1])*(vertex[0][2]-vertex[2][2])-(p[2]-vertex[2][2])*(vertex[0][1]-vertex[2][1]);
  }
  else if(b!=0){
    cp1=(p[2]-vertex[0][2])*(vertex[1][0]-vertex[0][0])-(p[0]-vertex[0][0])*(vertex[1][2]-vertex[0][2]);
    cp2=(p[2]-vertex[1][2])*(vertex[2][0]-vertex[1][0])-(p[0]-vertex[1][0])*(vertex[2][2]-vertex[1][2]);
    if (cp1 * cp2 <= 0.0) return false;
    cp3=(p[2]-vertex[2][2])*(vertex[0][0]-vertex[2][0])-(p[0]-vertex[2][0])*(vertex[0][2]-vertex[2][2]);
  }
  else{
    cp1=(p[0]-vertex[0][0])*(vertex[1][1]-vertex[0][1])-(p[1]-vertex[0][1])*(vertex[1][0]-vertex[0][0]);
    cp2=(p[0]-vertex[1][0])*(vertex[2][1]-vertex[1][1])-(p[1]-vertex[1][1])*(vertex[2][0]-vertex[1][0]);
    if (cp1 * cp2 <= 0.0) return false;
    cp3=(p[0]-vertex[2][0])*(vertex[0][1]-vertex[2][1])-(p[1]-vertex[2][1])*(vertex[0][0]-vertex[2][0]);
  }
  if (cp1 * cp3 <= 0.0) return false;
  else return true;
}
Position Triangle::GetSurfPoint(double r1, double r2){//Randomely determined a point in the triangle surface.
  Position p;
  double t1=sqrt(r1);
  double t2=1-t1;
  double s1=r2;
  double s2=1-s1;
  double xtmp[2],ytmp[2],ztmp[2];
  xtmp[0]=vertex[0][0]*t1+vertex[2][0]*t2;
  ytmp[0]=vertex[0][1]*t1+vertex[2][1]*t2;
  ztmp[0]=vertex[0][2]*t1+vertex[2][2]*t2;
  xtmp[1]=vertex[1][0]*t1+vertex[2][0]*t2;
  ytmp[1]=vertex[1][1]*t1+vertex[2][1]*t2;
  ztmp[1]=vertex[1][2]*t1+vertex[2][2]*t2;
  p[0]=xtmp[0]*s1+xtmp[1]*s2;
  p[1]=ytmp[0]*s1+ytmp[1]*s2;
  p[2]=ztmp[0]*s1+ztmp[1]*s2;
  return p;
}
Direction Triangle::GetNormal(){//Get normal vector of the triangle plane.
  return normal;
}
