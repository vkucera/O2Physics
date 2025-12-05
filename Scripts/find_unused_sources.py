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
@brief  Find C++ source files which do not appear in any CMake list.
@author Vít Kučera <vit.kucera@cern.ch>, Inha University
@date   2025-12-03
"""

from pathlib import Path
import re
import sys


if len(sys.argv) == 1:
    raise IOError("Provide a base path.")

dir_base = Path(sys.argv[1])
if not dir_base.is_dir():
    raise IOError("Provided path does not exist.")

name_cmake_lists = "CMakeLists.txt"

for path in sorted(dir_base.rglob("*.cxx")):
    # Files in Macros directories are not supposed to be in the O2Physics build.
    if "Macros" in path.parts:
        continue
    path_obj = Path(path)
    name_file_source = path_obj.name
    found = False
    # Look for the file name in the CMake lists in the parent directories.
    for directory in path_obj.parents:
        path_cmake_lists = directory / name_cmake_lists
        if not path_cmake_lists.is_file():
            continue
        with path_cmake_lists.open() as file_cmake_lists:
            for line in file_cmake_lists.readlines():
                if re.search(rf"(^|\W){name_file_source}", line):
                    found = True
                    break
        if found:
            break
        if directory == dir_base:
            print(path)
            break
