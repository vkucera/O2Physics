// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file TrackSelectorPID.h
/// \brief PID track selector class
///
/// \author Vít Kučera <vit.kucera@cern.ch>, CERN

#ifndef COMMON_CORE_TRACKSELECTORPID_H_
#define COMMON_CORE_TRACKSELECTORPID_H_

#include <TPDGCode.h>

#include "Framework/Logger.h"
#include "ReconstructionDataFormats/PID.h"

/// Class for track selection using PID detectors

struct TrackSelectorPID
{
  /// Selection status
  enum Status {
    PIDNotApplicable = 0,
    PIDRejected,
    PIDConditional,
    PIDAccepted
  };
};

template <uint64_t pdg = kPiPlus>
class TrackSelectorPIDBase : TrackSelectorPID
{
 public:
  /// Default constructor
  TrackSelectorPIDBase() = default;

  /// Conversion operator
  template <uint64_t pdgNew>
  operator TrackSelectorPIDBase<pdgNew>() const {
    TrackSelectorPIDBase<pdgNew> objNew;
    // TPC
    objNew.setRangePtTPC(mPtTPCMin, mPtTPCMax);
    objNew.setRangeNSigmaTPC(mNSigmaTPCMin, mNSigmaTPCMax);
    objNew.setRangeNSigmaTPCCondTOF(mNSigmaTPCMinCondTOF, mNSigmaTPCMaxCondTOF);
    // TOF
    objNew.setRangePtTOF(mPtTOFMin, mPtTOFMax);
    objNew.setRangeNSigmaTOF(mNSigmaTOFMin, mNSigmaTOFMax);
    objNew.setRangeNSigmaTOFCondTPC(mNSigmaTOFMinCondTPC, mNSigmaTOFMaxCondTPC);
    // RICH
    objNew.setRangePtRICH(mPtRICHMin, mPtRICHMax);
    objNew.setRangeNSigmaRICH(mNSigmaRICHMin, mNSigmaRICHMax);
    objNew.setRangeNSigmaRICHCondTOF(mNSigmaRICHMinCondTOF, mNSigmaRICHMaxCondTOF);
    // Bayesian
    objNew.setRangePtBayes(mPtBayesMin, mPtBayesMax);
    objNew.setProbBayesMin(mProbBayesMin);
    return objNew;
  }

  /// Default destructor
  ~TrackSelectorPIDBase() = default;

  // TPC

  /// Set pT range where TPC PID is applicable.
  void setRangePtTPC(float ptMin, float ptMax)
  {
    mPtTPCMin = ptMin;
    mPtTPCMax = ptMax;
  }

  /// Set TPC nσ range in which a track should be accepted.
  void setRangeNSigmaTPC(float nsMin, float nsMax)
  {
    mNSigmaTPCMin = nsMin;
    mNSigmaTPCMax = nsMax;
  }

  /// Set TPC nσ range in which a track should be conditionally accepted if combined with TOF. Set to 0 to disable.
  void setRangeNSigmaTPCCondTOF(float nsMin, float nsMax)
  {
    mNSigmaTPCMinCondTOF = nsMin;
    mNSigmaTPCMaxCondTOF = nsMax;
  }

  /// Checks if track is OK for TPC PID.
  /// \param track  track
  /// \return true if track is OK for TPC PID
  template <typename T>
  bool isValidTrackPIDTPC(const T& track)
  {
    auto pt = track.pt();
    return mPtTPCMin <= pt && pt <= mPtTPCMax;
  }

