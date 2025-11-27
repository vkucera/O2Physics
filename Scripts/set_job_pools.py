#!/usr/bin/env python3

# Copyright 2019-2020 CERN and copyright holders of ALICE O2.
# See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
# All rights not expressly granted are reserved.
#
# This software is distributed under the terms of the GNU General Public
# License v3 (GPL Version 3), copied verbatim in the file "COPYING".
#
# In applying this license CERN does not waive the privileges and immunities
# granted to it by virtue of its status as an Intergovernmental Organization
# or submit itself to any jurisdiction.

"""!
@brief  Set compilation job pools
@author Vít Kučera <vit.kucera@cern.ch>, Inha University
@date   2025-11-25
"""

import argparse
import re
import pandas as pd
import numpy as np


def estimate_memory(time):
    """Estimate maximum compilation memory [GB] based on compilation time [s]."""
    return np.power(np.add(np.multiply(time, 0.12952722945789094), -0.8697921057987378), 1.0 / 1.8)


def assign_pool(memory: float, pools: list[tuple[str, float]]) -> str | None:
    """Assign a pool index based on memory [GB]"""
    for pool in pools:
        if memory <= pool[1]:
            return pool[0]
    return None


def extract_target_info(target: str) -> tuple[str | None, str | None]:
    """Extract path to CMakeLists.txt and the executable name from the target."""
    if not target:
        return None, None
    if not (match := re.match(r"(.+)\/CMakeFiles\/O2Physicsexe-analysis(tutorial)?-(.+).dir\/.+\.cxx\.o$", target)):
        return None, None
    directory = match.group(1)
    executable = match.group(3)
    cmake_lists = f"{directory}/CMakeLists.txt"
    if re.match(r"(Tutorials\/)?PWG[A-Z]{2}", directory):
        executable = executable[3:]
    # print(f"Got {directory} {executable}")
    return cmake_lists, executable


def set_pool(path_repo: str, cmake_lists: str, executable: str, pool: str):
    if not all((path_repo, cmake_lists, executable, pool)):
        return
    path_cmake = f"{path_repo}/{cmake_lists}"
    content_old: list[str] = []
    content_new: list[str] = []
    try:
        print(f"Opening file: {path_cmake}, setting pool {pool} to {executable}")
        with open(path_cmake, encoding="utf-8") as file_cmake:
            content_old = file_cmake.readlines()
    except OSError as exc:
        raise OSError(f'Failed to open to file "{path_cmake}".') from exc

    found_block_start = False
    pool_set = False
    variable_pool = "JOB_POOL"
    for i_line, line_old in enumerate(content_old):
        line_new = line_old
        if not found_block_start and re.match(rf"\s*o2physics_add_(dpl_workflow|executable)\({executable}\n", line_old):
            found_block_start = True
            print(f"Found start of block on line {i_line + 1}")
        elif not pool_set and found_block_start:
            if match := re.match(rf"\s*{variable_pool} (\w+)", line_old):
                pool_old = match.group(1)
                print(f"Replacing pool {pool_old} with {pool} on line {i_line + 1}")
                line_new = line_old.replace(f"{variable_pool} {pool_old}", f"{variable_pool} {pool}")
                pool_set = True
            elif match := re.match(r"(\s*).*\)", line_old):
                padding = match.group(1)
                print(f"Adding pool {pool} on line {i_line + 1}")
                line_new = line_old.replace(")", f"\n{padding}{variable_pool} {pool})")
                pool_set = True
        content_new.append(line_new)
    if not pool_set:
        raise ValueError(f"Could not set pool for {executable} in {path_cmake}")

    try:
        with open(path_cmake, "w", encoding="utf-8") as file_cmake:
            file_cmake.writelines(content_new)
    except OSError as exc:
        raise OSError(f'Failed to write in file "{path_cmake}".') from exc


def main():
    """Main function"""
    parser = argparse.ArgumentParser(description="Set compilation job pools")
    parser.add_argument("--ninja", type=str, required=True, help=".ninja_log file")
    parser.add_argument("--repo", type=str, required=True, help="repository directory")
    args = parser.parse_args()
    path_ninja_log = args.ninja
    path_repo = args.repo

    # Process the .ninja_log file.
    data_ninja_log = pd.read_csv(
        path_ninja_log,
        sep="\t",
        header=0,
        names=["start", "stop", "x", "target", "y"],
        usecols=["start", "stop", "target"],
    )
    data_ninja_log = data_ninja_log[data_ninja_log["target"].str.endswith(".cxx.o")]
    data_ninja_log["cmake_lists"], data_ninja_log["executable"] = zip(
        *data_ninja_log["target"].apply(extract_target_info)
    )
    # Calculate compilation time.
    data_ninja_log["duration"] = (data_ninja_log["stop"] - data_ninja_log["start"]) / 1000
    # Estimate compilation memory for targets based on their compilation time.
    data_ninja_log["memory"] = estimate_memory(data_ninja_log["duration"])
    # Assign pools to targets.
    pools: list[tuple[str, float]] = [
        ("three", 3.0),
        ("five", 5.0),
        ("seven", 7.0),
        ("ten", 10.0),
    ]  # maximum memory per job pool
    data_ninja_log["pool"] = data_ninja_log["memory"].apply(lambda m: assign_pool(m, pools))
    n_targets = [sum(data_ninja_log["pool"] == m) for m in range(len(pools) + 1)]  # counters of targets per pool

    print(n_targets, sum(n_targets))

    data_ninja_log.apply(lambda x: set_pool(path_repo, x["cmake_lists"], x["executable"], x["pool"]), axis=1)


if __name__ == "__main__":
    main()
