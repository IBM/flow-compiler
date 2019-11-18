#!/bin/bash

get_credentials() {
    if [ -z "$rget_EMBEDDED_KEY_TOOL" ]
    then
        source "$(dirname "$0")/rr-keys.sh"
    else
        {{RR_KEYS_SH}}
    fi
}

last_modified_from_headers() {
    local HEADERS="$1"
    local LASTMOD="$(echo "$HEADERS" | grep -i 'last-modified' | cut -d ' ' -f 2- | cut -d ',' -f 2- | sed -e 's/^[[:space:]]*//' -e 's/[[:space:]]*$//')"
    if [ "$(uname -s)" == "Linux" ]
    then
        /bin/date -d "$LASTMOD" '+%Y%m%d%H%M.%S' 2> /dev/null
    else
        /bin/date -j -f '%d %b %Y %H:%M:%S %Z' "$LASTMOD" '+%Y%m%d%H%M.%S' 2> /dev/null
    fi
}
rget_HELP=0
args=()
rget_API_KEYS=()
rget_OUTPUT=.
while [ $# -gt 0 ]
do
case "$1" in
    -o|--output)
    rget_OUTPUT="${2-.}"
    shift; shift
    ;;
    -u|--untar)
    rget_UNTAR=-u
    shift
    ;;
    -k|--key-file)
    rget_API_KEYS+=("-f")
    rget_API_KEYS+=("$2")
    shift; shift
    ;;
    -a|--aws-file)
    rget_API_KEYS+=("-a")
    rget_API_KEYS+=("$2")
    shift; shift
    ;;
    -f|--force)
    rget_FORCE=-f
    shift
    ;;
    -s|--silent)
    rget_SILENT=-sS
    shift
    ;;
    -h|--help)
    rget_HELP=1
    shift
    ;;
    *)
    args+=("$1")
    shift
    ;;
esac
done
set -- "${args[@]}"

rget_URL="$1"
if [ $rget_HELP -ne 0 -o -z "$rget_URL" ]
then 
    echo "Usage $0 RESOURCE-NAME"
    echo ""
    echo "options:"
    echo "  -o, --output DIRECTORY       destination for the downloaded file"
    echo "  -f, --force                  download the file even if a newer file exists"
    echo "  -s, --silent                 don't show download progress"
    echo "  -u, --untar                  untar the file at the destination, output must be a directory"
    echo "  -k, --key-file FILE          look here for API keys" 
    echo "  -a, --aws-file FILE          look here for AWS style credentials" 
    echo ""
    echo "note: When -f is not given, $0 looks first for the environment variable API_KEY."
    echo "If the variabile is not found or is empty, it looks for a file named .api-key or"
    echo ".api-keys in the current directory and then in the home directory."
    echo ""
    [ $rget_HELP -ne 0 ] && exit 0
    exit 1
fi

rget_URL_SCHEME=$(echo "${rget_URL%%://*}" | tr '[:upper:]' '[:lower:]')

case "$rget_URL_SCHEME" in
    http|https)
        rget_KEY_SIZE=1
        use_API_KEY="$API_KEY"
        rget_METHOD=artifactory
        ;;
    s3)
        rget_KEY_SIZE=2
        use_API_KEY="$(tr ':' $'\t' <<< "$API_KEY")"
        rget_URL=https://${rget_URL#*://}
        rget_METHOD=s3
        ;;
    *)
        echo "URL scheme must be either http(s) or s3"
        exit 1
        ;;
esac

## If the API_KEY environment variable was not set, read the fiels and keep the 
## appropriate credentials 
if [ -z "$use_API_KEY" ]
then
try_API_KEYS=$(get_credentials "${rget_API_KEYS[@]}")
rget_API_KEYS=()
while IFS=$'\n' read KEY 
do
    [ -z "$KEY" ] && continue

    ## S3 credentials are user-id and key
    [ $rget_KEY_SIZE -eq 2 -a -z "$(cut -f2 -s <<< "$KEY")" ] && continue

    ## Artifactory credentials are key only
    [ $rget_KEY_SIZE -eq 1 -a ! -z "$(cut -f2 -s <<< "$KEY")" ] && continue
    rget_API_KEYS+=("$KEY")