  /// Checks if track is compatible with given particle species hypothesis within given TPC nσ range.
  /// \param track  track
  /// \param conditionalTOF  variable to store the result of selection with looser cuts for conditional accepting of track if combined with TOF
  /// \return true if track satisfies TPC PID hypothesis for given TPC nσ range
  template <typename T>
  bool isSelectedTrackPIDTPC(const T& track, bool& conditionalTOF)
  {
    // Accept if selection is disabled via large values.
    if (mNSigmaTPCMin < -999. && mNSigmaTPCMax > 999.) {
      return true;
    }

    // Get nσ for a given particle hypothesis.
    double nSigma = 100.;
      if constexpr (pdg == kElectron) {
        nSigma = track.tpcNSigmaEl();
      } else if constexpr (pdg == kMuonMinus) {
        nSigma = track.tpcNSigmaMu();
      } else if constexpr (pdg == kPiPlus) {
        nSigma = track.tpcNSigmaPi();
      } else if constexpr (pdg == kKPlus) {
        nSigma = track.tpcNSigmaKa();
      } else if constexpr (pdg == kProton) {
        nSigma = track.tpcNSigmaPr();
      } else {
        errorPdg();
      }

    if (mNSigmaTPCMinCondTOF < -999. && mNSigmaTPCMaxCondTOF > 999.) {
      conditionalTOF = true;
    } else {
      conditionalTOF = mNSigmaTPCMinCondTOF <= nSigma && nSigma <= mNSigmaTPCMaxCondTOF;
    }
    return mNSigmaTPCMin <= nSigma && nSigma <= mNSigmaTPCMax;
  }

  /// Returns status of TPC PID selection for a given track.
  /// \param track  track
  /// \return TPC selection status (see TrackSelectorPID::Status)
  template <typename T>
  int getStatusTrackPIDTPC(const T& track)
  {
    if (isValidTrackPIDTPC(track)) {
      bool condTOF = false;
      if (isSelectedTrackPIDTPC(track, condTOF)) {
        return Status::PIDAccepted; // accepted
      } else if (condTOF) {
        return Status::PIDConditional; // potential to be accepted if combined with TOF
      } else {
        return Status::PIDRejected; // rejected
      }
    } else {
      return Status::PIDNotApplicable; // PID not applicable
    }
  }

  // TOF

  /// Set pT range where TOF PID is applicable.
  void setRangePtTOF(float ptMin, float ptMax)
  {
    mPtTOFMin = ptMin;
    mPtTOFMax = ptMax;
  }

  /// Set TOF nσ range in which a track should be accepted.
  void setRangeNSigmaTOF(float nsMin, float nsMax)
  {
    mNSigmaTOFMin = nsMin;
    mNSigmaTOFMax = nsMax;
  }

  /// Set TOF nσ range in which a track should be conditionally accepted if combined with TPC. Set to 0 to disable.
  void setRangeNSigmaTOFCondTPC(float nsMin, float nsMax)
  {
    mNSigmaTOFMinCondTPC = nsMin;
    mNSigmaTOFMaxCondTPC = nsMax;
  }

  /// Checks if track is OK for TOF PID.
  /// \param track  track
  /// \return true if track is OK for TOF PID
  template <typename T>
  bool isValidTrackPIDTOF(const T& track)
  {
    auto pt = track.pt();
    return mPtTOFMin <= pt && pt <= mPtTOFMax;
  }

  /// Checks if track is compatible with given particle species hypothesis within given TOF nσ range.
  /// \param track  track
  /// \param conditionalTPC  variable to store the result of selection with looser cuts for conditional accepting of track if combined with TPC
  /// \return true if track satisfies TOF PID hypothesis for given TOF nσ range
  template <typename T>
  bool isSelectedTrackPIDTOF(const T& track, bool& conditionalTPC)
  {
    // Accept if selection is disabled via large values.
    if (mNSigmaTOFMin < -999. && mNSigmaTOFMax > 999.) {
      return true;
    }

    // Get nσ for a given particle hypothesis.
    double nSigma = 100.;
      if constexpr (pdg == kElectron) {
        nSigma = track.tofNSigmaEl();
      } else if constexpr (pdg == kMuonMinus) {
        nSigma = track.tofNSigmaMu();
      } else if constexpr (pdg == kPiPlus) {
        nSigma = track.tofNSigmaPi();
      } else if constexpr (pdg == kKPlus) {
        nSigma = track.tofNSigmaKa();
      } else if constexpr (pdg == kProton) {
        nSigma = track.tofNSigmaPr();
      } else {
        errorPdg();
      }

    if (mNSigmaTOFMinCondTPC < -999. && mNSigmaTOFMaxCondTPC > 999.) {
      conditionalTPC = true;
    } else {
      conditionalTPC = mNSigmaTOFMinCondTPC <= nSigma && nSigma <= mNSigmaTOFMaxCondTPC;
    }
    return mNSigmaTOFMin <= nSigma && nSigma <= mNSigmaTOFMax;
  }

