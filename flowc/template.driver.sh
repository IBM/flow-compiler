#!/bin/bash

##################################################################################
# Docker Compose/Swarm configuration generator for {{NAME}}
# generated from {{MAIN_FILE_SHORT}} ({{MAIN_FILE_TS}})
# with {{FLOWC_NAME}} version {{FLOWC_VERSION}} ({{FLOWC_BUILD}})
#

export kd_PROJECT_NAME={{NAME}}
export cur_KUBECTL=${KUBECTL-kubectl}
{D:DEFN{export fd_{{NAME/id/upper}}_FD_{{DEFN/id/upper}}="${{{NAME/id/upper}}_FD_{{DEFN/id/upper}}-{{DEFV}}}"
}D}
{F:EFR_FILENAME{flowc_tmp_FN{{EFR_ID}}={{EFR_FILENAME}}
}F}
{O:NODE_NAME{{I?NODE_IMAGE{export scale_{{NODE_NAME/id/upper}}=${{{NAME/id/upper}}_NODE_{{NODE_NAME/id/upper}}_REPLICAS-{{NODE_SCALE}}}
export image_{{NODE_NAME/id/upper}}=${{{NAME/id/upper}}_{{NODE_NAME/id/upper}}_IMAGE-{{NODE_IMAGE}}}
if [ ! -z "${{NAME/id/upper}}_{{NODE_NAME/id/upper}}_TAG" ]
then
    export image_{{IM_NODE_NAME/id/upper}}=${image_{{NODE_NAME/id/upper}}%:*}:${{NAME/id/upper}}_{{NODE_NAME/id/upper}}_TAG
fi}I}
}O}
export replicas_{{NAME/id/upper}}=${{{NAME/id/upper}}_REPLICAS-{{MAIN_SCALE}}}
{G:GROUP{export replicas_{{NAME/id/upper}}_GROUP_{{GROUP/id/upper}}=${{{NAME/id/upper}}_GROUP_{{GROUP/id/upper}}_REPLICAS-{{GROUP_SCALE}}}
}G}
{V?VOLUME_COUNT{
{A:VOLUME_NAME{export {{VOLUME_NAME/id/upper}}="${{{VOLUME_NAME/id/upper}}-{{VOLUME_LOCAL}}}"
export {{VOLUME_NAME/id/upper}}_SECRET_NAME=${{{VOLUME_NAME/id/upper}}_SECRET_NAME-{{VOLUME_SECRET}}}
export {{VOLUME_NAME/id/upper}}_URL=${{{VOLUME_NAME/id/upper}}_URL-{{VOLUME_COS}}}
}A}
}V}
export use_MODE=compose
export use_COMPOSE=
export use_SWARM="#"
export use_K8S="#"
export default_RUNTIME=
export docker_compose_TIMESTAMPS=
{V?RW_VOLUME_COUNT{export docker_compose_RW_GID=$(id -gn)
}V}
export grpc_PORT=${{{NAME/id/upper}}_GRPC_PORT-{{MAIN_PORT}}}
export rest_PORT=${{{NAME/id/upper}}_REST_PORT-{{REST_PORT}}}
dd_display_help() {
echo "Docker Compose/Swarm and Kubernetes configuration generator for {{NAME}}"
echo "From {{MAIN_FILE}} ({{MAIN_FILE_TS}})"
echo ""
echo "Usage $(basename "$0") <up|down|config|logs|run> [-p] [-r] [-s] [-C] [-T] [--project-name NAME] [--grpc-port PORT] [--rest-port PORT] [--htdocs DIRECTORY] {V?VOLUME_COUNT{{O:VOLUME_NAME{[--mount-{{VOLUME_NAME/option}} DIRECTORY] }O}}V}"
echo "   or $(basename "$0") <up|down|config> -S [--project-name NAME] [--grpc-port PORT] [--rest-port PORT]"
echo "   or $(basename "$0") -K [--project-name NAME] <delete|deploy|show|config>"
echo "   or $(basename "$0") [-T] <run|logs>"
{V?VOLUME_COUNT{
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
{V?VOLUME_COUNT{echo "   provision  Download the external data into local directories so it can be mounted inside the running containers (Docker Compose only)"
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
{V?VOLUME_COUNT{
echo "    --data-directory (or set {{NAME/id/upper}}_DATA_DIR)"
echo "        Set the data directory local volume mounts for Docker Compose and Swarm. Current directory is the default."
echo ""
}V}
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
{V?VOLUME_COUNT{
{O:VOLUME_NAME{
echo "    --mount-{{VOLUME_NAME/option-}} <DIRECTORY>  (or set {{VOLUME_NAME/id/upper}})"
echo "        Override the path to be mounted for {{VOLUME_NAME}} (default is ${{VOLUME_NAME/id/upper}})"
printf "        "{{VOLUME_COMMENT/sh}}"\n"
echo ""
echo "    --secret-{{VOLUME_NAME/option-}} <SECRET-NAME>  (or set {{VOLUME_NAME/id/upper}}_SECRET_NAME)"
echo "        Secret name for COS access for {{VOLUME_NAME}}, default is \"${{VOLUME_NAME/id/upper}}_SECRET_NAME\""
echo ""
echo "    --remote-{{VOLUME_NAME/option-}} <URL> (or set {{VOLUME_NAME/id/upper}}_URL)"
echo "        Set the URL to the remote resource for {{VOLUME_NAME}} (default is ${{VOLUME_NAME/id/upper}}_URL)"
echo ""
}O}
}V}
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
{I:DEFN{   
echo "    --fd-{{DEFN/option}} {{DEFT}}"
echo "       Override the value of variable '{{DEFN}}'"
echo ""
}I}
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
{V?VOLUME_COUNT{
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
}V}
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
    --data-directory)
    export {{NAME/id/upper}}_DATA_DIR="$2"
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
    export provision_ARGS=--no-download
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
{I:DEFN{   
    --fd-{{DEFN/option}})
    export fd_{{NAME/id/upper}}_FD_{{DEFN/id/upper}}="$2"
    shift
    shift
    ;;
}I}
    *)
    args+=("$1")
    shift
    ;;
esac
done
set -- "${args[@]}"
{V?VOLUME_COUNT{
{O:VOLUME_NAME{export {{VOLUME_NAME/id/upper}}="${{{VOLUME_NAME/id/upper}}-$flow_{{VOLUME_NAME/id/upper}}}"
if [ "${{{VOLUME_NAME/id/upper}}:0:1}" != "/" ]
then
    export {{VOLUME_NAME/id/upper}}="${{{NAME/id/upper}}_DATA_DIR-$(pwd)}/${{VOLUME_NAME/id/upper}}"
fi
}O}
}V}

if ! which envsubst > /dev/null 2>&1 
then
    echo "envsubst command not found" 2>&1
fi

kubernetes_YAML=$(cat <<"ENDOFYAML"
{{KUBERNETES_YAML}}
ENDOFYAML
)
docker_COMPOSE_YAML=$(cat <<"ENDOFYAML"
{{DOCKER_COMPOSE_YAML}}
ENDOFYAML
)

is_cosurl() {
    case "$(echo "$1" | tr '[:upper:]' '[:lower:]')" in
        s3://*|http://|https://)
            return 0
            ;;
        *)
            return 1
            ;;
    esac
}

[ {O:VOLUME_NAME{-z "${{VOLUME_NAME/id/upper}}" -o }O} 1 -eq 0 ]
have_ALL_VOLUME_DIRECTORIES=$?
if [ -z "$use_K8S" ]
then
[ {O:VOLUME_NAME{-z "${{VOLUME_NAME/id/upper}}_SECRET_NAME" -o }O} $have_ALL_VOLUME_DIRECTORIES -eq 0 ]
have_ALL_VOLUME_DIRECTORIES=$?
{A:VOLUME_NAME{
if is_cosurl "${{VOLUME_NAME/id/upper}}_URL"
then
    export {{VOLUME_NAME/id/upper}}_PVC="#"
    export {{VOLUME_NAME/id/upper}}_COS=
else
    export {{VOLUME_NAME/id/upper}}_PVC=
    export {{VOLUME_NAME/id/upper}}_COS="#"
fi
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
{O:NODE_NAME{{I?NODE_IMAGE{case "$use_COMPOSE:$use_K8S:$use_SWARM:$scale_{{NODE_NAME/id/upper}}" in 
    ":#"*) 
        export ehr_{{NAME/id/upper}}_{{NODE_NAME/id/upper}}={{NODE_NAME/id/option/lower}}
        ;;
    "#::"*) 
        export ehr_{{NAME/id/upper}}_{{NODE_NAME/id/upper}}={{NAME/id/option/lower}}-{{NODE_NAME/id/option/lower}}
        ;;
    "#:#::1") 
        export ehr_{{NAME/id/upper}}_{{NODE_NAME/id/upper}}="tasks.${kd_PROJECT_NAME}_{{NODE_NAME/id/option/lower}}"
        ;;
    "#:#::"*) 
        export ehr_{{NAME/id/upper}}_{{NODE_NAME/id/upper}}="@tasks.${kd_PROJECT_NAME}_{{NODE_NAME/id/option/lower}}"
        ;;
esac
}I}}O}
if which yq > /dev/null 
then
    flow_Uyq="yq e - -P -o y"
