#!/bin/bash

##################################################################################
# Docker Compose/Swarm configuration generator for {{NAME}}
# generated from {{MAIN_FILE_SHORT}} ({{MAIN_FILE_TS}})
# with {{FLOWC_NAME}} version {{FLOWC_VERSION}} ({{FLOWC_BUILD}})
#

export kd_PROJECT_NAME={{NAME}}
export cur_KUBECTL=${KUBECTL-kubectl}
{O:GLOBAL_TEMP_VARS{export {{GLOBAL_TEMP_VARS}}
}O}
{O:NODE_NAME{{I?NODE_IMAGE{export {{NAME/id/upper}}_NODE_{{NODE_NAME/id/upper}}_ENDPOINT_DN={{NODE_NAME/id/option/lower}}
export scale_{{NODE_NAME/id/upper}}=${{{NAME/id/upper}}_NODE_{{NODE_NAME/id/upper}}_REPLICAS-{{NODE_SCALE}}}
export image_{{NODE_NAME/id/upper}}=${{{NAME/id/upper}}_{{NODE_NAME/id/upper}}_IMAGE-{{NODE_IMAGE}}}
if [ ! -z "${{NAME/id/upper}}_{{NODE_NAME/id/upper}}_TAG" ]
then
    export image_{{IM_NODE_NAME/id/upper}}=${image_{{NODE_NAME/id/upper}}%:*}:${{NAME/id/upper}}_{{NODE_NAME/id/upper}}_TAG
fi}I}
}O}
export replicas_{{NAME/id/upper}}=${{{NAME/id/upper}}_REPLICAS-{{MAIN_SCALE}}}
{G:GROUP{export replicas_{{NAME/id/upper}}_GROUP_{{GROUP/id/upper}}=${{{NAME/id/upper}}_GROUP_{{GROUP/id/upper}}_REPLICAS-{{GROUP_SCALE}}}
}G}
{A:VOLUME_NAME{export {{VOLUME_NAME/id/upper}}="${{{VOLUME_NAME/id/upper}}-{{VOLUME_LOCAL}}}"
export {{VOLUME_NAME/id/upper}}_SECRET_NAME=${{{VOLUME_NAME/id/upper}}_SECRET_NAME-{{VOLUME_SECRET}}}
export {{VOLUME_NAME/id/upper}}_URL=${{{VOLUME_NAME/id/upper}}_URL-{{VOLUME_COS}}}
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
echo "        Set the Docker Compose project name or the Docker Swarm deployment to NAME (default is $kd_PROJECT_NAME)"
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
echo "    --htdocs-remote <URL> (or set {{NAME/id/upper}}_HTDOCS_URL)"
echo "        Secret URL pointing to htdocs data."
[ ! -z "${{NAME/id/upper}}_HTDOCS_URL" ] && echo "        Currently set to \"${{NAME/id/upper}}_HTDOCS_URL\""
echo ""
echo "    --htdocs-secret-name <SECRET-NAME>  (or set {{NAME/id/upper}}_HTDOCS_SECRET_NAME)"
echo "        Secret name for COS access for the custom application files, if a COS URL was given."
[ ! -z "${{NAME/id/upper}}_HTDOCS_SECRET_NAME" ] && echo "        Currently set to \"${{NAME/id/upper}}_HTDOCS_SECRET_NAME\""
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
echo "    --{{NAME}}-replicas <NUMBER>  (or set {{NAME/id/upper}}_REPLICAS)"
echo "        Number of replicas for the main pod in Kubernetes mode or for the aggregator image in Docker Swarm mode."
echo "        The default is $replicas_{{NAME/id/upper}}."
echo ""
{G:GROUP{
echo "    --pod-{{GROUP}}-replicas <NUMBER>  (or set {{NAME/id/upper}}_GROUP_{{GROUP/id/upper}}_REPLICAS)"
echo "        Number or replicas for pod \"{{GROUP}}\". The default is $replicas_{{NAME/id/upper}}_GROUP_{{GROUP/id/upper}}."
echo ""
}G}
{O:NODE_NAME{{I?NODE_IMAGE{
echo "    --node-{{NODE_NAME}}-replicas <NUMBER>  (or set {{NAME/id/upper}}_NODE_{{NODE_NAME/id/upper}}_REPLICAS)"
echo "       Number of replicas for node \"{{NODE_NAME}}\" in Docker Swarm mode. The default is $scale_{{NODE_NAME/id/upper}}."
echo ""
}I}}O}
echo "    --tag, --image  <NODE-NAME=STRING>"
echo "       Force the image name or tag for node NODE_NAME to STRING. The changes are applied in the order given in the commans line."
echo "       The valid node names are:{N:IM_NODE_NAME{ {{IM_NODE_NAME/id/option/lower}}}N}."
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
    --pod-{{GROUP}}-replicas)
    export replicas_{{NAME/id/upper}}_GROUP_{{GROUP/id/upper}}="$2"
    shift
    shift
    ;;
}G}
{O:NODE_NAME{{I?NODE_IMAGE{
    --node-{{NODE_NAME}}-replicas)
    export scale_{{NODE_NAME/id/upper}}="$2"
    shift
    shift
    ;; 
}I}}O}
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
{O:NODE_NAME{{I?NODE_IMAGE{    if [ $scale_{{NODE_NAME/id/upper}} -gt 1 ]
    then
        export {{NAME/id/upper}}_NODE_{{NODE_NAME/id/upper}}_ENDPOINT_DN="@tasks.${kd_PROJECT_NAME}_${{{NAME/id/upper}}_NODE_{{NODE_NAME/id/upper}}_ENDPOINT_DN}"
    else
        export {{NAME/id/upper}}_NODE_{{NODE_NAME/id/upper}}_ENDPOINT_DN="tasks.${kd_PROJECT_NAME}_${{{NAME/id/upper}}_NODE_{{NODE_NAME/id/upper}}_ENDPOINT_DN}"
    fi
}I}}O}
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
{N:IM_NODE_NAME{
            {{IM_NODE_NAME/id/option/lower}})
                export image_{{IM_NODE_NAME/id/upper}}=${2#*=}
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
{N:IM_NODE_NAME{
            {{IM_NODE_NAME/id/option/lower}})
                export image_{{IM_NODE_NAME/id/upper}}=${image_{{IM_NODE_NAME/id/upper}}%:*}:${2#*=}
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
export enable_main_pod_volumes="{{HAVE_MAIN_GROUP_VOLUMES-#}}"
if [ -z "$enable_custom_app" ]
then
   export enable_main_pod_volumes=
fi

case "$1" in
    up|provision|run)
        [ $have_ALL_VOLUME_DIRECTORIES -ne 0 ]
        rc=$?
        ;;
    down|config|logs|deploy|show|config-debug)
        rc=0
        ;;
    help|"")
        rc=1
        ;;
    *)
        echo "Invalid command '$1'"
        echo ""
        rc=1
