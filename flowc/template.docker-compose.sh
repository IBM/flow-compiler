#!/bin/bash

##################################################################################
# {{NAME}}-dc.sh
# generated from {{INPUT_FILE}} ({{MAIN_FILE_TS}})
# with {{FLOWC_NAME}} version {{FLOWC_VERSION}} ({{FLOWC_BUILD}})
#
{O:GLOBAL_TEMP_VARS{export {{GLOBAL_TEMP_VARS}}
}O}
{O:VOLUME_NAME_VAR{export flow_{{VOLUME_NAME_VAR}}={{VOLUME_LOCAL}}
}O}
docker_COMPOSE_PROJECT_NAME={{NAME}}
export provision_ENABLED=1
export default_RUNTIME=
export docker_compose_TIMESTAMPS=
export docker_compose_RW_GID=$(id -g)
export grpc_PORT=${{{NAME_UPPERID}}_GRPC_PORT-{{MAIN_PORT}}}
export rest_PORT=${{{NAME_UPPERID}}_REST_PORT-{{REST_PORT}}}
rget_EMBEDDED_KEY_TOOL=1
download_file() {
    {{RR_GET_SH:rget_EMBEDDED_KEY_TOOL=; source "$(dirname "$0")/rr-get.sh"}}
}

export export_PORTS="#"
args=()
while [ $# -gt 0 ]
do
case "$1" in
{A:VOLUME_OPTION{
    --mount-{{VOLUME_OPTION}})
    export {{VOLUME_NAME_VAR}}="$2"
    shift
    shift
    ;;
}A}
    --htdocs)
    if [ "${2:0:1}" == "/" ] 
    then
        export {{NAME_UPPERID}}_HTDOCS="$2"
    else 
        export {{NAME_UPPERID}}_HTDOCS="$PWD/${2#./}"
    fi
    shift
    shift
    ;;
    --grpc-port)
    export grpc_PORT="$2"
    shift
    shift
    ;;
    --rest-port)
    export rest_PORT="$2"
    shift
    shift
    ;;
    -s|--skip-provision)
    export provision_ENABLED=0
    shift
    ;;
    -p|--export-ports)
    export export_PORTS=
    shift
    ;;
    -r|--default-runtime)
    export default_RUNTIME="#"
    shift
    ;;
    -T|--timestamps)
    export docker_compose_TIMESTAMPS="--timestamps"
    shift
    ;;
    *)
    args+=("$1")
    shift
    ;;
esac
done
set -- "${args[@]}"

if ! which envsubst > /dev/null 2>&1 
then
    echo "envsubst command not found"
fi

docker_COMPOSE_YAML=$(cat <<"ENDOFYAML"
{{DOCKER_COMPOSE_YAML}}
ENDOFYAML
)

{O:VOLUME_NAME_VAR{export {{VOLUME_NAME_VAR}}="${{{VOLUME_NAME_VAR}}-$flow_{{VOLUME_NAME_VAR}}}"
if [ "${{{VOLUME_NAME_VAR}}:0:1}" != "/" ]
then
    export {{VOLUME_NAME_VAR}}="$(pwd)/${{VOLUME_NAME_VAR}}"
fi
}O}
[ {O:VOLUME_NAME_VAR{-z "${{VOLUME_NAME_VAR}}" -o }O} 1 -eq 0 ]
have_ALL_VOLUME_DIRECTORIES=$?
export enable_custom_app="#"
if [ ! -z "${{NAME_UPPERID}}_HTDOCS" ]
then
    if [ ! -d "${{NAME_UPPERID}}_HTDOCS" ]
    then
        echo "${{NAME_UPPERID}}_HTDOCS: must point to a valid directory"
        have_ALL_VOLUME_DIRECTORIES=0
    else
        export enable_custom_app=
    fi
fi

if [ $# -eq 0 -o "$1" == "up" -a $have_ALL_VOLUME_DIRECTORIES -eq 0 \
    -o "$1" == "provision" -a $have_ALL_VOLUME_DIRECTORIES -eq 0  \
    -o "$1" == "config" -a $have_ALL_VOLUME_DIRECTORIES -eq 0 \
    -o "$1" == "run" -a $have_ALL_VOLUME_DIRECTORIES -eq 0 \
    -o "$1" != "up" -a "$1" != "down" -a "$1" != "config" -a "$1" != "logs" -a "$1" != "provision" -a "$1" != "run" ]
