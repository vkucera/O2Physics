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
///
/// \author Alberto Caliva (alberto.caliva@cern.ch)
/// \since September 19, 2023

#include "Common/Core/RecoDecay.h"
#include "Common/Core/TrackSelection.h"
#include "Common/Core/trackUtilities.h"
#include "Common/DataModel/EventSelection.h"
#include "Common/DataModel/PIDResponse.h"
#include "Common/DataModel/TrackSelectionTables.h"
#include "Common/DataModel/Centrality.h"
#include "Framework/ASoAHelpers.h"
#include "Framework/AnalysisDataModel.h"
#include "Framework/AnalysisTask.h"
#include "Framework/runDataProcessing.h"
#include "PWGLF/DataModel/LFStrangenessTables.h"
#include "ReconstructionDataFormats/Track.h"

using namespace o2;
using namespace o2::framework;

using PIDTracks = soa::Join<
  aod::Tracks, aod::TracksExtra, aod::TracksDCA, aod::pidTOFbeta,
  aod::pidTOFmass, aod::TrackSelection, aod::TrackSelectionExtension,
  aod::pidTPCFullPi, aod::pidTPCFullKa, aod::pidTPCFullPr, aod::pidTPCFullDe,
  aod::pidTPCFullTr, aod::pidTPCFullHe, aod::pidTOFFullPi, aod::pidTOFFullKa,
  aod::pidTOFFullPr, aod::pidTOFFullDe, aod::pidTOFFullTr, aod::pidTOFFullHe>;

using SelectedCollisions = soa::Join<aod::Collisions, aod::EvSels, aod::CentFT0Cs>;

struct tpc_dedx_qa {

  // dE/dx for all charged particles
  HistogramRegistry registryCh{
    "registryCh",
    {},
    OutputObjHandlingPolicy::AnalysisObject,
    true,
    true};
  // dE/dx for pions
  HistogramRegistry registryPi{
    "registryPi",
    {},
    OutputObjHandlingPolicy::AnalysisObject,
    true,
    true};
  // dE/dx for kaons
  HistogramRegistry registryKa{
    "registryKa",
    {},
    OutputObjHandlingPolicy::AnalysisObject,
    true,
    true};
  // dE/dx for protons
  HistogramRegistry registryPr{
    "registryPr",
    {},
    OutputObjHandlingPolicy::AnalysisObject,
    true,
    true};
  // dE/dx for helium-3
  HistogramRegistry registryHe{
    "registryHe",
    {},
    OutputObjHandlingPolicy::AnalysisObject,
    true,
    true};

  // Configurable Parameters
  Configurable<float> minTPCnClsFound{"minTPCnClsFound", 70.0f,
                                      "min number of found TPC clusters"};
  Configurable<float> minNCrossedRowsTPC{
    "minNCrossedRowsTPC", 70.0f, "min number of found TPC crossed rows"};
  Configurable<float> minNClsTPCdEdx{
    "minNClsTPCdEdx", 50.0f, "min number of TPC clusters for PID"};
  Configurable<float> maxChi2TPC{"maxChi2TPC", 4.0f,
                                 "max chi2 per cluster TPC"};
  Configurable<float> maxChi2ITS{"maxChi2ITS", 36.0f,
                                 "max chi2 per cluster ITS"};
  Configurable<float> etaMin{"etaMin", -0.8f, "etaMin"};
  Configurable<float> etaMax{"etaMax", +0.8f, "etaMax"};
  Configurable<float> v0cospaMin{"v0cospaMin", 0.998, "Minimum V0 CosPA"};
  Configurable<float> minimumV0Radius{"minimumV0Radius", 0.5f,
                                      "Minimum V0 Radius"};
  Configurable<float> maximumV0Radius{"maximumV0Radius", 100.0f,
                                      "Maximum V0 Radius"};
  Configurable<float> dcaV0DaughtersMax{"dcaV0DaughtersMax", 0.5f,
                                        "Maximum DCA Daughters"};
  Configurable<float> nsigmaTOFmax{"nsigmaTOFmax", 3.0f, "Maximum nsigma TOF"};
  Configurable<float> minMassK0s{"minMassK0s", 0.4f, "Minimum Mass K0s"};
  Configurable<float> maxMassK0s{"maxMassK0s", 0.6f, "Maximum Mass K0s"};
  Configurable<float> minMassLambda{"minMassLambda", 1.1f,
                                    "Minimum Mass Lambda"};
  Configurable<float> maxMassLambda{"maxMassLambda", 1.2f,
                                    "Maximum Mass Lambda"};
  Configurable<float> minReqClusterITS{
    "minReqClusterITS", 4.0f, "min number of clusters required in ITS"};
  Configurable<float> maxDCAxy{"maxDCAxy", 0.1f, "maxDCAxy"};
  Configurable<float> maxDCAz{"maxDCAz", 0.1f, "maxDCAz"};
  Configurable<bool> eventSelection{"eventSelection", true, "event selection"};

