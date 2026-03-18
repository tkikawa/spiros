#ifndef Simulator_h
#define Simulator_h 1

#include "Global.hh"
#include "Source.hh"
#include "Material.hh"
#include "Charged.hh"
#include "Gui.hh"
#include <cfloat>
#include <string>
#include <vector>
#include <random>
#include <TFile.h>
#include <TTree.h>
#include <TCanvas.h>
#include <TView.h>
#include <TPolyLine3D.h>

class Simulator
{
public:
  Simulator(std::mt19937& MT, Config config, std::string OUTPUT, Source *SRC, std::vector<Material*> &MAT);
  virtual ~Simulator();
  void Run();
  void SetDisplay(Gui *GUI);
  Material* GetMaterial(int i){return mat[i];};
  int GetNMat(){return mat.size();};
  void SetNevt(int NEVT){nevt=NEVT;};
  Source* GetSource(){return src;};
  std::array<int, 5> GetResult(){return count;};
  
private:
  int PointMaterial(const Position& p);
  int FType(int bt);
  void Summary();
  bool Fresnel(const Direction& v, const Direction& p, Direction& newv, Direction& newp, Direction norm, double idx_in, double idx_out);
  void Specular(const Direction& v, const Direction& p, Direction& newv, Direction& newp, Direction norm);
  void Lambert(const Direction& v, Direction& newv, Direction& newp, Direction norm);
  void Rayleigh(const Direction& v, const Direction& p, Direction& newv, Direction& newp);
  void Mie(const Direction& v, const Direction& p, Direction& newv, Direction& newp);
  std::mt19937& mt;
  std::uniform_real_distribution<double> unirand;
  std::exponential_distribution<double> exprand;
  std::string output;
  Source *src;
  Charged *chg;
  Gui *gui=nullptr;
  std::vector<Material*> mat;
  int nevt;
  double index0, yield, delay, wlmin, wlmax,gmie,eff;
  std::string sci_type;
  bool displaymode,act_scinti,act_cherenkov,usemie;
  std::array<int, 5> count;
};

#endif
