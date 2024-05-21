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

esac 

OSVER="$OSNAME - $(uname -rs) $MACHINE"
if [ $EXT -eq 0 ]
then
    OSVER="$OSNAME - $(uname -rs) $MACHINE ($(uname -v))"
fi

echo "$OSVER"
