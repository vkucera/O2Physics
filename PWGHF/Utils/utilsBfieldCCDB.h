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

/// \file utilsBfieldCCDB.h
/// \brief Utility to set the B field in analysis querying it from CCDB
/// \author Mattia Faggin <mfaggin@cern.ch>, University and INFN Padova, Italy

#ifndef PWGHF_UTILS_UTILSBFIELDCCDB_H_
#define PWGHF_UTILS_UTILSBFIELDCCDB_H_

#include <string> // std::string

#include "CCDB/BasicCCDBManager.h"
#include "DetectorsBase/MatLayerCylSet.h"
#include "DataFormatsParameters/GRPMagField.h"
#include "DataFormatsParameters/GRPObject.h"

/// \brief Retreive magnetic field from CCDB and initialise the field in the propagator
/// \param ccdb BasicCCDBManager object
/// \param ccdbPathGrp path to the GRP oject
/// \param timestamp timestamp
template <typename TGrp, typename TCcdb, typename TPath, typename TTimeStamp>
static void initField(TCcdb const& ccdb, TPath const& ccdbPathGrp, const TTimeStamp timestamp) {
  TGrp* grpo = ccdb->template getForTimeStamp<TGrp>(ccdbPathGrp, timestamp);
  if (!grpo) {
    LOGF(fatal, "Failed to retrieve GRP object for path %s and timestamp %llu", ccdbPathGrp, timestamp);
  }
  o2::base::Propagator::initFieldFromGRP(grpo);
}

/// \brief Sets up the GRP object for magnetic field (w/o matCorr for propagation)
/// \param bc bunch crossing
/// \param runNumber run number of the previous iteration. If at the current iteration it changes, then the GRP object is updated
/// \param ccdb BasicCCDBManager object
/// \param ccdbPathGrp path to the GRP oject
/// \param lut pointer to the MatLayerCylSet object
/// \param isRun2 tells whether we are analysing Run2 converted data or not (different GRP object type)
template <typename TBc>
static void initCCDB(TBc const& bc,
              int& runNumber,
              o2::framework::Service<o2::ccdb::BasicCCDBManager> const& ccdb,
              std::string const& ccdbPathGrp,
              const o2::base::MatLayerCylSet* lut,
              bool isRun2)
{
  if (runNumber == bc.runNumber()) {
    return;
  }
  runNumber = bc.runNumber();
  LOGF(info, "initCCDB function called (isRun2 = %s) for run %d", isRun2 ? "true" : "false", runNumber);
  auto timestamp = bc.timestamp();
  if (isRun2) { // Run 2 GRP object
    initField<o2::parameters::GRPObject>(ccdb, ccdbPathGrp, timestamp);
  } else { // Run 3 GRP object
    initField<o2::parameters::GRPMagField>(ccdb, ccdbPathGrp, timestamp);
  }
  auto bz = o2::base::Propagator::Instance()->getNominalBz();
  LOG(info) << "Retrieved GRP for run " << runNumber << " and timestamp " << timestamp << " with magnetic field of " << bz << " kG";
  if (lut) {
    o2::base::Propagator::Instance()->setMatLUT(lut);
  }
} /// end initCCDB

#endif // PWGHF_UTILS_UTILSBFIELDCCDB_H_
