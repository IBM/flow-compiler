#!/bin/bash

##################################################################################
# Docker Compose/Swarm configuration generator for {{NAME}}
# generated from {{INPUT_FILE}} ({{MAIN_FILE_TS}})
# with {{FLOWC_NAME}} version {{FLOWC_VERSION}} ({{FLOWC_BUILD}})
#

export kd_PROJECT_NAME={{NAME}}
{N:NODE_NAME{export image_{{NODE_NAME/id/upper}}=${{{NAME/id/upper}}_{{NODE_NAME/id/upper}}_IMAGE-{{NODE_IMAGE}}}
if [ ! -z "${{NAME/id/upper}}_{{NODE_NAME/id/upper}}_TAG" ]
then
    export image_{{NODE_NAME/id/upper}}=${image_{{NODE_NAME/id/upper}}%:*}:${{{NAME/id/upper}}_{{NODE_NAME/id/upper}}_TAG}
fi
}N}
{O:GLOBAL_TEMP_VARS{export {{GLOBAL_TEMP_VARS}}
}O}
{O:MAIN_EP_ENVIRONMENT_NAME{export {{MAIN_EP_ENVIRONMENT_NAME}}_DN={{MAIN_DN_ENVIRONMENT_VALUE}}
}O}
{N:NODE_NAME{export scale_{{NODE_NAME/id/upper}}={{NODE_SCALE}}
}N}
export replicas_{{NAME/id/upper}}=${{{NAME/id/upper}}_REPLICAS-{{MAIN_SCALE}}}
{G:GROUP{export replicas_{{NAME/id/upper}}_{{GROUP/id/upper}}=${{{NAME/id/upper}}_{{GROUP/id/upper}}_REPLICAS-{{GROUP_SCALE}}}
}G}
{A:VOLUME_NAME{flow_{{VOLUME_NAME/id/upper}}="${{{VOLUME_NAME/id/upper}}-{{VOLUME_LOCAL}}}"
export {{VOLUME_NAME/id/upper}}="$flow_{{VOLUME_NAME/id/upper}}"
flow_{{VOLUME_NAME/id/upper}}_SECRET_NAME=${{{VOLUME_NAME/id/upper}}_SECRET_NAME-{{VOLUME_SECRET}}}
export {{VOLUME_NAME/id/upper}}_SECRET_NAME=$flow_{{VOLUME_NAME/id/upper}}_SECRET_NAME
flow_{{VOLUME_NAME/id/upper}}_URL=${{{VOLUME_NAME/id/upper}}_URL-{{VOLUME_COS}}}
export {{VOLUME_NAME/id/upper}}_URL=$flow_{{VOLUME_NAME/id/upper}}_URL
}A}
export use_MODE=compose
export use_COMPOSE=
export use_SWARM="#"
export use_K8S="#"
export provision_ENABLED=1
export default_RUNTIME=
export docker_compose_TIMESTAMPS=
export docker_compose_RW_GID=$(id -g)
export grpc_PORT=${{{NAME/id/upper}}_GRPC_PORT-{{MAIN_PORT}}}
export rest_PORT=${{{NAME/id/upper}}_REST_PORT-{{REST_PORT}}}
dd_display_help() {
echo "Docker Compose/Swarm and Kubernetes configuration generator for {{NAME}}"
echo "From {{MAIN_FILE}} ({{MAIN_FILE_TS}})"
echo ""
echo "Usage $(basename "$0") <up|down|config|logs|run> [-p] [-r] [-s] [-C] [-T] [--project-name NAME] [--grpc-port PORT] [--rest-port PORT] [--htdocs DIRECTORY] {O:VOLUME_NAME{[--mount-{{VOLUME_NAME/option}} DIRECTORY] }O}"
echo "   or $(basename "$0") <up|down|config> -S [--project-name NAME] [--grpc-port PORT] [--rest-port PORT]"
echo "   or $(basename "$0") -K [--project-name NAME] <delete|deploy|show|config>"
echo "   or $(basename "$0") [-T] <run|logs>"
{V:HAVE_VOLUMES{
echo "   or $(basename "$0") <provision> {O:VOLUME_NAME{[--mount-{{VOLUME_NAME/option}} DIRECTORY] }O}"
}V}
echo ""
echo "Commands:"
echo "   config     Display the configuration file that is sent to the underlying command"
echo "   deploy     Deploy and start application (Kubernetes mode)"
echo "   down       Stop the running application (Docker Compose or Swarm mode)"
echo "   help       Display this help screen"
echo "   logs       Display logs from all the containers (Docker Compose mode)"
echo "   up         Start the application in the background (Docker Compose or Swarm mode)"
{V:HAVE_VOLUMES{echo "   provision  Download thte external data into local directories so it can be mounted inside the running containers (Docker Compose only)"
}V}
echo "   run        Run the application in the foreground in (Docker Composer mode)"
echo "   show       Display the current status of the application (Kubernetes mode)"
echo ""
echo "Options:"
echo "    -h, --help"
echo "        Display this help screen and exit"
echo ""
echo "    -C, --compose"
echo "        Enable Docker Compose mode (default)"
echo ""
echo "    -K, --kubernetes"
echo "        Enable Kubernetes mode"
echo ""
echo "    -p, --export-ports"
echo "        Export the gRPC ports for all nodes"
echo ""
echo "    -r, --default-runtime"
echo "        Ignore runtime settings and run with the default Docker runtime"
echo ""
echo "    -S, --swarm"
echo "        Enable Docker Swarm mode"
echo ""
echo "    -s, --skip-provision"
echo "        Skip checking for new data files at startup"
echo ""
echo "    -T, --timestamps"
echo "        Show timestamps in logs"
echo ""
echo "    --project-name NAME"
echo "        Set the Docker Compose project name to NAME (default is $kd_PROJECT_NAME)"
echo ""
echo "    --grpc-port PORT (or set {{NAME/id/upper}}_GRPC_PORT)"
echo "        Set the locally mapped port for the GRPC service. The default is $grpc_PORT"
echo ""
echo "    --rest-port PORT (or set {{NAME/id/upper}}_REST_PORT)"
echo "        Set the locally mapped port for the REST API. The default is $rest_PORT"
echo ""
echo "    --htdocs DIRECTORY (or set {{NAME/id/upper}}_HTDOCS)"
echo "        Mount local DIRECTORY inside the container to use as a custom application (Docker compose mode)."
[ ! -z "${{NAME/id/upper}}_HTDOCS" ] && echo "       Currently set to \"${{NAME/id/upper}}_HTDOCS\""
echo ""
{O:VOLUME_NAME{
echo "    --mount-{{VOLUME_NAME/option-}} <DIRECTORY>  (or set {{VOLUME_NAME/id/upper}})"
echo "        Override the path to be mounted for {{VOLUME_NAME}} (default is ${{VOLUME_NAME/id/upper}})"
printf "        "{{VOLUME_COMMENT/sh}}"\n"
echo ""
echo "    --secret-{{VOLUME_NAME/option-}} <SECRET-NAME>  (or set {{VOLUME_NAME/id/upper}}_SECRET_NAME)"
echo "        Secret name for COS access for {{VOLUME_NAME}}, default is \"${{VOLUME_NAME/id/upper}}_SECRET_NAME\""
echo ""
echo "    --remote-{{VOLUME_NAME/option-}} <URL> (or set {{VOLUME_NAME/id/upper}}_URL)"
echo "        Set the URL to the remote resource for {{VOLUME_NAME}}, default is \"${{VOLUME_NAME/id/upper}}_URL\""
echo ""
}O}
echo "    --htdocs-secret-name <SECRET-NAME>  (or set {{NAME/id/upper}}_HTDOCS_SECRET_NAME)"
echo "        Secret name for COS access for the custom application files, if a COS URL was given."
[ ! -z "${{NAME/id/upper}}_HTDOCS_SECRET_NAME" ] && echo "        Currently set to \"${{NAME/id/upper}}_HTDOCS_SECRET_NAME\""
echo ""
echo "    --{{NAME}}-replicas <NUMBER>  (or set {{NAME/id/upper}}_REPLICAS)"
echo "        Number of replicas for the main pod [{{MAIN_POD}}]. The default is $replicas_{{NAME/id/upper}}."
echo ""
{G:GROUP{
echo "    --{{GROUP}}-replicas <NUMBER>  (or set {{NAME/id/upper}}_{{GROUP/id/upper}}_REPLICAS)"
echo "        Number or replicas for pod \"{{GROUP}}\" [{{GROUP_NODES}}]. The default is $replicas_{{NAME/id/upper}}_{{GROUP/id/upper}}."
echo ""
}G}
echo "    --tag, --image  <NODE-NAME=STRING>"
echo "       Force the image name or tag for node NODE_NAME to STRING. The changes are applied in the order given in the commans line."
echo "       The valid node names are: {N:NODE_NAME{{{NODE_NAME/option/lower}} }N}."
echo "" 
echo "Note: To access the Artifactory or Cloud Object Store $(basename $0) looks for the environment variable API_KEY."
echo "If not found, it looks for a file named .api-key in the current directory and then in the home directory."
echo ""
}
rget_EMBEDDED_KEY_TOOL=1
download_file() {
    {{RR_GET_SH-rget_EMBEDDED_KEY_TOOL=; source "$(dirname "$0")/rr-get.sh"}}
}
export export_PORTS="#"
args=()
while [ $# -gt 0 ]
do
case "$1" in
    -h|--help)
    dd_display_help
    exit 0
    ;;
{A:VOLUME_NAME{
    --mount-{{VOLUME_NAME/option}})
    export {{VOLUME_NAME/id/upper}}="$2"
    shift
    shift
    ;;
    --secret-{{VOLUME_NAME/option}})
    export {{VOLUME_NAME/id/upper}}_SECRET_NAME="$2"
    shift
    shift
    ;;
    --remote-{{VOLUME_NAME/option}}) 
    export {{VOLUME_NAME/id/upper}}_URL="$2"   
    shift
    shift
    ;;
}A}
    --{{NAME}}-replicas)
    export replicas_{{NAME/id/upper}}="$2"
    shift
    shift
    ;;
{G:GROUP{
    --{{GROUP}}-replicas)
    export replicas_{{NAME/id/upper}}_{{GROUP/id/upper}}="$2"
    shift
    shift
    ;;
}G}
    --htdocs)
    if [ "${2:0:1}" == "/" ] 
    then
        export {{NAME/id/upper}}_HTDOCS="$2"
    else 
        export {{NAME/id/upper}}_HTDOCS="$PWD/${2#./}"
    fi
    shift
    shift
    ;;
    --htdocs-secret-name)
    export {{NAME/id/upper}}_HTDOCS_SECRET_NAME="$2"
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
    -P|--project-name)
    export kd_PROJECT_NAME="$2"
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
    -S|--swarm)
    export use_COMPOSE="#"
    export use_K8S="#"
    export use_SWARM=
    export use_MODE=swarm
{O:MAIN_EP_ENVIRONMENT_NAME{    if [ $scale_{{NODE_NAME/id/upper}} -gt 1 ]
    then
        export {{MAIN_EP_ENVIRONMENT_NAME}}_DN="@tasks.${kd_PROJECT_NAME}_{{MAIN_DN_ENVIRONMENT_VALUE}}"
    else
        export {{MAIN_EP_ENVIRONMENT_NAME}}_DN="tasks.${kd_PROJECT_NAME}_{{MAIN_DN_ENVIRONMENT_VALUE}}"
    fi
}O}
    shift
    ;;
    -K|--kubernetes)
    export use_COMPOSE="#"
    export use_SWARM="#"
    export use_K8S=
    export use_MODE=k8s
    shift
    ;;
    -C|--compose)
    export use_COMPOSE=""
    export use_SWARM="#"
    export use_K8S="#"
    export use_MODE=compose
    shift
    ;;
    -T|--timestamps)
    export docker_compose_TIMESTAMPS="--timestamps"
    shift
    ;;
    --image)
        case "${2%=*}" in
{N:NODE_NAME{
            {{NODE_NAME/option/lower}})
                export image_{{NODE_NAME/id/upper}}=${2#*=}
                ;;
}N}
            *)
                echo "Unknown node name: '${2%=*}'"
                exit 1
                ;;
        esac
    shift
    shift
    ;;
    --tag)
        case "${2%=*}" in
{N:NODE_NAME{
            {{NODE_NAME/option/lower}})
                export image_{{NODE_NAME/id/upper}}=${image_{{NODE_NAME/id/upper}}%:*}:${2#*=}
                ;;
}N}
            *)
                echo "Unknown node name: '${2%=*}'"
                exit 1
                ;;
        esac
    shift
    shift
    ;;
    *)
    args+=("$1")
    shift
    ;;
