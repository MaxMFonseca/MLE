#!/bin/bash

if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
  echo "This script is intended to be sourced, not executed."
  echo "Use: source ./scripts/envsetup.sh from project root."
  exit 1
fi

_MLE_ROOT="$(pwd)"

MLE_SHADER_DIRS="${MLE_SHADER_DIRS:-${_MLE_ROOT}/res/shaders}"

if [[ ! -f "${_MLE_ROOT}/README.md" ]]; then
  echo "Error: Please source this script from the project root directory."
  return 1
fi

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

  if mle_compile_shaders_all; then
    echo "Shader compilation succeeded."
  else
    echo "Shader compilation failed."
    return 1
  fi

  cmake --build "${_MLE_ROOT}/build/${build_type}" -j "$(nproc --all)" "${args[@]}"
}

function mle_run_test() {
  local build_type="Debug"
  local test_name="Core"
  local args=()

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
      echo "Usage: mle_run_test [-n test_name] [-t build_type] -- [forward args]"
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

  if [[ -z "$test_name" ]]; then
    echo "Error: test_name (-n) is required"
    return 1
  fi

  cd "${_MLE_ROOT}" || return 1

  rm -rf "build/${build_type}/tests/${test_name}/res"
  mkdir -p "build/${build_type}/tests/${test_name}/res"
  local arr=("lua" "textures" "shaders" "fonts" "sounds" "models" "animations")
  cd "build/${build_type}/tests/${test_name}/res"

  for i in "${arr[@]}"; do
    mkdir -p "${i}"
    cd "${i}"

    ln -s "${_MLE_ROOT}/res/${i}" "mle" 2>/dev/null
    ln -s "${_MLE_ROOT}/tests/${test_name}/res/${i}" "i" 2>/dev/null
    cd ..
  done

  cd "${_MLE_ROOT}/build/${build_type}/tests/${test_name}" || return 1
  "./${test_name}" "${args[@]}"
  cd "${_MLE_ROOT}"

  rm -f latest.log
  ln -s "${_MLE_ROOT}/build/${build_type}/tests/${test_name}/logs/latest.log" latest.log 2>/dev/null
  echo "Latest logs in latest.log"
}

function mle_build_and_run_test() {
  mle_build "$@" && mle_run_test "$@"
}

function mle_ber() {
  mle_build_and_run_test "$@"
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
      echo "Usage: mle_nvim_dap [-n test_name] [-t build_type]"
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

mle_add_shader_dirs() {
  local d abs added=0
  for d in "$@"; do
    abs="$(
      cd "$d" 2>/dev/null && pwd -P
    )" || {
      echo "Skip (not found): $d"
      continue
    }

    if [[ ":${MLE_SHADER_DIRS}:" != *":${abs}:"* ]]; then
      MLE_SHADER_DIRS="${MLE_SHADER_DIRS:+${MLE_SHADER_DIRS}:}${abs}"
      added=1
      echo "Added: ${abs}"
    fi
  done
  export MLE_SHADER_DIRS
  ((added)) && echo "Current MLE_SHADER_DIRS=${MLE_SHADER_DIRS}"
}

mle_compile_shaders_all() {
  local force=0 jobs
  jobs=$(nproc --all)

  while [[ $# -gt 0 ]]; do
    case "$1" in
    -f | --force)
      force=1
      shift
      ;;
    -j)
      jobs="$2"
      shift 2
      ;;
    *)
      echo "Unknown arg: $1"
      return 2
      ;;
    esac
  done

  local -a dir_list=()
  if [[ -n "${ZSH_VERSION-}" ]]; then
    dir_list=(${(s/:/)MLE_SHADER_DIRS})
  else
    IFS=':' read -r -a dir_list <<< "$MLE_SHADER_DIRS"
  fi

  local -a files=()
  local dir
  for dir in "${dir_list[@]}"; do
    [[ -d "$dir" ]] || { echo "Missing dir: $dir"; continue; }
    while IFS= read -r -d '' f; do files+=("$f"); done < <(
      find "$dir" -type f \( -name '*.vert' -o -name '*.frag' -o -name '*.comp' -o -name '*.geom' -o -name '*.tesc' -o -name '*.tese' \) -print0
    )
  done

   [[ ${#files[@]} -eq 0 ]] && { echo "No shaders found."; return 0; }

  echo "Found ${#files[@]} shaders"

  printf '%s\0' "${files[@]}" | xargs -0 -I{} -P"$jobs" bash -c '
    shader="$1"; force="$2"; out="${shader}.spv"
    if [[ "$force" == "1" || ! -f "$out" || "$shader" -nt "$out" ]]; then
      echo "Compiling: $(basename "$shader")"
      glslangValidator -V "$shader" -o "$out" || { echo "$shader"; exit 1; }
    else
      echo "Up-to-date: $(basename "$shader")"
    fi
  ' bash {} "$force" || {
    echo "Some shaders failed."
    return 1
  }

  echo "All done."
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
  echo "  mle_run_test -n test_name [-t build_type] -- [forward args]"
  echo "  mle_build_and_run_test -n test_name [-t build_type] -- [forward args]"
  echo "  mle_ber alias for mle_build_and_run_test"
  echo "  mle_clean"
  echo "  mle_nvim_dap [-n test_name] [-t build_type]"
  echo "  mle_compile_shaders_all [-f|--force] [-j jobs]"
  echo "  mle_add_shader_dirs [dir1 dir2 ...]"
  echo "  mle_gen_docs"
  echo "  mle_help"
}

echo "---------------------------------------------"
echo "MLE environment loaded."
mle_help
