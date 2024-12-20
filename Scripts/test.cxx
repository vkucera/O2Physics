constexpr double PITwoThird = o2::constants::math::TwoPI / 3.; // should fail
constexpr double PITwoThird = 2. * o2::constants::math::PIThird; // should pass
cut_tof_nSigma->AddCut(GetAnalysisCut("lmee_commonDQEM_PID_TOF")); // M_PI should pass
if (MCtrack1.pdgCode() != 321 || !MCtrack1.isPhysicalPrimary()) // should fail
if (kDaughter.pdgCode() == -321) // should fail
