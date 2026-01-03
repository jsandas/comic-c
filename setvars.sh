ARCH=$(uname -m)
OS=$(uname -s | tr '[:upper:]' '[:lower:]')

if [ "$ARCH" = "x86_64" ] && [ "$OS" = "linux" ]; then
    BIN_PATH="binl64"
elif [ "$ARCH" = "i386" ] && [ "$OS" = "linux" ]; then
    BIN_PATH="binl"
elif [ "$ARCH" = "x86_64" ] && [ "$OS" = "darwin" ]; then
    BIN_PATH="bino64"
elif [ "$ARCH" = "aarch64" ] && [ "$OS" = "darwin" ]; then
    BIN_PATH="armo64"
else
    echo "Unsupported architecture or OS: $ARCH on $OS"
    return 1
fi

export WATCOM=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/watcom2
export PATH=$WATCOM/$BIN_PATH:$PATH
export EDPATH=$WATCOM/eddat
export WIPFC=$WATCOM/wipfc

# For DOS 8088/8086 development
export INCLUDE=$WATCOM/h
export LIB=$WATCOM/lib286