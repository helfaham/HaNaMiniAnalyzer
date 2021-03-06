#include "RooWorkspace.h"
#include "RooAbsPdf.h"
#include "RooDataSet.h"
#include "RooFitResult.h"
#include "RooPlot.h"
#include "RooRealVar.h"
#include "RooRandom.h"
#include "RooStats/ModelConfig.h"

#include<cmath>

using namespace RooFit; 

double getStatUnc(std::vector<double>& vec)
{
  // Unweighted events
  return(1./sqrt(vec.size()));
}

void deltarr()
{
   RooWorkspace w("w"); 
   w.factory("x[0,4]");  

   w.factory("nbackground[195, 0, 1950]"); 
   w.factory("nsignal[18, 0.0, 180]");

   w.factory("mean [1]");
   w.factory("sigma[0.6]");// mean + 5 sigma=4 // 
   w.factory("Gaussian::bmodel(x, mean, sigma)");
   w.factory("Gaussian::smodel(x, mean, sigma)");

   w.factory("SUM::model(nbackground*bmodel, nsignal*smodel)");
   RooAbsPdf *pdf  = w.pdf("model");

   RooRealVar  x("x","x",0,4) ;

   RooRandom::randomGenerator()->SetSeed(0);

   bool aprioriBinning(false);
   if(aprioriBinning)
     {
       RooBinning b(0,4);
       b.addUniform(5,0,2.2);// 95% events in 2 sigma, for 10% --> stat. is constrained to 15% if number of bins/ number of events = 0.15^2//
       b.addUniform(1,2.2,4);// 5% events at the tail of dist. --> stat. is constrained to 25% if --//-- = 0.25^2 //
       
       x.setBinning(b);
       RooDataSet * data   = pdf->generate(RooArgSet(x),AllBinned());
       RooDataSet * datac  = (RooDataSet*) data ->reduce(Cut("x > 0.4"));
       
       datac->SetName("datac");
       w.import(*datac);
       datac->Print();
       RooPlot * plot = x.frame(Title("x=min_Delta_R in 3ll"));
       datac->plotOn(plot,Binning(b)) ;
       //   pdf->plotOn(plot);
       cout << ">> number of bins: " << datac->numEntries() << endl ;
       for(Int_t i=0 ;i< datac->numEntries();i++)
	 {	
	   datac->get(i);
	   Double_t c = datac->weight(); 
	   Double_t d = sqrt(c); 
	   Double_t e = d/c; 
	   cout << "n("<<i<<"):"  << c << "-->"<< d << "-->" << e*100 <<"%"<< endl;
	 }
       plot->Draw();
     }
   else
     {
       // Let's first generate events according to the desired PDF, without forcing any binning
       RooDataSet* data  = pdf->generate(RooArgSet(x));
       RooDataSet* datac = (RooDataSet*) data->reduce(Cut("x > 0.4"));
       // Now let's build the binning from scratch, looping from the left on the dataset
       // We could as well loop from the right, it should not really matter at this point
       // It's only a matter of which side will have the last bin as over-populated. We prefer to have it in the right tail.
       cout << "Dataset entries: " << datac->numEntries() << endl;
       vector<double> allthedata; // Store all the data for sorting
       for(int i=0; i<datac->numEntries(); ++i)
	 {
	   const RooArgSet* set = datac->get(i);
	   RooRealVar* var = (RooRealVar*)set->find(x.GetName());
	   double deltar = var->getVal();
	   allthedata.push_back(deltar);
	 }
       if(allthedata.size() != datac->numEntries())
         cout << "There is a huge issue: we imported " << allthedata.size() << " events out of " << datac->numEntries() << " importable events! Please check." << endl;
       std::sort(allthedata.begin(), allthedata.end(), [](const double a, const double b) {return a > b; });
       
       // OK now start with the binning
       RooBinning b(0,4);
       double binBoundary(x.getBinning().highBound()); // This works even in the unbinned case, it kind of assumes the only bis the full range of the variable
       vector<double> binContent; // Store the current bin content
       for(auto& datum : allthedata)
         {
           binContent.push_back(datum); // Add current event to the current bin
           bool statuncOK(getStatUnc(binContent)<0.13);
           if(statuncOK) // If the current bin stat uncertainty is OK,
             {
               b.addBoundary(datum); // Store bin boundary
               cout << "FOUND BIN BOUNDARY AT " << datum << " for statunc of " << getStatUnc(binContent) << endl;
               binContent.clear(); // Start another bin
             }
         }
       b.Print();
       x.setBinning(b);
       w.import(*datac);
       datac->Print();
       RooPlot * plot = x.frame(Title("x=min_Delta_R in 3ll"));
       datac->plotOn(plot,Binning(b)) ;
       //   pdf->plotOn(plot);
        plot->Draw();

     }
}


void non_var_bins(TString var="deltar"){
  if(var=="deltar")
    deltarr();
  else
    cout << "This variable is not implemented yet. Have a nice day" << endl;

}
