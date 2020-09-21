#/bin/bash


if hash colormake 2>/dev/null; then
    colormake
else
    make
fi

cp image/* ../../image