esac
done
set -- "${args[@]}"
{O:VOLUME_NAME{export {{VOLUME_NAME/id/upper}}="${{{VOLUME_NAME/id/upper}}-$flow_{{VOLUME_NAME/id/upper}}}"
if [ "${{{VOLUME_NAME/id/upper}}:0:1}" != "/" ]
then
    export {{VOLUME_NAME/id/upper}}="$(pwd)/${{VOLUME_NAME/id/upper}}"
fi
}O}

if ! which envsubst > /dev/null 2>&1 
then
    echo "envsubst command not found"
fi

kubernetes_YAML=$(cat <<"ENDOFYAML"
{{KUBERNETES_YAML}}
ENDOFYAML
)
docker_COMPOSE_YAML=$(cat <<"ENDOFYAML"
{{DOCKER_COMPOSE_YAML}}
ENDOFYAML
)

[ {O:VOLUME_NAME{-z "${{VOLUME_NAME/id/upper}}" -o }O} 1 -eq 0 ]
have_ALL_VOLUME_DIRECTORIES=$?
if [ -z "$use_K8S" ]
then
[ {O:VOLUME_NAME{-z "${{VOLUME_NAME/id/upper}}_SECRET_NAME" -o }O} $have_ALL_VOLUME_DIRECTORIES -eq 0 ]
have_ALL_VOLUME_DIRECTORIES=$?
{A:VOLUME_NAME{
case "$(echo "${{VOLUME_NAME/id/upper}}_URL" | tr '[:upper:]' '[:lower:]')" in
    s3://*|http://|https://)
        export {{VOLUME_NAME/id/upper}}_PVC="#"
        export {{VOLUME_NAME/id/upper}}_COS=
        ;;
    *)
        export {{VOLUME_NAME/id/upper}}_PVC=
        export {{VOLUME_NAME/id/upper}}_COS="#"
        ;;
esac
}A}
fi
export enable_custom_app="#"
if [ ! -z "${{NAME/id/upper}}_HTDOCS" ]
then
    if [ ! -d "${{NAME/id/upper}}_HTDOCS" ]
    then
        echo "${{NAME/id/upper}}_HTDOCS: must point to a valid directory"
        have_ALL_VOLUME_DIRECTORIES=0
    else
        export enable_custom_app=
    fi
