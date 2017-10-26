// @(#)root/minuit2:$Id$
// Authors: M. Winkler, F. James, L. Moneta, A. Zsenei   2003-2005

/**********************************************************************
 *                                                                    *
 * Copyright (c) 2005 LCG ROOT Math team,  CERN/PH-SFT                *
 *                                                                    *
 **********************************************************************/

#include "Minuit2/Numerical2PGradientCalculator.h"
#include "Minuit2/InitialGradientCalculator.h"
#include "Minuit2/MnFcn.h"
#include "Minuit2/MnUserTransformation.h"
#include "Minuit2/MnMachinePrecision.h"
#include "Minuit2/MinimumParameters.h"
#include "Minuit2/FunctionGradient.h"
#include "Minuit2/MnStrategy.h"


//#define DEBUG
#if defined(DEBUG) || defined(WARNINGMSG)
#include "Minuit2/MnPrint.h"
#ifdef _OPENMP
#include <omp.h>
#include <iomanip>
#ifdef DEBUG
#define DEBUG_MP
#endif
#endif
#endif

#include <math.h>

#include "Minuit2/MPIProcess.h"

namespace ROOT {

   namespace Minuit2 {


FunctionGradient Numerical2PGradientCalculator::operator()(const MinimumParameters& par) const {
   // calculate gradient using Initial gradient calculator and from MinimumParameters object

   InitialGradientCalculator gc(fFcn, fTransformation, fStrategy);
   FunctionGradient gra = gc(par);

   return (*this)(par, gra);
}


// comment it, because it was added
FunctionGradient Numerical2PGradientCalculator::operator()(const std::vector<double>& params) const {
   // calculate gradient from an std;:vector of paramteters

   int npar = params.size();

   MnAlgebraicVector par(npar);
   for (int i = 0; i < npar; ++i) {
      par(i) = params[i];
   }

   double fval = Fcn()(par);

   MinimumParameters minpars = MinimumParameters(par, fval);

   return (*this)(minpars);

}



FunctionGradient Numerical2PGradientCalculator::operator()(const MinimumParameters& par, const FunctionGradient& Gradient) const {
   // calculate numerical gradient from MinimumParameters object
   // the algorithm takes correctly care when the gradient is approximatly zero

   std::cout<<"\n\n########### Numerical2PDerivative : START"<<std::endl;
   std::cout<<"grd: "<<Gradient.Grad()<<"\t";
   std::cout<<"G2: "<<Gradient.G2()<<"\t";
   std::cout<<"Gstep: "<<Gradient.Gstep()<<"\t";
   std::cout<<"position: "<<par.Vec()<< std::endl<< std::endl;

   assert(par.IsValid());


   double fcnmin = par.Fval();
   //   std::cout<<"fval: "<<fcnmin<<std::endl;

   double eps2 = Precision().Eps2();
   double eps = Precision().Eps();

   double dfmin = 8.*eps2*(fabs(fcnmin)+Fcn().Up());
   double vrysml = 8.*eps*eps;
   //   double vrysml = std::max(1.e-4, eps2);
   //    std::cout<<"dfmin= "<<dfmin<<std::endl;
   //    std::cout<<"vrysml= "<<vrysml<<std::endl;
   //    std::cout << " ncycle " << Ncycle() << std::endl;

   unsigned int n = (par.Vec()).size();
   unsigned int ncycle = Ncycle();
   //   MnAlgebraicVector vgrd(n), vgrd2(n), vgstp(n);
   MnAlgebraicVector grd = Gradient.Grad();
   MnAlgebraicVector g2 = Gradient.G2();
   MnAlgebraicVector gstep = Gradient.Gstep();

#ifndef _OPENMP
   MPIProcess mpiproc(n,0);
#endif

#ifdef DEBUG
   std::cout << "Calculating Gradient at x =   " << par.Vec() << std::endl;
   int pr = std::cout.precision(13);
   std::cout << "fcn(x) = " << fcnmin << std::endl;
   std::cout.precision(pr);
#endif

#ifndef _OPENMP
   // for serial execution this can be outside the loop
   MnAlgebraicVector x = par.Vec();

   unsigned int startElementIndex = mpiproc.StartElementIndex();
   unsigned int endElementIndex = mpiproc.EndElementIndex();

   for(unsigned int i = startElementIndex; i < endElementIndex; i++) {

#else

 // parallelize this loop using OpenMP
//#define N_PARALLEL_PAR 5
#pragma omp parallel
#pragma omp for
//#pragma omp for schedule (static, N_PARALLEL_PAR)

   for(int i = 0; i < int(n); i++) {

#endif

#ifdef DEBUG_MP
      int ith = omp_get_thread_num();
      //std::cout << "Thread number " << ith << "  " << i << std::endl;
#endif

#ifdef _OPENMP
       // create in loop since each thread will use its own copy
      MnAlgebraicVector x = par.Vec();
#endif

      double xtf = x(i);
      double epspri = eps2 + fabs(grd(i)*eps2);
      double stepb4 = 0.;
      for(unsigned int j = 0; j < ncycle; j++)  {
         double optstp = sqrt(dfmin/(fabs(g2(i))+epspri));
         double step = std::max(optstp, fabs(0.1*gstep(i)));
         //       std::cout<<"step: "<<step;
         if(Trafo().Parameter(Trafo().ExtOfInt(i)).HasLimits()) {
            if(step > 0.5) step = 0.5;
         }
         double stpmax = 10.*fabs(gstep(i));
         if(step > stpmax) step = stpmax;
         //       std::cout<<" "<<step;
         double stpmin = std::max(vrysml, 8.*fabs(eps2*x(i)));
         if(step < stpmin) step = stpmin;
         //       std::cout<<" "<<step<<std::endl;
         //       std::cout<<"step: "<<step<<std::endl;
         if(fabs((step-stepb4)/step) < StepTolerance()) {
            //    std::cout<<"(step-stepb4)/step"<<std::endl;
            //    std::cout<<"j= "<<j<<std::endl;
            //    std::cout<<"step= "<<step<<std::endl;
            break;
         }
         gstep(i) = step;
         stepb4 = step;
         //       MnAlgebraicVector pstep(n);
         //       pstep(i) = step;
         //       double fs1 = Fcn()(pstate + pstep);
         //       double fs2 = Fcn()(pstate - pstep);

         x(i) = xtf + step;
         double fs1 = Fcn()(x);
         x(i) = xtf - step;
         double fs2 = Fcn()(x);
         x(i) = xtf;

         double grdb4 = grd(i);
         grd(i) = 0.5*(fs1 - fs2)/step;
         g2(i) = (fs1 + fs2 - 2.*fcnmin)/step/step;

#ifdef DEBUG
         pr = std::cout.precision(13);
         std::cout << "cycle " << j << " x " << x(i) << " step " << step << " f1 " << fs1 << " f2 " << fs2
                   << " grd " << grd(i) << " g2 " << g2(i) << std::endl;
         std::cout.precision(pr);
#endif

         if(fabs(grdb4-grd(i))/(fabs(grd(i))+dfmin/step) < GradTolerance())  {
            //    std::cout<<"j= "<<j<<std::endl;
            //    std::cout<<"step= "<<step<<std::endl;
            //    std::cout<<"fs1, fs2: "<<fs1<<" "<<fs2<<std::endl;
            //    std::cout<<"fs1-fs2: "<<fs1-fs2<<std::endl;
            break;
         }
      }


#ifdef DEBUG_MP
#pragma omp critical
      {
         std::cout << "Gradient for thread " << ith << "  " << i << "  " << std::setprecision(15)  << grd(i) << "  " << g2(i) << std::endl;
      }
#endif

      //     vgrd(i) = grd;
      //     vgrd2(i) = g2;
      //     vgstp(i) = gstep;


#ifdef DEBUG
      pr = std::cout.precision(13);
      int iext = Trafo().ExtOfInt(i);
      std::cout << "Parameter " << Trafo().Name(iext) << " Gradient =   " << grd(i) << " g2 = " << g2(i) << " step " << gstep(i) << std::endl;
      std::cout.precision(pr);
#endif
   }

#ifndef _OPENMP
   mpiproc.SyncVector(grd);
   mpiproc.SyncVector(g2);
   mpiproc.SyncVector(gstep);
#endif

#ifdef DEBUG
   std::cout << "Calculated Gradient at x =   " << par.Vec() << std::endl;
   std::cout << "fcn(x) = " << fcnmin << std::endl;
   std::cout << "Computed gradient in N2PGC " << grd << std::endl;
#endif

   std::cout<<"\n\n########### Numerical2PDerivative : END"<<std::endl;
   std::cout<<"grd: "<<Gradient.Grad()<<"\t";
   std::cout<<"G2: "<<Gradient.G2()<<"\t";
   std::cout<<"Gstep: "<<Gradient.Gstep()<<"\t";
   std::cout<<"position: "<<par.Vec()<< std::endl<< std::endl;

   return FunctionGradient(grd, g2, gstep);
}

const MnMachinePrecision& Numerical2PGradientCalculator::Precision() const {
   // return global precision (set in transformation)
   return fTransformation.Precision();
}

unsigned int Numerical2PGradientCalculator::Ncycle() const {
   // return number of cycles for gradient calculation (set in strategy object)
   return Strategy().GradientNCycles();
}

double Numerical2PGradientCalculator::StepTolerance() const {
   // return gradient step tolerance (set in strategy object)
   return Strategy().GradientStepTolerance();
}

double Numerical2PGradientCalculator::GradTolerance() const {
   // return gradient tolerance (set in strategy object)
   return Strategy().GradientTolerance();
}


   }  // namespace Minuit2

}  // namespace ROOT
