#!/bin/bash

get_credentials() {
aws_credentials() {
	local AWS_FILE="$1"
	local PROFILE="$2"

	[ -r "$AWS_FILE" ] && cat "$AWS_FILE" | awk -v "profile=$PROFILE" '
		BEGIN {FS="="; OFS="\t"; in_profile = 0; id = ""; key = "";}
		!in_profile && $1 == "["profile"]" { in_profile = 1; next; }
		!in_profile { next; }
		$1 ~ "[[].*" { in_profile = 0; next; }
		$1 == "aws_access_key_id" { id = $2; next; }
		$1 == "aws_secret_access_key" { key = $2; next; }
		END { print id, key; }
	'
}

flat_credentials() {
	local FLAT_FILE="$1"
	local PROFILE="$2"
	while read PROF CREDS
	do
		[ "$PROFILE" == "$PROF" ] && cat <<< "$CREDS" | cut -d: --output-delimiter=$'\t' -f1,2 && break
		[ -z "$CREDS" -a "$PROFILE" == "default" ] && cat <<< "$PROF" | cut -d: --output-delimiter=$'\t' -f1,2 && break
	done < "$FLAT_FILE"
}

SEARCH_FILES=
SEARCH_PROFILES=
SEARCH_DEFAULT=default
ALL_AWS_SEARCH_FILES="-a.aws/credentials"$'\n'"-a$HOME/.aws/credentials"
ALL_FLAT_FILES="-f.api-key"$'\n'"-f.api-keys"$'\n'"-f.api_key"$'\n'"-f.api_keys"$'\n'"-f.s3-key"$'\n'"-f.s3-keys"$'\n'"-f.s3_key"$'\n'"-f.s3_keys"
ALL_FLAT_SEARCH_FILES="$ALL_FLAT_FILES"$'\n'"$(while IFS=$'\n' read FO
do
    echo "${FO:0:2}$HOME/${FO:2}"
done <<< "$ALL_FLAT_FILES")"

args=()
while [ $# -gt 0 ]
do
	case "$1" in
		-A|--all-aws)
            SEARCH_FILES="$SEARCH_FILES"$'\n'"$ALL_AWS_SEARCH_FILES"
			shift
			;;
		-F|--all-files)
            SEARCH_FILES="$SEARCH_FILES"$'\n'"$ALL_FLAT_SEARCH_FILES"
			shift
			;;
		-a|--aws)
            SEARCH_FILES="$SEARCH_FILES"$'\n'"-a$2"
			shift; shift
			;;
		-f|--file)
            SEARCH_FILES="$SEARCH_FILES"$'\n'"-f$2"
			shift; shift
			;;
		-p|--profile)
            SEARCH_PROFILES="$SEARCH_PROFILES $2"
			shift; shift
			;;
		-n|--no-default)
            SEARCH_DEFAULT=
			shift
			;;
		*)
			args+=("$1")
			shift
			;;
	esac
done
set -- "${args[@]}"

if [ -z "$SEARCH_FILES" ]
then
    SEARCH_FILES="$ALL_FLAT_SEARCH_FILES"$'\n'"$ALL_AWS_SEARCH_FILES"
fi

if [ $# -ne 0 ]
then
	echo "usage $0 [options]"
	echo ""
	echo "options:"
    echo "  -A, --all-aws           Search all the aws files"
    echo "  -a, --aws-file FILE     Search FILE for aws style credentials"
	echo "  -f, --file FILE         Search FILE for [[profile ]user:]keys"
    echo "  -F, --all-files         Search all the flat files"
    echo "  -p, --profile PROFILE   Search for PROFILE first, then for default credentials"
    echo "  -n, --no-default        Search only the given profile(s)"
	echo ""
	echo "notes:"
    echo "  If no file option is given, all files will be searched, as with -F -A"
	echo ""
	exit 1
fi

CHECKED_FILES="$(while IFS=$'\n' read FO
    do
        FILE="${FO:2}"
        [ -z "$FILE" -o ! -r "$FILE" ] && continue
        echo "$FO"
    done <<< "$SEARCH_FILES")"

if [ -z "$CHECKED_FILES" ]
then
    echo "No file with keys found"
    exit 1
fi

for PROFILE in $SEARCH_PROFILES $SEARCH_DEFAULT
do
    while IFS=$'\n' read FO
    do
        FTYPE="${FO:0:2}"
        FILE="${FO:2}"
        [ -z "$FILE" -o ! -r "$FILE" ] && continue
        [ "$FTYPE" == "-a" ] && aws_credentials "$FILE" $PROFILE
        [ "$FTYPE" == "-f" ] && flat_credentials "$FILE" $PROFILE
    done <<< "$SEARCH_FILES"
done
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
    [ $rget_KEY_SIZE -eq 2 -a -z "$(cut -sf2 <<< "$KEY")" ] && continue

    ## Artifactory credentials are key only
    [ $rget_KEY_SIZE -eq 1 -a ! -z "$(cut -sf2 <<< "$KEY")" ] && continue
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
 
[ ! -z "$rget_HAVE_FILE" ] && echo "$rget_HAVE_FILE is newer than the remote file" && exit 

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
    rc=$?
else 
    get_file $rget_METHOD $rget_URL "$use_API_KEY" | tar -x $tar_Z
    rc=$?
    [ $rc -eq 0 ] && touch "$rget_FILE"
fi
exit $rc
