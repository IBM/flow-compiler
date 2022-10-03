#!/bin/sh

args=()
df_OUTPUT=$(pwd)/.
df_FLOWF=
df_INPUT=
while [ $# -gt 0 ]
do
case "$1" in
    -o|--output-directory)
        df_OUTPUT="$2"
        shift; shift
    ;;
    --output-directory=*)
        df_OUTPUT="${1#*=}"
        shift
    ;;
    *.flow)
        df_INPUT=$(dirname "$1")
        df_FLOWF="/input/$(basename "$1")"
        shift
    ;;
    *)
    args+=("$1")
    shift
    ;;
esac
done
set -- "${args[@]}"

docker run --rm -v "$df_INPUT:/input" -v "$df_OUTPUT:/output" ${FLOWC_IMAGE-flowc:v0.21.0.redhat-ubi8} "$@" -o "/output/" "$df_FLOWF"