done <<< "$try_API_KEYS"
else 
    rget_API_KEYS=("$use_API_KEY")
fi
if [ ${#rget_API_KEYS[@]} -eq 0 ]
then
    echo "No appropriate credentials found, cannot continue"
    exit 1
fi

aws_curl_options() {
    local METHOD=${1-GET}
    local URL=$2
    local ACCESS_KEY=$(cut -f2 <<< "$3")
    local ACCESS_ID=$(cut -f1 <<< "$3")
    local CONTENT_TYPE=${4-application/x-compressed-tar}

    local URL_NO_SCHEME=${URL#*://}
    local URL_HOST_PORT=${URL_NO_SCHEME%%/*}
    local HOST=${URL_HOST_PORT%%:*}
    local RESOURCE=/${URL_NO_SCHEME#*/}
    local DATE_VALUE="$(date +'%a, %d %b %Y %H:%M:%S %z')" 

    local TO_SIGN=$METHOD$'\n\n'"$CONTENT_TYPE"$'\n'"$DATE_VALUE"$'\n'$RESOURCE
    local SIGNATURE=$(head -c -1 <<< "$TO_SIGN" | openssl sha1 -hmac "$ACCESS_KEY" -binary | base64)
    local EXTRA=
    if [ "$METHOD" == "HEAD" ]
    then
        EXTRA="--head -sS"
    fi
    curl -f $EXTRA $rget_SILENT -H "Host: $HOST" -H "Date: $DATE_VALUE" -H "Content-Type: $CONTENT_TYPE" \
         -H "Authorization: AWS $ACCESS_ID:$SIGNATURE" $URL --output -
}
get_headers() {
    local METHOD="$1"
    local URL="$2"
    local KEY="$3"
    case $METHOD in
        artifactory)
            curl -fsS --head -H "X-JFrog-Art-Api:$KEY" $URL
            ;;
        s3)
            aws_curl_options HEAD $URL "$KEY"
            ;;
    esac
}
get_file() {
    local METHOD="$1"
    local URL="$2"
    local KEY="$3"
    case $METHOD in
        artifactory)
            curl -f $rget_SILENT -H "X-JFrog-Art-Api:$KEY" "$URL" --output -
            ;;
        s3)
            aws_curl_options GET $URL "$KEY"
            ;;
    esac
}
## Get last-modified for the remote resource
rc=1
rget_MTS=
use_API_KEY=
for KEY in "${rget_API_KEYS[@]}"
do
    check_HEADERS="$(get_headers $rget_METHOD $rget_URL "$KEY")"
    rc=$?
    [ $rc -ne 0 ] && continue
    use_API_KEY="$KEY"
    rget_MTS=$(last_modified_from_headers "$check_HEADERS")
    break
done

[ $rc -ne 0 ] && echo "Failed to get resource" && exit $rc

rget_FILE="$(basename "$rget_URL")"
[ -d "$rget_OUTPUT" ] || mkdir -p "$rget_OUTPUT"

if [ -z "$rget_FORCE" ]
then
    touch -t "$rget_MTS" "$rget_OUTPUT/tmp$$"
    rget_HAVE_FILE="$(find "$rget_OUTPUT" -newer "$rget_OUTPUT/tmp$$" -name "$rget_FILE")"
    rm -f "$rget_OUTPUT/tmp$$"
fi
 
if [ -z "$rget_HAVE_FILE" ] 
then

case "$rget_FILE" in
    *.tgz|*.tar.gz)
        tar_Z=-z
    ;;
    *.tar.bz2)
        tar_Z=-j
    ;;
    *.tar.xz)
        tar_Z=--xz
    ;;
    *)
        tar_Z=
    ;;
esac

cd "$rget_OUTPUT"
echo "Downloading $rget_URL"

if [ -z "$rget_UNTAR" ] 
then
    get_file $rget_METHOD $rget_URL "$use_API_KEY"  > "$rget_FILE"
else 
    get_file $rget_METHOD $rget_URL "$use_API_KEY" | tar -x $tar_Z  && touch "$rget_FILE"
fi
else
    echo "$rget_HAVE_FILE is newer than the remote file" 
fi
