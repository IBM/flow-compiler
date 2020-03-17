FROM flowc:{{FLOWC_VERSION}}.{{BASE_IMAGE}} AS base

USER worker
RUN mkdir -p /home/worker/{{NAME}}/docs && mkdir -p /tmp/{{NAME}}/docs && mkdir -p /home/worker/{{NAME}}/www && mkdir -p /tmp/{{NAME}}/www
COPY --chown=worker:worker docs/{{MAIN_FILE}} {P:SERVER_XTRA_H{{{SERVER_XTRA_H}} }P} {P:SERVER_XTRA_C{{{SERVER_XTRA_C}} }P} {P:PROTO_FILE{docs/{{PROTO_FILE}} }P} /tmp/{{NAME}}/
RUN cd /tmp/{{NAME}} && flowc --client --server {{MAIN_FILE}} --name {{NAME}} && make -j2 -f {{NAME}}.mak deploy && cd /tmp && rm -fr {{NAME}}

WORKDIR /home/worker
RUN tar -cf /home/worker/bin.tar {{NAME}}/*
RUN mkdir -p /home/worker/lib && chown -R worker:worker /home/worker/lib
RUN ldd /home/worker/{{NAME}}/{{NAME}}-server  2>/dev/null | grep -E -o '/.*\(0x[0-9A-Fa-f]+\)$' | sed -E -e 's/\s+\(0x[0-9A-Fa-f]+\)$//' >> needed-libs-a.txt
RUN ldd /home/worker/{{NAME}}/{{NAME}}-client  2>/dev/null | grep -E -o '/.*\(0x[0-9A-Fa-f]+\)$' | sed -E -e 's/\s+\(0x[0-9A-Fa-f]+\)$//' >> needed-libs-a.txt
RUN sort -u needed-libs-a.txt | while read F; do cp "$F" /home/worker/lib; done 
RUN tar -cf /home/worker/so.tar lib/*

FROM flow-runtime

USER root
WORKDIR /home/worker
COPY --chown=worker:worker --from=base /home/worker/*.tar /home/worker/
RUN tar -xvf /home/worker/bin.tar && rm /home/worker/bin.tar && chown -R worker:worker {{NAME}}
RUN tar -xvf /home/worker/so.tar && rm /home/worker/so.tar
RUN echo $'#!/bin/bash\n\
function ldd_deps() {\n\
    ldd "$1" 2>/dev/null | grep -E -o "^.*\s/.*\(0x[0-9A-Fa-f]+\)$" | sed -E -e "s/(^\s+)//" -e "s/\s*\(0x[0-9A-Fa-f]+\)$//" -e "s/\s+=>\s+/ /"\n\
}\n\
function need() {\n\
	readelf -d "$1" | grep NEEDED | grep -o -E "\[.*\]$" | sed -E "s/(^\[)|(\]$)//g" | sort -u \n\
}\n\
function have() {\n\
    ldd_deps "$1" | cut -d " " -f 1 | while read F; do basename "$F"; done \n\
    ldd_deps "$1" | cut -d " " -f 2 | while read F; do basename "$F"; done \n\
}\n\
function have_fp() {\n\
    ldd_deps "$1" | cut -d " " -f 2\n\
}\n\
function minus() {\n\
	for A in $1\n\
	do \n\
		P=1\n\
		for B in $2\n\
		do \n\
			if [ "$A" == "$B" ] \n\
			then \n\
				P=0\n\
				break\n\
			fi\n\
		done\n\
		if [ $P -ne 0 ]\n\
		then\n\
			echo "$A"\n\
		fi\n\
	done\n\
}\n\
function install_linked() {\n\
    MISSING="$(minus "$(need "$1")" "$(have "$1")")"\n\
    C=0\n\
    for L in $MISSING\n\
    do\n\
        echo "Adding $L" 1>&2\n\
        C=$((C + 1))\n\
        cp "lib/$L" /usr/local/lib\n\
    done\n\
    ldconfig /usr/local/lib\n\
    echo $C\n\
}\n\
install_linked "$1" > /dev/null\n\
ADDED=1\n\
while [ $ADDED -ne 0 ]\n\
do\n\
    ADDED=0\n\
    for L in $(have_fp "$1")\n\
    do\n\
        ADDEDC="$(install_linked "$L")"\n\
        ADDED=$((ADDED + $ADDEDC))\n\
    done\n\
done\n\
' > install.sh && chown -R worker:worker install.sh && chmod a+x install.sh
RUN ./install.sh /home/worker/{{NAME}}/{{NAME}}-server && ./install.sh /home/worker/{{NAME}}/{{NAME}}-client && rm -fr /home/worker/lib
USER worker
WORKDIR /home/worker/{{NAME}}
