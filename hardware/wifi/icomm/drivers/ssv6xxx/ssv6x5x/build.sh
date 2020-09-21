#/bin/bash

# generate version information header
./ver_info.pl include/ssv_version.h

if [ $? -eq 0 ]; then
    echo "Please check SVN first !!"
else
if hash colormake 2>/dev/null; then
    colormake;colormake install
else
    make;make install
fi
fi
