#!/bin/sh

if [ "$1" == "--no-includes" ]
then
    NOINCS=1
    shift
fi

if [ -z "$2" ]
then
    echo "Usage $(dirname $0) [--no-includes] C-FILE TEMPLATE-FILE [LABEL]"
    exit 1
fi

CFILE="$1"
TFILE="$2"

if [ -z "$3" ]
then
    TID=$(echo "$TFILE" | tr -- ' -./' '____')
else
    TID=$3
fi

if [ ${NOINCS-0} -ne 1 ]
then
echo '#include <string>
#include <sstream>
#include "filu.H"' > "$CFILE"
fi
echo "unsigned char const ${TID}_z[] = {" >> "$CFILE"
cat "$TFILE" | gzip -c | xxd -i >> "$CFILE"
echo '};' >> "$CFILE"
echo "unsigned const ${TID}_zlen = " >> "$CFILE"
cat "$TFILE" | gzip -c | wc -c >> $CFILE
echo ';' >> "$CFILE"
echo "std::string get_${TID}() {" >> "$CFILE"
echo '    std::ostringstream buf;' >> "$CFILE"
echo "    std::istringstream zbuf(std::string((char const *)${TID}_z, ${TID}_zlen));" >> "$CFILE"
echo '    filu::gunzip(buf, zbuf, true);' >> "$CFILE"
echo '    return buf.str();' >> "$CFILE" 
echo '}'  >> "$CFILE"
