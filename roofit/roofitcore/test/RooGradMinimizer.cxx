/*
 * RooGradMinimizer tests
 * Copyright (c) 2017, Patrick Bos, Netherlands eScience Center
 */

//#include <iostream>

#include <TRandom.h>
#include <RooWorkspace.h>
#include <RooDataSet.h>
#include <RooRealVar.h>
#include <RooAbsPdf.h>
#include <RooTimer.h>
#include <RooMinimizer.h>
#include <RooFitResult.h>
#include <RooAddPdf.h>

#include <RooGradMinimizer.h>

#include "gtest/gtest.h"

#include "ULPdiff.h"


TEST(GradMinimizer, Gaussian1D) {
  for (int i = 0; i < 10; ++i) {
    std::cout << std::endl << "run " << i << std::endl;
    // produce the same random stuff every time
    gRandom->SetSeed(1);

    RooWorkspace w = RooWorkspace();

    w.factory("Gaussian::g(x[-5,5],mu[0,-3,3],sigma[1])");

    auto x = w.var("x");
    RooAbsPdf *pdf = w.pdf("g");
    RooRealVar *mu = w.var("mu");

    RooDataSet *data = pdf->generate(RooArgSet(*x), 10000);
    mu->setVal(-2.9);
//    mu->removeRange();
//    mu->setError(0.3);

    auto nll = pdf->createNLL(*data);

    // save initial values for the start of all minimizations
    RooArgSet values = RooArgSet(*mu, *pdf, *nll);
    std::cout << std::hexfloat << "mu: " << mu->getVal() << std::endl;

    RooArgSet *savedValues = dynamic_cast<RooArgSet *>(values.snapshot());
    if (savedValues == nullptr) {
      throw std::runtime_error("params->snapshot() cannot be casted to RooArgSet!");
    }

    // --------

    RooWallTimer wtimer;

    // --------

  std::cout << "starting nominal calculation" << std::endl;

    RooMinimizer m0(*nll);
    m0.setMinimizerType("Minuit2");

    m0.setStrategy(0);
    m0.setPrintLevel(-1);

    wtimer.start();
    m0.migrad();
    wtimer.stop();

  std::cout << "  -- nominal calculation wall clock time:        " << wtimer.timing_s() << "s" << std::endl;

    RooFitResult *m0result = m0.lastMinuitFit();
    double minNll0 = m0result->minNll();
    double edm0 = m0result->edm();
    double mu0 = mu->getVal();
    double muerr0 = mu->getError();

  std::cout << " ======== resetting initial values ======== " << std::endl;
    values = *savedValues;

    std::cout << std::hexfloat << "mu: " << mu->getVal() << std::endl;

  std::cout << "starting GradMinimizer" << std::endl;

    RooGradMinimizer m1(*nll);
    m1.setMinimizerType("Minuit2");

    m1.setStrategy(0);
    m1.setPrintLevel(-1);

    wtimer.start();
    m1.migrad();
    wtimer.stop();

  std::cout << "  -- GradMinimizer calculation wall clock time:  " << wtimer.timing_s() << "s" << std::endl;

    RooFitResult *m1result = m1.lastMinuitFit();
    double minNll1 = m1result->minNll();
    double edm1 = m1result->edm();
    double mu1 = mu->getVal();
    double muerr1 = mu->getError();

    // for unknown reasons, the results are often not exactly equal
    // see discussion: https://github.com/roofit-dev/root/issues/11
    // we cast to float to do the comparison, because at double precision
    // the differences are often just slightly too large
//    EXPECT_FLOAT_EQ(minNll0, minNll1);
//    EXPECT_FLOAT_EQ(mu0, mu1);
//    EXPECT_FLOAT_EQ(muerr0, muerr1);
//    EXPECT_NEAR(edm0, edm1, 1e-4);
    EXPECT_EQ(minNll0, minNll1);
    EXPECT_EQ(mu0, mu1);
    EXPECT_EQ(muerr0, muerr1);
    EXPECT_EQ(edm0, edm1);

    // these ULP functions can be used for further analysis of the differences
//    std::cout << "ulp_diff muerr: " << ulp_diff(muerr0, muerr1) << std::endl;
//    std::cout << "ulp_diff muerr float-casted: " << ulp_diff((float) muerr0, (float) muerr1) << std::endl;
  }
}

