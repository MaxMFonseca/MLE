#!/bin/bash

if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    echo "This script is intended to be sourced, not executed."
    echo "Use: source ./scripts/envsetup.sh"
    exit 1
fi

_MLE_ROOT="$(pwd)"

function mle_setup() {
    cd "${_MLE_ROOT}" || return 1
    git submodule update --init --recursive
    cd external/LuaJIT
    make
    cd "${_MLE_ROOT}" || return 1
}

function mle_config() {
    local build_type="Debug"
    local max_log_level=""
    local default_log_level_stdout=""
    local extra_args=()

    while [[ $# -gt 0 ]]; do
        case "$1" in
        -t)
            build_type="$2"
            shift 2
            ;;
        -m)
            max_log_level="$2"
            shift 2
            ;;
        -s)
            default_log_level_stdout="$2"
            shift 2
            ;;
        -h)
            echo "Usage: mle_config [-t build_type] [-m max_log_level] [-s stdout_log_level] -- [extra cmake args]"
            return 0
            ;;
        --)
            shift
            extra_args+=("$@")
            break
            ;;
        *)
            shift
            ;;
        esac
    done

    local build_dir="${_MLE_ROOT}/build/${build_type}"
    local cmake_opts=(-S "${_MLE_ROOT}" -B "${build_dir}")
    cmake_opts+=(-DCMAKE_EXPORT_COMPILE_COMMANDS=1)
    cmake_opts+=(-DCMAKE_BUILD_TYPE="${build_type}")

    [[ -n "$max_log_level" ]] && cmake_opts+=(-DMLE_MAX_LOG_LEVEL="$max_log_level")
    [[ -n "$default_log_level_stdout" ]] && cmake_opts+=(-DMLE_DEFAULT_LOG_LEVEL_STDOUT="$default_log_level_stdout")

    echo "Configuring project with CMake..."
    echo "CMake options: ${cmake_opts[*]} ${extra_args[*]}"
    cmake "${cmake_opts[@]}" "${extra_args[@]}"

    rm -f compile_commands.json
    ln -s "${build_dir}/compile_commands.json" .
}

function mle_build() {
    local build_type="Debug"
    local args=()

    while [[ $# -gt 0 ]]; do
        case "$1" in
        -t)
            build_type="$2"
            shift 2
            ;;
        -h)
            echo "Usage: mle_build [-t build_type]" # maybe add a -c for extra cmake args if needed
            return 0
            ;;
        *)
            shift
            ;;
        esac
    done

    cmake --build "${_MLE_ROOT}/build/${build_type}" -j "$(nproc --all)" "${args[@]}"
}

function mle_run_tool() {
    local build_type="Debug"
    local tool_name="MLECubes" # Default tool name for now
    local args=()

    while [[ $# -gt 0 ]]; do
        case "$1" in
        -t)
            build_type="$2"
            shift 2
            ;;
        -n)
            tool_name="$2"
            shift 2
            ;;
        -h)
            echo "Usage: mle_run_tool -n tool_name [-t build_type] -- [extra args to tool]"
            return 0
            ;;
        --)
            shift
            args+=("$@")
            break
            ;;
        *)
            shift
            ;;
        esac
    done

    if [[ -z "$tool_name" ]]; then
        echo "Error: tool_name (-n) is required"
        return 1
    fi

    cd "${_MLE_ROOT}" || return 1

    rm -rf "build/${build_type}/tools/${tool_name}/res"
    mkdir -p "build/${build_type}/tools/${tool_name}/res/mle"
    mkdir -p "build/${build_type}/tools/${tool_name}/res/i"
    local arr=("lua" "textures" "shaders" "fonts" "sounds")
    for i in "${arr[@]}"; do
        ln -s "${_MLE_ROOT}/res/${i}" "build/${build_type}/tools/${tool_name}/res/mle/${i}" 2>/dev/null
        ln -s "${_MLE_ROOT}/tools/${tool_name}/res/${i}" "build/${build_type}/tools/${tool_name}/res/i/${i}" 2>/dev/null
    done

    cd "${_MLE_ROOT}/build/${build_type}/tools/${tool_name}" || return 1
    "./${tool_name}" "${args[@]}"
    cd "${_MLE_ROOT}" || return 1

    rm -f latest.log
    ln -s "${_MLE_ROOT}/build/${build_type}/tools/${tool_name}/logs/latest.log" latest.log 2>/dev/null
    echo "Latest logs in latest.log"
}

function mle_build_and_run_tool() {
    mle_build "$@" && mle_run_tool "$@"
}

function mle_ber() {
    mle_build_and_run_tool "$@"
}

function mle_clean() {
    rm -rf "${_MLE_ROOT}/build"
    rm -f compile_commands.json latest.log
}

function mle_nvim_dap() {
    local build_type="Debug"
    local test_name=""

    while [[ $# -gt 0 ]]; do
        case "$1" in
        -t)
            build_type="$2"
            shift 2
            ;;
        -n)
            test_name="$2"
            shift 2
            ;;
        -h)
            echo "Usage: mle_nvim_dap -n test_name [-t build_type]"
            return 0
            ;;
        *)
            shift
            ;;
        esac
    done

    if [[ -z "$test_name" ]]; then
        echo "Error: test_name (-n) is required"
        return 1
    fi

    export NVIM_DAP_CWD="${_MLE_ROOT}/build/${build_type}/tests/${test_name}"
    export NVIM_DAP_PROGRAM="${NVIM_DAP_CWD}/${test_name}"
    echo "nvim-dap configured:"
    echo "  NVIM_DAP_CWD=$NVIM_DAP_CWD"
    echo "  NVIM_DAP_PROGRAM=$NVIM_DAP_PROGRAM"
}

function mle_gen_docs() {
    cd "${_MLE_ROOT}"
    doxygen docs/Doxyfile.in
    echo "---------------------------------------------"
    echo "Generated Doxygen docs from docs/Doxyfile.in"
    echo "${_MLE_ROOT}/docs/compiled"
}

function mle_help() {
    echo "MLE HELP"
    echo "Available commands:"
    echo "  mle_setup"
    echo "  mle_config [-t build_type] [-m max_log_level] [-s stdout_log_level] -- [extra cmake args]"
    echo "  mle_build [-t build_type]"
    echo "  mle_run_tool -n tool_name [-t build_type] [-a 'args to pass'] -- [extra args to tool]"
    echo "  mle_build_and_run_tool -n tool_name [-t build_type] -- [extra args]"
    echo "  mle_ber alias for mle_build_and_run_tool"
    echo "  mle_clean"
    echo "  mle_nvim_dap -n test_name [-t build_type]"
    echo "  mle_gen_docs"
    echo "  mle_help"
}

echo "---------------------------------------------"
echo "MLE environment loaded."
mle_help