else
    flow_Uyq="cat -"
fi
case "$1" in
    up|provision|run)
        [ $have_ALL_VOLUME_DIRECTORIES -ne 0 ]
        rc=$?
        ;;
    down|config|logs|deploy|show|config-debug)
        rc=0
{F:EFR_FILENAME{        if [ -r "$flowc_tmp_FN{{EFR_ID}}" ]
        then
            export flowc_tmp_FILE{{EFR_ID}}="$(cat "$flowc_tmp_FN{{EFR_ID}}")"
        else
            echo "$flowc_tmp_FN{{EFR_ID}}: cannot read file"
            rc=1
        fi
}F}
{A?EFR_COUNT{        [ $rc -eq 0 ] || exit 1
}A}
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
chkpath() {
    pushd "$1" > /dev/null 2>&1
    if [ $? -ne 0 ]
    then
        echo -n "$1"
        return 1
    fi
    pwd | tr -d $'\r'$'\n'
    popd > /dev/null 2>&1
    return 0
}
provision() {
    [ "$1" == "--no-download" ]
    local skip_download=$?
    local ec=0
    local rc=0
{O:VOLUME_NAME{    [ $skip_download -eq 0 -o -z "${{VOLUME_NAME/id/upper}}_URL" ] || \
        download_file -o "${{VOLUME_NAME/id/upper}}" --untar "${{VOLUME_NAME/id/upper}}_URL" || return 1
    rc=0
    {{VOLUME_NAME/id/upper}}="$(chkpath "${{VOLUME_NAME/id/upper}}")"
    if [ $? -ne 0 ]
    then
        ec=$((ec + 1))
        rc=1
        echo "${{VOLUME_NAME/id/upper}}: not a directory" 1>&2
    else
        export {{VOLUME_NAME/id/upper}}
    fi

    [ $rc -eq 1 -o $skip_download -eq 0 -o {{VOLUME_ISRO}} -eq 1 ] || chmod -fR g+w "${{VOLUME_NAME/id/upper}}"
}O}
    return $ec
}
case "$1" in
    help)
        dd_display_help
        exit 0
        ;;
    provision)
        case "$use_COMPOSE:$use_K8S:$use_SWARM" in
            ":#:#"|"#:#:")
                provision $provision_ARGS
                exit $?
                ;;
            *)
                echo "provision is not available in Kubernetes mode" 2>&1
                exit 1
                ;;
        esac
        ;;
    run)
        case "$use_COMPOSE:$use_K8S:$use_SWARM" in
            ":#:#")
                provision $provision_ARGS || exit 1
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
                provision --no-download
                echo "$docker_COMPOSE_YAML" | envsubst | docker-compose -f - -p "$kd_PROJECT_NAME" config
                ;;
            "#::#")
                echo "$kubernetes_YAML" | envsubst | grep -v -E '^(#.*|\s*)$' | $flow_Uyq
                ;;
            "#:#:")
                provision --no-download
                echo "$docker_COMPOSE_YAML" | envsubst | grep -v -E '^(#.*|\s*)$' | $flow_Uyq
                ;;
        esac
        exit $?
        ;;
    config-debug)
        case "$use_COMPOSE:$use_K8S:$use_SWARM" in
            ":#:#")
                provision --no-download
                echo "$docker_COMPOSE_YAML" | envsubst 
                ;;
            "#::#")
                echo "$kubernetes_YAML" | envsubst 
                ;;
            "#:#:")
                provision --no-download
                echo "$docker_COMPOSE_YAML" | envsubst 
                ;;
        esac
        exit $?
        ;;
   deploy|up)
        case "$use_COMPOSE:$use_K8S:$use_SWARM" in
            ":#:#")
                provision $provision_ARGS || exit 1
                echo "$docker_COMPOSE_YAML" | envsubst | docker-compose -f - -p "$kd_PROJECT_NAME" up -d
                ;;
            "#::#")
                echo "$kubernetes_YAML" | envsubst | grep -v -E '^(#.*|\s*)$' | $cur_KUBECTL create -f -
                ;;
            "#:#:")
                provision $provision_ARGS || exit 1
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