esac
if [ $rc -ne 0 ]
then
    dd_display_help
    exit $rc
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
        case "$use_COMPOSE:$use_K8S:$use_SWARM" in
            ":#:#")
                provision 
                exit $?
                ;;
            *)
                echo "provision is only available in Docker Compose mode"
                exit 1
                ;;
        esac
        ;;
    run)
        case "$use_COMPOSE:$use_K8S:$use_SWARM" in
            ":#:#")
                [ $provision_ENABLED -eq 0 ] || provision || exit 1
                echo "$docker_COMPOSE_YAML" | envsubst | docker-compose -f - -p "$kd_PROJECT_NAME" up 
                rc=$?
                ;;
            *)
                echo "run is only avaialble in Docker Compose mode"
                exit 1
                ;;
        esac
        ;;
    logs)
        case "$use_COMPOSE:$use_K8S:$use_SWARM" in
            ":#:#")
                echo "$docker_COMPOSE_YAML" | envsubst | docker-compose -f - -p "$kd_PROJECT_NAME" logs -f $docker_compose_TIMESTAMPS
                exit $?
                ;;
            *)
                echo "logs are avaialble only in Docker Compose mode"
                exit 1
                ;;
        esac
        ;;
    config)
        case "$use_COMPOSE:$use_K8S:$use_SWARM" in
            ":#:#")
                echo "$docker_COMPOSE_YAML" | envsubst | docker-compose -f - -p "$kd_PROJECT_NAME" config
                ;;
            "#::#")
                echo "$kubernetes_YAML" | envsubst | grep -v -E '^(#.*|\s*)$'
                ;;
            "#:#:")
                echo "$docker_COMPOSE_YAML" | envsubst | grep -v -E '^(#.*|\s*)$'
                ;;
        esac
        exit $?
        ;;
    config-debug)
        case "$use_COMPOSE:$use_K8S:$use_SWARM" in
            ":#:#")
                echo "$docker_COMPOSE_YAML" | envsubst
                ;;
            "#::#")
                echo "$kubernetes_YAML" | envsubst 
                ;;
            "#:#:")
                echo "$docker_COMPOSE_YAML" | envsubst
                ;;
        esac
        exit $?
        ;;
   deploy|up)
        case "$use_COMPOSE:$use_K8S:$use_SWARM" in
            ":#:#")
                [ $provision_ENABLED -eq 0 ] || provision || exit 1
                echo "$docker_COMPOSE_YAML" | envsubst | docker-compose -f - -p "$kd_PROJECT_NAME" up -d
                ;;
            "#::#")
                echo "$kubernetes_YAML" | envsubst | grep -v -E '^(#.*|\s*)$' | $cur_KUBECTL create -f -
                ;;
            "#:#:")
                echo "$docker_COMPOSE_YAML" | envsubst | docker stack deploy -c - "$kd_PROJECT_NAME"
                ;;
        esac
        rc=$?
        ;;
    down)
        case "$use_COMPOSE:$use_K8S:$use_SWARM" in
            ":#:#")
                echo "$docker_COMPOSE_YAML" | envsubst | docker-compose -f - -p "$kd_PROJECT_NAME" down -v
                ;;
            "#::#")
                $cur_KUBECTL delete service {{NAME/id/option/lower}}
{S:GROUP{           $cur_KUBECTL delete service {{NAME/id/option/lower}}-{{GROUP/id/option/lower}}
}S}
                $cur_KUBECTL delete deploy {{NAME/id/option/lower}}
{G:GROUP{           $cur_KUBECTL delete deploy {{NAME/id/option/lower}}-{{GROUP/id/option/lower}}
}G}
                ;;
            "#:#:")
                docker stack down "$kd_PROJECT_NAME"
                ;;
        esac
        exit $?
        ;;
    show)
        case "$use_COMPOSE:$use_K8S:$use_SWARM" in
            ":#:#")
                docker ps -a | grep "$kd_PROJECT_NAME"
                ;;
            "#::#")
                $cur_KUBECTL get po -l "flow-group={{NAME}}" &&\
                $cur_KUBECTL get service -l "flow-group={{NAME}}"
                ;;
            "#:#:")
                docker stack ps "$kd_PROJECT_NAME"
                ;;
        esac
        exit $?
        ;;
esac
if [ $rc -eq 0 -a "$1" == "up" ]
then
    echo "{{NAME}} gRPC service listening on port $grpc_PORT"
    echo "{{NAME}} REST service listening on port $rest_PORT"
fi
if [ -z "$use_SWARM" ] 
then
    docker stack ps "$kd_PROJECT_NAME"
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
