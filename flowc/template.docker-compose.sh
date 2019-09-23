#!/bin/bash

##################################################################################
# {{NAME}}-dc.sh
# generated from {{INPUT_FILE}} ({{MAIN_FILE_TS}})
# with {{FLOWC_NAME}} version {{FLOWC_VERSION}} ({{FLOWC_BUILD}})
#
{O:VOLUME_NAME_VAR{export flow_{{VOLUME_NAME_VAR}}={{VOLUME_LOCAL}}
}O}
docker_COMPOSE_PROJECT_NAME={{NAME}}
export trace_ENABLED=0
export provision_ENABLED=1
{R:REST_NODE_NAME{
# protorest mount path (original path: {{PROTO_FILES_PATH}})
case "$0" in
    /*) 
        export local_PROTO_FILES_PATH="$(dirname "$0")"
        ;;
    *)
        export local_PROTO_FILES_PATH="$PWD/$(dirname "$0")"
        ;;
esac
if [ -d "$local_PROTO_FILES_PATH/htdocs" ]
then
    export {{NAME_UPPERID}}_HTDOCS="$local_PROTO_FILES_PATH/htdocs"
fi
export rest_PORT=${{{NAME_UPPERID}}_REST_PORT-{{REST_NODE_PORT}}}
export gui_PORT=${{{NAME_UPPERID}}_REST_PORT-{{GUI_NODE_PORT}}}
export app_PORT=${{{NAME_UPPERID}}_REST_PORT-{{CUSTOM_GUI_NODE_PORT}}}
{P:PROTO_FILE{if [ ! -f "$local_PROTO_FILES_PATH/{{PROTO_FILE}}" ]
then
    echo "proto file \"$local_PROTO_FILES_PATH/{{PROTO_FILE}}\" is missing, cannot continue"
    exit 1
fi
}P}
}R}
export grpc_PORT=${{{NAME_UPPERID}}_GRPC_PORT-{{MAIN_PORT}}}
# default to running in the foreground
fg_OR_bg=
export export_PORTS="#"
args=()
while [ $# -gt 0 ]
do
case "$1" in
{A:VOLUME_OPTION{
    {{VOLUME_OPTION}})
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
    --app-port)
    export app_PORT="$2"
    shift
    shift
    ;;
    --grpc-port)
    export grpc_PORT="$2"
    shift
    shift
    ;;
    --gui-port)
    export gui_PORT="$2"
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
    -t|--enable-trace)
    export trace_ENABLED=1
    shift
    ;;
    -p|--export-ports)
    export export_PORTS=
    shift
    ;;
    -b)
    fg_OR_bg=-d
    shift
    ;;
    *)
    args+=("$1")
    shift
    ;;
esac
done
set -- "${args[@]}"

docker_COMPOSE_YAML=$(cat <<"ENDOFYAML"
{{DOCKER_COMPOSE_YAML}}
ENDOFYAML
)

{O:VOLUME_NAME_VAR{export {{VOLUME_NAME_VAR}}="${{{VOLUME_NAME_VAR}}-$flow_{{VOLUME_NAME_VAR}}}"
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

if [ $# -eq 0 -o "$1" == "up" -a $have_ALL_VOLUME_DIRECTORIES -eq 0 -o "$1" == "provision" -a $have_ALL_VOLUME_DIRECTORIES -eq 0  -o "$1" == "config" -a $have_ALL_VOLUME_DIRECTORIES -eq 0 -o "$1" != "up" -a "$1" != "down" -a "$1" != "config" -a "$1" != "logs" -a "$1" != "provision" ]
then
echo "{{NAME}}-dc.sh generated from {{MAIN_FILE}} ({{MAIN_FILE_TS}})"
echo ""
echo "Usage $0 <up|config|provision> [-b] [-t] [--xxx-port PORT] {R:REST_NODE_NAME{--htdocs DIRECTORY }R}{O:VOLUME_OPTION{{{VOLUME_OPTION}} DIRECTORY  }O}"
echo "   or $0 <down|logs>"
echo ""
echo "    -b  run docker compose in the background"
echo ""
echo "    -p, --export-ports  export the gRPC ports for all nodes"
echo ""
echo "    -s, --skip-provision  skip checking for new data files at startup"
echo ""
echo "    -t, --enable-trace  enable orchestrator call tracing"
echo ""
echo   "    --grpc-port PORT  (or set {{NAME_UPPERID}}_GRPC_PORT)"
echo   "        Override GRPC port (default is $grpc_PORT)"
echo ""
{R:REST_NODE_NAME{
echo   "    --rest-port PORT  (or set {{NAME_UPPERID}}_REST_PORT)"
echo   "        Override REST API port (default is $rest_PORT)"
echo ""
echo   "    --gui-port PORT  (or set {{NAME_UPPERID}}_GUI_PORT)"
echo   "        Override REST API port (default is $gui_PORT)"
echo ""
echo   "    --app-port PORT  (or set {{NAME_UPPERID}}_APP_PORT)"
echo   "        Override REST API port (default is $app_PORT)"
echo ""
echo   "    --htdocs DIRECTORY  (or set {{NAME_UPPERID}}_HTDOCS)"
echo   "        Directory with custom application files (default is \"${{NAME_UPPERID}}_HTDOCS\")"
echo ""
}R}
{O:VOLUME_OPTION{
echo   "    {{VOLUME_OPTION:}} DIRECTORY  (or set {{VOLUME_NAME_VAR}})"
echo   "        Override path to be mounted for {{VOLUME_NAME}} (default is $flow_{{VOLUME_NAME_VAR}})"
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
{O:VOLUME_OPTION{artifactory_{{VOLUME_NAME_VAR}}="{{VOLUME_ARTIFACTORY}}"   
}O}
}A}
case "$1" in
    provision)
        rc=0
{A:HAVE_ARTIFACTORY{
        {{HAVE_ARTIFACTORY}}
{O:VOLUME_OPTION{
        if [ ! -z "$artifactory_{{VOLUME_NAME_VAR}}" ]
        then
            "$(dirname "$0")/{{NAME}}-ag.sh" -o "${{VOLUME_NAME_VAR}}" --untar "$artifactory_{{VOLUME_NAME_VAR}}"
            rc=$?
        fi
        [ $rc -eq 0 ] || exit $rc
}O}
}A}
        exit $rc
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
        echo "$docker_COMPOSE_YAML" | envsubst | docker-compose -f - -p "$docker_COMPOSE_PROJECT_NAME" logs -f
        exit $?
        ;;
    up)
        rc=0
{A:HAVE_ARTIFACTORY{
        if [ $provision_ENABLED -ne 0 ]{{HAVE_ARTIFACTORY}}
        then
{O:VOLUME_OPTION{
            if [ ! -z "$artifactory_{{VOLUME_NAME_VAR}}" ]
            then
                "$(dirname "$0")/{{NAME}}-ag.sh" -o "${{VOLUME_NAME_VAR}}" --untar "$artifactory_{{VOLUME_NAME_VAR}}"
                rc=$?
            fi
            [ $rc -eq 0 ] || exit $rc
}O}
        fi
}A}
        if [ $rc -eq 0 ]
        then
            echo "$docker_COMPOSE_YAML" | envsubst | docker-compose -f - -p "$docker_COMPOSE_PROJECT_NAME" up $fg_OR_bg
            rc=$?
        fi
        ;;
esac
if [ $rc -eq 0 -a "$fg_OR_bg" != "" ]
then
    echo "{{NAME}} gRPC service listening on port $grpc_PORT"
    {R:REST_NODE_NAME{
    echo "{{NAME}} REST service listening on port $rest_PORT"
    echo "{{NAME}} GUI available on port $gui_PORT"
    if [ -z "$enable_custom_app" ]
    then
        echo "{{NAME}} App available on port $app_PORT"
    fi
}R}
fi
exit $rc

