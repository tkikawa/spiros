#include "Geometry.hh"

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
  
}
bool Geometry::InSolid(const Position& pos){//Check if the point is inside or outside of the geometry
  if(!InAABB(pos))return false;
  Position far, tmp;
  far[0]=1.2345*world; far[1]=2.3456*world; far[2]=3.4567*world;
  int ncol = 0;
  for(unsigned int i=0; i < triangle.size(); i++){
    if(triangle[i].Collision(pos,far,tmp))ncol++;
  }
  if(ncol%2 == 1) return true;
  else            return false;
}
double Geometry::Round(double p0){
  double eps = 1e-5;
  return std::round(p0 / eps) * eps;
}
bool Geometry::IntersectsAABB(const Position& origin, const Position& end){
  for (int i = 0; i < 3; ++i) {
    double dir = end[i] - origin[i];
    if (dir == 0) {
      if (origin[i] < box_min[i] || origin[i] > box_max[i]) return false;
      continue;
    }
    double invD = 1.0 / dir;
    double t0 = (box_min[i] - origin[i]) * invD;
    double t1 = (box_max[i] - origin[i]) * invD;
    if (t0 > t1) std::swap(t0, t1);
    if (t1 < 0) return false;
  }
  return true;
}
bool Geometry::InAABB(const Position& pos){
  for (int i = 0; i < 3; ++i) {
    if(pos[i]<box_min[i]||pos[i]>box_max[i])return false;
  }
  return true;
}
