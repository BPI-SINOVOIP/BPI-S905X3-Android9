#opyright (C) 2018 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


function auto_patch()
{
    local patch_dir=$1

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
                    #echo "###patch ${file##*/}###      "
                    cd $dir; git am -q $file 1>/dev/null 2>&1;
                    if [ $? != 0 ]
                    then
                        git am --abort
                        echo "!!! Patch Warnning !!! "
                        echo "### patch_dir ${patch_dir##*/} $file failed,maybe already patched    "
                    fi
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
        # ATV don't apply AOSP patch
        if [[ "$is_android_tv" == "true" && "$file" =~ "aosp_ui" ]]
        then
            # echo "###ATV version, don't need this patch:$file###      "
            continue
        fi
        # ATV patch only patch in ATV version
        if [[ "$is_android_tv" != "true" && "$file" =~ "gtvs_ui" ]]
        then
            #echo "### AOSP version, don't need this patch:$file###      "
            continue
        fi
        # if no LiveTv, don't apply tv patch
        if [[ "$need_tv_feature" != "true" && "$file" =~ "tv_platform" ]]
        then
            #echo "###no need tv featrue, don't need this patch:$file###      "
            continue
        fi

        if [ -d $local_dir$file ]
        then
            local dest_dir=$local_dir$file
            auto_patch $dest_dir
        fi
    done
    echo "!!! Patch Finish !!! "
    cd $T
}

T=$(pwd)
LOCAL_PATH=$T/$(dirname $0)/

need_tv_feature=$1
is_android_tv=$2

echo -e "###need_tv_feature=$need_tv_feature, is_android_tv=$is_android_tv###      "

traverse_patch_dir $LOCAL_PATH

if [ $? != 0 ]
then
    echo "patch error"
    return 1
fi
