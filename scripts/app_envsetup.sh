#!/bin/bash

if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
  echo "This script is intended to be sourced, not executed."
  echo "Use: source ./scripts/envsetup.sh"
  exit 1
fi

_APP_ROOT="$(pwd)"
#MLE_APP_NAME="<fillme>"

function mle_setup() {
  echo "TODO"
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

  local build_dir="${_APP_ROOT}/build/${build_type}"
  local cmake_opts=(-S "${_APP_ROOT}" -B "${build_dir}")
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
      echo "Usage: mle_build [-t build_type]"
      return 0
      ;;
    *)
      shift
      ;;
    esac
  done

  cmake --build "${_APP_ROOT}/build/${build_type}" -j "$(nproc --all)" "${args[@]}"
}

function mle_run() {
  local build_type="Debug"
  local args=()

  while [[ $# -gt 0 ]]; do
    case "$1" in
    -t)
      build_type="$2"
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

  cd "${_APP_ROOT}"
  rm -rf "build/${build_type}/bin/res"
  mkdir -p "build/${build_type}/bin/res"
  cd "build/${build_type}/bin/res"

  local arr=("lua" "textures" "shaders" "fonts" "sounds")
  for i in "${arr[@]}"; do
    mkdir -p "${i}"
    cd "${i}"

    ln -s "${_APP_ROOT}/external/MLE/res/${i}" "mle" 2>/dev/null
    ln -s "${_APP_ROOT}/res/${i}" "i" 2>/dev/null
    cd ..
  done

  cd "${_APP_ROOT}/build/${build_type}/bin" || return 1
  "./${MLE_APP_NAME}" "${args[@]}"
  cd "${_APP_ROOT}"

  rm -f latest.log
  ln -s "${_APP_ROOT}/build/${build_type}/bin/logs/latest.log" latest.log 2>/dev/null
  echo "Latest logs in latest.log"
}

function _mle_compile_shaders() {
  local shader_dir="$1"

  find "$shader_dir" -type f \( \
    -name "*.vert" -o -name "*.frag" -o -name "*.comp" -o -name "*.geom" -o -name "*.tesc" -o -name "*.tese" \
    \) | while read -r shader; do
    output="${shader}.spv"
    echo "Compiling $(basename "$shader") -> $(basename "$output")"
    glslangValidator -V "$shader" -o "$output"
    if [ $? -ne 0 ]; then
      echo "❌ Failed to compile: $shader"
      return 1
    fi
  done
}

function mle_compile_mle_shaders() {
  _mle_compile_shaders "${_APP_ROOT}/external/MLE/res/shaders"
}

function mle_compile_app_shaders() {
  _mle_compile_shaders "${_APP_ROOT}/res/shaders"
}

function mle_clean() {
  local build_type="Debug"

  while [[ $# -gt 0 ]]; do
    case "$1" in
    -t)
      build_type="$2"
      shift 2
      ;;
    -h)
      echo "Usage: mle_clean [-t build_type]"
      return 0
      ;;
    *)
      shift
      ;;
    esac
  done

  rm -rf "${_APP_ROOT}/build/${build_type}"
  echo "Cleaned build directory: ${_APP_ROOT}/build/${build_type}"
}

function mle_build_and_run() {
  mle_build "$@" && mle_run "$@"
}

function mle_ber() {
  mle_build_and_run "$@"
}
