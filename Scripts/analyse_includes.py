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
@brief  Analyse header includes.
@author Vít Kučera <vit.kucera@cern.ch>, Inha University
@date   2025-12-09
"""

from pathlib import Path
import re
import sys


if len(sys.argv) == 1:
    raise IOError("Provide a project base path.")

dir_base = Path(sys.argv[1])
if not dir_base.is_dir():
    raise IOError("Provided path does not exist.")

headers = dict.fromkeys(sorted(dir_base.rglob("*.h")), 0)
path_dir_base = str(dir_base) + "/"
headers_local = dict.fromkeys([str(p).removeprefix(path_dir_base) for p in headers if not p.name.endswith("LinkDef.h")], 0)

for path in dir_base.rglob("*"):
    if not re.search(r"^.+\.(h|cxx)$", str(path)):
        continue
    with path.open() as file:
        for i_line, line in enumerate(file.readlines()):
            if not (match := re.match(r"#include ([\"<])(.+)[\">]", line)):
                continue
            location = f"{path}:{i_line + 1}"
            header = match.group(2)
            style = match.group(1)
            is_local = False
            if header in headers_local:
                # print(f"{location}: Known header: {header}")
                headers_local[header] += 1
                is_local = True
            else:
                for h_local in headers_local:
                    if h_local.endswith(f"/{header}"):
                        is_local = True
                        headers_local[h_local] += 1
                        if str(path.parent / header).removeprefix(path_dir_base) != h_local:
                            print(f"{location}: Incomplete path {header} to {h_local} [incomplete-path]")
                        break
            if is_local and style == "<":
                print(f"{location}: Wrong include style for project header {header} [wrong-style-local]")
                continue
            if not is_local and style == "\"":
                print(f"{location}: Wrong include style for external header {header} [wrong-style-external]")

for header, count in headers_local.items():
    if count == 0:
        print(f"{header}:0: Unused header [unused]")