  /// Returns status of TOF PID selection for a given track.
  /// \param track  track
  /// \return TOF selection status (see TrackSelectorPID::Status)
  template <typename T>
  int getStatusTrackPIDTOF(const T& track)
  {
    if (isValidTrackPIDTOF(track)) {
      bool condTPC = false;
      if (isSelectedTrackPIDTOF(track, condTPC)) {
        return Status::PIDAccepted; // accepted
      } else if (condTPC) {
        return Status::PIDConditional; // potential to be accepted if combined with TPC
      } else {
        return Status::PIDRejected; // rejected
      }
    } else {
      return Status::PIDNotApplicable; // PID not applicable
    }
  }

  // RICH

  /// Set pT range where RICH PID is applicable.
  void setRangePtRICH(float ptMin, float ptMax)
  {
    mPtRICHMin = ptMin;
    mPtRICHMax = ptMax;
  }

  /// Set RICH nσ range in which a track should be accepted.
  void setRangeNSigmaRICH(float nsMin, float nsMax)
  {
    mNSigmaRICHMin = nsMin;
    mNSigmaRICHMax = nsMax;
  }

  /// Set RICH nσ range in which a track should be conditionally accepted if combined with TOF.
  void setRangeNSigmaRICHCondTOF(float nsMin, float nsMax)
  {
    mNSigmaRICHMinCondTOF = nsMin;
    mNSigmaRICHMaxCondTOF = nsMax;
  }

  /// Checks if track is OK for RICH PID.
  /// \param track  track
  /// \return true if track is OK for RICH PID
  template <typename T>
  bool isValidTrackPIDRICH(const T& track)
  {
    if (track.richId() < 0) {
      return false;
    }
    auto pt = track.pt();
    return mPtRICHMin <= pt && pt <= mPtRICHMax;
  }

  /// Checks if track is compatible with given particle species hypothesis within given RICH nσ range.
  /// \param track  track
  /// \param conditionalTOF  variable to store the result of selection with looser cuts for conditional accepting of track if combined with TOF
  /// \return true if track satisfies RICH PID hypothesis for given RICH nσ range
  template <typename T>
  bool isSelectedTrackPIDRICH(const T& track, bool& conditionalTOF)
  {
    // Accept if selection is disabled via large values.
    if (mNSigmaRICHMin < -999. && mNSigmaRICHMax > 999.) {
      return true;
    }

    // Get nσ for a given particle hypothesis.
    double nSigma = 100.;
        if constexpr (pdg == kElectron) {
          nSigma = track.rich().richNsigmaEl();
        } else if constexpr (pdg == kMuonMinus) {
          nSigma = track.rich().richNsigmaMu();
        } else if constexpr (pdg == kPiPlus) {
          nSigma = track.rich().richNsigmaPi();
        } else if constexpr (pdg == kKPlus) {
          nSigma = track.rich().richNsigmaKa();
        } else if constexpr (pdg == kProton) {
          nSigma = track.rich().richNsigmaPr();
        } else {
          errorPdg();
        }

    if (mNSigmaRICHMinCondTOF < -999. && mNSigmaRICHMaxCondTOF > 999.) {
      conditionalTOF = true;
    } else {
      conditionalTOF = mNSigmaRICHMinCondTOF <= nSigma && nSigma <= mNSigmaRICHMaxCondTOF;
    }
    return mNSigmaRICHMin <= nSigma && nSigma <= mNSigmaRICHMax;
  }

  /// Returns status of RICH PID selection for a given track.
  /// \param track  track
  /// \return RICH selection status (see TrackSelectorPID::Status)
  template <typename T>
  int getStatusTrackPIDRICH(const T& track)
  {
    if (isValidTrackPIDRICH(track)) {
      bool condTOF = false;
      if (isSelectedTrackPIDRICH(track, condTOF)) {
        return Status::PIDAccepted; // accepted
      } else if (condTOF) {
        return Status::PIDConditional; // potential to be accepted if combined with TOF
      } else {
        return Status::PIDRejected; // rejected
      }
    } else {
      return Status::PIDNotApplicable; // PID not applicable
    }
  }

  // MID

  /// Checks if track is OK for MID PID.
  /// \param track  track
  /// \return true if track is OK for MID PID
  template <typename T>
  bool isValidTrackPIDMID(const T& track)
  {
    if constexpr (pdg == kMuonMinus) {
      return track.midId() > -1;
    } else {
      errorPdg();
      return false;
    }
  }

