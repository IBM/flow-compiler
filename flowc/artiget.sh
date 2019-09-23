#!/bin/bash

art_get_UNTAR=0
art_get_OUTPUT=.
art_get_FORCE=0
art_get_HELP=0
art_API_KEYS=

function clsp() {
    echo "$1" | tr $'\n' ' ' | sed -e 's/^[[:space:]]*//' -e 's/[[:space:]]*$//' -e 's/[[:space:]][[:space:]]*/ /g'
}

if [ -f ~/.api-key ]
then
    art_API_KEYS="$(cat ~/.api-key) $art_API_KEYS"
fi
if [ -f ~/.api_key ]
then
    art_API_KEYS="$(cat ~/.api_key) $art_API_KEYS"
fi
if [ -f .api-key ] 
then
    art_API_KEYS="$(cat .api-key) $art_API_KEYS"
fi
if [ -f .api_key ] 
then
    art_API_KEYS="$(cat .api_key) $art_API_KEYS"
fi
art_API_KEYS="$API_KEY $art_API_KEYS"

curl_SILENT=
args=()
while [ $# -gt 0 ]
do
case "$1" in

    -o|--output)
    art_get_OUTPUT="${2-.}"
    shift; shift
    ;;

    -u|--untar)
    art_get_UNTAR=1
    shift
    ;;

    --api-key)
    art_API_KEYS="$2 $art_API_KEYS"
    shift; shift
    ;;

    --api-key-file)
    if [ -f "$2" ] 
    then
        art_API_KEYS="$(cat "$2") $art_API_KEYS"
    fi
    shift; shift
    ;;

    -f|--force)
    art_get_FORCE=1
    shift
    ;;

    -s|--silent)
    curl_SILENT=-s
    shift
    ;;

    --help)
    art_get_HELP=1
    shift
    ;;

    *)
    args+=("$1")
    shift
    ;;
esac
done
set -- "${args[@]}"

art_API_KEYS=$(clsp "$art_API_KEYS")

if [ $art_get_HELP -ne 0 -o -z "$1" ]
then 
    echo "Usage $0 RESOURCE-NAME"
    echo ""
    echo "options:"
    echo "  --help                              print this and exit"
    echo "  -o, --output <FILE|DIRECTORY>       destination for the downloaded file"
    echo "  -f, --force                         download the file even if a newer file exists"
    echo "  -s, --silent                        don't show download progress"
    echo "  -u, --untar                         untar the file at the destination, output must be a directory"
    echo "  --api-key <STRING>                  API key"
    echo "  --api-key-file <FILE>               file with the API key"
    echo ""
    echo "note: $0 looks for the environment variable API_KEY."
    echo "If not found, it looks for a file named .api-key in the current directory and then in the home directory."
    echo ""
    [ $art_get_HELP -ne 0 ] && exit 0
    exit 1
fi

chk_url="$(echo "$1" | tr '[:lower:]' '[:upper:]')"
if [ "${chk_url:0:7}" == "HTTP://" -o "${chk_url:0:8}" == "HTTPS://" ]
then
    art_get_URL="$1"
else
    if [ ! -z "$ARTIFACTORY_URL" ] 
    then
        art_get_URL="$ARTIFACTORY_URL/$1"
    else
        echo "Argument must be an HTTP URL"
        exit $1
    fi
fi

art_get_FILE="$(basename "$art_get_URL")"
case "$art_get_FILE" in
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

art_get_HAVE_FILE=
download_API_KEY=

rc=1
for api_key in $art_API_KEYS 
do
    HEADERS=$(curl -f -s -I -H "X-JFrog-Art-Api:$api_key" "$art_get_URL")
    rc=$?
    [ $rc -ne 0 ] && continue
    download_API_KEY=$api_key
    break
done

if [ $rc -ne 0 ]
then
    echo "Can't get resource: $art_get_URL"
    exit $rc
fi

if [ $art_get_FORCE -eq 0 ]
then

LASTMOD="$(echo "$HEADERS" | grep -i 'last-modified' | cut -d ' ' -f 2- | cut -d ',' -f 2- | sed -e 's/^[[:space:]]*//' -e 's/[[:space:]]*$//')"

if [ "$(uname -s)" == "Linux" ]
then
    LMTS=$(/bin/date -d "$LASTMOD" '+%Y%m%d%H%M.%S')
else
    LMTS=$(/bin/date -j -f '%d %b %Y %H:%M:%S %Z' "$LASTMOD" '+%Y%m%d%H%M.%S')
fi

[ -d "$art_get_OUTPUT" ] || mkdir -p "$art_get_OUTPUT"

touch -t "$LMTS" "$art_get_OUTPUT/tmp$$"
if [ $art_get_UNTAR -ne 0 ]
then
    art_get_HAVE_FILE="$(find "$art_get_OUTPUT" -newer "$art_get_OUTPUT/tmp$$" -name ".downloaded.$art_get_FILE")"
else
    art_get_HAVE_FILE="$(find "$art_get_OUTPUT" -newer "$art_get_OUTPUT/tmp$$" -name "$art_get_FILE")"
fi
rm -f "$art_get_OUTPUT/tmp$$"
fi

rc=0
if [ -z "$art_get_HAVE_FILE" ]
then
    [ -d "$art_get_OUTPUT" ] || mkdir -p "$art_get_OUTPUT"
    cd "$art_get_OUTPUT"
    echo "getting $art_get_URL"
    if [ $art_get_UNTAR -ne 0 ] 
    then
        curl $curl_SILENT -f -H "X-JFrog-Art-Api:$download_API_KEY" "$art_get_URL" --output - | tar -x $tar_Z 
        rc=$?
        [ $rc -eq 0 ] && touch .downloaded.$art_get_FILE
    else
        curl $curl_SILENT -f -O -H "X-JFrog-Art-Api:$download_API_KEY" "$art_get_URL"
        rc=$?
    fi
else
    echo "$art_get_HAVE_FILE is newer that the remote resource"
fi
exit $rc
