#include <statistics/GaussianHMM.h>
#include <math/matrix.h>
#include <math/indexing.h>
#include <time.h>
#include <fstream>
#include <sstream>
using namespace std;
using namespace Statistics;

bool ReadCSV(const char* fn,vector<vector<string> >& array,char delim=',')
{
  array.resize(0);
  string line;
  int pos;
 
  ifstream in(fn,ios::in);
  if(!in) { return false;   }
  while( getline(in,line) ) {
    vector<string> ln;
    while( (pos = line.find(delim)) >= 0)
      {
	string field = line.substr(0,pos);
	line = line.substr(pos+1);
	ln.push_back(field);
      }
    ln.push_back(line.substr(0,line.length()-1));
    array.push_back(ln);
  }
  return true;
}

bool ReadGHMM(GaussianHMM& ghmm,const char* fn)
{
  //input GMM
  ifstream in(fn,ios::in);
  in>>ghmm;
  if(!in) return false;
  in.close();
  return true;
}

int RunGHMMRTest(const char* ghmmfn,const char* data,const char* trialLabel,const vector<string>& dependentVariables)
{
  GaussianHMM ghmm;
  if(!ReadGHMM(ghmm,ghmmfn)) {
    printf("Error reading ghmm file %s\n",ghmmfn);
    return 1;
  }
  vector<vector<string> > entries;
  if(!ReadCSV(data,entries)) {
    printf("Error reading CSV file %s\n",data);
    return 1;
  }
  if(entries.empty()) {
    printf("CSV file %s is empty\n",data);
    return 1;
  }
  printf("Done reading %s\n",data);
  vector<string> labels = entries[0];
  vector<Vector> examples(entries.size()-1);
  for(size_t i=1;i<entries.size();i++) {
    examples[i-1].resize(entries[i].size());
    for(size_t j=0;j<entries[i].size();j++) {
      stringstream ss(entries[i][j]);
      ss >> examples[i-1][j];
      if(ss.bad()) {
	printf("Error: non numeric value on line %d, column %d\n",i,j);
	return 1;
      }
    }
  }
  if((int)labels.size() != ghmm.emissionModels[0].mu.n+1) {
    printf("Data does not match the size of the GHMM\n");
    return 1;
  }

  //TODO: select the trial index dynamically using the trialLabel name
  int trialIndex = 0;
  vector<vector<Vector> > trials;
  int curTrial=-1;
  for(size_t i=0;i<examples.size();i++) {
    if(examples[i][trialIndex] != curTrial) {
      curTrial = examples[i][trialIndex];
      trials.resize(trials.size()+1);
    }
    trials.back().push_back(examples[i]);
  }

  //now take out the trial variable from examples, trials, and labels
  vector<int> trialIndices(1,trialIndex);
  for(size_t i=0;i<examples.size();i++)
    RemoveElements(examples[i],trialIndices);
  for(size_t i=0;i<trials.size();i++)
    for(size_t j=0;j<trials[i].size();j++)
      RemoveElements(trials[i][j],trialIndices);
  labels.erase(labels.begin()+trialIndex);

  //parse out dependent/independent variables
  vector<int> depmap(labels.size(),-1);
  for(size_t i=0;i<dependentVariables.size();i++) {
    bool found=false;
    for(size_t j=0;j<labels.size();j++) {
      if(labels[j] == dependentVariables[i]) {
	//duplicate dependent variables?
	assert(depmap[j] < 0);
	depmap[j] = (int)i;
	found = true;
	break;
      }
    }
    if(!found) {
      fprintf(stderr,"Dependent variable %s not found in data\n",dependentVariables[i].c_str());
      return 1;
    }
  }
  vector<int> depindices,indepindices;
  for(size_t i=0;i<depmap.size();i++){
    if(depmap[i] < 0)
      indepindices.push_back(i);
    else
      depindices.push_back(i);
  }

  //scale/translate
  /*
  Vector emin=examples[0],emax=examples[0];
  Vector mean=examples[0],stddev(emin.n,Zero);
  for(size_t i=1;i<examples.size();i++) {
    for(int j=0;j<emin.n;j++)
      if(examples[i](j) < emin(j)) emin(j)=examples[i](j);
      else if(examples[i](j) > emax(j)) emax(j)=examples[i](j);
    mean += examples[i];
  }
  mean /= examples.size();
  for(size_t i=0;i<examples.size();i++) {
    for(int j=0;j<emin.n;j++)
      stddev(j) += Sqr(examples[i](j) - mean(j));
  }
  stddev /= examples.size();
  for(int j=0;j<emin.n;j++)
    stddev(j) = Sqrt(stddev(j));
  */
  /*
  printf("Ranges:\n");
  for(size_t i=0;i<labels.size();i++)
    cout<<"  "<<labels[i]<<": "<<emax(i)-emin(i)<<endl;
  printf("Midpoints:\n");
  for(size_t i=0;i<labels.size();i++)
    cout<<"  "<<labels[i]<<": "<<(emax(i)+emin(i))*0.5<<endl;
  printf("Stddevs:\n");
  for(size_t i=0;i<labels.size();i++)
    cout<<"  "<<labels[i]<<": "<<stddev(i)<<endl;
  printf("Means:\n");
  for(size_t i=0;i<labels.size();i++)
    cout<<"  "<<labels[i]<<": "<<mean(i)<<endl;
  */
  /*
  //scale to range [-1,1]
  for(size_t i=0;i<examples.size();i++) {
    examples[i] -= mean;
    for(int j=0;j<emin.n;j++)
      examples[i](j) /= stddev(j);
  }
  */

  Vector ysqerr(depindices.size(),Zero),yavgerr(depindices.size(),Zero);
  Real ll=Zero;
  GaussianHMMRegression ghmmr(ghmm);
  ghmmr.SetXIndices(indepindices);
  assert(depindices == ghmmr.yindices);
  assert(indepindices == ghmmr.xindices);
  GaussianMixtureModelRaw ygmm;
  Vector xsub(indepindices.size()),ysub(depindices.size());
  Vector temp;
  int forecastLength = 40;
  Vector ysqerr_forecast(depindices.size(),Zero);
  int numforecast = 0;
  printf("Writing predictions to ghmmr_predict.csv\n");
  ofstream out("ghmmr_predict.csv");
  out<<trialLabel;
  for(size_t i=0;i<depindices.size();i++)
    out<<","<<labels[depindices[i]];
  for(size_t i=0;i<depindices.size();i++)
    out<<","<<labels[depindices[i]]<<" var";
  for(size_t i=0;i<depindices.size();i++)
    out<<","<<labels[depindices[i]]<<" pred "<<forecastLength;
  for(size_t i=0;i<depindices.size();i++)
    out<<","<<labels[depindices[i]]<<" pred "<<forecastLength<<" var";
  out<<endl;
  for(size_t i=0;i<trials.size();i++) {
    printf("Trial %d / %d\n",i,trials.size());
    Vector pDiscrete = ghmm.discretePrior;
    for(size_t j=0;j<trials[i].size();j++) {
      GetElements(trials[i][j],indepindices,xsub);
      GetElements(trials[i][j],depindices,ysub);

      //predict
      ghmmr.joint.Predict(pDiscrete,temp);
      ghmmr.GetY(temp,xsub,ygmm);
      //advance
      ghmmr.Filter(pDiscrete,xsub,temp);
      pDiscrete = temp;

      //evaluate error
      //ygmm.Get(ygmm_temp);
      //ll += Log(ygmm_temp.Probability(ysub));
      Vector ymean;
      Matrix ycov;
      ygmm.GetMean(ymean);
      ygmm.GetCovariance(ycov);
      for(int k=0;k<ysqerr.n;k++) {
	ysqerr(k) += Sqr(ymean(k)-ysub(k));
	yavgerr(k) += ysub(k)-ymean(k);
      }
      Assert(ymean.n == (int)depindices.size());

      //write to disk
      out<<i;
      for(int k=0;k<ymean.n;k++) out<<","<<ymean(k);
      for(int k=0;k<ymean.n;k++) out<<","<<ycov(k,k);

      //forecast error
      if(j+forecastLength < (int)trials[i].size()) {
	GetElements(trials[i][j+forecastLength],depindices,ysub);

	ghmmr.joint.Predict(pDiscrete,temp,1+forecastLength);
	ghmmr.GetYUnconditional(temp,ygmm);
	ygmm.GetMean(ymean);
	ygmm.GetCovariance(ycov);
	for(int k=0;k<ysqerr.n;k++) {
	  ysqerr_forecast(k) += Sqr(ymean(k)-ysub(k));
	}
	numforecast++;

	//write to disk
	for(int k=0;k<ymean.n;k++) out<<","<<ymean(k);
	for(int k=0;k<ymean.n;k++) out<<","<<ycov(k,k);
      }
      out<<endl;
    }
  }
  out.close();
  /*
  for(int j=0;j<ysqerr.n;j++) {
    ysqerr(j) *= Sqr(stddev(j));
    yavgerr(j) *= stddev(j);
  }
  */
  ysqerr /= examples.size();
  yavgerr /= examples.size();
  ysqerr_forecast /= numforecast;

  cout<<"Standard errors:"<<endl;
  for(size_t i=0;i<depindices.size();i++)
    cout<<"  "<<labels[depindices[i]]<<": "<<Sqrt(ysqerr(i))<<endl;
  cout<<"Average errors:"<<endl;
  for(size_t i=0;i<depindices.size();i++)
    cout<<"  "<<labels[depindices[i]]<<": "<<yavgerr(i)<<endl;
  cout<<"Average forecast squared errors:"<<endl;
  for(size_t i=0;i<depindices.size();i++)
    cout<<"  "<<labels[depindices[i]]<<": "<<ysqerr_forecast(i)<<endl;
  cout<<"Log likelihood: "<<ll<<endl;
}

int main(int argc,const char** argv)
{
  if(argc < 5) {
    printf("Usage: %s ghmm data.csv trialLabel dep1 dep2 ...\n",argv[0]);
    return 0;
  }
  const char* gmmfn = argv[1];
  const char* datafn = argv[2];
  const char* trialLabel = argv[3];
  vector<string> deps;
  for(int i=4;i<argc;i++)
    deps.push_back(argv[i]);

  return RunGHMMRTest(gmmfn,datafn,trialLabel,deps);
}
