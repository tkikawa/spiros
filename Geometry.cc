#include "Geometry.hh"
#include <cfloat>

namespace {
  const int SMALL_GEOM_THRESHOLD = 16;
  const int LEAF_TRIANGLE_COUNT = 8;
}

Geometry::Geometry(std::mt19937& MT)
  : mt(MT)
{
  for(int i=0;i<3;i++){
    box_max[i]=-world;
    box_min[i]=world;
  }
}
Geometry::~Geometry()
{
}
void Geometry::LoadCAD(std::string name){//Read out the input CAD file as a assembly of trianbles

  std::ifstream cadfile(name);
  if(cadfile.good()){
    cadfile.close();
  }
  else{
    std::cerr<<"Error: CAD file "<<name.c_str()<<" is not found."<<std::endl;
    exit(1);
  }

  Assimp::Importer importer;
  const aiScene* scene;
  aiMesh* aim;
  
  scene = importer.ReadFile(name,
			    aiProcess_Triangulate           |
			    aiProcess_JoinIdenticalVertices |
			    aiProcess_CalcTangentSpace);

  aim = scene->mMeshes[0];

  double vertex[3][3];
  for(unsigned int i=0; i < aim->mNumFaces; i++){
    const aiFace& face = aim->mFaces[i];
    for(int j=0;j<3;j++){
      vertex[j][0]=Round(aim->mVertices[face.mIndices[j]].x*cadunit);
      vertex[j][1]=Round(aim->mVertices[face.mIndices[j]].y*cadunit);
      vertex[j][2]=Round(aim->mVertices[face.mIndices[j]].z*cadunit);
      Compare(box_max[0],box_min[0],vertex[j][0]);
      Compare(box_max[1],box_min[1],vertex[j][1]);
      Compare(box_max[2],box_min[2],vertex[j][2]);
    }
    triangle.push_back(Triangle(vertex));
  }

  BuildBVH();
}

namespace {
  bool SegmentIntersectsAABB(const Position& s, const Position& t,
			     const Position& bbmin, const Position& bbmax)
  {
    double tmin = 0.0;
    double tmax = 1.0;
    
    for(int i=0; i<3; i++){
      double d = t[i] - s[i];
      
      if(std::fabs(d) < 1e-12){
	if(s[i] < bbmin[i] || s[i] > bbmax[i]) return false;
      }
      else{
	double invd = 1.0 / d;
	double t0 = (bbmin[i] - s[i]) * invd;
	double t1 = (bbmax[i] - s[i]) * invd;
	
	if(t0 > t1){
	  double tmp = t0;
	  t0 = t1;
	  t1 = tmp;
	}
	
	if(t0 > tmin) tmin = t0;
	if(t1 < tmax) tmax = t1;
	
	if(tmin > tmax) return false;
      }
    }
    
    return true;
  }
}

bool Geometry::FirstHit(const Position& s, const Position& t, Position& hit, Direction& normal)
{
  if(root < 0){
    bool found = false;
    double best = DBL_MAX;
    Position cand;

    for(unsigned int i=0; i<triangle.size(); i++){
      if(triangle[i].Collision(s, t, cand)){
        double d = Distance(s, cand);
        if(!found || d < best){
          best = d;
          for(int j=0; j<3; j++) hit[j] = cand[j];
          normal = triangle[i].GetNormal();
          found = true;
        }
      }
    }

    return found;
  }

  bool found = false;
  double best = DBL_MAX;

  std::vector<int> stack;
  stack.push_back(root);

  while(!stack.empty()){
    int node_id = stack.back();
    stack.pop_back();

    BVHNode& node = bvh[node_id];

    if(!SegmentIntersectsAABB(s, t, node.bbmin, node.bbmax)) continue;

    if(node.leaf){
      for(int i=node.begin; i<node.end; i++){
        int tid = tri_index[i];
        Position cand;
        if(triangle[tid].Collision(s, t, cand)){
          double d = Distance(s, cand);
          if(!found || d < best){
            best = d;
            for(int j=0; j<3; j++) hit[j] = cand[j];
            normal = triangle[tid].GetNormal();
            found = true;
          }
        }
      }
    }
    else{
      if(node.left >= 0) stack.push_back(node.left);
      if(node.right >= 0) stack.push_back(node.right);
    }
  }

  return found;
}