///*
TEST(GradMinimizer, GaussianND) {
  // test RooGradMinimizer class with simple N-dimensional pdf
  int n = 5;
  int N_events = 1000;
  // produce the same random stuff every time
  gRandom->SetSeed(1);

  RooWorkspace w("w", kFALSE);

  RooArgSet obs_set;

  // create gaussian parameters
  float mean[n], sigma[n];
  for (int ix = 0; ix < n; ++ix) {
    mean[ix] = gRandom->Gaus(0, 2);
    sigma[ix] = 0.1 + abs(gRandom->Gaus(0, 2));
  }

  // create gaussians and also the observables and parameters they depend on
  for (int ix = 0; ix < n; ++ix) {
    std::ostringstream os;
    os << "Gaussian::g" << ix
       << "(x" << ix << "[-10,10],"
       << "m" << ix << "[" << mean[ix] << ",-10,10],"
       << "s" << ix << "[" << sigma[ix] << ",0.1,10])";
    w.factory(os.str().c_str());
  }

  // create uniform background signals on each observable
  for (int ix = 0; ix < n; ++ix) {
    {
      std::ostringstream os;
      os << "Uniform::u" << ix << "(x" << ix << ")";
      w.factory(os.str().c_str());
    }

    // gather the observables in a list for data generation below
    {
      std::ostringstream os;
      os << "x" << ix;
      obs_set.add(*w.arg(os.str().c_str()));
    }
  }

  RooArgSet pdf_set = w.allPdfs();

  // create event counts for all pdfs
  RooArgSet count_set;

  // ... for the gaussians
  for (int ix = 0; ix < n; ++ix) {
    std::stringstream os, os2;
    os << "Nsig" << ix;
    os2 << "#signal events comp " << ix;
    RooRealVar a(os.str().c_str(), os2.str().c_str(), N_events/10, 0., 10*N_events);
    w.import(a);
    // gather in count_set
    count_set.add(*w.arg(os.str().c_str()));
  }
  // ... and for the uniform background components
  for (int ix = 0; ix < n; ++ix) {
    std::stringstream os, os2;
    os << "Nbkg" << ix;
    os2 << "#background events comp " << ix;
    RooRealVar a(os.str().c_str(), os2.str().c_str(), N_events/10, 0., 10*N_events);
    w.import(a);
    // gather in count_set
    count_set.add(*w.arg(os.str().c_str()));
  }

  RooAddPdf sum("sum", "gaussians+uniforms", pdf_set, count_set);

  // --- Generate a toyMC sample from composite PDF ---
  RooDataSet *data = sum.generate(obs_set, N_events);

  auto nll = sum.createNLL(*data);

  // set values randomly so that they actually need to do some fitting
  for (int ix = 0; ix < n; ++ix) {
    {
      std::ostringstream os;
      os << "m" << ix;
      dynamic_cast<RooRealVar *>(w.arg(os.str().c_str()))->setVal(gRandom->Gaus(0, 2));
    }
    {
      std::ostringstream os;
      os << "s" << ix;
      dynamic_cast<RooRealVar *>(w.arg(os.str().c_str()))->setVal(0.1 + abs(gRandom->Gaus(0, 2)));
    }
  }

  // gather all values of parameters, observables, pdfs and nll here for easy
  // saving and restoring
  RooArgSet some_values = RooArgSet(obs_set, pdf_set, "some_values");
  RooArgSet all_values = RooArgSet(some_values, count_set, "all_values");
  all_values.add(*nll);
  all_values.add(sum);
  for (int ix = 0; ix < n; ++ix) {
    {
      std::ostringstream os;
      os << "m" << ix;
      all_values.add(*w.arg(os.str().c_str()));
    }
    {
      std::ostringstream os;
      os << "s" << ix;
      all_values.add(*w.arg(os.str().c_str()));
    }
  }

  // save initial values for the start of all minimizations
  RooArgSet* savedValues = dynamic_cast<RooArgSet*>(all_values.snapshot());
  if (savedValues == nullptr) {
    throw std::runtime_error("params->snapshot() cannot be casted to RooArgSet!");
  }

  // --------

  RooWallTimer wtimer;

  // --------

  std::cout << "running nominal calculation" << std::endl;

  RooMinimizer m0(*nll);
  m0.setMinimizerType("Minuit2");

  m0.setStrategy(0);
  // m0.setVerbose();
  m0.setPrintLevel(0);

  wtimer.start();
  m0.migrad();
  wtimer.stop();

  std::cout << "  -- nominal calculation wall clock time:        " << wtimer.timing_s() << "s" << std::endl;

  RooFitResult *m0result = m0.lastMinuitFit();
  double minNll0 = m0result->minNll();
  double edm0 = m0result->edm();
  double mean0[n];
  double std0[n];
  for (int ix = 0; ix < n; ++ix) {
    {
      std::ostringstream os;
      os << "m" << ix;
      mean0[ix] = dynamic_cast<RooRealVar*>(w.arg(os.str().c_str()))->getVal();
    }
    {
      std::ostringstream os;
      os << "s" << ix;
      std0[ix] = dynamic_cast<RooRealVar*>(w.arg(os.str().c_str()))->getVal();
    }
  }


  std::cout << " ====================================== " << std::endl;
  // --------

  std::cout << " ======== reset initial values ======== " << std::endl;
  all_values = *savedValues;

  // --------
  std::cout << " ====================================== " << std::endl;


  std::cout << "running GradMinimizer" << std::endl;

  RooGradMinimizer m1(*nll);

  m1.setStrategy(0);
  m1.setPrintLevel(0);

  wtimer.start();
  m1.migrad();
  wtimer.stop();

  std::cout << "  -- GradMinimizer calculation wall clock time:  " << wtimer.timing_s() << "s" << std::endl;

  RooFitResult *m1result = m1.lastMinuitFit();
  double minNll1 = m1result->minNll();
  double edm1 = m1result->edm();
  double mean1[n];
  double std1[n];
  for (int ix = 0; ix < n; ++ix) {
    {
      std::ostringstream os;
      os << "m" << ix;
      mean1[ix] = dynamic_cast<RooRealVar*>(w.arg(os.str().c_str()))->getVal();
    }
    {
      std::ostringstream os;
      os << "s" << ix;
      std1[ix] = dynamic_cast<RooRealVar*>(w.arg(os.str().c_str()))->getVal();
    }
  }

  EXPECT_FLOAT_EQ(minNll0, minNll1);
  EXPECT_NEAR(edm0, edm1, 1e-4);

  for (int ix = 0; ix < n; ++ix) {
    EXPECT_FLOAT_EQ(mean0[ix], mean1[ix]);
    EXPECT_FLOAT_EQ(std0[ix], std1[ix]);
  }
}

