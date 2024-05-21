#!/bin/bash 

[ "$1" == "-r" ]
EXT=$?

MACHINE="$(uname -m)"
MACHINE_ARCH="$(uname -p)"
if [ -z "$MACHINE" ]
then
    MACHINE="$MACHINE_ARCH"
fi
if [ "$MACHINE" != "$MACHINE_ARCH" -a ! -x "$MACHINE_ARCH" ]
then
    MACHINE="$MACHINE $MACHINE_ARCH"
fi
if [ $EXT -eq 0 ]
then
KNAME="$(uname -rs) $MACHINE ($(uname -v))"
else 
KNAME="$(uname -rs) $MACHINE"
fi

case "$(uname -s)" in

    Darwin)
        OSNAME="$(sw_vers | grep -i ProductName | sed -E -e 's/[a-zA-Z0-9]+:(\t|" ")+//')"
        OSVER="$(sw_vers | grep -i ProductVersion | sed -E -e 's/[a-zA-Z0-9]+:(\t|" ")+//')"
        OSBUILD="$(sw_vers | grep -i BuildVersion | sed -E -e 's/[a-zA-Z0-9]+:(\t|" ")+//')"
        OSNAME="$OSNAME $OSVER"
        if [ $EXT -eq 0 -a ! -z "$OSBUILD" ]
        then
            OSNAME="$OSNAME ($OSBUILD)"
        fi
    ;;
    Linux)
        REL=$(cat /etc/*-release 2> /dev/null)
        if [ $? -eq 0 ]
        then
            OSNAME="$(cat /etc/*-release | grep -E -i '(^|\s+)NAME=' | sed -E -e 's/[a-zA-Z0-9]+=(\t|" ")*\"//' -e s'/\"$//')"
            OSVER="$(cat /etc/*-release | grep -E -i '(^|\s+)VERSION=' | sed -E -e 's/[a-zA-Z0-9]+=(\t|" ")*\"//' -e s'/\"$//')"
            OSNAME="$OSNAME $OSVER"
        else 
            OSNAME=what
        fi
    ;;
    *)
    ;;

esac 

echo "$OSNAME - $KNAME"