  void init(InitContext const&)
  {
    // Charged Particles
    registryCh.add(
      "dEdx_vs_Momentum", "dE/dx", HistType::kTH2F,
      {{200, -10.0, 10.0, "z#upoint p (GeV/c)"}, {1500, 0, 1500, "dE/dx (a. u.)"}});

    // Pions
    registryPi.add(
      "dEdx_vs_Momentum_Pi_m0806", "dE/dx", HistType::kTH3F,
      {{200, -10.0, 10.0, "z#upoint p (GeV/c)"}, {200, 0, 200, "dE/dx (a. u.)"}, {10, 0, 100, "centrality"}});

    registryPi.add(
      "dEdx_vs_Momentum_Pi_m0604", "dE/dx", HistType::kTH3F,
      {{200, -10.0, 10.0, "z#upoint p (GeV/c)"}, {200, 0, 200, "dE/dx (a. u.)"}, {10, 0, 100, "centrality"}});

    registryPi.add(
      "dEdx_vs_Momentum_Pi_m0402", "dE/dx", HistType::kTH3F,
      {{200, -10.0, 10.0, "z#upoint p (GeV/c)"}, {200, 0, 200, "dE/dx (a. u.)"}, {10, 0, 100, "centrality"}});

    registryPi.add(
      "dEdx_vs_Momentum_Pi_m0200", "dE/dx", HistType::kTH3F,
      {{200, -10.0, 10.0, "z#upoint p (GeV/c)"}, {200, 0, 200, "dE/dx (a. u.)"}, {10, 0, 100, "centrality"}});

    registryPi.add(
      "dEdx_vs_Momentum_Pi_p0002", "dE/dx", HistType::kTH3F,
      {{200, -10.0, 10.0, "z#upoint p (GeV/c)"}, {200, 0, 200, "dE/dx (a. u.)"}, {10, 0, 100, "centrality"}});

    registryPi.add(
      "dEdx_vs_Momentum_Pi_p0204", "dE/dx", HistType::kTH3F,
      {{200, -10.0, 10.0, "z#upoint p (GeV/c)"}, {200, 0, 200, "dE/dx (a. u.)"}, {10, 0, 100, "centrality"}});

    registryPi.add(
      "dEdx_vs_Momentum_Pi_p0406", "dE/dx", HistType::kTH3F,
      {{200, -10.0, 10.0, "z#upoint p (GeV/c)"}, {200, 0, 200, "dE/dx (a. u.)"}, {10, 0, 100, "centrality"}});

    registryPi.add(
      "dEdx_vs_Momentum_Pi_p0608", "dE/dx", HistType::kTH3F,
      {{200, -10.0, 10.0, "z#upoint p (GeV/c)"}, {200, 0, 200, "dE/dx (a. u.)"}, {10, 0, 100, "centrality"}});

    // Kaons
    registryKa.add(
      "dEdx_vs_Momentum_Ka_m0806", "dE/dx", HistType::kTH3F,
      {{200, -10.0, 10.0, "z#upoint p (GeV/c)"}, {600, 0, 600, "dE/dx (a. u.)"}, {10, 0, 100, "centrality"}});

    registryKa.add(
      "dEdx_vs_Momentum_Ka_m0604", "dE/dx", HistType::kTH3F,
      {{200, -10.0, 10.0, "z#upoint p (GeV/c)"}, {600, 0, 600, "dE/dx (a. u.)"}, {10, 0, 100, "centrality"}});

    registryKa.add(
      "dEdx_vs_Momentum_Ka_m0402", "dE/dx", HistType::kTH3F,
      {{200, -10.0, 10.0, "z#upoint p (GeV/c)"}, {600, 0, 600, "dE/dx (a. u.)"}, {10, 0, 100, "centrality"}});

    registryKa.add(
      "dEdx_vs_Momentum_Ka_m0200", "dE/dx", HistType::kTH3F,
      {{200, -10.0, 10.0, "z#upoint p (GeV/c)"}, {600, 0, 600, "dE/dx (a. u.)"}, {10, 0, 100, "centrality"}});

    registryKa.add(
      "dEdx_vs_Momentum_Ka_p0002", "dE/dx", HistType::kTH3F,
      {{200, -10.0, 10.0, "z#upoint p (GeV/c)"}, {600, 0, 600, "dE/dx (a. u.)"}, {10, 0, 100, "centrality"}});

    registryKa.add(
      "dEdx_vs_Momentum_Ka_p0204", "dE/dx", HistType::kTH3F,
      {{200, -10.0, 10.0, "z#upoint p (GeV/c)"}, {600, 0, 600, "dE/dx (a. u.)"}, {10, 0, 100, "centrality"}});

    registryKa.add(
      "dEdx_vs_Momentum_Ka_p0406", "dE/dx", HistType::kTH3F,
      {{200, -10.0, 10.0, "z#upoint p (GeV/c)"}, {600, 0, 600, "dE/dx (a. u.)"}, {10, 0, 100, "centrality"}});

    registryKa.add(
      "dEdx_vs_Momentum_Ka_p0608", "dE/dx", HistType::kTH3F,
      {{200, -10.0, 10.0, "z#upoint p (GeV/c)"}, {600, 0, 600, "dE/dx (a. u.)"}, {10, 0, 100, "centrality"}});

    // Protons
    registryPr.add(
      "dEdx_vs_Momentum_Pr_m0806", "dE/dx", HistType::kTH3F,
      {{200, -10.0, 10.0, "z#upoint p (GeV/c)"}, {1000, 0, 1000, "dE/dx (a. u.)"}, {10, 0, 100, "centrality"}});

    registryPr.add(
      "dEdx_vs_Momentum_Pr_m0604", "dE/dx", HistType::kTH3F,
      {{200, -10.0, 10.0, "z#upoint p (GeV/c)"}, {1000, 0, 1000, "dE/dx (a. u.)"}, {10, 0, 100, "centrality"}});

    registryPr.add(
      "dEdx_vs_Momentum_Pr_m0402", "dE/dx", HistType::kTH3F,
      {{200, -10.0, 10.0, "z#upoint p (GeV/c)"}, {1000, 0, 1000, "dE/dx (a. u.)"}, {10, 0, 100, "centrality"}});

    registryPr.add(
      "dEdx_vs_Momentum_Pr_m0200", "dE/dx", HistType::kTH3F,
      {{200, -10.0, 10.0, "z#upoint p (GeV/c)"}, {1000, 0, 1000, "dE/dx (a. u.)"}, {10, 0, 100, "centrality"}});

    registryPr.add(
      "dEdx_vs_Momentum_Pr_p0002", "dE/dx", HistType::kTH3F,
      {{200, -10.0, 10.0, "z#upoint p (GeV/c)"}, {1000, 0, 1000, "dE/dx (a. u.)"}, {10, 0, 100, "centrality"}});

    registryPr.add(
      "dEdx_vs_Momentum_Pr_p0204", "dE/dx", HistType::kTH3F,
      {{200, -10.0, 10.0, "z#upoint p (GeV/c)"}, {1000, 0, 1000, "dE/dx (a. u.)"}, {10, 0, 100, "centrality"}});

    registryPr.add(
      "dEdx_vs_Momentum_Pr_p0406", "dE/dx", HistType::kTH3F,
      {{200, -10.0, 10.0, "z#upoint p (GeV/c)"}, {1000, 0, 1000, "dE/dx (a. u.)"}, {10, 0, 100, "centrality"}});

    registryPr.add(
      "dEdx_vs_Momentum_Pr_p0608", "dE/dx", HistType::kTH3F,
      {{200, -10.0, 10.0, "z#upoint p (GeV/c)"}, {1000, 0, 1000, "dE/dx (a. u.)"}, {10, 0, 100, "centrality"}});

    // Helium-3
    registryHe.add(
      "dEdx_vs_Momentum_He_m0806", "dE/dx", HistType::kTH3F,
      {{200, -10.0, 10.0, "z#upoint p (GeV/c)"}, {1000, 0, 1500, "dE/dx (a. u.)"}, {10, 0, 100, "centrality"}});

    registryHe.add(
      "dEdx_vs_Momentum_He_m0604", "dE/dx", HistType::kTH3F,
      {{200, -10.0, 10.0, "z#upoint p (GeV/c)"}, {1000, 0, 1500, "dE/dx (a. u.)"}, {10, 0, 100, "centrality"}});

    registryHe.add(
      "dEdx_vs_Momentum_He_m0402", "dE/dx", HistType::kTH3F,
      {{200, -10.0, 10.0, "z#upoint p (GeV/c)"}, {1000, 0, 1500, "dE/dx (a. u.)"}, {10, 0, 100, "centrality"}});

    registryHe.add(
      "dEdx_vs_Momentum_He_m0200", "dE/dx", HistType::kTH3F,
      {{200, -10.0, 10.0, "z#upoint p (GeV/c)"}, {1000, 0, 1500, "dE/dx (a. u.)"}, {10, 0, 100, "centrality"}});

    registryHe.add(
      "dEdx_vs_Momentum_He_p0002", "dE/dx", HistType::kTH3F,
      {{200, -10.0, 10.0, "z#upoint p (GeV/c)"}, {1000, 0, 1500, "dE/dx (a. u.)"}, {10, 0, 100, "centrality"}});

    registryHe.add(
      "dEdx_vs_Momentum_He_p0204", "dE/dx", HistType::kTH3F,
      {{200, -10.0, 10.0, "z#upoint p (GeV/c)"}, {1000, 0, 1500, "dE/dx (a. u.)"}, {10, 0, 100, "centrality"}});

    registryHe.add(
      "dEdx_vs_Momentum_He_p0406", "dE/dx", HistType::kTH3F,
      {{200, -10.0, 10.0, "z#upoint p (GeV/c)"}, {1000, 0, 1500, "dE/dx (a. u.)"}, {10, 0, 100, "centrality"}});

    registryHe.add(
      "dEdx_vs_Momentum_He_p0608", "dE/dx", HistType::kTH3F,
      {{200, -10.0, 10.0, "z#upoint p (GeV/c)"}, {1000, 0, 1500, "dE/dx (a. u.)"}, {10, 0, 100, "centrality"}});

    // Event Counter
    registryCh.add("histRecVtxZData", "collision z position", HistType::kTH1F, {{100, -20.0, +20.0, "z_{vtx} (cm)"}});
  }

