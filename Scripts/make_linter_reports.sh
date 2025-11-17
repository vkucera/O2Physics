#!/bin/bash

# Run all C++ linters and produce Markdown reports.

repo="O2Physics"
# repo="O2"

run_linters=1
make_reports=1

do_gcc=1
do_clang_tidy=1
do_cppcheck=1
do_cpplint=1
do_o2_linter=1

make_report() {
    linter="$1"
    echo "Making report for $linter"
    python3 $HOME/alice/O2Physics/Scripts/process_linter_log.py -p "${linter}.log" -l "${linter}" -r "${repo}" > "report_${linter}.md"
}

print_date() { echo "$(date +"%F_%H-%M-%S") $@"; }

grep_logs() { grep -A 1 "parallel: Warning: This job was killed" "$@"; }

print_date "Start"

# gcc
if [[ $do_gcc -eq 1 ]]; then
    linter="gcc"
    print_date "$linter"
    if [[ $run_linters -eq 1 ]]; then
        echo "Running $linter"
        { rsync -avhq "$HOME/alice/sw/BUILD/${repo}-latest/log" "${linter}.log"; }
    fi
    [[ $make_reports -eq 1 ]] && make_report "$linter"
fi

# clang-tidy
if [[ $do_clang_tidy -eq 1 ]]; then
    linter="clang-tidy"
    print_date "$linter"
    if [[ $run_linters -eq 1 ]]; then
        echo "Running $linter"
        find . -type f -name "*.h" -o -name "*.cxx" -o -name "*.C" | parallel --timeout "1h" "clang-tidy {}" 1> "${linter}.log" 2> "${linter}_err.log"
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
        find . -type f -name "*.h" -o -name "*.cxx" -o -name "*.C" | parallel --timeout "1h" "cppcheck --language=c++ --std=c++20 --check-level=exhaustive --suppressions-list=cppcheck_config --force {}" 1> "${linter}_err.log" 2> "${linter}.log"
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
        find . -type f -name "*.h" -o -name "*.cxx" -o -name "*.C" | parallel "cpplint --extensions=h,cxx,C {}" 1> "${linter}_err.log" 2> "${linter}.log"
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
        find . -type f -name "*.h" -o -name "*.cxx" -o -name "*.C" -o -name "*.py" -o -name "CMakeLists.txt" | parallel "python3 $HOME/alice/O2Physics/Scripts/o2_linter.py {}" 1> "${linter}.log" 2> "${linter}_err.log"
        grep_logs "${linter}.log" "${linter}_err.log"
    fi
    [[ $make_reports -eq 1 ]] && make_report "$linter"
fi

print_date "End"