fi
if [ $# -eq 0 \
    -o "$1" == "up" -a $have_ALL_VOLUME_DIRECTORIES -eq 0 \
    -o "$1" == "provision" -a $have_ALL_VOLUME_DIRECTORIES -eq 0  \
    -o "$1" == "run" -a $have_ALL_VOLUME_DIRECTORIES -eq 0 \
    -o "$1" != "help" -a "$1" != "up" -a "$1" != "down" -a "$1" != "config" -a "$1" != "logs" -a "$1" != "provision" -a "$1" != "run" -a "$1" != "deploy" -a "$1" != "show" ]
then
    dd_display_help
    exit 1
fi
provision() {
{A:HAVE_COS{{{HAVE_COS}}
{O:VOLUME_NAME{    [ -z "${{VOLUME_NAME/id/upper}}_URL" ] || \
        download_file -o "${{VOLUME_NAME/id/upper}}" --untar "${{VOLUME_NAME/id/upper}}_URL" || return 1
}O}
}A}
{O:VOLUME_NAME{    [ {{VOLUME_ISRO}} -eq 0 ] && chmod -fR g+w "${{VOLUME_NAME/id/upper}}"
}O}
    return 0
}
case "$1" in
    help)
        dd_display_help
        exit 0
        ;;
    provision)
        if [ -z "$use_COMPOSE" ]
        then
            provision
            exit $?
        else 
            echo "provision is only available in Docker Compose mode"
            exit 1
        fi
        ;;
    config)
        if [ -z "$use_COMPOSE" ]
        then
            echo "$docker_COMPOSE_YAML" | envsubst | docker-compose -f - -p "$kd_PROJECT_NAME" config
            exit $?
        fi
        if [ -z "$use_K8S" ]
        then
            echo "$kubernetes_YAML" | envsubst | grep -v -E '^(#.*|\s*)$'
            exit $?
        fi
        if [ -z "$use_SWARM" ]
        then
            echo "$docker_COMPOSE_YAML" | envsubst | grep -v -E '^(#.*|\s*)$'
            exit $?
        fi
        ;;
    down)
        if [ -z "$use_COMPOSE" ]
        then
            echo "$docker_COMPOSE_YAML" | envsubst | docker-compose -f - -p "$kd_PROJECT_NAME" down -v
            exit $?
        fi
        if [ -z "$use_SWARM" ]
        then 
            docker stack delete "$kd_PROJECT_NAME"
            exit $?
        fi
        if [ -z "$use_K8S" ]
        then 
            $cur_KUBECTL delete service {{NAME}}
{S:GROUP{   $cur_KUBECTL delete service {{NAME}}-{{GROUP}}
}S}
            $cur_KUBECTL delete deploy {{NAME}}-{{MAIN_POD}}
{G:GROUP{   $cur_KUBECTL delete deploy {{NAME}}-{{GROUP}}
}G}
            exit $?
        fi
        ;;
    logs)
        if [ -z "$use_COMPOSE" ]
        then
            echo "$docker_COMPOSE_YAML" | envsubst | docker-compose -f - -p "$kd_PROJECT_NAME" logs -f $docker_compose_TIMESTAMPS
            exit $?
        else
            echo "logs are only available in Docker Compose mode"
            exit 1
        fi
        ;;
   deploy|up)
        if [ -z "$use_COMPOSE" ]
        then
            [ $provision_ENABLED -eq 0 ] || provision || exit 1
            echo "$docker_COMPOSE_YAML" | envsubst | docker-compose -f - -p "$kd_PROJECT_NAME" up -d
            rc=$?
        fi
        if [ -z "$use_SWARM" ]
        then
            echo "$docker_COMPOSE_YAML" | envsubst | docker stack deploy -c - "$kd_PROJECT_NAME"
            rc=$?
        fi
        if [ -z "$use_K8S" ]
        then
            echo "$docker_COMPOSE_YAML" | envsubst | docker stack deploy -c - "$kd_PROJECT_NAME"
            rc=$?
        fi
        ;;
    run)
        if [ -z "$use_COMPOSE" ]
        then
            [ $provision_ENABLED -eq 0 ] || provision || exit 1
            echo "$docker_COMPOSE_YAML" | envsubst | docker-compose -f - -p "$kd_PROJECT_NAME" up 
            rc=$?
        else
            echo "run is only avaialble in Docker Compose mode"
            exit 1
        fi
        ;;
    show)
        if [ -z "$use_K8S" ]
        then
            $cur_KUBECTL get po -l "flow-group={{NAME}}" &&\
            $cur_KUBECTL get service -l "flow-group={{NAME}}"
            exit $?
        fi
        if [ -z "$use_SWARM" ]
        then
            docker stack ls "$kd_PROJECT_NAME"
            exit $?
        fi
        if [ -z "$use_COMPOSE" ]
        then
            docker ps -a | grep "$kd_PROJECT_NAME"
            exit $?
        fi
        ;;
esac
if [ $rc -eq 0 -a "$1" == "up" ]
then
    echo "{{NAME}} gRPC service listening on port $grpc_PORT"
    echo "{{NAME}} REST service listening on port $rest_PORT"
fi
if [ -z "$use_SWARM" ] 
then
    docker stack ls "$kd_PROJECT_NAME"
fi
if [ -z "$use_K8S" ] 
then
    $cur_KUBECTL get po -l "flow-group={{NAME}}" &&\
    $cur_KUBECTL get service -l "flow-group={{NAME}}"
fi
if [ -z "$use_COMPOSE" ] 
then
    docker ps -a | grep "$kd_PROJECT_NAME"
fi
exit $rc
