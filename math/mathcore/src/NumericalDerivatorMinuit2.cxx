// @(#)root/mathcore:$Id$
// Authors: L. Moneta, J.T. Offermann    08/2013 
//          E.G.P. Bos    09/2017
/**********************************************************************
 *                                                                    *
 * Copyright (c) 2013 , LCG ROOT MathLib Team                         *
 * Copyright (c) 2017 , Netherlands eScience Center                   *
 *                                                                    *
 **********************************************************************/
/*
 * NumericalDerivatorMinuit2.cxx
 *
 *  Original version (NumericalDerivator) created on: Aug 14, 2013
 *      Authors: L. Moneta, J. T. Offermann
 *  Modified version (NumericalDerivatorMinuit2) created on: Sep 27, 2017
 *      Author: E. G. P. Bos
 *
 *      NumericalDerivator was essentially a slightly modified copy of code
 *      written by M. Winkler, F. James, L. Moneta, and A. Zsenei for Minuit2,
 *      Copyright (c) 2005 LCG ROOT Math team, CERN/PH-SFT.
 *
 *      This class attempts to more closely follow the Minuit2 implementation.
 *      Remaining differences are clearly indicated by a DIFFERS comment.
 *      Modified things (w.r.t. NumericalDerivator) are indicated by MODIFIED.
 */

#include "Math/NumericalDerivatorMinuit2.h"
#include <cmath>
#include <algorithm>
#include <Math/IFunction.h>
#include <iostream>
#include <TMath.h>
#include <cassert>
// MODIFIED: Fitter.h
#include <Fit/Fitter.h>



