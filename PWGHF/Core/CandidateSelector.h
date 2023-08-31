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

/// \file CandidateSelector.h
/// \brief Class to apply candidate-level selections
///
/// \author Vít Kučera <vit.kucera@cern.ch>, Inha University

#ifndef PWGHF_CORE_CANDIDATESELECTOR_H_
#define PWGHF_CORE_CANDIDATESELECTOR_H_

#include <array>     // std::array
#include <iterator>  // std::distance
#include <string>    // std::string
#include <vector>    // std::vector

#include "Framework/Array2D.h"

class CutDefinition
{
public:
  // types of cut condition
  enum Type : int {
    Min = 0,
    Max,
    AbsMin,
    AbsMax,
    Bit
  };

  // cut names
  enum Name : int {
    CpaMin = 0,
    DecayLengthMin,
    EtaMax,
    NCutNames
  };

  CutDefinition() {
    // define labels of supported cuts
    // can we do this at compilation? std::map?
    cutLabels[CpaMin] = "cpa_min";
    cutLabels[DecayLengthMin] = "declen_min";
  }

  std::array<std::string, NCutNames> cutLabels; // labels of supported cuts (cut array column names)
private:
};

template <typename CandTable>
class CandidateSelector : CutDefinition
{
public:
  CandidateSelector();

    /// applies all cuts for a given candidate
    template <typename Cand>
    bool applySelection(const Cand& candidate)
    {
        for (size_t cutName = 0; cutName < CutDefinition::NCutNames; cutName++)
        {
            if (!TESTBIT(enabledCuts, cutName)) {
                continue;
            }
            if (!applyCut(candidate, cutName)) {
                return false;
            }
        }
        return true;
    }

private:
    // array of cuts
    o2::framework::LabeledArray<double> arrCuts;
    // array of pT bins
    std::vector<double> binsPt;
    // bitmap of activated cuts
    uint enabledCuts;

    /// set the cut array and pt bins and activate cuts
    void configure(const o2::framework::LabeledArray<double>& cuts, const std::vector<double>& bins) {
        /// set the cut array
        arrCuts = cuts;
        /// set the pt bins
        binsPt = bins;
        /// enable cuts by setting bits in enabledCuts
        enabledCuts = 0;
        for (const auto& label : arrCuts->getLabelsCols()) {
            // debug print "enabling cut: label"
            size_t cutName = 0; // cutName = position of label in cutLabels
            // throw fatal if label not found
            SETBIT(enabledCuts, cutName);
        }
        if (enabledCuts == 0) {
            // throw a warning about empty selection
        }
    }

    /// Finds pT bin in an array.
    /// \param bins  array of pT bins
    /// \param value  pT
    /// \return index of the pT bin
    /// \note Accounts for the offset so that pt bin array can be used to also configure a histogram axis.
    template <typename T1, typename T2>
    int findBin(T1 const& binsPt, T2 value)
    {
        if (value < binsPt->front()) {
            return -1;
        }
        if (value >= binsPt->back()) {
            return -1;
        }
        return std::distance(binsPt->begin(), std::upper_bound(binsPt->begin(), binsPt->end(), value)) - 1;
    }

    /// get the cut value from the cut array based on the pt bin and the cut name
    auto getCutValue(size_t binPt, const CutDefinition::Name& cutName) {
        return arrCuts->get(binPt, CutDefinition::cutLabels[cutName]);
    }

    /// makes the decision whether a value passes a cut
    template <typename Val>
    bool decide(Val value, Val cut, CutDefinition::Type type)
    {
        switch (type)
        {
            case CutDefinition::Min:
                return value >= cut;
                break;
            case CutDefinition::Max:
                return value <= cut;
                break;
            case CutDefinition::AbsMin:
                return std::abs(value) >= cut;
                break;
            case CutDefinition::AbsMax:
                return std::abs(value) <= cut;
                break;
            case CutDefinition::Bit:
                return TESTBIT(value, cut);
                break;
            default:
                // undefined cut type
                return false;
                break;
        }
    }

    /// applies the cut for a given cut name of a given candidate
    template <typename Cand>
    bool applyCut(const Cand& candidate, const CutDefinition::Name& cutName)
    {
        // debug print "Applying cut: CutDefinition::cutLabels[cutName]"
        // get pt bin
        size_t binPt = findBin(binsPt, candidate.pt());

        // This is where all the cuts have to be defined.
        // "if constexpr" may be needed based on the candidate type.
        switch (cutName)
        {
        case CutDefinition::CpaMin:
            return decide(candidate.cpa(), getCutValue(binPt, cutName), CutDefinition::Min);
            break;
        default:
            // undefined cut cut name
            return false;
            break;
        }
    }
};

#endif // PWGHF_CORE_CANDIDATESELECTOR_H_
