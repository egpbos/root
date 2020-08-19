/*****************************************************************************
 * Project: RooFit                                                           *
 * Package: RooFitCore                                                       *
 *    File: $Id: RooBinnedL.h,v 1.10 2007/07/21 21:32:52 wouter Exp $
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

#ifndef ROOT_ROOFIT_TESTSTATISTICS_RooBinnedL
#define ROOT_ROOFIT_TESTSTATISTICS_RooBinnedL

#include "RooAbsReal.h"
#include <vector>

#include <TestStatistics/RooAbsL.h>

// forward declarations
class RooAbsPdf;
class RooAbsData;

namespace RooFit {
namespace TestStatistics {

class RooBinnedL :
   public RooAbsL {
public:
   RooBinnedL(RooAbsPdf* pdf, RooAbsData* data, bool do_offset, double offset, double offset_carry);
private:
   double evaluate_partition(std::size_t events_begin, std::size_t events_end, std::size_t components_begin,
                             std::size_t components_end) override;
   mutable bool _first = true;       //!
   mutable std::vector<double> _binw; //!
   double get_carry() const override;
   mutable double _evalCarry = 0;   //! carry of Kahan sum in evaluatePartitio
};

} // namespace TestStatistics
} // namespace RooFit

#endif // ROOT_ROOFIT_TESTSTATISTICS_RooBinnedL