  /// Checks if track is compatible with muon hypothesis in the MID detector.
  /// \param track  track
  /// \return true if track has been identified as muon by the MID detector
  template <typename T>
  bool isSelectedTrackPIDMID(const T& track)
  {
    if constexpr (pdg == kMuonMinus) {
      return track.mid().midIsMuon() == 1; // FIXME: change to return track.midIsMuon() once the column is bool.
    } else {
      errorPdg();
      return false;
    }
  }

  /// Returns status of MID PID selection for a given track.
  /// \param track  track
  /// \return MID selection status (see TrackSelectorPID::Status)
  template <typename T>
  int getStatusTrackPIDMID(const T& track)
  {
    if constexpr (pdg == kMuonMinus) {
      if (isValidTrackPIDMID(track)) {
        if (isSelectedTrackPIDMID(track)) {
          return Status::PIDAccepted; // accepted
        } else {
          return Status::PIDRejected; // rejected
        }
      } else {
        return Status::PIDNotApplicable; // PID not applicable
      }
    } else {
      errorPdg();
      return Status::PIDRejected;
    }
  }

  // Combined selection (TPC + TOF)

  /// Returns status of combined PID (TPC or TOF) selection for a given track.
  /// \param track  track
  /// \return status of combined PID (TPC or TOF) (see TrackSelectorPID::Status)
  template <typename T>
  int getStatusTrackPIDTpcOrTof(const T& track)
  {
    int statusTPC = getStatusTrackPIDTPC(track);
    int statusTOF = getStatusTrackPIDTOF(track);

    if (statusTPC == Status::PIDAccepted || statusTOF == Status::PIDAccepted) {
      return Status::PIDAccepted;
    }
    if (statusTPC == Status::PIDConditional && statusTOF == Status::PIDConditional) {
      return Status::PIDAccepted;
    }
    if (statusTPC == Status::PIDRejected || statusTOF == Status::PIDRejected) {
      return Status::PIDRejected;
    }
    return Status::PIDNotApplicable; // (NotApplicable for one detector) and (NotApplicable or Conditional for the other)
  }

  /// Returns status of combined PID (TPC and TOF) selection for a given track when both detectors are applicable. Returns status of single PID otherwise.
  /// \param track  track
  /// \return status of combined PID (TPC and TOF) (see TrackSelectorPID::Status)
  template <typename T>
  int getStatusTrackPIDTpcAndTof(const T& track)
  {
    int statusTPC = Status::PIDNotApplicable;
    if (track.hasTPC()) {
      statusTPC = getStatusTrackPIDTPC(track);
    }
    int statusTOF = Status::PIDNotApplicable;
    if (track.hasTOF()) {
      statusTOF = getStatusTrackPIDTOF(track);
    }

    if (statusTPC == Status::PIDAccepted && statusTOF == Status::PIDAccepted) {
      return Status::PIDAccepted;
    }
    if (statusTPC == Status::PIDAccepted && (statusTOF == Status::PIDNotApplicable || statusTOF == Status::PIDConditional)) {
      return Status::PIDAccepted;
    }
    if ((statusTPC == Status::PIDNotApplicable || statusTPC == Status::PIDConditional) && statusTOF == Status::PIDAccepted) {
      return Status::PIDAccepted;
    }
    if (statusTPC == Status::PIDConditional && statusTOF == Status::PIDConditional) {
      return Status::PIDAccepted;
    }
    if (statusTPC == Status::PIDRejected || statusTOF == Status::PIDRejected) {
      return Status::PIDRejected;
    }
    return Status::PIDNotApplicable; // (NotApplicable for one detector) and (NotApplicable or Conditional for the other)
  }

