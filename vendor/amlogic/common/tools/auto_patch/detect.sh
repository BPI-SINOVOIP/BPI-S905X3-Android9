#! /bin/bash

function auto_patch()
{
    local patch_dir=$1
    echo "### ${patch_dir##*/}###      "

    for file in $patch_dir/*
    do
        if [ -f "$file" ] && [ "${file##*.}" == "patch" ]
        then
            local file_name=${file%.*};           #echo file_name $file_name
            local resFile=`basename $file_name`;  #echo resFile $resFile
            local dir_name1=${resFile//#/\/};     #echo dir_name $dir_name
            local dir_name=${dir_name1%/*};       #echo dir_name $dir_name
            local dir=$T/$dir_name;               #echo $dir
            local change_id=`grep 'Change-Id' $file | cut -f 2 -d " "`
            if [ -d "$dir" ]
            then
                cd $dir; git log -n 100 | grep $change_id 1>/dev/null 2>&1;
                if [ $? -ne 0 ]; then
                    echo "    ###patch ${file##*/}###      "
                    cd $dir; git am -q $file 1>/dev/null 2>&1;
                    if [ $? != 0 ]
                    then
                        git am --abort
                        echo "    ### patch_dir ${patch_dir##*/} $file failed,maybe already patched    "
                    fi
                else
                    echo "    ###${file##*/} has patched###      "
                fi
            fi
        fi
    done
}

function traverse_patch_dir()
{
    local local_dir=$1
    for file in `ls $local_dir`
    do
        if [[ "$file" =~ ".md" || "$file" =~ ".sh" ]]
        then
            continue
        fi

        if [ -d $local_dir$file ]
        then
            local dest_dir=$local_dir$file
            auto_patch $dest_dir
        fi
    done
    cd $T
}

T=$(pwd)
LOCAL_PATH=$T/$(dirname $0)/
echo $LOCAL_PATH
traverse_patch_dir $LOCAL_PATH