void Geometry::BuildBVH()
{
  tri_index.clear();
  bvh.clear();

  for(unsigned int i=0; i<triangle.size(); i++){
    tri_index.push_back(i);
  }

  if(triangle.size()==0){
    root = -1;
    return;
  }

  if(triangle.size() <= SMALL_GEOM_THRESHOLD){
    root = -1;
    return;
  }

  root = BuildNode(0, triangle.size());
}

int Geometry::BuildNode(int begin, int end)
{
  BVHNode node;

  // Make overall AABB for triangles in this node
  Position tmin = triangle[tri_index[begin]].BBMin();
  Position tmax = triangle[tri_index[begin]].BBMax();

  for(int j=0; j<3; j++){
    node.bbmin[j] = tmin[j];
    node.bbmax[j] = tmax[j];
  }

  for(int i=begin+1; i<end; i++){
    Position bb0 = triangle[tri_index[i]].BBMin();
    Position bb1 = triangle[tri_index[i]].BBMax();
    for(int j=0; j<3; j++){
      if(bb0[j] < node.bbmin[j]) node.bbmin[j] = bb0[j];
      if(bb1[j] > node.bbmax[j]) node.bbmax[j] = bb1[j];
    }
  }

  int ntri = end - begin;

  // leaf condition
  if(ntri <= LEAF_TRIANGLE_COUNT){
    node.leaf = true;
    node.begin = begin;
    node.end = end;
    node.left = -1;
    node.right = -1;

    bvh.push_back(node);
    return bvh.size() - 1;
  }

  // Select the longest axis
  double lenx = node.bbmax[0] - node.bbmin[0];
  double leny = node.bbmax[1] - node.bbmin[1];
  double lenz = node.bbmax[2] - node.bbmin[2];

  int axis = 0;
  if(leny > lenx && leny >= lenz) axis = 1;
  else if(lenz > lenx && lenz > leny) axis = 2;

  // Sort by centroid
  std::sort(tri_index.begin() + begin, tri_index.begin() + end,
    [&](int a, int b){
      return triangle[a].Centroid()[axis] < triangle[b].Centroid()[axis];
    });

  int mid = (begin + end) / 2;

  node.leaf = false;
  node.begin = -1;
  node.end = -1;

  // Temporality add node
  bvh.push_back(node);
  int node_id = bvh.size() - 1;

  int left_id = BuildNode(begin, mid);
  int right_id = BuildNode(mid, end);

  bvh[node_id].left = left_id;
  bvh[node_id].right = right_id;

  return node_id;
}

int Geometry::CountIntersections(const Position& s, const Position& t)
{
  if(root < 0){
    int ncol = 0;
    Position cand;

    for(unsigned int i=0; i<triangle.size(); i++){
      if(triangle[i].Collision(s, t, cand)){
        ncol++;
      }
    }

    return ncol;
  }  

  int ncol = 0;

  std::vector<int> stack;
  stack.push_back(root);

  while(!stack.empty()){
    int node_id = stack.back();
    stack.pop_back();

    BVHNode& node = bvh[node_id];

    if(!SegmentIntersectsAABB(s, t, node.bbmin, node.bbmax)) continue;

    if(node.leaf){
      for(int i=node.begin; i<node.end; i++){
        int tid = tri_index[i];
        Position cand;
        if(triangle[tid].Collision(s, t, cand)){
          ncol++;
        }
      }
    }
    else{
      if(node.left >= 0) stack.push_back(node.left);
      if(node.right >= 0) stack.push_back(node.right);
    }
  }

  return ncol;
}

bool Geometry::InSolid(const Position& pos)//Check if the point is inside or outside of the geometry
{
  if(!InAABB(pos)) return false;
  Position far;
  far[0]=1.2345*world; far[1]=2.3456*world; far[2]=3.4567*world;
  int ncol = CountIntersections(pos, far);
  return (ncol % 2 == 1);
}

double Geometry::Round(double p0){
  double eps = 1e-5;
  return std::round(p0 / eps) * eps;
}
bool Geometry::IntersectsAABB(const Position& origin, const Position& end){
  return SegmentIntersectsAABB(origin, end, box_min, box_max);
}
bool Geometry::InAABB(const Position& pos){
  for (int i = 0; i < 3; ++i) {
    if(pos[i]<box_min[i]||pos[i]>box_max[i])return false;
  }
  return true;
}