  /// Checks whether a track is identified as electron and rejected as pion by TOF or RICH.
  /// \param track  track
  /// \param useTOF  switch to use TOF
  /// \param useRICH  switch to use RICH
  /// \return true if track is selected by TOF or RICH
  /// \note Ported from https://github.com/feisenhu/ALICE3-LoI-LMee/blob/main/efficiency/macros/anaEEstudy.cxx
  template <typename T>
  bool isElectronAndNotPion(const T& track, bool useTOF = true, bool useRICH = true)
  {
    bool isSelTOF = false;
    bool isSelRICH = false;
    bool hasRICH = track.richId() > -1;
    bool hasTOF = isValidTrackPIDTOF(track);
    auto nSigmaTOFEl = track.tofNSigmaEl();
    auto nSigmaTOFPi = track.tofNSigmaPi();
    auto nSigmaRICHEl = hasRICH ? track.rich().richNsigmaEl() : -1000.;
    auto nSigmaRICHPi = hasRICH ? track.rich().richNsigmaPi() : -1000.;
    auto p = track.p();

    // TOF
    if (useTOF && hasTOF && (p < 0.6)) {
      if (p > 0.4 && hasRICH) {
        if ((std::abs(nSigmaTOFEl) < mNSigmaTOFMax) && (std::abs(nSigmaRICHEl) < mNSigmaRICHMax)) {
          isSelTOF = true; // is selected as electron by TOF and RICH
        }
      } else if (p <= 0.4) {
        if (std::abs(nSigmaTOFEl) < mNSigmaTOFMax) {
          isSelTOF = true; // is selected as electron by TOF
        }
      } else {
        isSelTOF = false; // This is rejecting all the heavier particles which do not have a RICH signal in the p area of 0.4-0.6 GeV/c
      }
      if (std::abs(nSigmaTOFPi) < mNSigmaTOFMax) {
        isSelTOF = false; // is selected as pion by TOF
      }
    } else {
      isSelTOF = false;
    }

    // RICH
    if (useRICH && hasRICH) {
      if (std::abs(nSigmaRICHEl) < mNSigmaRICHMax) {
        isSelRICH = true; // is selected as electron by RICH
      }
      if ((std::abs(nSigmaRICHPi) < mNSigmaRICHMax) && (p > 1.0) && (p < 2.0)) {
        isSelRICH = false; // is selected as pion by RICH
      }
    } else {
      isSelRICH = false;
    }

    return isSelRICH || isSelTOF;
  }

  // Bayesian

  /// Set pT range where Bayes PID is applicable.
  void setRangePtBayes(float ptMin, float ptMax)
  {
    mPtBayesMin = ptMin;
    mPtBayesMax = ptMax;
  }

  /// Set minimum Bayesian probability above which a track should be accepted.
  void setProbBayesMin(float cut)
  {
    mProbBayesMin = cut;
  }

  /// Checks if track is OK for Bayesian PID.
  /// \param track  track
  /// \return true if track is OK for Bayesian PID
  template <typename T>
  bool isValidTrackBayesPID(const T& track)
  {
    auto pt = track.pt();
    return (mPtBayesMin <= pt && pt <= mPtBayesMax);
  }

  /// Bayesian maximum probability algorithm.
  /// \param track  track
  /// \return true if selected species has the highest Bayesian probability
  template <typename T>
  bool isSelectedTrackBayesPID(const T& track)
  {
    // Get index of the most probable species for a given track.
    if constexpr (pdg == kElectron) {
      return track.bayesID() == o2::track::PID::Electron;
    } else if constexpr (pdg == kMuonMinus) {
      return track.bayesID() == o2::track::PID::Muon;
    } else if constexpr (pdg == kPiPlus) {
      return track.bayesID() == o2::track::PID::Pion;
    } else if constexpr (pdg == kKPlus) {
      return track.bayesID() == o2::track::PID::Kaon;
    } else if constexpr (pdg == kProton) {
      return track.bayesID() == o2::track::PID::Proton;
    } else {
      errorPdg();
      return false;
    }
  }

  /// Checks if track is compatible with given particle species hypothesis within given Bayesian probability range.
  /// \param track  track
  /// \return true if track satisfies PID hypothesis for given Bayesian probability range
  template <typename T>
  bool isSelectedTrackBayesProbPID(const T& track)
  {
    if (mProbBayesMin < 0.) { // switch off with negative values
      return true;
    }

    // Get probability for a given particle hypothesis.
    double prob = 0.;
        if constexpr (pdg == kElectron) {
          prob = track.bayesEl();
        } else if constexpr (pdg == kMuonMinus) {
          prob = track.bayesMu();
        } else if constexpr (pdg == kPiPlus) {
          prob = track.bayesPi();
        } else if constexpr (pdg == kKPlus) {
          prob = track.bayesKa();
        } else if constexpr (pdg == kProton) {
          prob = track.bayesPr();
        } else {
        errorPdg();
      }

    return mProbBayesMin <= prob;
  }