then
echo "{{NAME}}-dc.sh generated from {{MAIN_FILE}} ({{MAIN_FILE_TS}})"
echo ""
echo "Usage $0 <up|run|config|provision> [-p] [-r] [-s] [-T] [--grpc-port PORT] [--rest-port PORT] [--htdocs DIRECTORY] {R:REST_NODE_NAME{--htdocs DIRECTORY }R}{O:VOLUME_OPTION{--mount-{{VOLUME_OPTION}} DIRECTORY  }O}"
echo "   or $0 <down|logs>"
echo ""
echo "    -p, --export-ports  export the gRPC ports for all nodes"
echo ""
echo "    -r, --default-runtime  ignore runtime settings and run with the default Docker runtime"
echo ""
echo "    -s, --skip-provision  skip checking for new data files at startup"
echo ""
echo "    -T, --timestamps  show timestamps in logs"
echo ""
echo   "    --grpc-port PORT  (or set {{NAME_UPPERID}}_GRPC_PORT)"
echo   "        Override GRPC port (default is $grpc_PORT)"
echo ""
echo   "    --rest-port PORT  (or set {{NAME_UPPERID}}_REST_PORT)"
echo   "        Override REST API port (default is $rest_PORT)"
echo ""
echo   "    --htdocs DIRECTORY  (or set {{NAME_UPPERID}}_HTDOCS)"
echo   "        Directory with custom application files (default is \"${{NAME_UPPERID}}_HTDOCS\")"
echo ""
{O:VOLUME_OPTION{
echo   "    --mount-{{VOLUME_OPTION:}} DIRECTORY  (or set {{VOLUME_NAME_VAR}})"
echo   "        Override path to be mounted for {{VOLUME_NAME}} (default is ${{VOLUME_NAME_VAR}})"
printf "        "{{VOLUME_HELP}}"\n"
echo ""
}O}
{A:HAVE_ARTIFACTORY{
{{HAVE_ARTIFACTORY}}
echo "Note: To access the Artifactory $0 looks for the environment variable API_KEY."
echo "If not found, it looks for a file named .api-key in the current directory and then in the home directory."
echo ""
}A}
exit 1
fi
{A:HAVE_ARTIFACTORY{
{{HAVE_ARTIFACTORY}}
{O:VOLUME_OPTION{remote_resource_{{VOLUME_NAME_VAR}}="{{VOLUME_ARTIFACTORY}}"   
}O}
}A}

provision() {
{A:HAVE_ARTIFACTORY{{{HAVE_ARTIFACTORY}}
{O:VOLUME_OPTION{    [ -z "$remote_resource_{{VOLUME_NAME_VAR}}" ] || \
        download_file -o "${{VOLUME_NAME_VAR}}" --untar "$remote_resource_{{VOLUME_NAME_VAR}}" || return 1
}O}
}A}
{O:VOLUME_NAME_VAR{    [ {{VOLUME_IS_RO}} -eq 0 ] && chmod -fR g+w "${{VOLUME_NAME_VAR}}"
}O}
    return 0
}

case "$1" in
    provision)
        provision
        exit $?
        ;;
    config)
        echo "$docker_COMPOSE_YAML" | envsubst | docker-compose -f - -p "$docker_COMPOSE_PROJECT_NAME" config
        exit $?
        ;;
    down)
        echo "$docker_COMPOSE_YAML" | envsubst | docker-compose -f - -p "$docker_COMPOSE_PROJECT_NAME" down -v
        exit $?
        ;;
    logs)
        echo "$docker_COMPOSE_YAML" | envsubst | docker-compose -f - -p "$docker_COMPOSE_PROJECT_NAME" logs -f $docker_compose_TIMESTAMPS
        exit $?
        ;;
    up)
        [ $provision_ENABLED -eq 0 ] || provision || exit 1
        echo "$docker_COMPOSE_YAML" | envsubst | docker-compose -f - -p "$docker_COMPOSE_PROJECT_NAME" up -d
        rc=$?
        ;;
    run)
        [ $provision_ENABLED -eq 0 ] || provision || exit 1
        echo "$docker_COMPOSE_YAML" | envsubst | docker-compose -f - -p "$docker_COMPOSE_PROJECT_NAME" up 
        rc=$?
        ;;
esac
if [ $rc -eq 0 -a "$1" == "up" ]
then
    echo "{{NAME}} gRPC service listening on port $grpc_PORT"
    echo "{{NAME}} REST service listening on port $rest_PORT"
fi
exit $rc

