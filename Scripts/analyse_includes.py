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

# Covered cases:

# Path is expl. relative. -> Explicit relative path
# Path is expl. relative and does not end with ".h" -> Suspicious suffix
# Path is expl. relative and does not match end of any absolute path. -> Not found
# Path is not expl. relative and does not match end of any absolute path. -> OK (external)
# Path matches an absolute path. -> OK (valid absolute path)

# Path matches a relative path in the same directory. -> OK (valid relative path)
# Path prefixed with "/include/" matches end of one absolute path. -> OK (special include directory)
# Path matches a relative path in a higher parent directory. -> Incomplete relative path
# Path is expl. relative and does not match a relative path in any parent directory. -> Wrong relative path
# Path is not expl. relative and matches end of one absolute path. -> Incomplete absolute path

# Path matches end of multiple absolute paths.
#   -> There are multiple closest absolute paths. -> Ambiguous path
#   -> There is one closest absolute path. -> Incomplete absolute path

from pathlib import Path
import re
import sys
import os


if len(sys.argv) == 1:
    raise IOError("Provide a project base path.")


class Resolver:
    def __init__(self, path_base: str):
        self.dir_base = Path(path_base)
        if not self.dir_base.is_dir():
            raise IOError("Provided path does not exist.")
        self.path_dir_base = str(self.dir_base) + "/"
        self.path_dir_base_absolute = str(self.dir_base.resolve()) + "/"
        self.headers_local = dict.fromkeys(
            [
                str(path).removeprefix(self.path_dir_base)
                for path in self.dir_base.rglob("*.h")
                if not path.name.endswith("LinkDef.h")
            ],
            0,
        )
        self.path: Path = Path()
        self.path_local: str = ""
        self.location: str = ""
        self.header: str = ""
        self.style: str = ""
        self.is_local: bool = False
        self.is_found: bool = False
        self.verbose: bool = True

    def __check_format(self) -> None:
        if self.is_local and self.style == "<":
            print(f"{self.location}: Wrong include format for project header {self.header} [wrong-format-local]")
            return
        if not self.is_local and self.style == '"':
            print(f"{self.location}: Wrong include format for external header {self.header} [wrong-format-external]")

    def __check_unused(self) -> None:
        for header, count in self.headers_local.items():
            if count == 0:
                print(f"{header}:0: Unused header [unused]")

    def __check_compiled(self) -> None:
        for path_cmake_lists in sorted(self.dir_base.rglob("*CMakeLists.txt")):
            # print(f"Checking {path_cmake_lists}")
            with path_cmake_lists.open() as file_cmake_lists:
                for i_line, line in enumerate(file_cmake_lists.readlines()):
                    if not (iterators := re.finditer(r" ([\w/\-\.]+\.h)", line)):
                        continue
                    matches = [it.group(1) for it in iterators]
                    if not matches:
                        continue
                    for match in matches:
                        self.header = match
                        # print(f"Found header {self.header}")
                        if self.header.startswith("."):
                            h_local = str((path_cmake_lists.parent / self.header).resolve()).removeprefix(
                                self.path_dir_base_absolute
                            )
                        else:
                            h_local = str(path_cmake_lists.parent / self.header).removeprefix(self.path_dir_base)
                        if h_local in self.headers_local:
                            self.location = f"{path_cmake_lists}:{i_line + 1}"
                            if self.verbose:
                                print(f"{self.location}: Compiled header {self.header} [compiled]")
                            self.headers_local[h_local] += 1

    def __process_include(self) -> None:
        # Check the explicit relative path.
        header_stripped = self.header
        is_explicit_relative = False
        if self.header.startswith("."):
            print(f"{self.location}: Explicit relative path {self.header} [explicit-relative]")
            if not self.header.endswith(".h"):
                print(f"{self.location}: Not a header suffix in {self.header} [suspicious-suffix]")
                return
            self.is_local = True
            is_explicit_relative = True
            # Strip any relative path made of "." and "/".
            header_stripped = self.header.lstrip("./")

        # Find all matching headers.
        candidates_h_local = [h_local for h_local in self.headers_local if h_local.endswith(f"/{header_stripped}")]
        n_candidates = len(candidates_h_local)

        if n_candidates == 0:
            if is_explicit_relative:
                print(f"{self.location}: Failed to find {self.header} [not-found]")
            else:
                if self.verbose:
                    print(f"{self.location}: External header {self.header} [ok-external]")
            return

        self.is_local = True

        # Check the absolute path.
        if self.header in self.headers_local:
            if self.verbose:
                print(f"{self.location}: Valid absolute path {self.header} [ok-absolute")
            self.headers_local[self.header] += 1
            self.is_found = True
            return

        # Check the relative path w.r.t. the same directory.
        if is_explicit_relative:
            h_local = str((self.path.parent / self.header).resolve()).removeprefix(self.path_dir_base_absolute)
        else:
            h_local = str(self.path.parent / self.header).removeprefix(self.path_dir_base)
        if h_local in self.headers_local:
            if self.verbose:
                print(f"{self.location}: Valid relative path {self.header} to {h_local} [ok-relative]")
            self.headers_local[h_local] += 1
            self.is_found = True
            return

        # Check for a path in a special "include" directory.
        if not is_explicit_relative:
            candidates_h_local_special = [
                h_local for h_local in candidates_h_local if h_local.endswith(f"/include/{self.header}")
            ]
            if len(candidates_h_local_special) == 1:
                h_local = candidates_h_local_special[0]
                if self.verbose:
                    print(
                        f"{self.location}: Valid relative path {self.header} to {h_local} in a special directory [ok-special]"
                    )
                self.headers_local[h_local] += 1
                self.is_found = True
                return

        # Check the relative path w.r.t. parent directories.
        for directory in self.path.parent.parents:
            if directory == self.path_dir_base:
                break
            if is_explicit_relative:
                h_local = str((directory / self.header).resolve()).removeprefix(self.path_dir_base_absolute)
            else:
                h_local = str(directory / self.header).removeprefix(self.path_dir_base)
            if h_local in candidates_h_local:
                print(
                    f"{self.location}: Incomplete relative path {self.header} to {h_local} [incomplete-relative-path]"
                )
                self.headers_local[h_local] += 1
                self.is_found = True
                return
        if is_explicit_relative:
            print(f"{self.location}: Wrong relative path {self.header} to {h_local} [wrong-relative-path]")
            return

        # Incomplete absolute path.
        if n_candidates == 1:
            h_local = candidates_h_local[0]
            print(f"{self.location}: Incomplete absolute path {self.header} to {h_local} [incomplete-absolute-path]")
            self.headers_local[h_local] += 1
            self.is_found = True
            return

        # Handle multiple path candidates.
        if n_candidates > 1:
            if self.verbose:
                print(
                    f"{self.location}: Found multiple candidates for {self.header}: {candidates_h_local} [multiple-candidates]"
                )
            # Find the header with the longest common path.
            len_prefix_max = 0
            len_prefix_all = []
            index_prefix_max = 0
            for i, candidate in enumerate(candidates_h_local):
                len_i = len(os.path.commonpath((candidate, self.path_local)))
                len_prefix_all.append(len_i)
                if len_i > len_prefix_max:
                    len_prefix_max = len_i
                    index_prefix_max = i
            if (
                len_prefix_max == 0 or len_prefix_all.count(len_prefix_max) > 1
            ):  # no common prefix or several longest of same length
                print(f"{self.location}: Ambiguous path {self.header} [ambiguous]")
                return
            h_local = candidates_h_local[index_prefix_max]
            if self.verbose:
                print(f"{self.location}: Best candidate for {self.header}: {h_local} [best-candidate]")
            print(f"{self.location}: Incomplete absolute path {self.header} to {h_local} [incomplete-absolute-path]")
            self.headers_local[h_local] += 1
            self.is_found = True
            return

        print(f"{self.location}: Unhandled case {self.header} [error-exception]")

    def run(self) -> None:
        for path in self.dir_base.rglob("*"):
            if not re.search(r"^.+\.(h|cxx|cu|c|C)$", str(path)):
                continue
            self.path = path
            self.path_local = str(path).removeprefix(self.path_dir_base)
            with self.path.open(encoding="utf-8") as file:
                for i_line, line in enumerate(file.readlines()):
                    if not (match := re.match(r"#include ([\"<])(.+)[\">]", line)):
                        continue
                    self.location = f"{path}:{i_line + 1}"
                    self.header = match.group(2)
                    self.style = match.group(1)
                    self.is_local = False
                    self.is_found = False
                    self.__process_include()
                    self.__check_format()
        self.__check_compiled()
        self.__check_unused()


resolver = Resolver(sys.argv[1])
resolver.run()
