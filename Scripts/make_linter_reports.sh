#!/bin/bash

# Run all C++ code analysers and produce Markdown reports.

# Repository
repo="O2Physics"
# repo="O2"

# Path to the repository
path="." # Do not change

# Action switches
run_linters=1
make_reports=1

# Analyser switches
do_gcc=1
do_clang_tidy=1
do_cppcheck=1
do_cpplint=1
do_o2_linter=1
do_unused_files=1
do_includes=1

# File with number of C++ lines per directory
file_n_lines="n_lines.txt"

# Commands to execute the analyser for a file
cmd_gcc="rsync -avhq "$ALIBUILD_WORK_DIR/BUILD/${repo}-latest/log""
# cmd_clang_tidy="clang-tidy {}"
cmd_clang_tidy="clang-tidy {} -checks=bugprone-assignment-in-if-condition,bugprone-copy-constructor-init,bugprone-derived-method-shadowing-base-method,bugprone-fold-init-type,bugprone-incorrect-roundings,bugprone-integer-division,bugprone-misplaced-widening-cast,bugprone-narrowing-conversions,bugprone-non-zero-enum-to-bool-conversion,bugprone-redundant-branch-condition,bugprone-reserved-identifier,bugprone-suspicious-*,bugprone-swapped-arguments,bugprone-switch-missing-default-case,bugprone-throw-keyword-missing,bugprone-too-small-loop-variable,bugprone-unchecked-optional-access,bugprone-unused-local-non-trivial-variable,bugprone-unused-return-value,bugprone-use-after-move,cppcoreguidelines-avoid-const-or-ref-data-members,cppcoreguidelines-explicit-virtual-functions,cppcoreguidelines-init-variables,cppcoreguidelines-missing-std-forward,cppcoreguidelines-pro-type-const-cast,cppcoreguidelines-pro-type-cstyle-cast,cppcoreguidelines-pro-type-member-init,cppcoreguidelines-pro-type-reinterpret-cast,cppcoreguidelines-pro-type-union-access,cppcoreguidelines-slicing,cppcoreguidelines-special-member-functions,cppcoreguidelines-virtual-class-destructor,google-global-names-in-headers,misc-header-include-cycle,misc-include-cleaner,misc-misplaced-const,misc-override-with-different-visibility,misc-redundant-expression,misc-unused-alias-decls,misc-unused-parameters,misc-unused-using-decls"
cmd_cppcheck="cppcheck {} --language=c++ --std=c++20 --enable=warning --check-level=exhaustive --suppressions-list=cppcheck_config --force"
cmd_cpplint="cpplint --extensions=h,cxx,C {}"
cmd_o2_linter="python3 $HOME/alice/O2Physics/Scripts/o2_linter.py {}"
cmd_unused_files="python3 $HOME/alice/O2Physics/Scripts/find_unused_sources.py $path"
cmd_includes="python3 $HOME/alice/O2Physics/Scripts/analyse_includes.py $path"

# Call the script to convert the log file into a Markdown report.
make_report() {
    linter="$1"
    echo "Making report for $linter"
    python3 $HOME/alice/O2Physics/Scripts/process_linter_log.py -p "${linter}.log" -l "${linter}" -r "${repo}" -n "${file_n_lines}" > "report_${linter}.md"
}

print_date() { echo "$(date +"%F_%H-%M-%S")" "$@"; }

grep_logs() { grep -A 1 "parallel: Warning: This job was killed" "$@"; }

print_date "Start"

# Count lines of C++ code per directory.
while IFS= read -r -d '' dir; do echo "${dir/$path\//}" "$(find "$dir" \( -path "${path}/PWGHF/ALICE3" \) -prune -o \( -name "*.h" -o -name "*.cxx" -o -name "*.C" \) -print0 | xargs -0 grep -vE "^ *($|/[/\*])" | wc -l)"; done < <(find "$path" -mindepth 1 -maxdepth 1 -type d -print0) > "${file_n_lines}"

# gcc
if [[ $do_gcc -eq 1 ]]; then
    linter="gcc"
    print_date "$linter"
    if [[ $run_linters -eq 1 ]]; then
        echo "Running $linter"
        { $cmd_gcc "${linter}.log"; }
    fi
    [[ $make_reports -eq 1 ]] && make_report "$linter"
fi

# clang-tidy
if [[ $do_clang_tidy -eq 1 ]]; then
    linter="clang-tidy"
    print_date "$linter"
    if [[ $run_linters -eq 1 ]]; then
        echo "Running $linter"
        find "$path" -type f -name "*.h" -o -name "*.cxx" -o -name "*.C" | parallel --timeout "2h" "$cmd_clang_tidy" 1> "${linter}.log" 2> "${linter}_err.log"
        grep_logs "${linter}.log" "${linter}_err.log"
    fi
    [[ $make_reports -eq 1 ]] && make_report "$linter"
fi

# cppcheck
if [[ $do_cppcheck -eq 1 ]]; then
    linter="cppcheck"
    print_date "$linter"
    if [[ $run_linters -eq 1 ]]; then
        echo "Running $linter"
        find "$path" -type f -name "*.h" -o -name "*.cxx" -o -name "*.C" | parallel --timeout "1h" "$cmd_cppcheck" 1> "${linter}_err.log" 2> "${linter}.log"
        grep_logs "${linter}.log" "${linter}_err.log"
    fi
    [[ $make_reports -eq 1 ]] && make_report "$linter"
fi

# cpplint
if [[ $do_cpplint -eq 1 ]]; then
    linter="cpplint"
    print_date "$linter"
    if [[ $run_linters -eq 1 ]]; then
        echo "Running $linter"
        find "$path" -type f -name "*.h" -o -name "*.cxx" -o -name "*.C" | parallel "$cmd_cpplint" 1> "${linter}_err.log" 2> "${linter}.log"
        grep_logs "${linter}.log" "${linter}_err.log"
    fi
    [[ $make_reports -eq 1 ]] && make_report "$linter"
fi

# o2-linter
if [[ $do_o2_linter -eq 1 ]]; then
    linter="o2-linter"
    print_date "$linter"
    if [[ $run_linters -eq 1 ]]; then
        echo "Running $linter"
        find "$path" -type f -name "*.h" -o -name "*.cxx" -o -name "*.C" -o -name "*.py" -o -name "CMakeLists.txt" | parallel "$cmd_o2_linter" 1> "${linter}.log" 2> "${linter}_err.log"
        grep_logs "${linter}.log" "${linter}_err.log"
    fi
    [[ $make_reports -eq 1 ]] && make_report "$linter"
fi

# unused-files
if [[ $do_unused_files -eq 1 ]]; then
    linter="unused-files"
    print_date "$linter"
    if [[ $run_linters -eq 1 ]]; then
        echo "Running $linter"
        $cmd_unused_files 1> "${linter}.log" 2> "${linter}_err.log"
        grep_logs "${linter}.log" "${linter}_err.log"
    fi
    [[ $make_reports -eq 1 ]] && make_report "$linter"
fi

# includes
if [[ $do_includes -eq 1 ]]; then
    linter="includes"
    print_date "$linter"
    if [[ $run_linters -eq 1 ]]; then
        echo "Running $linter"
        $cmd_includes 1> "${linter}.log" 2> "${linter}_err.log"
        grep_logs "${linter}.log" "${linter}_err.log"
    fi
    [[ $make_reports -eq 1 ]] && make_report "$linter"
fi

print_date "End"