TEST(GradMinimizer, BranchingPDF) {
  // test RooGradMinimizer class with an N-dimensional pdf that forms a tree of
  // pdfs, where one subpdf is the parameter of a higher level pdf
  int N_events = 1000;
  // produce the same random stuff every time
  gRandom->SetSeed(1);

  RooWorkspace w("w", kFALSE);

  // 3rd level
  w.factory("Gamma::ga0_0_1(k0_0_1[3,2,10],u[1,20],1,0)"); // leaf pdf
  // Gamma(mu,N+1,1,0) ~ Pois(N,mu), so this is a "continuous Poissonian"

  // 2nd level that will be linked to from 3rd level
  w.factory("Gamma::ga1_0(k1_0[4,2,10],z[1,20],1,0)"); // leaf pdf

  // rest of 3rd level
  w.factory("Gaussian::g0_0_0(v[-10,10],m0_0_0[0.6,-10,10],ga1_0)"); // two branch pdf, one up a level to different 1st level branch

  // rest of 2nd level
  w.factory("Gaussian::g0_0(g0_0_0,m0_0[6,-10,10],ga0_0_1)"); // branch pdf

  // 1st level
  w.factory("Gaussian::g0(x[-10,10],g0_0,s0[3,0.1,10])"); // branch pdf
  w.factory("Gaussian::g1(y[-10,10],m1[-2,-10,10],ga1_0)"); // branch pdf
  RooArgSet level1_pdfs;
  level1_pdfs.add(*w.arg("g0"));
  level1_pdfs.add(*w.arg("g1"));

  // event counts for 1st level pdfs
  RooRealVar N_g0("N_g0", "#events g0", N_events/10, 0., 10*N_events);
  RooRealVar N_g1("N_g1", "#events g1", N_events/10, 0., 10*N_events);
  w.import(N_g0);
  w.import(N_g1);
  // gather in count_set
  RooArgSet level1_counts;
  level1_counts.add(N_g0);
  level1_counts.add(N_g1);

  // finally, sum the top level pdfs
  RooAddPdf sum("sum", "gaussian tree", level1_pdfs, level1_counts);

  // gather observables
  RooArgSet obs_set;
  for (auto obs : {"x", "y", "z", "u", "v"}) {
    obs_set.add(*w.arg(obs));
  }

  // --- Generate a toyMC sample from composite PDF ---
  RooDataSet *data = sum.generate(obs_set, N_events);

  auto nll = sum.createNLL(*data);

  // gather all values of parameters, observables, pdfs and nll here for easy
  // saving and restoring
  RooArgSet some_values = RooArgSet(obs_set, w.allPdfs(), "some_values");
  RooArgSet most_values = RooArgSet(some_values, level1_counts, "most_values");
  most_values.add(*nll);
  most_values.add(sum);

  RooArgSet * param_set = nll->getParameters(obs_set);

  RooArgSet all_values = RooArgSet(most_values, *param_set, "all_values");

  // set parameter values randomly so that they actually need to do some fitting
  auto it = all_values.fwdIterator();
  while (RooRealVar * val = dynamic_cast<RooRealVar *>(it.next())) {
    val->setVal(gRandom->Uniform(val->getMin(), val->getMax()));
  }

  // save initial values for the start of all minimizations
  RooArgSet* savedValues = dynamic_cast<RooArgSet*>(all_values.snapshot());
  if (savedValues == nullptr) {
    throw std::runtime_error("params->snapshot() cannot be casted to RooArgSet!");
  }

  // --------

  RooWallTimer wtimer;

  // --------

  std::cout << "running nominal calculation" << std::endl;

  RooMinimizer m0(*nll);
  m0.setMinimizerType("Minuit2");

  m0.setStrategy(0);
  m0.setPrintLevel(0);

  wtimer.start();
  m0.migrad();
  wtimer.stop();

  std::cout << "  -- nominal calculation wall clock time:        " << wtimer.timing_s() << "s" << std::endl;

  RooFitResult *m0result = m0.lastMinuitFit();
  double minNll0 = m0result->minNll();
  double edm0 = m0result->edm();

  double N_g0__0 = N_g0.getVal();
  double N_g1__0 = N_g1.getVal();
  double k0_0_1__0 = dynamic_cast<RooRealVar *>(w.arg("k0_0_1"))->getVal();
  double k1_0__0 = dynamic_cast<RooRealVar *>(w.arg("k1_0"))->getVal();
  double m0_0__0 = dynamic_cast<RooRealVar *>(w.arg("m0_0"))->getVal();
  double m0_0_0__0 = dynamic_cast<RooRealVar *>(w.arg("m0_0_0"))->getVal();
  double m1__0 = dynamic_cast<RooRealVar *>(w.arg("m1"))->getVal();
  double s0__0 = dynamic_cast<RooRealVar *>(w.arg("s0"))->getVal();


  std::cout << " ====================================== " << std::endl;
  // --------

  std::cout << " ======== reset initial values ======== " << std::endl;
  all_values = *savedValues;

  // --------
  std::cout << " ====================================== " << std::endl;


  std::cout << "running GradMinimizer" << std::endl;

  RooGradMinimizer m1(*nll);

  m1.setStrategy(0);
  m1.setPrintLevel(0);

  wtimer.start();
  m1.migrad();
  wtimer.stop();

  std::cout << "  -- GradMinimizer calculation wall clock time:  " << wtimer.timing_s() << "s" << std::endl;

  RooFitResult *m1result = m1.lastMinuitFit();
  double minNll1 = m1result->minNll();
  double edm1 = m1result->edm();

  EXPECT_FLOAT_EQ(minNll0, minNll1);
  EXPECT_NEAR(edm0, edm1, 1e-4);

  double N_g0__1 = N_g0.getVal();
  double N_g1__1 = N_g1.getVal();
  double k0_0_1__1 = dynamic_cast<RooRealVar *>(w.arg("k0_0_1"))->getVal();
  double k1_0__1 = dynamic_cast<RooRealVar *>(w.arg("k1_0"))->getVal();
  double m0_0__1 = dynamic_cast<RooRealVar *>(w.arg("m0_0"))->getVal();
  double m0_0_0__1 = dynamic_cast<RooRealVar *>(w.arg("m0_0_0"))->getVal();
  double m1__1 = dynamic_cast<RooRealVar *>(w.arg("m1"))->getVal();
  double s0__1 = dynamic_cast<RooRealVar *>(w.arg("s0"))->getVal();

  EXPECT_FLOAT_EQ(N_g0__0, N_g0__1);
  EXPECT_FLOAT_EQ(N_g1__0, N_g1__1);
  EXPECT_FLOAT_EQ(k0_0_1__0, k0_0_1__1);
  EXPECT_FLOAT_EQ(k1_0__0, k1_0__1);
  EXPECT_FLOAT_EQ(m0_0__0, m0_0__1);
  EXPECT_FLOAT_EQ(m0_0_0__0, m0_0_0__1);
  EXPECT_FLOAT_EQ(m1__0, m1__1);
  EXPECT_FLOAT_EQ(s0__0, s0__1);

// N_g0    = 494.514  +/-  18.8621 (limited)
// N_g1    = 505.817  +/-  24.6705 (limited)
// k0_0_1    = 2.96883  +/-  0.00561152  (limited)
// k1_0    = 4.12068  +/-  0.0565994 (limited)
// m0_0    = 8.09563  +/-  1.30395 (limited)
// m0_0_0    = 0.411472   +/-  0.183239  (limited)
// m1    = -1.99988   +/-  0.00194089  (limited)
// s0    = 3.04623  +/-  0.0982477 (limited)

}
//*/