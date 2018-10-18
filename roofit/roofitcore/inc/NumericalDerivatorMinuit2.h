// @(#)root/mathcore:$Id$
// Authors: L. Moneta, J.T. Offermann, E.G.P. Bos    2013-2018
//
/**********************************************************************
 *                                                                    *
 * Copyright (c) 2013 , LCG ROOT MathLib Team                         *
 * Copyright (c) 2017 Patrick Bos, Netherlands eScience Center        *
 *                                                                    *
 **********************************************************************/
/*
 * NumericalDerivatorMinuit2.h
 *
 *  Original version (NumericalDerivator) created on: Aug 14, 2013
 *      Authors: L. Moneta, J. T. Offermann
 *  Modified version (NumericalDerivatorMinuit2) created on: Sep 27, 2017
 *      Author: E. G. P. Bos
 */

#ifndef RooFit_NumericalDerivatorMinuit2
#define RooFit_NumericalDerivatorMinuit2

#ifndef ROOT_Math_IFunctionfwd
#include <Math/IFunctionfwd.h>
#endif

#include <vector>
#include "Fit/ParameterSettings.h"
#include "Minuit2/SinParameterTransformation.h"
#include "Minuit2/SqrtUpParameterTransformation.h"
#include "Minuit2/SqrtLowParameterTransformation.h"
#include "Minuit2/MnMachinePrecision.h"

#include "Minuit2/FunctionGradient.h"


namespace RooFit {

  class NumericalDerivatorMinuit2 {
  public:

    NumericalDerivatorMinuit2(const ROOT::Math::IBaseFunctionMultiDim *f, ROOT::Minuit2::FunctionGradient & grad,
                              bool always_exactly_mimic_minuit2 = true);
    NumericalDerivatorMinuit2(const NumericalDerivatorMinuit2 &other, ROOT::Minuit2::FunctionGradient & grad);
    NumericalDerivatorMinuit2(const NumericalDerivatorMinuit2 &other, ROOT::Minuit2::FunctionGradient & grad, const ROOT::Math::IBaseFunctionMultiDim *f);
//    NumericalDerivatorMinuit2& operator=(const NumericalDerivatorMinuit2 &other);
    NumericalDerivatorMinuit2(const ROOT::Math::IBaseFunctionMultiDim *f, ROOT::Minuit2::FunctionGradient & grad,
                              double step_tolerance, double grad_tolerance,
                              unsigned int ncycles, double error_level,
                              bool always_exactly_mimic_minuit2 = true);
    //   NumericalDerivatorMinuit2(const ROOT::Math::IBaseFunctionMultiDim &f, const ROOT::Fit::Fitter &fitter);
    //   NumericalDerivatorMinuit2(const ROOT::Math::IBaseFunctionMultiDim &f, const ROOT::Fit::Fitter &fitter, const ROOT::Minuit2::MnStrategy &strategy);
    virtual ~NumericalDerivatorMinuit2();

    void setup_differentiate(const double* cx, const std::vector<ROOT::Fit::ParameterSettings>& parameters);
    ROOT::Minuit2::FunctionGradient Differentiate(const double* x, const std::vector<ROOT::Fit::ParameterSettings>& parameters);
    ROOT::Minuit2::FunctionGradient operator() (const double* x, const std::vector<ROOT::Fit::ParameterSettings>& parameters);

    std::tuple<double, double, double> partial_derivative(const double *x, const std::vector<ROOT::Fit::ParameterSettings>& parameters, unsigned int i_component);
    void do_fast_partial_derivative(const std::vector<ROOT::Fit::ParameterSettings>& parameters, unsigned int i_component);
    std::tuple<double, double, double> operator()(const double *x, const std::vector<ROOT::Fit::ParameterSettings>& parameters, unsigned int i_component);

    double GetFValue() const {
      return fVal;
    }
    const double * GetG2() const {
      return fG.G2().Data();
    }
    void SetStepTolerance(double value);
    void SetGradTolerance(double value);
    void SetNCycles(int value);

    double Int2ext(const ROOT::Fit::ParameterSettings& parameter, double val) const;
    double Ext2int(const ROOT::Fit::ParameterSettings& parameter, double val) const;
    double DInt2Ext(const ROOT::Fit::ParameterSettings& parameter, double val) const;
    double D2Int2Ext(const ROOT::Fit::ParameterSettings& parameter, double val) const;
    double GStepInt2Ext(const ROOT::Fit::ParameterSettings& parameter, double val) const;

    void SetInitialGradient(std::vector<ROOT::Fit::ParameterSettings>& parameters);

    void set_step_tolerance(double step_tolerance);
    void set_grad_tolerance(double grad_tolerance);
    void set_ncycles(unsigned int ncycles);
    void set_error_level(double error_level);

  private:

    // 26 July 2018: I DON'T THINK THE BELOW TEXT IS STILL TRUE, CHECK AND REMOVE

    // CAUTION: we only use fFunction to check whether the same function is used on every call.
    // We do not use it directly as a callable function, as this would require us to:
    // 1. either pass it at construction time, which would introduce a circular dependency from
    //    classes that use this class as a derivative implementation, like RooGradMinimizerFcn
    //    and RooGradientFunction,
    // 2. or let the user initialize it manually before use, which is error prone, as the user
    //    may forget to do so and at some future point new functions may omit to check for it.
    // Our current implementation still requires the user to manually set the fFunction before
    // use (otherwise the asserts in Differentiate and SetInitialGradient will fail), but
    const ROOT::Math::IBaseFunctionMultiDim* fFunction;

    double fStepTolerance = 0.5;
    double fGradTolerance = 0.1;
    unsigned int fNCycles = 2;
    double Up = 1;
    double fVal = 0;
    unsigned int fN;

    ROOT::Minuit2::FunctionGradient& fG;

    // TODO: find out why FunctionGradient keeps its data const.. but work around it in the meantime
    ROOT::Minuit2::MnAlgebraicVector& mutable_grad() const;
    ROOT::Minuit2::MnAlgebraicVector& mutable_g2() const;
    ROOT::Minuit2::MnAlgebraicVector& mutable_gstep() const;

    std::vector<double> vx, vx_external;
    double dfmin;
    double vrysml;

    // MODIFIED: Minuit2 determines machine precision in a slightly different way than
    // std::numeric_limits<double>::epsilon()). We go with the Minuit2 one.
    ROOT::Minuit2::MnMachinePrecision precision;

    ROOT::Minuit2::SinParameterTransformation fDoubleLimTrafo;
    ROOT::Minuit2::SqrtUpParameterTransformation fUpperLimTrafo;
    ROOT::Minuit2::SqrtLowParameterTransformation fLowerLimTrafo;

  private:
    bool _always_exactly_mimic_minuit2;
  public:
    bool always_exactly_mimic_minuit2() const;
    void set_always_exactly_mimic_minuit2(bool flag);

  private:
    std::vector<double> vx_fVal_cache;
#ifndef NDEBUG
    std::size_t fVal_eval_counter = 0; //!
#endif
  };

} // namespace RooFit

#endif /* NumericalDerivatorMinuit2_H_ */