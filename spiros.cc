/********************************/
/*                              */
/*  SPIROS (optical simulator)  */
/*                              */
/*  Author: Tatsuya Kikawa      */
/*                              */
/********************************/

#include <iostream>
#include <iomanip>
#include <vector>
#include <sstream>
#include <random>
#include <regex>
#include <algorithm>
#include <getopt.h>
#include <sys/time.h>
#include <TApplication.h>
#include "Global.hh"
#include "Charged.hh"
#include "Gui.hh"
#include "Geometry.hh"
#include "Material.hh"
#include "Mixture.hh"
#include "Source.hh"
#include "Triangle.hh"
#include "Simulator.hh"

void PrintUsage(){
    std::cout << "Usage:" << std::endl;
    std::cout << " spiros [input card file] [output root file]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << " -s : Set the seed of random number generator" << std::endl;
    std::cout << " -d : Event display mode" << std::endl;
    exit(1);
}

bool is_number(const std::string& s) {
    std::regex re(R"(^\d*\.?\d+(e[+-]?\d+)?$)", std::regex::icase);
    return std::regex_match(s, re);
}

std::vector<Material*> CofigMaterials(std::mt19937 mt, Config config){
  std::vector<Material*> materials;
  std::string matfile;
  int id;
  double index, attlen, scatlen;
  for(std::map<std::string, std::string>::iterator i = config["Material"].begin(); i != config["Material"].end(); i++){
    std::istringstream(i->first) >> id;
    std::istringstream ss(i->second);
    ss >> matfile;
    std::string elem;
    if (ss >> elem && is_number(elem)) {
      // Process as a mixture
      std::vector<std::pair<double, std::string>> mix;
      do {
        double ratio = std::stod(elem);
        std::string subtype;
        // If the next token after subtype is a number, then this is an incomplete pair (no type name), so break
        std::streampos pos_before = ss.tellg();
        if (!(ss >> subtype) || is_number(subtype)) {
	  // The value read as subtype is actually the next material property (e.g., index), not a type name
	  // Restore the stream position
	  if (pos_before != std::streampos(-1)) ss.seekg(pos_before);
	  break;
        }
        mix.emplace_back(ratio, subtype);
      } while (ss >> elem && is_number(elem));
      
      // If there is any leftover token that is not a pair (i.e., a property value), put it back into the stream
      if (is_number(elem)) {
        // If there are still material properties such as index, restore the stream position for parsing
        ss.seekg((int)ss.tellg() - (int)elem.length());
      }
      // Initialize material property values
      index=1; attlen=0; scatlen=0;
      ss >> index >> attlen >> scatlen;
      Material *mat = new Mixture(mt, id, matfile, mix, index, attlen, scatlen);
      materials.push_back(mat);
    } else {
      // Process as a normal (single) Material
      index=1; attlen=0; scatlen=0;
      ss >> index >> attlen >> scatlen;
      Material *mat = new Material(mt, id, matfile, elem, index, attlen, scatlen);
      materials.push_back(mat);
    }
  }
  return materials;  
}


int main(int argc, char *argv[])
{
  if (argc < 3) {
    PrintUsage();
  }
  TApplication* app = new TApplication("app", 0, 0);

  struct timeval start_time, end_time;
  gettimeofday(&start_time, NULL);

  int seed=-1;
  bool dmode=false;
  int c;
  while ((c = getopt(argc, argv, "s:d")) != -1) {
    switch(c){
    case 's':
      seed = atoi(optarg);
      break;
    case 'd':
      dmode = true;
      break;
    default:
      PrintUsage();
    }
  }

  std::string input;
  std::string output;
  int op=0;
  while (optind < argc){
    switch(op){
    case 0:
      input = argv[optind++];
      break;
    case 1:
      output = argv[optind++];
      break;
    default:
      std::cerr << "Too many arguments." << std::endl;
      PrintUsage();
    }
    op++;
  }
  if(input.length()==0 || output.length()==0)PrintUsage();
  
  std::cout<<"Reading input card file."<<std::endl;
  Config config;
  ReadInFile(input.c_str(), config);

  std::cout<<"Setting pseudorandom number generator."<<std::endl;
  std::mt19937 mt;//Mersenne twister generator
  if(seed!=-1){
    mt.seed(seed);
  }
  else{
    std::random_device rnd;//Non-deterministic random number generator
    mt.seed(rnd());
  }

  std::cout<<"Loading source data."<<std::endl;
  Source *src = new Source(mt,config);
  
  std::cout<<"Loading material data."<<std::endl;
  std::vector<Material*> materials = CofigMaterials(mt, config);
  
  std::cout<<"Setting up simulator."<<std::endl;
  Simulator *sim = new Simulator(mt,config,output,src,materials);
  
  std::cout<<"Start simulation."<<std::endl;
  if(!dmode)sim->Run();//Normal run mode
  else{//Event display mode
    Gui* gui = new Gui(gClient->GetRoot(), 1000, 800, app);
    sim->SetDisplay(gui);
    app->Run();
  }
  gettimeofday(&end_time, NULL);
  double time_diff = end_time.tv_sec - start_time.tv_sec + (double)(end_time.tv_usec - start_time.tv_usec)/1e6;
  std::cout << "CPU time: "<< std::setprecision(4) << time_diff <<" sec."<< std::endl;
  std::cout << "All finished. \\(^o^)/" << std::endl;

  app->Terminate();
  return 0;
}