  // Single-Track Selection
  template <typename T1, typename C>
  bool passedSingleTrackSelection(const T1& track, const C& collision)
  {
    // Single-Track Selections
    if (!track.hasTPC())
      return false;
    if (track.tpcNClsFound() < minTPCnClsFound)
      return false;
    if (track.tpcNClsCrossedRows() < minNCrossedRowsTPC)
      return false;
    if (track.tpcChi2NCl() > maxChi2TPC)
      return false;
    if (track.eta() < etaMin || track.eta() > etaMax)
      return false;

    return true;
  }

  // General V0 Selections
  template <typename T1, typename C>
  bool passedV0Selection(const T1& v0, const C& collision)
  {
    if (v0.v0cosPA(collision.posX(), collision.posY(), collision.posZ()) < v0cospaMin)
      return false;
    if (v0.v0radius() < minimumV0Radius || v0.v0radius() > maximumV0Radius)
      return false;

    return true;
  }

  // K0s Selections
  template <typename T1, typename T2, typename C>
  bool passedK0Selection(const T1& v0, const T2& ntrack, const T2& ptrack,
                         const C& collision)
  {
    // Single-Track Selections
    if (!passedSingleTrackSelection(ptrack, collision))
      return false;
    if (!passedSingleTrackSelection(ntrack, collision))
      return false;

    if (ptrack.tpcInnerParam() > 0.6) {
      if (!ptrack.hasTOF())
        return false;
      if (TMath::Abs(ptrack.tofNSigmaPi()) > nsigmaTOFmax)
        return false;
    }

    if (ntrack.tpcInnerParam() > 0.6) {
      if (!ntrack.hasTOF())
        return false;
      if (TMath::Abs(ntrack.tofNSigmaPi()) > nsigmaTOFmax)
        return false;
    }

    // Invariant-Mass Selection
    if (v0.mK0Short() < minMassK0s || v0.mK0Short() > maxMassK0s)
      return false;

    return true;
  }

