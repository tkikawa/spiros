#ifndef Geometry_h
#define Geometry_h 1

#include <string>
#include <vector>
#include <random>
#include "Global.hh"
#include "Triangle.hh"

#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags

struct BVHNode{
  Position bbmin;
  Position bbmax;
  int left;
  int right;
  int begin;
  int end;
  bool leaf;
};

class Geometry
{
public:
  Geometry(std::mt19937& MT);
  virtual ~Geometry();
  void LoadCAD(std::string name);
  int NTriangle(){return triangle.size();}
  Triangle GetTriangle(int n){return triangle[n];}
  bool FirstHit(const Position& s, const Position& t, Position& hit, Direction& normal);
  int CountIntersections(const Position& s, const Position& t);
  bool InSolid(const Position& pos);
  void BuildBVH();
  int BuildNode(int begin, int end);
  double Round(double p0);
  bool IntersectsAABB(const Position& origin, const Position& end);
  bool InAABB(const Position& pos);
  std::mt19937& mt;
  std::vector<Triangle> triangle;
  std::vector<int> tri_index;
  std::vector<BVHNode> bvh;
  int root;
  std::uniform_real_distribution<double> unirand;
  std::normal_distribution<double> gausrand;
  Position box_max, box_min;
};

#endif
