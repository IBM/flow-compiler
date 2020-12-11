#!/bin/bash

##################################################################################
# Kubernetes configuration generator for {{NAME}}
# generated from {{INPUT_FILE}} ({{MAIN_FILE_TS}})
# with {{FLOWC_NAME}} version {{FLOWC_VERSION}} ({{FLOWC_BUILD}})
#
{A:VOLUME_NAME{flow_{{VOLUME_NAME/id/upper}}="${{{VOLUME_NAME/id/upper}}-{{VOLUME_COS}}}"
if [ -z "$flow_{{VOLUME_NAME/id/upper}}" ]
then
    flow_{{VOLUME_NAME/id/upper}}="{{VOLUME_PVC}}"
fi
export {{VOLUME_NAME/id/upper}}="$flow_{{VOLUME_NAME/id/upper}}"
flow_{{VOLUME_NAME/id/upper}}_SECRET_NAME=${{{VOLUME_NAME/id/upper}}_SECRET_NAME-{{VOLUME_SECRET}}}
export {{VOLUME_NAME/id/upper}}_SECRET_NAME=$flow_{{VOLUME_NAME/id/upper}}_SECRET_NAME
}A}
cur_KUBECTL=${KUBECTL-kubectl}
kube_PROJECT_NAME={{NAME}}
export replicas_{{NAME/id/upper}}=${{{NAME/id/upper}}_REPLICAS-{{MAIN_SCALE}}}
{G:GROUP{export replicas_{{NAME/id/upper}}_{{GROUP/id/upper}}=${{{NAME/id/upper}}_{{GROUP/id/upper}}_REPLICAS-{{GROUP_SCALE}}}
}G}
kubernetes_YAML=$(cat <<"ENDOFYAML"
{{KUBERNETES_YAML}}
ENDOFYAML
)
args=()
while [ $# -gt 0 ]
do
case "$1" in
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
}A}
    --htdocs)
    export {{NAME/id/upper}}_HTDOCS="$2"
    shift
    shift
    ;;
    --htdocs-secret-name)
    export {{NAME/id/upper}}_HTDOCS_SECRET_NAME="$2"
    shift
    shift
    ;;
    --{{NAME}}-replicas)
    export replicas_{{NAME/id/upper}}="$2"
    shift
    shift
    ;;
{G:GROUP_NAME{
    --{{GROUP}}-replicas)
    export replicas_{{NAME/id/upper}}_{{GROUP_UPPERID}}="$2"
    shift
    shift
    ;;
}G}
    *)
    args+=("$1")
    shift
    ;;
esac
done
set -- "${args[@]}"

