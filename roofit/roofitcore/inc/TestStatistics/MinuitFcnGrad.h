/*
 * Project: RooFit
 * Authors:
 *   PB, Patrick Bos, Netherlands eScience Center, p.bos@esciencecenter.nl
 *
 * Copyright (c) 2016-2020, Netherlands eScience Center
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 */
#ifndef ROOT_ROOFIT_TESTSTATISTICS_MinuitFcnGrad
#define ROOT_ROOFIT_TESTSTATISTICS_MinuitFcnGrad

#include <Fit/ParameterSettings.h>
#include "ROOT/RMakeUnique.hxx"
#include "Math/IFunction.h"  // ROOT::Math::IMultiGradFunction
#include "RooArgList.h"
#include "RooRealVar.h"
#include "TestStatistics/LikelihoodWrapper.h"
#include "TestStatistics/LikelihoodGradientWrapper.h"
#include "TestStatistics/LikelihoodJob.h"
#include "TestStatistics/LikelihoodGradientJob.h"

// forward declaration
class RooAbsL;
class RooAbsReal;

namespace RooFit {
namespace TestStatistics {

class MinuitFcnGrad : public ROOT::Math::IMultiGradFunction {
public:
   template <typename LWrapper = LikelihoodJob, typename LGWrapper = LikelihoodGradientJob>
   explicit MinuitFcnGrad(RooAbsL * _likelihood) : likelihood(std::make_unique<LWrapper>(_likelihood)), gradient(std::make_unique<LGWrapper>(_likelihood)) {}
   MinuitFcnGrad(const MinuitFcnGrad &other);

   ROOT::Math::IMultiGradFunction *Clone() const override;

   ~MinuitFcnGrad();

   // inform Minuit through its parameter_settings vector of RooFit parameter properties
   Bool_t synchronize_parameter_settings(std::vector<ROOT::Fit::ParameterSettings> &parameter_settings,
                                         Bool_t optConst = kTRUE, Bool_t verbose = kFALSE);

   // let gradient calculator know of synced settings
   void synchronize_gradient_parameter_settings(std::vector<ROOT::Fit::ParameterSettings>& parameter_settings) const;

   // used inside Minuit:
   bool returnsInMinuit2ParameterSpace() const override;

   // used for export to RooFitResult from Minimizer:
   RooArgList *GetFloatParamList();
   RooArgList *GetConstParamList();
   RooArgList *GetInitFloatParamList();
   RooArgList *GetInitConstParamList();
   Int_t GetNumInvalidNLL();

   // need access from Minimizer:
   void SetEvalErrorWall(Bool_t flag);
   void SetPrintEvalErrors(Int_t numEvalErrors);
   Double_t &GetMaxFCN();
   Int_t evalCounter() const;
   void zeroEvalCount();
   void SetVerbose(Bool_t flag = kTRUE);


   // TODO: for what is this used?
   void updateFloatVec();

private:
   // used in BackProp (Minuit results -> RooFit) and ApplyCovarianceMatrix
   void SetPdfParamErr(Int_t index, Double_t value);
   void ClearPdfParamAsymErr(Int_t index);
   void SetPdfParamErr(Int_t index, Double_t loVal, Double_t hiVal);
   inline Bool_t SetPdfParamVal(const Int_t &index, const Double_t &value) const {
      RooRealVar* par = (RooRealVar*)_floatParamVec[index];

      if (par->getVal()!=value) {
         if (_verbose) std::cout << par->GetName() << "=" << value << ", " ;

         par->setVal(value);
         return kTRUE;
      }

      return kFALSE;
   }

   // IMultiGradFunction overrides necessary for Minuit: DoEval, Gradient, (has)G2ndDerivative and (has)GStepSize
   double DoEval(const double *x) const override;
public:
   void Gradient(const double *x, double *grad) const override;
   void G2ndDerivative(const double *x, double *g2) const override;
   void GStepSize(const double *x, double *gstep) const override;
   bool hasG2ndDerivative() const override;
   bool hasGStepSize() const override;

   // part of IMultiGradFunction interface, used widely both in Minuit and in RooFit:
   unsigned int NDim() const override;

   // miscellaneous
   // TODO: hier getters, zoals bijv die covariance matrix, tenzij dat bij sync hoort, mss bij RooFitResult maken...


private:
   RooAbsReal *_funct;
   std::unique_ptr<LikelihoodWrapper> likelihood;
   std::unique_ptr<LikelihoodGradientWrapper> gradient;
   // the following four are mutable because DoEval is const
   mutable Int_t _evalCounter = 0;
   // Reset the *largest* negative log-likelihood value we have seen so far:
   mutable double _maxFCN = -1e30;
   mutable int _numBadNLL = 0;
   mutable int _printEvalErrors = 10;

   Bool_t _doEvalErrorWall = kTRUE;
   unsigned int _nDim = 0;

   RooArgList *_floatParamList;
   std::vector<RooAbsArg *> _floatParamVec;
   RooArgList *_constParamList;
   RooArgList *_initFloatParamList;
   RooArgList *_initConstParamList;

   bool _verbose;
};

}
} // namespace RooFit

#endif // ROOT_ROOFIT_TESTSTATISTICS_MinuitFcnGrad