  // Lambda Selections
  template <typename T1, typename T2, typename C>
  bool passedLambdaSelection(const T1& v0, const T2& ntrack, const T2& ptrack,
                             const C& collision)
  {
    // Single-Track Selections
    if (!passedSingleTrackSelection(ptrack, collision))
      return false;
    if (!passedSingleTrackSelection(ntrack, collision))
      return false;

    if (ptrack.tpcInnerParam() > 0.6) {
      if (!ptrack.hasTOF())
        return false;
      if (TMath::Abs(ptrack.tofNSigmaPr()) > nsigmaTOFmax)
        return false;
    }

    if (ntrack.tpcInnerParam() > 0.6) {
      if (!ntrack.hasTOF())
        return false;
      if (TMath::Abs(ntrack.tofNSigmaPi()) > nsigmaTOFmax)
        return false;
    }

    // Invariant-Mass Selection
    if (v0.mLambda() < minMassLambda || v0.mLambda() > maxMassLambda)
      return false;

    return true;
  }

  // AntiLambda Selections
  template <typename T1, typename T2, typename C>
  bool passedAntiLambdaSelection(const T1& v0, const T2& ntrack,
                                 const T2& ptrack, const C& collision)
  {

    // Single-Track Selections
    if (!passedSingleTrackSelection(ptrack, collision))
      return false;
    if (!passedSingleTrackSelection(ntrack, collision))
      return false;

    if (ptrack.tpcInnerParam() > 0.6) {
      if (!ptrack.hasTOF())
        return false;
      if (TMath::Abs(ptrack.tofNSigmaPi()) > nsigmaTOFmax)
        return false;
    }

    if (ntrack.tpcInnerParam() > 0.6) {
      if (!ntrack.hasTOF())
        return false;
      if (TMath::Abs(ntrack.tofNSigmaPr()) > nsigmaTOFmax)
        return false;
    }

    // Invariant-Mass Selection
    if (v0.mAntiLambda() < minMassLambda || v0.mAntiLambda() > maxMassLambda)
      return false;

    return true;
  }