namespace ROOT {
namespace Math {

NumericalDerivatorMinuit2::NumericalDerivatorMinuit2() :
   fFunction(0), 
   fStepTolerance(0.5), 
   fGradTolerance(0.1),
   fNCycles(2), 
   fVal(0), 
   fN(0),
   Up(1)
{}


NumericalDerivatorMinuit2::NumericalDerivatorMinuit2(const ROOT::Math::IBaseFunctionMultiDim &f, double step_tolerance, double grad_tolerance, unsigned int ncycles, double error_level):
    fFunction(&f),
    fStepTolerance(step_tolerance),
    fGradTolerance(grad_tolerance),
    fNCycles(ncycles),
    Up(error_level)
{
  // constructor with function, and tolerances (coordinates must be specified for differentiate function, not constructor)
//    fStepTolerance=step_tolerance;
//    fGradTolerance=grad_tolerance;
//    fFunction=&f;
    
    fN = f.NDim(); //number of dimensions, will look at vector size
    fGrd.resize(fN);
    fGstep.resize(fN);
    fG2.resize(fN);

    for (unsigned int i = 0; i<fN; i++) {
        fGrd[i]=0.1;
        fG2[i]=0.1;
        fGstep[i]=0.001;
    }
    fVal = 0;
}


// MODIFIED: ctor with higher level arguments
// The parameters can be extracted from a ROOT::Fit::Fitter object, for
// simpler initialization.
NumericalDerivatorMinuit2::NumericalDerivatorMinuit2(const ROOT::Math::IBaseFunctionMultiDim &f, const ROOT::Fit::Fitter &fitter) :
    strategy(fitter.Strategy()),
    NumericalDerivatorMinuit2::NumericalDerivatorMinuit2(f,
                                                         strategy.GradientStepTolerance(),
                                                         strategy.GradientTolerance(),
                                                         strategy.GradientNCycles(),
                                                         fitter.GetMinimizer()->ErrorDef()
                                                         )
{}


void NumericalDerivatorMinuit2::SetStepTolerance(double value) {
    fStepTolerance = value;
}

void NumericalDerivatorMinuit2::SetGradTolerance(double value) {
    fGradTolerance = value;
}

void NumericalDerivatorMinuit2::SetNCycles(int value) {
    fNCycles = value;
}

NumericalDerivatorMinuit2::~NumericalDerivatorMinuit2() {
  // TODO Auto-generated destructor stub
}

void NumericalDerivatorMinuit2::SetInitialValues(const double* g, const double* g2, const double* s) {
    for (unsigned int i = 0; i<fN; i++) {
        fGrd[i]=g[i];
        fG2[i]=g2[i];
        fGstep[i]=s[i];
    }
}

const double* NumericalDerivatorMinuit2::Differentiate(const double* cx) {
    // std::cout <<"Start:" << std::endl;
    // for (unsigned int i = 0; i<fN; i++) {
    //     std::cout << "fGrd[" << i <<"] = " << fGrd[i] << std::endl;
    //     //std::cout << "fG2[" << i <<"] = " << fG2[i] << std::endl;
    //     std::cout << "fGstep[" << i <<"] = " << fGstep[i] << std::endl;
    //  }
    assert(fFunction != 0);
    std::vector<double> vx(fFunction->NDim());
    assert (vx.size() > 0);
    double *x = &vx[0];
    std::copy (cx, cx+fFunction->NDim(), x);
    double step_tolerance = fStepTolerance;
    double grad_tolerance = fGradTolerance;
    const ROOT::Math::IBaseFunctionMultiDim &f = *fFunction;
    fVal = f(x); //value of function at given points

    // DIFFERS: eps, eps2
    // Minuit2 determines machine precision itself in MnMachinePrecision.cxx
    double eps = std::numeric_limits<double>::epsilon();
    double eps2 = std::sqrt(eps);//1.e-8; //sqrt(epsilon)

    // MODIFIED: Up
    // In Minuit2, this depends on the type of function to minimize, e.g.
    // chi-squared or negative log likelihood. It is set in the RooMinimizer
    // ctor and can be set in the Derivator in the ctor as well using
    // _theFitter->GetMinimizer()->ErrorDef() in the initialization call.
    // const double Up = 1;

    double dfmin = double(8.*eps2*(std::abs(fVal)+Up)); //had to cast to double, otherwise "statement has no effect"
    double vrysml = 8.*eps*eps;
    unsigned int ncycle = fNCycles;

    for (int i = 0; i < int(fN); i++) {

       double xtf=x[i]; //current value of coordinate x(i) (looping on i)
       double epspri = eps2 + std::abs(double(fGrd[i]*eps2)); //had to cast to double because I am using std::abs instead of fabs
       double step_old = 0.;
       for (unsigned int j = 0; j < ncycle; ++ j) {
          
          double optstp = std::sqrt(dfmin/(std::abs(fG2[i])+epspri));
          double step = std::max(optstp, std::abs(0.1*fGstep[i])); //had to cast to double again

          // DIFFERS: in Minuit2 we have here the following condition:
          // if(Trafo().Parameter(Trafo().ExtOfInt(i)).HasLimits()) {
          //   if(step > 0.5) step = 0.5;
          // }
          // See the discussion above NumericalDerivatorMinuit2::SetInitialGradient
          // below on how to pass parameter information to this derivator.

          double stpmax = 10.*std::abs(fGstep[i]);
          if (step > stpmax) step = stpmax;
          
          double stpmin = std::max(vrysml, 8.*std::abs(eps2*x[i])); //8.*std::abs(double(eps2*x[i]))
          if (step < stpmin) step = stpmin;
          if (std::abs((step-step_old)/step) < step_tolerance) {
             //answer = fGrd[i];
             break;
          }
          fGstep[i] = step;
          step_old = step;
          // std::cout << "step = " << step << std::endl;
          x[i] = xtf + step;
          //std::cout << "x[" << i << "] = " << x[i] <<std::endl;
          double fs1 = f(x);
          //std::cout << "xtf + step = " << x[i] << ", fs1 = " << fs1 << std::endl;
          x[i] = xtf - step;
          double fs2 = f(x);
          //std::cout << "xtf - step = " << x[i] << ", fs2 = " << fs2 << std::endl;
          x[i] = xtf;
          
          
          double fGrd_old = fGrd[i];
          fGrd[i] = 0.5*(fs1-fs2)/step;
//            std::cout << "int i = " << i << std::endl;
//            std::cout << "fs1 = " << fs1 << std::endl;
//            std::cout << "fs2 = " << fs2 << std::endl;
//            std::cout << "fVal = " << fVal << std::endl;
//            std::cout << "step^2 = " << (step*step) << std::endl;
//            std::cout << std::endl;
          fG2[i] = (fs1 + fs2 -2.*fVal)/step/step;
          
          // MODIFIED:
          // The condition below had a closing parenthesis differently than
          // Minuit. Fixed in this version.
          // if (std::abs(fGrd_old-fGrd[i])/(std::abs(fGrd[i]+dfmin/step)) < grad_tolerance)
          if (std::abs(fGrd_old-fGrd[i])/(std::abs(fGrd[i])+dfmin/step) < grad_tolerance)
          {
             //answer = fGrd[i];
             break;
          }
       }
    }
    
    // std::cout <<"End:" << std::endl;
    // for (unsigned int i = 0; i<fN; i++) {
    //     std::cout << "fGrd[" << i <<"] = " << fGrd[i] << std::endl;
    //     //std::cout << "fG2[" << i <<"] = " << fG2[i] << std::endl;
    //     std::cout << "fGstep[" << i <<"] = " << fGstep[i] << std::endl;
    // }
    
    return &fGrd[0];
}


// MODIFIED:
// This function was not implemented as in Minuit2. Now it copies the behavior
// of InitialGradientCalculator.
// 
// In the ROOT minimizer, in Math/Minimizer.h, SetVariables shows that different
// calls are used to set different types of parameters: bound vs fixed vs
// "regular" parameters. Something like that should be used here as well.
// SetVariables is fed a std::vector of ROOT::Fit::ParameterSettings objects,
// see Fit/FitConfig.h. These are stored in fSettings in a FitConfig object.
// In RooMinimizerFcn, this kind of information is normally synchronized with
// Minuit before each minimization call through RooMinimizerFcn::Synchronize.
// To use this kind of information in this derivator, it also has to be
// synchronized to here.
void NumericalDerivatorMinuit2::SetInitialGradient() {
   // set an initial gradient using some given steps 
   // (used in the first iteration)
 
  for (unsigned int i = 0; i < fN; ++i)  {
    //   //double eps2 = TMath::Sqrt(fEpsilon);
    //   //double gsmin = 8.*eps2*(fabs(x[i])) + eps2;
    // double dirin = s[i];
    // double g2 = 2.0 /(dirin*dirin);
    // double gstep = 0.1*dirin;
    // double grd = g2*dirin;
    
    // fGrd[i] = grd;
    // fG2[i] = g2;
    // fGstep[i] = gstep;
    
    unsigned int exOfIn = Trafo().ExtOfInt(i);

    double var = par.Vec()(i);
    double werr = Trafo().Parameter(exOfIn).Error();
    double sav = Trafo().Int2ext(i, var);
    double sav2 = sav + werr;
    if(Trafo().Parameter(exOfIn).HasLimits()) {
       if(Trafo().Parameter(exOfIn).HasUpperLimit() &&
          sav2 > Trafo().Parameter(exOfIn).UpperLimit())
          sav2 = Trafo().Parameter(exOfIn).UpperLimit();
    }
    double var2 = Trafo().Ext2int(exOfIn, sav2);
    double vplu = var2 - var;
    sav2 = sav - werr;
    if(Trafo().Parameter(exOfIn).HasLimits()) {
       if(Trafo().Parameter(exOfIn).HasLowerLimit() &&
          sav2 < Trafo().Parameter(exOfIn).LowerLimit())
          sav2 = Trafo().Parameter(exOfIn).LowerLimit();
    }
    var2 = Trafo().Ext2int(exOfIn, sav2);
    double vmin = var2 - var;
    double gsmin = 8.*Precision().Eps2()*(fabs(var) + Precision().Eps2());
    // protect against very small step sizes which can cause dirin to zero and then nan values in grd
    double dirin = std::max(0.5*(fabs(vplu) + fabs(vmin)),  gsmin );
    double g2 = 2.0*fFcn.ErrorDef()/(dirin*dirin);
    double gstep = std::max(gsmin, 0.1*dirin);
    double grd = g2*dirin;
    if(Trafo().Parameter(exOfIn).HasLimits()) {
       if(gstep > 0.5) gstep = 0.5;
    }
    gr(i) = grd;
    gr2(i) = g2;
    gst(i) = gstep;

  }
}


} // namespace Math
} // namespace ROOT


