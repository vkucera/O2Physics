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
@brief  Process a linter log file and make a Markdown report.

@author Vít Kučera <vit.kucera@cern.ch>, Inha University
@date   2025-11-16
"""

import argparse
from enum import Enum
import re


def main():
    """Main function"""

    class Repo(Enum):
        O2Physics = 1
        O2 = 2

    class Linter(Enum):
        ClangTidy = 1
        Cppcheck = 2
        Cpplint = 3
        O2Linter = 4
        GCC = 5

    class LinterSpec(Enum):
        Name = 1
        Regex = 2
        GroupLineOut = 3
        GroupPath = 4
        GroupSeverity = 5
        GroupCategory = 6
        IgnoredCategories = 7
        IgnoredSeverities = 8

    config = {
        Linter.GCC: {
            LinterSpec.Name: "GCC",
            LinterSpec.Regex: r"/home/\w+/alice/(sw/SOURCES/)?O2\w*/(\w+/0/)?((.+):\d+:\d+: (.+): .+ \[(-W.+)\])",
            LinterSpec.GroupLineOut: 3,
            LinterSpec.GroupPath: 4,
            LinterSpec.GroupSeverity: 5,
            LinterSpec.GroupCategory: 6,
            LinterSpec.IgnoredCategories: [],
            LinterSpec.IgnoredSeverities: ["note"],
        },
        Linter.ClangTidy: {
            LinterSpec.Name: "Clang-Tidy",
            LinterSpec.Regex: r"/home/\w+/alice/(sw/SOURCES/)?O2\w*/(\w+/0/)?((.+):\d+:\d+: (.+): .+ \[(.+)\])",
            LinterSpec.GroupLineOut: 3,
            LinterSpec.GroupPath: 4,
            LinterSpec.GroupSeverity: 5,
            LinterSpec.GroupCategory: 6,
            LinterSpec.IgnoredCategories: ["clang-diagnostic-unknown-pragmas"],
            LinterSpec.IgnoredSeverities: ["note"],
        },
        Linter.Cppcheck: {
            LinterSpec.Name: "Cppcheck",
            LinterSpec.Regex: r"((.+):\d+:\d+: (.+): .+ \[(.+)\])",
            LinterSpec.GroupLineOut: 1,
            LinterSpec.GroupPath: 2,
            LinterSpec.GroupSeverity: 3,
            LinterSpec.GroupCategory: 4,
            LinterSpec.IgnoredCategories: ["selfInitialization"],
            LinterSpec.IgnoredSeverities: [],
        },
        Linter.Cpplint: {
            LinterSpec.Name: "cpplint",
            LinterSpec.Regex: r"./((.+):\d+:  .+  \[(.+)\] \[\d+\])",
            LinterSpec.GroupLineOut: 1,
            LinterSpec.GroupPath: 2,
            LinterSpec.GroupCategory: 3,
            LinterSpec.IgnoredCategories: [],
            LinterSpec.IgnoredSeverities: [],
        },
        Linter.O2Linter: {
            LinterSpec.Name: "O2 linter",
            LinterSpec.Regex: r"./((.+):\d+: (info|warning|error): .+ \[(.+)\])",
            LinterSpec.GroupLineOut: 1,
            LinterSpec.GroupPath: 2,
            LinterSpec.GroupSeverity: 3,
            LinterSpec.GroupCategory: 4,
            LinterSpec.IgnoredCategories: [],
            LinterSpec.IgnoredSeverities: ["info", "warning"],
        },
    }

    names_repos: dict[str, Repo] = {
        "O2": Repo.O2,
        "O2Physics": Repo.O2Physics,
    }

    names_linters: dict[str, Linter] = {
        "gcc": Linter.GCC,
        "clang-tidy": Linter.ClangTidy,
        "cppcheck": Linter.Cppcheck,
        "cpplint": Linter.Cpplint,
        "o2-linter": Linter.O2Linter,
    }

    paths_ignored = [r"^PWGHF/ALICE3/", r"LinkDef\.h$"]

    parser = argparse.ArgumentParser(description="Process a linter output and make a Markdown report.")
    parser.add_argument("-p", dest="path", type=str, required=True, help="linter log file path")
    parser.add_argument("-l", dest="linter", type=str, required=True, choices=names_linters.keys(), help="linter")
    parser.add_argument("-r", dest="repo", type=str, required=True, choices=names_repos.keys(), help="repository")
    parser.add_argument(
        "-n", dest="n_lines", type=str, required=True, help="file with number of code lines per directory"
    )
    args = parser.parse_args()

    linter = names_linters[args.linter]
    repo = names_repos[args.repo]
    path_logfile = args.path
    path_n_lines = args.n_lines
    config_linter = config[linter]
    if repo == Repo.O2:
        if linter == Linter.O2Linter:
            config_linter[LinterSpec.IgnoredCategories] = [
                "const-ref-in-for-loop",
                "const-ref-in-process",
                "import-std-name",
                "name/configurable",
                "name/o2-table",
                "name/workflow-file",
                "o2-workflow-options",
                "pdg/database",
                "two-pi-add-subtract",
            ]
        elif linter == Linter.Cpplint:
            config_linter[LinterSpec.IgnoredCategories] = ["build/header_guard"]

    regex_message = config_linter[LinterSpec.Regex]
    categories_ignored = config_linter[LinterSpec.IgnoredCategories]
    severities_ignored = (
        config_linter[LinterSpec.IgnoredSeverities] if LinterSpec.IgnoredSeverities in config_linter else []
    )

    try:
        with open(path_logfile, encoding="utf-8") as file:
            content = file.readlines()
    except OSError as exc:
        raise OSError(f'Failed to open file "{path_logfile}".') from exc

    try:
        with open(path_n_lines, encoding="utf-8") as file:
            n_lines_code_per_dir = {}
            for line in file:
                key, val = line.strip().split()
                n_lines_code_per_dir[key] = int(val)
    except OSError as exc:
        raise OSError(f'Failed to open file "{path_n_lines}".') from exc

    counter_category: dict[str, int] = {}
    counter_directory: dict[str, int] = {}
    dic_issues: dict[str, dict[str, dict[str, int]]] = {}

    n_line_max = 0
    i_line = 0
    for line in content:
        if n_line_max > 0 and i_line == n_line_max:
            break
        if not (match := re.match(regex_message, line.strip())):
            continue
        line_out = match.group(config_linter[LinterSpec.GroupLineOut])
        path_file_code = match.group(config_linter[LinterSpec.GroupPath])
        severity = (
            match.group(config_linter[LinterSpec.GroupSeverity]) if LinterSpec.GroupSeverity in config_linter else ""
        )
        category = match.group(config_linter[LinterSpec.GroupCategory])
        dir_file_code = path_file_code.split("/")[0]
        if severity and severities_ignored and severity in severities_ignored:
            continue
        if category in categories_ignored:
            continue
        if any(re.search(p, path_file_code) for p in paths_ignored):
            continue
        i_line += 1

        # print(line_out)
        counter_category.setdefault(category, 0)
        counter_directory.setdefault(dir_file_code, 0)
        dic_issues.setdefault(dir_file_code, {})
        dic_issues[dir_file_code].setdefault(path_file_code, {})
        if line_out not in dic_issues[dir_file_code][path_file_code]:
            dic_issues[dir_file_code][path_file_code].setdefault(line_out, 1)
            counter_category[category] += 1
            counter_directory[dir_file_code] += 1
        # print(f"path: {path_file_code}")
        # print(f"category: {category}")
        # print(f"line out: {line_out}")

    print(f"# Report from {config_linter[LinterSpec.Name]}")

    n_issues_total = sum(counter_category.values())
    if n_issues_total == 0:
        print("\nNo issues found")
        return
    print("\n## Summary")

    # Counts per category
    print("\n### Per category")
    print("\n| category | issues |")
    print("|---|---|")
    len_gap = 0
    len_column = max(len(key) for key in counter_category) + len_gap
    for cat in sorted(counter_category.keys()):
        print(f"| `{cat}`{(len_column - len(cat)) * ' '} | {counter_category[cat]} |")
    cat_total = "total"
    print(f"| {cat_total}{(len_column - len(cat_total) + 2) * ' '} | {n_issues_total} |")

    # Counts per directory
    n_lines_norm = 1000
    print("\n### Per directory, per line")
    print(f"\n| directory | issues | issues per {n_lines_norm} lines |")
    print("|---|---|---|")
    len_column = max(len(key) for key in counter_directory) + len_gap
    n_lines_code_total = sum(n_lines_code_per_dir.values())
    for directory in sorted(counter_directory.keys()):
        n_lines_code = n_lines_code_per_dir[directory]
        print(
            f"| `{directory}`{(len_column - len(directory)) * ' '} | {counter_directory[directory]} | {counter_directory[directory] / n_lines_code * n_lines_norm:.3g} |"
        )
    print(
        f"| {cat_total}{(len_column - len(cat_total) + 2) * ' '} | {n_issues_total} | {n_issues_total / n_lines_code_total * n_lines_norm:.3g} |"
    )

    print("\n## Issues")
    for directory in sorted(dic_issues.keys()):
        print(f"\n### {directory}")
        for file in sorted(dic_issues[directory].keys()):
            print(f"\n#### `{file}`\n")
            print("```text")
            for line in dic_issues[directory][file]:
                print(line)
            print("```")


if __name__ == "__main__":
    main()
