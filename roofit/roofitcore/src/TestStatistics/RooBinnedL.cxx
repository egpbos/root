/*****************************************************************************
 * Project: RooFit                                                           *
 * Package: RooFitCore                                                       *
 * @(#)root/roofitcore:$Id$
 * Authors:                                                                  *
 *   WV, Wouter Verkerke, UC Santa Barbara, verkerke@slac.stanford.edu       *
 *   DK, David Kirkby,    UC Irvine,         dkirkby@uci.edu                 *
 *   PB, Patrick Bos,     NL eScience Center, p.bos@esciencecenter.nl        *
 *                                                                           *
 * Copyright (c) 2000-2020, Regents of the University of California          *
 *                          and Stanford University. All rights reserved.    *
 *                                                                           *
 * Redistribution and use in source and binary forms,                        *
 * with or without modification, are permitted according to the terms        *
 * listed in LICENSE (http://roofit.sourceforge.net/license.txt)             *
 *****************************************************************************/

/**
\file RooBinnedL.cxx
\class RooBinnedL
\ingroup Roofitcore

Class RooBinnedL implements a a -log(likelihood) calculation from a dataset
and a PDF. The NLL is calculated as
<pre>
 Sum[data] -log( pdf(x_data) )
</pre>
In extended mode, a (Nexpect - Nobserved*log(NExpected) term is added
**/

#include "TMath.h"

#include "RooAbsData.h"
#include "RooAbsPdf.h"
#include "RooAbsDataStore.h"
#include "RooRealSumPdf.h"
#include "RooRealVar.h"

#include <TestStatistics/RooBinnedL.h>

namespace RooFit {
namespace TestStatistics {

RooBinnedL::RooBinnedL(RooAbsPdf* pdf, RooAbsData* data, bool do_offset, double offset, double offset_carry) :
   RooAbsL(pdf, data, do_offset, offset, offset_carry, 1, data->numEntries())
{
   // pdf must be a RooRealSumPdf representing a yield vector for a binned likelihood calculation
   if (!dynamic_cast<RooRealSumPdf *>(pdf)) {
      throw std::logic_error("RooBinnedL can only be created from pdf of type RooRealSumPdf!");
   }

   // Retrieve and cache bin widths needed to convert unnormalized binned pdf values back to yields

   // The Active label will disable pdf integral calculations
   pdf->setAttribute("BinnedLikelihoodActive");

   RooArgSet *obs = pdf->getObservables(data);
   if (obs->getSize() != 1) {
      throw std::logic_error("RooBinnedL can only be created from combination of pdf and data which has exactly one observable!");
   } else {
      RooRealVar *var = (RooRealVar *)obs->first();
      std::list<Double_t> *boundaries = pdf->binBoundaries(*var, var->getMin(), var->getMax());
      std::list<Double_t>::iterator biter = boundaries->begin();
      _binw.resize(boundaries->size() - 1);
      Double_t lastBound = (*biter);
      ++biter;
      int ibin = 0;
      while (biter != boundaries->end()) {
         _binw[ibin] = (*biter) - lastBound;
         lastBound = (*biter);
         ibin++;
         ++biter;
      }
   }
}

//////////////////////////////////////////////////////////////////////////////////
///// Calculate and return likelihood on subset of data from firstEvent to lastEvent
///// processed with a step size of 'stepSize'. If this an extended likelihood and
///// and the zero event is processed the extended term is added to the return
///// likelihood.
//
double RooBinnedL::evaluate_partition(std::size_t /*events_begin*/, std::size_t /*events_end*/, std::size_t components_begin,
                                      std::size_t components_end)
{
   // Throughout the calculation, we use Kahan's algorithm for summing to
   // prevent loss of precision - this is a factor four more expensive than
   // straight addition, but since evaluating the PDF is usually much more
   // expensive than that, we tolerate the additional cost...
   Double_t result(0), carry(0);

   // cout << "RooNLLVar::evaluatePartition(" << GetName() << ") projDeps = " << (_projDeps?*_projDeps:RooArgSet()) <<
   // endl ;

//   data->store()->recalculateCache(_projDeps, firstEvent, lastEvent, stepSize, (_binnedPdf?kFALSE:kTRUE));
   // TODO: check when we might need _projDeps (it seems to be mostly empty); ties in with TODO below
   data->store()->recalculateCache(nullptr, components_begin, components_end, 1, kFALSE);

   Double_t sumWeight(0), sumWeightCarry(0);

   for (std::size_t i = components_begin; i < components_end; ++i) {

      data->get(i);

      if (!data->valid())
         continue;

      Double_t eventWeight = data->weight();

      // Calculate log(Poisson(N|mu) for this bin
      Double_t N = eventWeight;
      Double_t mu = pdf->getVal() * _binw[i];
      // cout << "RooNLLVar::binnedL(" << GetName() << ") N=" << N << " mu = " << mu << endl ;

      if (mu <= 0 && N > 0) {

         // Catch error condition: data present where zero events are predicted
//         logEvalError(Form("Observed %f events in bin %d with zero event yield", N, i));
         // TODO: check if using regular stream vs logEvalError error gathering is ok
         oocoutI(static_cast<RooAbsArg *>(nullptr), Minimization)
            << "Observed " << N << " events in bin " << i << " with zero event yield" << std::endl;

      } else if (fabs(mu) < 1e-10 && fabs(N) < 1e-10) {

         // Special handling of this case since log(Poisson(0,0)=0 but can't be calculated with usual log-formula
         // since log(mu)=0. No update of result is required since term=0.

      } else {

         Double_t term = -1 * (-mu + N * log(mu) - TMath::LnGamma(N + 1));

         // Kahan summation of sumWeight
         Double_t y = eventWeight - sumWeightCarry;
         Double_t t = sumWeight + y;
         sumWeightCarry = (t - sumWeight) - y;
         sumWeight = t;

         // Kahan summation of result
         y = term - carry;
         t = result + y;
         carry = (t - result) - y;
         result = t;
      }
   }



   // TODO: check if this can indeed be left out for (un)binned likelihoods
//   // If part of simultaneous PDF normalize probability over
//   // number of simultaneous PDFs: -sum(log(p/n)) = -sum(log(p)) + N*log(n)
//   if (_simCount > 1) {
//      Double_t y = sumWeight * log(1.0 * _simCount) - carry;
//      Double_t t = result + y;
//      carry = (t - result) - y;
//      result = t;
//   }

   // timer.Stop() ;
   // cout << "RooNLLVar::evalPart(" << GetName() << ") SET=" << _setNum << " first=" << firstEvent << ", last=" <<
   // lastEvent << ", step=" << stepSize << ") result = " << result << " CPU = " << timer.CpuTime() << endl ;

   // At the end of the first full calculation, wire the caches
   if (_first) {
      _first = false;
      pdf->wireAllCaches();
   }

   // Check if value offset flag is set.
   if (_do_offset) {

      // If no offset is stored enable this feature now
      if (_offset == 0 && result != 0) {
         oocoutI(static_cast<RooAbsArg *>(nullptr), Minimization)
            << "RooBinnedL::evaluate_partition(" << GetName() << ") first = " << components_begin
            << " last = " << components_end << " Likelihood offset now set to " << result << std::endl;
         _offset = result;
         _offset_carry = carry;
      }

      // Substract offset
      Double_t y = -_offset - (carry + _offset_carry);
      Double_t t = result + y;
      carry = (t - result) - y;
      result = t;
   }

   _evalCarry = carry;
   return result;
}

double RooBinnedL::get_carry() const
{
   return _evalCarry;
}

} // namespace TestStatistics
} // namespace RooFit
