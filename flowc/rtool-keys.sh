#!/bin/bash

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
