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
#include <TestStatistics/RooAbsL.h>
#include "RooAbsPdf.h"

namespace RooFit {
namespace TestStatistics {

RooArgSet *RooAbsL::getParameters()
{
   return pdf->getParameters(*data);
}

void RooAbsL::constOptimizeTestStatistic(RooAbsArg::ConstOpCode opcode) {
   throw std::logic_error("RooAbsL::constOptimizeTestStatistic is not yet implemented.");
}

} // namespace TestStatistics
} // namespace RooFit