  /// Returns status of Bayesian PID selection for a given track, based on the most probable particle species.
  /// \param track  track
  /// \return Bayesian selection status (see TrackSelectorPID::Status)
  template <typename T>
  int getStatusTrackBayesPID(const T& track)
  {
    if (isValidTrackBayesPID(track)) {
      if (isSelectedTrackBayesPID(track)) {
        return Status::PIDAccepted; // accepted
      } else {
        return Status::PIDRejected; // rejected
      }
    } else {
      return Status::PIDNotApplicable; // PID not applicable
    }
  }

  /// Returns status of Bayesian PID selection for a given track, based on the probability for a given particle species.
  /// \param track  track
  /// \return Bayesian selection status (see TrackSelectorPID::Status)
  template <typename T>
  int getStatusTrackBayesProbPID(const T& track)
  {
    if (isValidTrackBayesPID(track)) {
      if (isSelectedTrackBayesProbPID(track)) {
        return Status::PIDAccepted; // accepted
      } else {
        return Status::PIDRejected; // rejected
      }
    } else {
      return Status::PIDNotApplicable; // PID not applicable
    }
  }

 private:
  // TPC
  float mPtTPCMin = 0.;                ///< minimum pT for TPC PID [GeV/c]
  float mPtTPCMax = 100.;              ///< maximum pT for TPC PID [GeV/c]
  float mNSigmaTPCMin = -3.;           ///< minimum number of TPC σ
  float mNSigmaTPCMax = 3.;            ///< maximum number of TPC σ
  float mNSigmaTPCMinCondTOF = 0.;     ///< minimum number of TPC σ if combined with TOF
  float mNSigmaTPCMaxCondTOF = 0.;     ///< maximum number of TPC σ if combined with TOF

  // TOF
  float mPtTOFMin = 0.;                ///< minimum pT for TOF PID [GeV/c]
  float mPtTOFMax = 100.;              ///< maximum pT for TOF PID [GeV/c]
  float mNSigmaTOFMin = -3.;           ///< minimum number of TOF σ
  float mNSigmaTOFMax = 3.;            ///< maximum number of TOF σ
  float mNSigmaTOFMinCondTPC = 0.;     ///< minimum number of TOF σ if combined with TPC
  float mNSigmaTOFMaxCondTPC = 0.;     ///< maximum number of TOF σ if combined with TPC

  // RICH
  float mPtRICHMin = 0.;                ///< minimum pT for RICH PID [GeV/c]
  float mPtRICHMax = 100.;              ///< maximum pT for RICH PID [GeV/c]
  float mNSigmaRICHMin = -3.;           ///< minimum number of RICH σ
  float mNSigmaRICHMax = 3.;            ///< maximum number of RICH σ
  float mNSigmaRICHMinCondTOF = 0.;     ///< minimum number of RICH σ if combined with TOF
  float mNSigmaRICHMaxCondTOF = 0.;     ///< maximum number of RICH σ if combined with TOF

  // Bayesian
  float mPtBayesMin = 0.;    ///< minimum pT for Bayesian PID [GeV/c]
  float mPtBayesMax = 100.;  ///< maximum pT for Bayesian PID [GeV/c]
  float mProbBayesMin = -1.; ///< minimum Bayesian probability [%]

  /// Throw fatal for unsupported PDG values.
  void errorPdg() {
    LOGF(fatal, "Species with PDG code %d not supported", pdg);
  }
};

// Predefined types
using TrackSelectorPIDEl = TrackSelectorPIDBase<kElectron>;  // El
using TrackSelectorPIDMu = TrackSelectorPIDBase<kMuonMinus>; // Mu
using TrackSelectorPIDPi = TrackSelectorPIDBase<kPiPlus>;    // Pi
using TrackSelectorPIDKa = TrackSelectorPIDBase<kKPlus>;     // Ka
using TrackSelectorPIDPr = TrackSelectorPIDBase<kProton>;    // Pr

#endif // COMMON_CORE_TRACKSELECTORPID_H_
