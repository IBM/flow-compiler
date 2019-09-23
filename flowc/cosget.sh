#!/bin/bash

s3_get_UNTAR=0
s3_get_OUTPUT=.
ACCESS_KEY_ID="${S3_ACCESS_KEY_ID}"
SECRET_ACCESS_KEY="${S3_SECRET_ACCESS_KEY}"

args=()
while [ $# -gt 0 ]
do
case "$1" in

    -o|--output)
    s3_get_OUTPUT="${2-.}"
    shift; shift
    ;;

    -u|--untar)
    s3_get_UNTAR=1
    shift
    ;;

    --access-key-id)
    ACCESS_KEY_ID="$2"
    shift; shift
    ;;

    --secret-access-key)
    SECRET_ACCESS_KEY="$2"
    shift; shift
    ;;

    -a|--access)
    ACCESS_KEY_ID=${2%:*}
    SECRET_ACCESS_KEY=${2#*:}
    shift; shift
    ;;

    -d|--access-directory)
    ACCESS_KEY_ID="$(cat "$2/ACCESS_KEY_ID")"
    SECRET_ACCESS_KEY="$(cat "$2/SECRET_ACCESS_KEY")"
    shift; shift
    ;;

    *)
    args+=("$1")
    shift
    ;;
esac
done
set -- "${args[@]}"

URL="$1"
if [ -z "$URL" ]
then
    echo "usage $0 [options] <URL>"
    echo ""
    echo "options:"
    echo "  -o, --output <FILE|DIRECTORY>       destination for the downloaded file"
    echo "  -u, --untar                         untar the file at the destination, output must be a directory"
    echo "  --access-key-id <STRING>            key id"
    echo "  --secret-access-key <STRING>        access secret"
    echo "  -a, --access <KEY:SECRET>           access key and secret"
    echo "  -d, --access-directory <DIRECTORY>  directory with files ACCESS_KEY_ID and SECRET_ACCESS_KEY"
    echo ""
    echo "note: default access information can be set in S3_ACCESS_KEY_ID and S3_SECRET_ACCESS_KEY"
    echo ""
    exit 1
fi

URLHP=${URL#*//}
FILE=$(basename $URLHP)
OUTPUT_FILE="${s3_get_OUTPUT-$FILE}"
if [ "$s3_get_OUTPUT" != "-" -a -d "$s3_get_OUTPUT" -a $s3_get_UNTAR -eq 0 ]
then
    OUTPUT_FILE="${s3_get_OUTPUT}/${FILE}"
fi


if [ ! -d "$OUTPUT_FILE" -a $s3_get_UNTAR -ne 0 ]
then
    echo "Output must be a directory"
    exit 1
fi

HOST="${URLHP%%/*}"
RESOURCE="${URLHP#*/}"

CONTENT_TYPE="application/x-compressed-tar"
DATE_VALUE="$(date +'%a, %d %b %Y %H:%M:%S %z')" 
TO_SIGN="GET

$CONTENT_TYPE
$DATE_VALUE
/$RESOURCE" 

SIGNATURE=$(/bin/echo -n "$TO_SIGN" | openssl sha1 -hmac "$SECRET_ACCESS_KEY" -binary | base64)

if [ $s3_get_UNTAR -eq 0 ]
then
curl -fsS -H "Host: $HOST" \
  -H "Date: $DATE_VALUE" \
  -H "Content-Type: $CONTENT_TYPE" \
  -H "Authorization: AWS $ACCESS_KEY_ID:$SIGNATURE" \
  "https://$URLHP" --output "$OUTPUT_FILE"
RC=$?
else 
case "$RESOURCE" in
    *.tgz|*.tar.gz)
        TAR_Z=-z
    ;;
    *.tar.bz2)
        TAR_Z=-j
    ;;
    *.tar.xz)
        TAR_Z=--xz
    ;;
    *)
        TAR_Z=
    ;;
esac
curl -fsS -H "Host: $HOST" \
  -H "Date: $DATE_VALUE" \
  -H "Content-Type: $CONTENT_TYPE" \
  -H "Authorization: AWS $ACCESS_KEY_ID:$SIGNATURE" \
  "https://$URLHP" --output - | tar -C "$OUTPUT_FILE" $TAR_Z -xv 
RC=$?
fi

exit $RC