  // Process Data
  void process(SelectedCollisions::iterator const& collision,
               aod::V0Datas const& fullV0s, PIDTracks const& tracks)
  {
    // Event Selection
    if (!collision.sel8())
      return;

    // Event Counter
    registryCh.fill(HIST("histRecVtxZData"), collision.posZ());

    // Centrality
    float centrality = collision.centFT0C();

    // Kaons
    for (auto& trk : tracks) {

      if (!passedSingleTrackSelection(trk, collision))
        continue;
      if (!trk.passedTPCRefit())
        continue;
      float signedP = trk.sign() * trk.tpcInnerParam();

      // Charged Particles
      registryCh.fill(HIST("dEdx_vs_Momentum"), signedP, trk.tpcSignal());

      // Kaons
      if (trk.tpcInnerParam() > 0.4 && trk.hasTOF() && TMath::Abs(trk.tofNSigmaKa()) < 2.0) {
        if (trk.eta() > -0.8 && trk.eta() < -0.6)
          registryKa.fill(HIST("dEdx_vs_Momentum_Ka_m0806"), signedP, trk.tpcSignal(), centrality);
        if (trk.eta() > -0.6 && trk.eta() < -0.4)
          registryKa.fill(HIST("dEdx_vs_Momentum_Ka_m0604"), signedP, trk.tpcSignal(), centrality);
        if (trk.eta() > -0.4 && trk.eta() < -0.2)
          registryKa.fill(HIST("dEdx_vs_Momentum_Ka_m0402"), signedP, trk.tpcSignal(), centrality);
        if (trk.eta() > -0.2 && trk.eta() < 0.0)
          registryKa.fill(HIST("dEdx_vs_Momentum_Ka_m0200"), signedP, trk.tpcSignal(), centrality);
        if (trk.eta() > 0.0 && trk.eta() < 0.2)
          registryKa.fill(HIST("dEdx_vs_Momentum_Ka_p0002"), signedP, trk.tpcSignal(), centrality);
        if (trk.eta() > 0.2 && trk.eta() < 0.4)
          registryKa.fill(HIST("dEdx_vs_Momentum_Ka_p0204"), signedP, trk.tpcSignal(), centrality);
        if (trk.eta() > 0.4 && trk.eta() < 0.6)
          registryKa.fill(HIST("dEdx_vs_Momentum_Ka_p0406"), signedP, trk.tpcSignal(), centrality);
        if (trk.eta() > 0.6 && trk.eta() < 0.8)
          registryKa.fill(HIST("dEdx_vs_Momentum_Ka_p0608"), signedP, trk.tpcSignal(), centrality);
      }

      if (trk.tpcInnerParam() < 0.4) {
        if (trk.eta() > -0.8 && trk.eta() < -0.6)
          registryKa.fill(HIST("dEdx_vs_Momentum_Ka_m0806"), signedP, trk.tpcSignal(), centrality);
        if (trk.eta() > -0.6 && trk.eta() < -0.4)
          registryKa.fill(HIST("dEdx_vs_Momentum_Ka_m0604"), signedP, trk.tpcSignal(), centrality);
        if (trk.eta() > -0.4 && trk.eta() < -0.2)
          registryKa.fill(HIST("dEdx_vs_Momentum_Ka_m0402"), signedP, trk.tpcSignal(), centrality);
        if (trk.eta() > -0.2 && trk.eta() < 0.0)
          registryKa.fill(HIST("dEdx_vs_Momentum_Ka_m0200"), signedP, trk.tpcSignal(), centrality);
        if (trk.eta() > 0.0 && trk.eta() < 0.2)
          registryKa.fill(HIST("dEdx_vs_Momentum_Ka_p0002"), signedP, trk.tpcSignal(), centrality);
        if (trk.eta() > 0.2 && trk.eta() < 0.4)
          registryKa.fill(HIST("dEdx_vs_Momentum_Ka_p0204"), signedP, trk.tpcSignal(), centrality);
        if (trk.eta() > 0.4 && trk.eta() < 0.6)
          registryKa.fill(HIST("dEdx_vs_Momentum_Ka_p0406"), signedP, trk.tpcSignal(), centrality);
        if (trk.eta() > 0.6 && trk.eta() < 0.8)
          registryKa.fill(HIST("dEdx_vs_Momentum_Ka_p0608"), signedP, trk.tpcSignal(), centrality);
      }

      // Helium-3
      if (trk.tpcSignal() > 180 && trk.tpcInnerParam() > 0.6 && trk.tpcNSigmaDe() > 3.0) {
        if (trk.eta() > -0.8 && trk.eta() < -0.6)
          registryHe.fill(HIST("dEdx_vs_Momentum_He_m0806"), signedP, trk.tpcSignal(), centrality);
        if (trk.eta() > -0.6 && trk.eta() < -0.4)
          registryHe.fill(HIST("dEdx_vs_Momentum_He_m0604"), signedP, trk.tpcSignal(), centrality);
        if (trk.eta() > -0.4 && trk.eta() < -0.2)
          registryHe.fill(HIST("dEdx_vs_Momentum_He_m0402"), signedP, trk.tpcSignal(), centrality);
        if (trk.eta() > -0.2 && trk.eta() < 0.0)
          registryHe.fill(HIST("dEdx_vs_Momentum_He_m0200"), signedP, trk.tpcSignal(), centrality);
        if (trk.eta() > 0.0 && trk.eta() < 0.2)
          registryHe.fill(HIST("dEdx_vs_Momentum_He_p0002"), signedP, trk.tpcSignal(), centrality);
        if (trk.eta() > 0.2 && trk.eta() < 0.4)
          registryHe.fill(HIST("dEdx_vs_Momentum_He_p0204"), signedP, trk.tpcSignal(), centrality);
        if (trk.eta() > 0.4 && trk.eta() < 0.6)
          registryHe.fill(HIST("dEdx_vs_Momentum_He_p0406"), signedP, trk.tpcSignal(), centrality);
        if (trk.eta() > 0.6 && trk.eta() < 0.8)
          registryHe.fill(HIST("dEdx_vs_Momentum_He_p0608"), signedP, trk.tpcSignal(), centrality);
      }
    }

    // Loop over Reconstructed V0s
    for (auto& v0 : fullV0s) {

      // Standard V0 Selections
      if (!passedV0Selection(v0, collision)) {
        continue;
      }

      if (v0.dcaV0daughters() > dcaV0DaughtersMax) {
        continue;
      }

      // Positive and Negative Tracks
      const auto& posTrack = v0.posTrack_as<PIDTracks>();
      const auto& negTrack = v0.negTrack_as<PIDTracks>();

      if (!posTrack.passedTPCRefit())
        continue;
      if (!negTrack.passedTPCRefit())
        continue;

      float signedPpos = posTrack.sign() * posTrack.tpcInnerParam();
      float signedPneg = negTrack.sign() * negTrack.tpcInnerParam();

      // K0s Selection
      if (passedK0Selection(v0, negTrack, posTrack, collision)) {

        if (negTrack.eta() > -0.8 && negTrack.eta() < -0.6)
          registryPi.fill(HIST("dEdx_vs_Momentum_Pi_m0806"), signedPneg, negTrack.tpcSignal(), centrality);
        if (negTrack.eta() > -0.6 && negTrack.eta() < -0.4)
          registryPi.fill(HIST("dEdx_vs_Momentum_Pi_m0604"), signedPneg, negTrack.tpcSignal(), centrality);
        if (negTrack.eta() > -0.4 && negTrack.eta() < -0.2)
          registryPi.fill(HIST("dEdx_vs_Momentum_Pi_m0402"), signedPneg, negTrack.tpcSignal(), centrality);
        if (negTrack.eta() > -0.2 && negTrack.eta() < 0.0)
          registryPi.fill(HIST("dEdx_vs_Momentum_Pi_m0200"), signedPneg, negTrack.tpcSignal(), centrality);
        if (negTrack.eta() > 0.0 && negTrack.eta() < 0.2)
          registryPi.fill(HIST("dEdx_vs_Momentum_Pi_p0002"), signedPneg, negTrack.tpcSignal(), centrality);
        if (negTrack.eta() > 0.2 && negTrack.eta() < 0.4)
          registryPi.fill(HIST("dEdx_vs_Momentum_Pi_p0204"), signedPneg, negTrack.tpcSignal(), centrality);
        if (negTrack.eta() > 0.4 && negTrack.eta() < 0.6)
          registryPi.fill(HIST("dEdx_vs_Momentum_Pi_p0406"), signedPneg, negTrack.tpcSignal(), centrality);
        if (negTrack.eta() > 0.6 && negTrack.eta() < 0.8)
          registryPi.fill(HIST("dEdx_vs_Momentum_Pi_p0608"), signedPneg, negTrack.tpcSignal(), centrality);

        if (posTrack.eta() > -0.8 && posTrack.eta() < -0.6)
          registryPi.fill(HIST("dEdx_vs_Momentum_Pi_m0806"), signedPpos, posTrack.tpcSignal(), centrality);
        if (posTrack.eta() > -0.6 && posTrack.eta() < -0.4)
          registryPi.fill(HIST("dEdx_vs_Momentum_Pi_m0604"), signedPpos, posTrack.tpcSignal(), centrality);
        if (posTrack.eta() > -0.4 && posTrack.eta() < -0.2)
          registryPi.fill(HIST("dEdx_vs_Momentum_Pi_m0402"), signedPpos, posTrack.tpcSignal(), centrality);
        if (posTrack.eta() > -0.2 && posTrack.eta() < 0.0)
          registryPi.fill(HIST("dEdx_vs_Momentum_Pi_m0200"), signedPpos, posTrack.tpcSignal(), centrality);
        if (posTrack.eta() > 0.0 && posTrack.eta() < 0.2)
          registryPi.fill(HIST("dEdx_vs_Momentum_Pi_p0002"), signedPpos, posTrack.tpcSignal(), centrality);
        if (posTrack.eta() > 0.2 && posTrack.eta() < 0.4)
          registryPi.fill(HIST("dEdx_vs_Momentum_Pi_p0204"), signedPpos, posTrack.tpcSignal(), centrality);
        if (posTrack.eta() > 0.4 && posTrack.eta() < 0.6)
          registryPi.fill(HIST("dEdx_vs_Momentum_Pi_p0406"), signedPpos, posTrack.tpcSignal(), centrality);
        if (posTrack.eta() > 0.6 && posTrack.eta() < 0.8)
          registryPi.fill(HIST("dEdx_vs_Momentum_Pi_p0608"), signedPpos, posTrack.tpcSignal(), centrality);
      }

      // Lambda Selection
      if (passedLambdaSelection(v0, negTrack, posTrack, collision)) {

        if (posTrack.eta() > -0.8 && posTrack.eta() < -0.6)
          registryPr.fill(HIST("dEdx_vs_Momentum_Pr_m0806"), signedPpos, posTrack.tpcSignal(), centrality);
        if (posTrack.eta() > -0.6 && posTrack.eta() < -0.4)
          registryPr.fill(HIST("dEdx_vs_Momentum_Pr_m0604"), signedPpos, posTrack.tpcSignal(), centrality);
        if (posTrack.eta() > -0.4 && posTrack.eta() < -0.2)
          registryPr.fill(HIST("dEdx_vs_Momentum_Pr_m0402"), signedPpos, posTrack.tpcSignal(), centrality);
        if (posTrack.eta() > -0.2 && posTrack.eta() < 0.0)
          registryPr.fill(HIST("dEdx_vs_Momentum_Pr_m0200"), signedPpos, posTrack.tpcSignal(), centrality);
        if (posTrack.eta() > 0.0 && posTrack.eta() < 0.2)
          registryPr.fill(HIST("dEdx_vs_Momentum_Pr_p0002"), signedPpos, posTrack.tpcSignal(), centrality);
        if (posTrack.eta() > 0.2 && posTrack.eta() < 0.4)
          registryPr.fill(HIST("dEdx_vs_Momentum_Pr_p0204"), signedPpos, posTrack.tpcSignal(), centrality);
        if (posTrack.eta() > 0.4 && posTrack.eta() < 0.6)
          registryPr.fill(HIST("dEdx_vs_Momentum_Pr_p0406"), signedPpos, posTrack.tpcSignal(), centrality);
        if (posTrack.eta() > 0.6 && posTrack.eta() < 0.8)
          registryPr.fill(HIST("dEdx_vs_Momentum_Pr_p0608"), signedPpos, posTrack.tpcSignal(), centrality);

        if (negTrack.eta() > -0.8 && negTrack.eta() < -0.6)
          registryPi.fill(HIST("dEdx_vs_Momentum_Pi_m0806"), signedPneg, negTrack.tpcSignal(), centrality);
        if (negTrack.eta() > -0.6 && negTrack.eta() < -0.4)
          registryPi.fill(HIST("dEdx_vs_Momentum_Pi_m0604"), signedPneg, negTrack.tpcSignal(), centrality);
        if (negTrack.eta() > -0.4 && negTrack.eta() < -0.2)
          registryPi.fill(HIST("dEdx_vs_Momentum_Pi_m0402"), signedPneg, negTrack.tpcSignal(), centrality);
        if (negTrack.eta() > -0.2 && negTrack.eta() < 0.0)
          registryPi.fill(HIST("dEdx_vs_Momentum_Pi_m0200"), signedPneg, negTrack.tpcSignal(), centrality);
        if (negTrack.eta() > 0.0 && negTrack.eta() < 0.2)
          registryPi.fill(HIST("dEdx_vs_Momentum_Pi_p0002"), signedPneg, negTrack.tpcSignal(), centrality);
        if (negTrack.eta() > 0.2 && negTrack.eta() < 0.4)
          registryPi.fill(HIST("dEdx_vs_Momentum_Pi_p0204"), signedPneg, negTrack.tpcSignal(), centrality);
        if (negTrack.eta() > 0.4 && negTrack.eta() < 0.6)
          registryPi.fill(HIST("dEdx_vs_Momentum_Pi_p0406"), signedPneg, negTrack.tpcSignal(), centrality);
        if (negTrack.eta() > 0.6 && negTrack.eta() < 0.8)
          registryPi.fill(HIST("dEdx_vs_Momentum_Pi_p0608"), signedPneg, negTrack.tpcSignal(), centrality);
      }

      // AntiLambda Selection
      if (passedAntiLambdaSelection(v0, negTrack, posTrack, collision)) {

        if (negTrack.eta() > -0.8 && negTrack.eta() < -0.6)
          registryPr.fill(HIST("dEdx_vs_Momentum_Pr_m0806"), signedPneg, negTrack.tpcSignal(), centrality);
        if (negTrack.eta() > -0.6 && negTrack.eta() < -0.4)
          registryPr.fill(HIST("dEdx_vs_Momentum_Pr_m0604"), signedPneg, negTrack.tpcSignal(), centrality);
        if (negTrack.eta() > -0.4 && negTrack.eta() < -0.2)
          registryPr.fill(HIST("dEdx_vs_Momentum_Pr_m0402"), signedPneg, negTrack.tpcSignal(), centrality);
        if (negTrack.eta() > -0.2 && negTrack.eta() < 0.0)
          registryPr.fill(HIST("dEdx_vs_Momentum_Pr_m0200"), signedPneg, negTrack.tpcSignal(), centrality);
        if (negTrack.eta() > 0.0 && negTrack.eta() < 0.2)
          registryPr.fill(HIST("dEdx_vs_Momentum_Pr_p0002"), signedPneg, negTrack.tpcSignal(), centrality);
        if (negTrack.eta() > 0.2 && negTrack.eta() < 0.4)
          registryPr.fill(HIST("dEdx_vs_Momentum_Pr_p0204"), signedPneg, negTrack.tpcSignal(), centrality);
        if (negTrack.eta() > 0.4 && negTrack.eta() < 0.6)
          registryPr.fill(HIST("dEdx_vs_Momentum_Pr_p0406"), signedPneg, negTrack.tpcSignal(), centrality);
        if (negTrack.eta() > 0.6 && negTrack.eta() < 0.8)
          registryPr.fill(HIST("dEdx_vs_Momentum_Pr_p0608"), signedPneg, negTrack.tpcSignal(), centrality);

        if (posTrack.eta() > -0.8 && posTrack.eta() < -0.6)
          registryPi.fill(HIST("dEdx_vs_Momentum_Pi_m0806"), signedPpos, posTrack.tpcSignal(), centrality);
        if (posTrack.eta() > -0.6 && posTrack.eta() < -0.4)
          registryPi.fill(HIST("dEdx_vs_Momentum_Pi_m0604"), signedPpos, posTrack.tpcSignal(), centrality);
        if (posTrack.eta() > -0.4 && posTrack.eta() < -0.2)
          registryPi.fill(HIST("dEdx_vs_Momentum_Pi_m0402"), signedPpos, posTrack.tpcSignal(), centrality);
        if (posTrack.eta() > -0.2 && posTrack.eta() < 0.0)
          registryPi.fill(HIST("dEdx_vs_Momentum_Pi_m0200"), signedPpos, posTrack.tpcSignal(), centrality);
        if (posTrack.eta() > 0.0 && posTrack.eta() < 0.2)
          registryPi.fill(HIST("dEdx_vs_Momentum_Pi_p0002"), signedPpos, posTrack.tpcSignal(), centrality);
        if (posTrack.eta() > 0.2 && posTrack.eta() < 0.4)
          registryPi.fill(HIST("dEdx_vs_Momentum_Pi_p0204"), signedPpos, posTrack.tpcSignal(), centrality);
        if (posTrack.eta() > 0.4 && posTrack.eta() < 0.6)
          registryPi.fill(HIST("dEdx_vs_Momentum_Pi_p0406"), signedPpos, posTrack.tpcSignal(), centrality);
        if (posTrack.eta() > 0.6 && posTrack.eta() < 0.8)
          registryPi.fill(HIST("dEdx_vs_Momentum_Pi_p0608"), signedPpos, posTrack.tpcSignal(), centrality);
      }
    }
  }
};

WorkflowSpec defineDataProcessing(ConfigContext const& cfgc)
{
  return WorkflowSpec{adaptAnalysisTask<tpc_dedx_qa>(cfgc)};
}