{A:VOLUME_NAME{
case "$(echo "${{VOLUME_NAME/id/upper}}" | tr '[:upper:]' '[:lower:]')" in
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

case "$(echo "${{NAME/id/upper}}_HTDOCS" | tr '[:upper:]' '[:lower:]')" in
    s3://*|http://|https://)
        export htdocs_PVC="#"
        export htdocs_COS=
        ;;
    *)
        export htdocs_PVC=
        export htdocs_COS="#"
        ;;
esac

have_ALL_VOLUME_CLAIMS=1

export enable_htdocs="#"
if [ ! -z "${{NAME/id/upper}}_HTDOCS" ]
then
    export enable_htdocs=""
    if [ -z "$htdocs_COS" -a -z "${{NAME/id/upper}}_HTDOCS_SECRET_NAME" ]
    then 
        have_ALL_VOLUME_CLAIMS=0
        echo "Please set the secret name for accessing ${{NAME/id/upper}}_HTDOCS"
    fi
fi


{O:VOLUME_NAME{
if [ -z "${{VOLUME_NAME/id/upper}}" ] 
then
    have_ALL_VOLUME_CLAIMS=0
    echo "Please set information for {{VOLUME_NAME/option}}" 1>&2
else
    if [ -z "${{VOLUME_NAME/id/upper}}_PVC" -a -z "${{VOLUME_NAME/id/upper}}_SECRET_NAME" ]
    then
        have_ALL_VOLUME_CLAIMS=0
        echo "Please set the secret name for accessing ${{VOLUME_NAME/id/upper}}" 1>&2
    fi
fi
}O}

if [ $# -eq 0 -o "$1" == "deploy" -a $have_ALL_VOLUME_CLAIMS -eq 0 -o "$1" != "deploy" -a "$1" != "delete" -a "$1" != "config" -a "$1" != "show" ]
then
echo "Kubernetes configuration generator for {{NAME}}"
echo "From {{MAIN_FILE}} ({{MAIN_FILE_TS}})"
echo ""
echo "Usage $(basename "$0") <deploy|config> [OPTIONS]"
# {O:VOLUME_NAME/option{--mount-{{VOLUME_NAME/option}} <VOLUME-CLAIM-NAME> }O} --{{NAME}}-replicas <NUM> {G:GROUP{--{{GROUP}}-replicas <NUM> }G}
echo "   or $(basename "$0") <show|delete>"
echo ""
echo "Commands:"
echo "   deploy     Invoke kubectl deploy with the generated confing file"
echo "   config     Display the configuration file that is sent to kubectl"
echo "   show       Display the status of the deployment"
echo "   delete     Delete all objects associated with this deployment"
echo ""
echo "Options:"
{O:VOLUME_NAME{
    echo   "    --mount-{{VOLUME_NAME/option-}} <VOLUME-CLAIM-LABEL|CLOUD-OBJECT-STORE-URL>  (or set \${{VOLUME_NAME/id/upper}})"
    echo   "        Default is \"${{VOLUME_NAME/id/upper}}\""
    printf "        "{{VOLUME_HELP}}"\n"
    echo ""
    echo   "    --secret-{{VOLUME_NAME/option-}} <SECRET-NAME>  (or set \${{VOLUME_NAME/id/upper}}_SECRET_NAME)"
    printf "        Secret name for COS access for {{VOLUME_NAME/option}}, default is \"${{VOLUME_NAME/id/upper}}_SECRET_NAME\"\n"
    echo ""
}O}
    echo   "    --htdocs <VOLUME-CLAIM-LABEL|CLOUD-OBJECT-STORE-URL>  (or set \${{NAME/id/upper}}_HTDOCS)"
    echo   "        Default is \"${{NAME/id/upper}}_HTDOCS\""
    echo   "        Volume or COS URL for tarball with custom application files"
    echo ""
    echo   "    --htdocs-secret-name <SECRET-NAME>  (or set \${{NAME/id/upper}}_HTDOCS_SECRET_NAME)"
    printf "        Secret name for COS access for the custom application files, default is \"${{NAME/id/upper}}_HTDOCS_SECRET_NAME\"\n"
    echo ""
    echo   "    --{{NAME}}-replicas <NUMBER>  (or set \${{NAME/id/upper}}_REPLICAS)"
    echo   "        Number of replicas for the main pod [{{MAIN_GROUP_NODES}}] default is $replicas_{{NAME/id/upper}}"
    echo ""

{G:GROUP{
    echo   "    --{{GROUP}}-replicas <NUMBER>  (or set \${{NAME/id/upper}}_{{GROUP/id/upper}}_REPLICAS)"
    echo   "        Number or replicas for pod \"{{GROUP}}\" [{{GROUP_NODES}}] default is $replicas_{{NAME/id/upper}}_{{GROUP/id/upper}}"
    echo ""
}G}
echo "This script uses '$(which $cur_KUBECTL)'. Set \$KUBECTL to override."
echo ""
echo "Note: kubectl port-forward service/{{NAME}} --address 0.0.0.0 8443:8081"
echo ""
echo "Note: secret template for COS access:"
echo ""
echo "apiVersion: v1"
echo "kind: Secret"
echo "metadata:"
echo "  name: cos-access"
echo "data:"
echo "  SECRET_ACCESS_KEY: xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
echo "  ACCESS_KEY_ID: xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
echo ""
exit 1
fi
if [ "$1" == "config" ]
then
echo "$kubernetes_YAML" | envsubst | grep -v -E '^(#.*|\s*)$'
exit 0
fi
if [ "$1" == "deploy" ]
then
echo "$kubernetes_YAML" | envsubst | $cur_KUBECTL apply -f -
exit $?
fi
if [ "$1" == "show" ]
then
$cur_KUBECTL get po -l "flow-group={{NAME}}" &&\
$cur_KUBECTL get service -l "flow-group={{NAME}}"
exit $?
fi
if [ "$1" == "delete" ]
then
    $cur_KUBECTL delete service {{NAME}}
    {S:GROUP{$cur_KUBECTL delete service {{NAME}}-{{GROUP}}
}S}
    $cur_KUBECTL delete deploy {{NAME}}-{{MAIN_POD}}
    {G:GROUP{$cur_KUBECTL delete deploy {{NAME}}-{{GROUP}}
}G}
## ignore errors
exit 0
fi
