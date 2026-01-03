export WATCOM=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/watcom2
export PATH=$WATCOM/bino64:$PATH
export EDPATH=$WATCOM/eddat
export WIPFC=$WATCOM/wipfc

# For DOS 8088/8086 development
export INCLUDE=$WATCOM/h
export LIB=$WATCOM/lib286