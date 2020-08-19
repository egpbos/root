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
#include <TestStatistics/RooRealL.h>
#include <TestStatistics/RooAbsL.h>

ClassImp(RooFit::TestStatistics::RooRealL);

namespace RooFit {
namespace TestStatistics {

// TODO: initialize RooListProxy _vars

RooRealL::RooRealL(const char *name, const char *title, std::shared_ptr<RooAbsL> likelihood)
   : RooAbsReal(name, title), likelihood(std::move(likelihood))
{
}

RooRealL::RooRealL(const RooRealL &other, const char *name) : RooAbsReal(other, name), likelihood(other.likelihood) {}

double RooRealL::globalNormalization() const
{
   // Default value of global normalization factor is 1.0
   return 1.0;
}

double RooRealL::get_carry() const
{
   return eval_carry;
}

Double_t RooRealL::evaluate() const
{
   // Evaluate as straight FUNC
   std::size_t last_event = likelihood->get_N_events(), last_component = likelihood->get_N_components();

   Double_t ret = likelihood->evaluate_partition(0, last_event, 0, last_component);

   const Double_t norm = globalNormalization();
   ret /= norm;
   eval_carry = likelihood->get_carry() / norm;

   return ret;
}

TObject *RooRealL::clone(const char *newname) const
{
   return new RooRealL(*this, newname);
}

} // namespace TestStatistics
} // namespace RooFit
