#include "Simulator.hh"

Simulator::Simulator(std::mt19937& MT, Config config, std::string OUTPUT, Source *SRC, std::vector<Material*> &MAT)
  : mt(MT),
    output(OUTPUT),
    src(SRC),
    mat(MAT),
    nevt(10000),
    index0(1),
    yield(0),
    delay(-99999),
    wlmin(-99999),
    wlmax(-99999),
    gmie(-99999),
    eff(1),
    sci_type(""),
    displaymode(false),
    act_scinti(false),
    act_cherenkov(false),
    usemie(false)
{
  std::istringstream(config["Global"]["Number"]) >> nevt;
  if(nevt<=0){
    std::cout<<"Number of primary particles is "<<nevt<<std::endl;
    std::cout<<"It must be more than 0."<<std::endl;
  }
  std::istringstream(config["Global"]["Index"]) >> index0;
  if(index0<1){
    std::cout<<"Refractive index of surroundings is"<<index0<<std::endl;
    std::cout<<"It must be 1 or more."<<std::endl;
  }
  std::istringstream(config["Global"]["Scintillation"]) >> sci_type >> yield >> delay;
  if(!sci_type.empty()){
    check_defined(sci_type);
    if(yield==0){
      std::cout<<"Scintillation is activated with "<<sci_type <<" scintillator with default light yield."<<std::endl;
    }
    else{
      std::cout<<"Scintillation is activated with "<<sci_type <<" scintillator with "<<yield<<" photons/MeV."<<std::endl;
    }
    act_scinti=true;
  }
  std::istringstream(config["Global"]["Cherenkov"]) >> wlmin >> wlmax;
  if(wlmin>100&&wlmin<wlmax&&wlmax<700){
    std::cout<<"Cherenkov radiation is activated for wavelendth range of "<<wlmin<<" nm to "<<wlmax<<" nm."<<std::endl;
    act_cherenkov=true;
  }
  else if(wlmin!=-99999||wlmax!=-99999){
    std::cout<<"Setting of wavelength range for Cherenkov radiation is strange."<<std::endl;
    exit(1);
  }
  std::istringstream(config["Global"]["Mie"]) >> gmie;
  if(gmie>-1&&gmie<1){
    std::cout<<"Mie scattering is activated with asymmetry parameter = "<<gmie<<std::endl;
    usemie=true;
  }
  else if(gmie!=-99999){
    std::cout<<"Asymmetry parameter in Mie scattering is "<<gmie<<std::endl;
    std::cout<<"It must be between -1 and 1."<<std::endl;
    exit(1);
  }
  std::istringstream(config["Global"]["Efficiency"]) >> eff;
  if(eff<0||eff>1){
    std::cout<<"Photon detection efficiency is "<<eff<<std::endl;
    std::cout<<"It must be between 0 and 1."<<std::endl;
    exit(1);
  }
  chg = new Charged(mt,mat);
  if(act_scinti||act_cherenkov){
    chg->SetBeta(src->GetBeta());
    chg->SetElectron(src->IsElectron());
  }
  if(act_scinti)chg->SetScinti(sci_type,yield,delay);
  if(act_cherenkov)chg->SetCherenkov(wlmin,wlmax);
}
Simulator::~Simulator()
{
}
void Simulator::Run(){//Run simulation

  Position pos,vec,pol,cross,cand,newpos;
  Direction newvec,newpol,normal;
  double index=1,newindex=1,attlen=0,newattlen=0,scatlen=0,newscatlen=0;
  int matid=0, newmatid=0, btype, mn, newmn;
  double pl,apl,spl;
  bool converted=false;
  
  double ipos[3],fpos[3],ivec[3],fvec[3],ipol[3],fpol[3],time,length; //Branch variables for TTree *tree
  int imat, fmat, ftype, nref, npas, id;                              //Branch variables for TTree *tree

  double cpos[3],cvec[3]; //Branch variables for TTree *charged
  int nph, ntype[5];      //Branch variables for TTree *charged
  
  TFile *file = new TFile(output.c_str(), "recreate");//Output ROOT file
  TTree *tree = new TTree("photon","photon");
  TTree *charged = new TTree("charged","charged");
  tree->Branch("ipos[3]",&ipos,"ipos[3]/D");//Initial position of optical photon (mm). [0], [1], [2] for x, y, z.
  tree->Branch("fpos[3]",&fpos,"fpos[3]/D");//Final position of optical photon (mm). [0], [1], [2] for x, y, z.
  tree->Branch("ivec[3]",&ivec,"ivec[3]/D");//Initial direction of optical photon. [0], [1], [2] for x, y, z.
  tree->Branch("fvec[3]",&fvec,"fvec[3]/D");//Final direction of optical photon. [0], [1], [2] for x, y, z.
  tree->Branch("ipol[3]",&ipol,"ipol[3]/D");//Initial polarization of optical photon. [0], [1], [2] for x, y, z.
  tree->Branch("fpol[3]",&fpol,"fpol[3]/D");//Final polarization of optical photon. [0], [1], [2] for x, y, z.
  tree->Branch("time",&time,"time/D");//Time from generation to end of optical photon (ns).
  tree->Branch("length",&length,"length/D");//Total path length traveled by optical photon (mm).
  tree->Branch("imat",&imat,"imat/I");//Material ID in which optical photon was generated.
  tree->Branch("fmat",&fmat,"fmat/I");//Material ID in which optical photon ended.
  tree->Branch("ftype",&ftype,"ftype/I");//ID which identifies how optical photon ended. (See README)
  tree->Branch("nref",&nref,"nref/I");//Number of reflections of optical photon in boundaries.
  tree->Branch("npas",&npas,"npas/I");//Number of transmissions of optical photon in boundaries.
  if(src->ChargedMode()){
    tree->Branch("id",&id,"id/I");//ID of corresponding charged particle.
 
    charged->Branch("ipos[3]",&cpos,"ipos[3]/D");//Initial position of charged particle (mm). [0], [1], [2] for x, y, z.
    charged->Branch("ivec[3]",&cvec,"ivec[3]/D");//Initial direction of charged particle (mm). [0], [1], [2] for x, y, z.
    charged->Branch("nph",&nph,"nph/I");//Number of produced photons by the charged particle
    charged->Branch("ntype[5]",&ntype,"ntype[5]/I");//Number of photons in each end process
    charged->Branch("id",&id,"id/I");//ID for charged particles.
  }
  
  for(int j=0;j<5;j++)count[j]=0;
  if(!displaymode){
    if(src->ChargedMode()){
      std::cout<<"Generate "<<nevt<<" charged particles."<<std::endl;
    }
    else{
      std::cout<<"Generate "<<nevt<<" optical photons."<<std::endl;
    }
  }
  bool progbar=false;
  if(nevt>=100&&!displaymode)progbar=true;
  if(progbar){
    std::cout<<"0%       25%       50%       75%       100%"<<std::endl;
    std::cout<<"+---------+---------+---------+---------+"<<std::endl;;
  }
  double ndev40=nevt/40.;
  int nprog=0;
  for(int i=0;i<nevt;i++){
    if(progbar){
      if(i>=nprog*ndev40){
	std::cout<<"#"<<std::flush;
	nprog++;
      }
      else if(i==nevt-1){
	std::cout<<"#"<<std::endl;
      }
    }
    if(src->ChargedMode()){
      src->Generate(pos, vec, pol);//Determine the initial position and direction of charged particle
      if(displaymode){
	for(int j=0;j<3;j++)cross[j]=pos[j]+vec[j]*world;
	gui->DrawTrack(pos,cross,1);
      }
      chg->Simulate(pos, vec);//Simualtion for charged particles.
      nph=chg->GetNPhotons();
      for(int j=0;j<3;j++){
	cpos[j]=pos[j];
	cvec[j]=vec[j];
      }
      for(int j=0;j<5;j++)ntype[j]=0;
      id=i;
    }
    else nph=1;
    for(int p=0;p<nph;p++){
      if(src->ChargedMode()){
	chg->Generate(pos, vec, pol, time, p);//Determine the initial position, direction and time
	mn=PointMaterial(pos);//Check the material of initial position
      }
      else{
	time=0;
	while(1){
	  src->Generate(pos, vec, pol);//Determine the initial position and direction
	  mn=PointMaterial(pos);//Check the material of initial position
	  if(mn==-1)break;//Check if initial position of out of material volumes (i.e. air).
	  else if(mat[mn]->TType()<2||mat[mn]->TType()==6)break;//Check if the material type is medium, converter or mixture including medium or converter
	}
      }
      if(mn!=-1){//Initial position is in the defined material volumes.
	matid=mat[mn]->ID();
	index=mat[mn]->Index();
	attlen=mat[mn]->AttLen();
	scatlen=mat[mn]->ScatLen();
      }
      else{//Initial position is out of the defined material volumes and is in surrounding (air).
	matid=0;
	index=index0;
	attlen=0;
	scatlen=0;
      }

      //Initialization of variables
      for(int j=0;j<3;j++){
	ipos[j]=pos[j];
	ivec[j]=vec[j];
	ipol[j]=pol[j];
	newpos[j]=pos[j];
	newvec[j]=vec[j];
	newpol[j]=pol[j];
      }
      imat=matid;
      nref=0;
      npas=0;
      length=0;
      converted=false;
      
      while(1){
	btype = -2;
	for(int j=0;j<3;j++)cross[j]=pos[j]+vec[j]*world;//far point on the straight-line of the track
	for(unsigned int m=0;m<mat.size();m++){//loop for Materials
	  if(matid!=0&&matid<mat[m]->ID()){
	    if(btype>-1){
	      break;
	    }
	    else if(btype==-1){
	      if(mat[m]->InSolid(cross)){
		newmatid=mat[m]->ID();
		newindex=mat[m]->Index();
		newattlen=mat[m]->AttLen();
		newscatlen=mat[m]->ScatLen();
		btype=mat[m]->Type();
		newmn=m;
		break;
	      }
	      continue;
	    }
	  }
	  if(matid==mat[m]->ID()||mat[m]->IntersectsAABB(pos, cross)){
	    for(int t=0;t<mat[m]->NTriangle();t++){//loop for Triangles
	      if(mat[m]->GetTriangle(t).Collision(pos,cross,cand)){//Check if a line between two points collides with the triangle or not
		for(int j=0;j<3;j++){
		  newpos[j]=cand[j];
		  if(matid==0){
		    cross[j]=cand[j];
		  }
		  else if(matid==mat[m]->ID()){
		    cross[j]=cand[j]+vec[j]*micro;
		  }
		  else{
		    cross[j]=cand[j]-vec[j]*micro;
		  }
		}
		normal=mat[m]->GetTriangle(t).GetNormal();
		if(matid==mat[m]->ID()){
		  newmatid=0;
		  newindex=index0;
		  newattlen=0;
		  newscatlen=0;
		  btype=-1;
		  newmn=-1;
		}
		else{
		  newmatid=mat[m]->ID();
		  newindex=mat[m]->Index();
		  newattlen=mat[m]->AttLen();
		  newscatlen=mat[m]->ScatLen();
		  btype=mat[m]->Type();
		  newmn=m;
		}
	      }//end of if(mat[m]->GetTriangle(t).Collision(pos,cross,cand))
	    }//end of for(int t=0;t<mat[m]->NTriangle();t++)
	  }//end of for(int t=0;t<mat[m]->NTriangle();t++)
	}//end of if(matid==mat[m]->ID()||mat[m]->IntersectsAABB(pos, cross))
	if(btype!=-2){
	  pl=Distance(newpos,pos);
	  if(attlen>0||scatlen>0){//When attlen==0 (scatlen==0), absorption (scattering) in the medium does not occur.
	    if(attlen>0)apl=attlen*exprand(mt);
	    else apl=DBL_MAX;
	    if(scatlen>0)spl=scatlen*exprand(mt);
	    else spl=DBL_MAX;
	    if(apl<pl || spl<pl){
	      if(apl<spl){//Absorption in the medium
		pl=apl;
		btype=-3;
		newmatid=matid;
	      }
	      else{//Rayleigh or Mie scattering in the medium
		pl=spl;
		btype=10;
		if(!usemie) Rayleigh(vec,pol,newvec,newpol);
		else        Mie(vec,pol,newvec,newpol);
	      }
	      for(int j=0;j<3;j++){
		newpos[j]=pos[j]+vec[j]*pl;
	      }
	    }
	  }
	  length+=pl;
	  time+=pl/c_0*index;
	  if(displaymode){
	    gui->DrawTrack(pos,newpos);
	  }
	}
	else{
	  if(displaymode){
	    for(int j=0;j<3;j++){
	      cand[j]=pos[j]+vec[j]*world*2;
	    }
	    gui->DrawTrack(pos,cand);
	  }
	}
	if((btype >= -1 && btype <4) || btype==10){
	  if(btype <= 1){
	    if(Fresnel(vec,pol,newvec,newpol,normal,index,newindex)){//Determine if the optical photon is reflected or transmitted with refraction following the Fresnel equation
	      mn=newmn;
	      matid=newmatid;
	      index=newindex;
	      attlen=newattlen;
	      scatlen=newscatlen;
	      npas++;
	      if(btype == 1 && !converted){//Isotropic scattering for converter
		Isotropic(mt,newvec);
		RandomPolarization(mt,newvec,newpol);
		converted=true;
	      }
	    }
	    else{
	      nref++;
	    }
	  }
	  else if(btype == 2){
	    Specular(vec,pol,newvec,newpol,normal);//Specular reflection
	    nref++;
	  }
	  else if(btype == 3){
	    Lambert(vec,newvec,newpol,normal);//Diffusion following the Lambert's cosine law
	    nref++;
	  }
	  Normalize(newvec);
	  Normalize(newpol);
	  for(int j=0;j<3;j++){
	    pos[j]=newpos[j]+newvec[j]*nano;
	    vec[j]=newvec[j];
	    pol[j]=newpol[j];
	  }
	}
	else break;//Absorbed, detected or go out of world volume.
	if(nref>nreflimit){
	  btype=-4;
	  break;
	}
      }//end of while
      for(int j=0;j<3;j++){
	fpos[j]=newpos[j];
	fvec[j]=newvec[j];
	fpol[j]=newpol[j];
      }
      fmat=newmatid;
      ftype=FType(btype);//Conversion of type ID.
      tree->Fill();
      count[ftype]++;
      if(src->ChargedMode())ntype[ftype]++;
    }//end of for
    if(src->ChargedMode())charged->Fill();
  }//end of for
  if(!displaymode){
    Summary();
  }
  tree->Write();
  if(src->ChargedMode())charged->Write();
  file->Purge();
  file->Close();  
}
void Simulator::SetDisplay(Gui *GUI){
  gui = GUI;
  gui->SetSimulator(this);
  displaymode=true;
}
int Simulator::PointMaterial(const Position& p){//Check the material of initial position
  for(unsigned int m=0;m<mat.size();m++){
    if(!mat[m]->InAABB(p)) continue;
    if(mat[m]->InSolid(p)) return m;
  }
  return -1;
}
int Simulator::FType(int bt){//Conver the temporary type ID to final type ID
  if(bt==-2)return 0;     //Go out of world volume.
  else if(bt==-4)return 1;//Exceed the limit of reflections
  else if(bt==-3)return 2;//Absorbed in normal medium.
  else if(bt==4)return 3; //Absorbed by absorber
  else if(bt==5){//Absorbed by detector (considering photon detection efficiency)
    if(unirand(mt)<eff)return 4;//Detected
    else return 3;//Not detected
  }
  else{
    std::cerr<<"Error in FType"<<std::endl;
    exit(1);
  }
}
void Simulator::Summary(){//Display the summary of simulation
  std::cout<<"Go out of world volume     : "<<count[0]<<std::endl;
  std::cout<<"Exceed limit of reflections: "<<count[1]<<std::endl;
  std::cout<<"Absorved in normal medium  : "<<count[2]<<std::endl;
  std::cout<<"Absorbed by absorber       : "<<count[3]<<std::endl;
  std::cout<<"Detected by detector       : "<<count[4]<<std::endl;
}
bool Simulator::Fresnel(const Direction& v, const Direction& p, Direction& newv, Direction& newp, Direction norm, double idx_in, double idx_out) {//Determine if the optical photon is reflected or transmitted with refraction following the Fresnel equation
  double cos_in = Dot(v, norm);
  if (cos_in > 0) {
    for (int i = 0; i < 3; ++i) norm[i] *= -1;
  } else {
    cos_in *= -1;
  }

  double eta = idx_in / idx_out;
  double sin2_t = eta * eta * (1.0 - cos_in * cos_in);

  // Define s and p polarization directions for the incident ray
  Direction s_dir, p_dir;
  Cross(norm, v, s_dir);
  Normalize(s_dir);
  Cross(s_dir, v, p_dir);
  Normalize(p_dir);

  double s_amp = Dot(p, s_dir);
  double p_amp = Dot(p, p_dir);

  if (sin2_t > 1.0) {  // Total internal reflection
    Specular(v, p, newv, newp, norm);  // use new polarization-aware version
    return false;
  }

  double cos_out = sqrt(1.0 - sin2_t);
  double rs = (idx_in * cos_in - idx_out * cos_out) / (idx_in * cos_in + idx_out * cos_out);
  double rp = (idx_out * cos_in - idx_in * cos_out) / (idx_out * cos_in + idx_in * cos_out);
  double ts = 2.0 * idx_in * cos_in / (idx_in * cos_in + idx_out * cos_out);
  double tp = 2.0 * idx_in * cos_in / (idx_out * cos_in + idx_in * cos_out);

  double ref_prob = 0.5 * (rs * rs + rp * rp);
  if (unirand(mt) < ref_prob) {  // Reflection
    Specular(v, p, newv, newp, norm);  // use new polarization-aware version
    // Uncomment below to amplitude scaling to be preserved
    /*
    Direction s_dir_reflected, p_dir_reflected;
    Cross(norm, newv, s_dir_reflected);
    Normalize(s_dir_reflected);
    Cross(s_dir_reflected, newv, p_dir_reflected);
    Normalize(p_dir_reflected);

    double s_out = Dot(newp, s_dir_reflected) * rs;
    double p_out = Dot(newp, p_dir_reflected) * rp;

    for (int i = 0; i < 3; ++i) {
      newp[i] = s_out * s_dir_reflected[i] + p_out * p_dir_reflected[i];
    }
    Normalize(newp);
    */
    return false;
  } else {  // Refraction
    for (int i = 0; i < 3; ++i) {
      newv[i] = eta * v[i] + (eta * cos_in - cos_out) * norm[i];
    }
    Normalize(newv);

    // Recalculate new p-direction in the transmitted geometry
    Direction trans_p_dir;
    Cross(s_dir, newv, trans_p_dir);
    Normalize(trans_p_dir);

    for (int i = 0; i < 3; ++i) {
      newp[i] = ts * s_amp * s_dir[i] + tp * p_amp * trans_p_dir[i];
    }
    Normalize(newp);
    return true;
  }
}
void Simulator::Specular(const Direction& v, const Direction& p, Direction& newv, Direction& newp, Direction norm) {
  // Calculate mirror-reflected direction ---
  double cp = Dot(v, norm);
  for (int i = 0; i < 3; ++i) {
    newv[i] = v[i] - 2.0 * cp * norm[i];
  }
  Normalize(newv);

  // Construct the s-polarization direction (perpendicular to the plane of incidence) ---
  Direction s_dir;
  Cross(norm, v, s_dir);
  Normalize(s_dir);

  // Construct the p-polarization direction (in the plane of incidence) ---
  Direction p_dir;
  Cross(s_dir, v, p_dir);
  Normalize(p_dir);

  // Decompose incident polarization vector into s and p components ---
  double s_amp = Dot(p, s_dir);
  double p_amp = Dot(p, p_dir);

  // Reconstruct p-polarization direction after reflection ---
  Direction new_p_dir;
  Cross(s_dir, newv, new_p_dir);  // p-direction in the reflected geometry
  Normalize(new_p_dir);

  // Reconstruct new polarization vector using s and p components ---
  for (int i = 0; i < 3; ++i) {
    newp[i] = s_amp * s_dir[i] + p_amp * new_p_dir[i];
  }
  Normalize(newp);
}
void Simulator::Lambert(const Direction& v, Direction& newv, Direction& newp, Direction norm) {//Diffusion following the Lambert's cosine law
  // Flip normal if needed to ensure it's against incoming ray ---
  double cp = Dot(v, norm);
  if (cp > 0) {
    for (int i = 0; i < 3; i++) norm[i] *= -1;
  }

  // Construct local frame aligned with norm ---
  double cost = norm[2];
  double sint = sqrt(1.0 - cost * cost);
  double cosp = (sint != 0.0) ? norm[0] / sint : 1.0;
  double sinp = (norm[1] > 0.0) ? sqrt(1.0 - cosp * cosp) : -sqrt(1.0 - cosp * cosp);

  // Sample newv using Lambert's cosine law ---
  Direction ranv;
  ranv[2] = sqrt(unirand(mt)); // cosine-weighted zenith angle
  double ang = unirand(mt) * 2.0 * pi;
  double vr = sqrt(1.0 - ranv[2] * ranv[2]);
  ranv[0] = vr * cos(ang);
  ranv[1] = vr * sin(ang);

  // Rotate sampled direction into global coordinates ---
  newv[0] = -sinp * ranv[0] + cost * cosp * ranv[1] + sint * cosp * ranv[2];
  newv[1] =  cosp * ranv[0] + cost * sinp * ranv[1] + sint * sinp * ranv[2];
  newv[2] =                     -sint * ranv[1]      + cost * ranv[2];
  Normalize(newv);
  RandomPolarization(mt,newv, newp);
}
void Simulator::Rayleigh(const Direction& v, const Direction& p, Direction& newv, Direction& newp){//Rayleigh scattering in the medium
  double costheta;

  // Rejection sampling from P(cosθ) ∝ 1 + cos²θ
  while (true) {
    double x = 2.0 * unirand(mt) - 1.0;
    double y = unirand(mt) * 2.0;
    if (y <= 1 + x * x) {
      costheta = x;
      break;
    }
  }

  double sintheta = sqrt(1.0 - costheta * costheta);
  double phi = 2 * pi * unirand(mt);
  double cosphi = cos(phi);
  double sinphi = sin(phi);

  // Construct orthonormal basis (vx, wx, v)
  Direction vx, wx;
  if (fabs(v[2]) < 0.99) {
    vx = {-v[1], v[0], 0};
  } else {
    vx = {0, -v[2], v[1]};
  }
  Normalize(vx);
  Cross(v, vx, wx);
  Normalize(wx);

  // Compute newv
  for (int i = 0; i < 3; ++i) {
    newv[i] = sintheta * cosphi * vx[i] + sintheta * sinphi * wx[i] + costheta * v[i];
  }
  Normalize(newv);

  // Compute scattering plane normal: perp to both v and newv
  Direction scatter_plane_normal;
  Cross(v, newv, scatter_plane_normal);
  Normalize(scatter_plane_normal);

  // Project original polarization p onto new polarization direction
  // Remove components along newv
  Direction temp_p;
  double dot_pv = Dot(p, newv);
  for (int i = 0; i < 3; ++i) {
    temp_p[i] = p[i] - dot_pv * newv[i]; // make it orthogonal to newv
  }

  // Project temp_p onto the scatter_plane_normal direction (this is Rayleigh)
  double proj = Dot(temp_p, scatter_plane_normal);
  for (int i = 0; i < 3; ++i) {
    newp[i] = proj * scatter_plane_normal[i];
  }

  Normalize(newp);
}
void Simulator::Mie(const Direction& v, const Direction& p, Direction& newv, Direction& newp) {
  // Sampling scattering direction
  double xi = unirand(mt);
  double costheta;

  if (fabs(gmie) < 1e-6) {
    costheta = 2.0 * xi - 1.0; // Isotropic case
  } else {
    double sq = (1.0 - gmie * gmie) / (1.0 - gmie + 2.0 * gmie * xi);
    costheta = (1.0 + gmie * gmie - sq * sq) / (2.0 * gmie);
  }

  double sintheta = sqrt(std::max(0.0, 1.0 - costheta * costheta));
  double phi = 2.0 * pi * unirand(mt);
  double cosphi = cos(phi);
  double sinphi = sin(phi);

  // Define of local coordinate
  Direction vx, wx;
  if (fabs(v[2]) < 0.99) {
    vx = {-v[1], v[0], 0};
  } else {
    vx = {0, -v[2], v[1]};
  }
  Normalize(vx);
  Cross(v, vx, wx);
  Normalize(wx);

  // Calculate new direction
  for (int i = 0; i < 3; ++i) {
    newv[i] = sintheta * cosphi * vx[i] + sintheta * sinphi * wx[i] + costheta * v[i];
  }
  Normalize(newv);

  // Define incident plane and polarization direction
  Direction s_dir, p_dir;
  Cross(newv, v, s_dir); // Perpendicular to incident plane (s-polarization)
  Normalize(s_dir);
  Cross(s_dir, v, p_dir); // In incident plane (p-polarization)
  Normalize(p_dir);

  // Decompose incident poralization vector into s/p
  double s_amp = Dot(p, s_dir);
  double p_amp = Dot(p, p_dir);

  // theta dependent model
  double theta = acos(Dot(v, newv)); // Scattering angle
  double S_s = 1.0;                  // s-polarization constant
  double S_p = cos(theta);           // p-polarization constant

  // Polarizaiton direction acter scattering
  Direction new_s_dir;
  Cross(newv, v, new_s_dir);
  Normalize(new_s_dir);

  Direction new_p_dir;
  Cross(new_s_dir, newv, new_p_dir);
  Normalize(new_p_dir);

  // Define new polarizaiton vector
  for (int i = 0; i < 3; ++i) {
    newp[i] = S_s * s_amp * new_s_dir[i] + S_p * p_amp * new_p_dir[i];
  }
  Normalize(newp);
}
void Simulator::check_defined(const std::string& input){
  const std::string candidates[6] = {
     "organic", "NaI", "CsI", "BGO", "LYSO", "LaBr3"
  };
  
  for (int i = 0; i < 6; ++i) {
    if (input == candidates[i]) {
      return;
    }
  }
  std::cerr<<"Error: scintillator type must be selected from organic, NaI, CsI, BGO, LYSO or LaBr3."<<std::endl;
  exit(1);
}
