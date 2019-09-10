// Author: Akira Okumura <mailto:oxon@mac.com>
/******************************************************************************
 * Copyright (C) 2006-, Akira Okumura                                         *
 * All rights reserved.                                                       *
 *****************************************************************************/

#ifndef A_MULTILAYER_H
#define A_MULTILAYER_H

#include <memory>
#include <thread>

#include "ARefractiveIndex.h"

///////////////////////////////////////////////////////////////////////////////
//
// AMultilayer
//
///////////////////////////////////////////////////////////////////////////////

class AMultilayer : public TObject {
public:
  enum EPolarization {kS, kP};
  
private:
  std::vector<std::shared_ptr<ARefractiveIndex>> fRefractiveIndexList;
  std::vector<Double_t> fThicknessList;

  Bool_t IsForwardAngle(std::complex<Double_t> n, std::complex<Double_t> theta) const;
  void ListSnell(std::complex<Double_t> th_0,
                 const std::vector<std::complex<Double_t>>& n_list,
                 std::vector<std::complex<Double_t>>& th_list) const;
  void CoherentTMMMixedMultiAngle(std::vector<std::complex<Double_t>>::const_iterator th_0_cbegin,
                                  std::vector<std::complex<Double_t>>::const_iterator th_0_cend,
                                  Double_t lam_vac, std::vector<Double_t>::iterator reflectance_it,
                                  std::vector<Double_t>::iterator transmittance_it
                                  ) {
    for (auto cit = th_0_cbegin; cit != th_0_cend; ++cit) {
      Double_t r, t;
      CoherentTMMMixed(*cit, lam_vac, r, t);
      *reflectance_it = r;
      *transmittance_it = t;
      ++reflectance_it;
      ++transmittance_it;
    }
  }
  void CoherentTMMMixedMultiWavelength(std::complex<Double_t> th_0,
                                       std::vector<Double_t>::const_iterator lam_vac_cbegin,
                                       std::vector<Double_t>::const_iterator lam_vac_cend,
                                       std::vector<Double_t>::iterator reflectance_it,
                                       std::vector<Double_t>::iterator transmittance_it
                                       ) {
    for (auto cit = lam_vac_cbegin; cit != lam_vac_cend; ++cit) {
      Double_t r, t;
      CoherentTMMMixed(th_0, *cit, r, t);
      *reflectance_it = r;
      *transmittance_it = t;
      ++reflectance_it;
      ++transmittance_it;
    }
  }

 public:
  AMultilayer(std::shared_ptr<ARefractiveIndex> top,
              std::shared_ptr<ARefractiveIndex> bottom);
  virtual ~AMultilayer();

  void InsertLayer(std::shared_ptr<ARefractiveIndex> idx, Double_t thickness);
  void ChangeThickness(std::size_t i, Double_t thickness) {
    if (i < 1 || i > fThicknessList.size() - 2) {
      Error("ChangeThickness", "Cannot change the thickness of the %luth layer", i);
    } else {
      fThicknessList[i] = thickness;
    }
  }

  void CoherentTMM(EPolarization polarization, std::complex<Double_t> th_0,
                   Double_t lam_vac, Double_t& reflectance,
                   Double_t& transmittance) const;
  void CoherentTMMMixed(std::complex<Double_t> th_0,
                        Double_t lam_vac, Double_t& reflectance,
                        Double_t& transmittance) const {
    Double_t r = 0;
    Double_t t = 0;
    CoherentTMMP(th_0, lam_vac, reflectance, transmittance);
    r += reflectance;
    t += transmittance;
    CoherentTMMS(th_0, lam_vac, reflectance, transmittance);
    r += reflectance;
    t += transmittance;

    reflectance = r / 2.;
    transmittance = t/ 2.;
  }
  void CoherentTMMMixed(std::vector<std::complex<Double_t>>& th_0,
                        Double_t lam_vac, std::vector<Double_t>& reflectance,
                        std::vector<Double_t>& transmittance) const {
    auto n = th_0.size();
    reflectance.resize(n);
    transmittance.resize(n);

    const auto nthreads = std::thread::hardware_concurrency();
    std::vector<std::thread> threads(nthreads);

    auto th_0_cbegin = th_0.cbegin();
    auto th_0_cend = th_0.end();
    auto reflectance_begin = reflectance.begin();
    auto transmittance_begin = transmittance.begin();

    auto step = n/nthreads;

    for (std::size_t i = 0; i < nthreads; ++i) {     
      if (i == nthreads - 1) {
        threads[i] = std::thread(&AMultilayer::CoherentTMMMixedMultiAngle, *this, th_0_cbegin, th_0_cbegin + step, lam_vac, reflectance_begin, transmittance_begin);
        th_0_cbegin += step;
      } else {
        threads[i] = std::thread(&AMultilayer::CoherentTMMMixedMultiAngle, *this, th_0_cbegin, th_0_cend, lam_vac, reflectance_begin, transmittance_begin);
      }
    }
    for (std::size_t i = 0; i < nthreads; ++i) {
      threads[i].join();
    }
  }
  void CoherentTMMMixed(std::complex<Double_t> th_0,
                        std::vector<Double_t>& lam_vac, std::vector<Double_t>& reflectance,
                        std::vector<Double_t>& transmittance) const {
    auto n = lam_vac.size();
    reflectance.resize(n);
    transmittance.resize(n);

    const auto nthreads = std::thread::hardware_concurrency();
    std::vector<std::thread> threads(nthreads);

    auto lam_vac_cbegin = lam_vac.cbegin();
    auto lam_vac_cend = lam_vac.end();
    auto reflectance_begin = reflectance.begin();
    auto transmittance_begin = transmittance.begin();

    auto step = n/nthreads;

    for (std::size_t i = 0; i < nthreads; ++i) {     
      if (i == nthreads - 1) {
        threads[i] = std::thread(&AMultilayer::CoherentTMMMixedMultiWavelength, *this, th_0, lam_vac_cbegin, lam_vac_cbegin + step, reflectance_begin, transmittance_begin);
        lam_vac_cbegin += step;
      } else {
        threads[i] = std::thread(&AMultilayer::CoherentTMMMixedMultiWavelength, *this, th_0, lam_vac_cbegin, lam_vac_cend, reflectance_begin, transmittance_begin);
      }
    }
    for (std::size_t i = 0; i < nthreads; ++i) {
      threads[i].join();
    }
  }
  void CoherentTMMP(std::complex<Double_t> th_0,
                    Double_t lam_vac, Double_t& reflectance,
                    Double_t& transmittance) const {
    CoherentTMM(kP, th_0, lam_vac, reflectance, transmittance);
  }
  void CoherentTMMS(std::complex<Double_t> th_0,
                    Double_t lam_vac, Double_t& reflectance,
                    Double_t& transmittance) const {
    CoherentTMM(kS, th_0, lam_vac, reflectance, transmittance);
  }
  void PrintLayers(Double_t lambda) const;

  ClassDef(AMultilayer, 1)
};

#endif  // A_MULTILAYER_H
