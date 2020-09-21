#!/bin/bash

function change_to()  
{
    chmod $2 $1
    echo \# chmod $2 $1.
}

function search_file()
{
    if [ "$1" = "" ]
    then
        echo \# Please specify a file access permissions mode that you want to change.
        echo \# such as \'./chmod_filse 666\'.
        return
    fi

    #echo Will change files access permissions to $1 by chmod.

    for file in ./*
    do
        if test -f $file
        then
            #echo [\'${file##*/}\' is file.]
            if [ "${file##*/}" != "${0##*/}" ]; 
            then
                change_to ${file##*/} $1
            fi
        fi
        
        if test -d $file
        then
            echo \# [\'${file##*/}\' is folder.]
            cd $file
            search_file $1
            cd ..
        fi
    done
}

#############################################################
echo \# Hello, my name is ${0##*/}.
echo \# I will change files access permissions to $1 by chmod.
search_file $1
echo \# Over.
