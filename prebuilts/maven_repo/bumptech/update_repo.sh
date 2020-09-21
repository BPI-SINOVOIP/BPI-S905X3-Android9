#!/bin/bash

# Populates glide maven repository with current snapshot from google3


TARGET_DIR=`pwd`

p4 g4d -f sync_glide_google3_android
pushd /google/src/cloud/$USER/sync_glide_google3_android/google3 >> /dev/null

SYNCED_CL=`g4 sync | grep @ | sed s/.*@//`

blaze build \
   third_party/java_src/android_libs/glide/annotation:libannotation.jar \
   third_party/java_src/android_libs/glide/annotation:libannotation-src.jar \
   third_party/java_src/android_libs/glide/annotation/compiler:libcompiler_lib.jar \
   third_party/java_src/android_libs/glide/annotation/compiler:libcompiler_lib-src.jar \
   third_party/java_src/android_libs/glide/library/src/main:libglide.jar \
   third_party/java_src/android_libs/glide/library/src/main:libglide-src.jar \
   third_party/java_src/android_libs/glide/third_party/disklrucache:libdisklrucache.jar \
   third_party/java_src/android_libs/glide/third_party/disklrucache:libdisklrucache-src.jar \
   third_party/java_src/android_libs/glide/third_party/gif_decoder:libgif_decoder.jar \
   third_party/java_src/android_libs/glide/third_party/gif_decoder:libgif_decoder-src.jar

cp -f blaze-bin/third_party/java_src/android_libs/glide/annotation/libannotation.jar \
   $TARGET_DIR/com/github/bumptech/glide/annotation/SNAPSHOT/annotation-SNAPSHOT.jar
cp -f blaze-bin/third_party/java_src/android_libs/glide/annotation/libannotation-src.jar \
   $TARGET_DIR/com/github/bumptech/glide/annotation/SNAPSHOT/annotation-SNAPSHOT-sources.jar
cp -f blaze-bin/third_party/java_src/android_libs/glide/annotation/compiler/libcompiler_lib.jar \
   $TARGET_DIR/com/github/bumptech/glide/compiler/SNAPSHOT/compiler-SNAPSHOT.jar
cp -f blaze-bin/third_party/java_src/android_libs/glide/annotation/compiler/libcompiler_lib-src.jar \
   $TARGET_DIR/com/github/bumptech/glide/compiler/SNAPSHOT/compiler-SNAPSHOT-sources.jar
cp -f blaze-bin/third_party/java_src/android_libs/glide/library/src/main/libglide.jar \
   $TARGET_DIR/com/github/bumptech/glide/glide/SNAPSHOT/glide-SNAPSHOT.jar
cp -f blaze-bin/third_party/java_src/android_libs/glide/library/src/main/libglide-src.jar \
   $TARGET_DIR/com/github/bumptech/glide/glide/SNAPSHOT/glide-SNAPSHOT-sources.jar
cp -f blaze-bin/third_party/java_src/android_libs/glide/third_party/disklrucache/libdisklrucache.jar \
   $TARGET_DIR/com/github/bumptech/glide/disklrucache/SNAPSHOT/disklrucache-SNAPSHOT.jar
cp -f blaze-bin/third_party/java_src/android_libs/glide/third_party/disklrucache/libdisklrucache-src.jar \
   $TARGET_DIR/com/github/bumptech/glide/disklrucache/SNAPSHOT/disklrucache-SNAPSHOT-sources.jar
cp -f blaze-bin/third_party/java_src/android_libs/glide/third_party/gif_decoder/libgif_decoder.jar \
   $TARGET_DIR/com/github/bumptech/glide/gifdecoder/SNAPSHOT/gifdecoder-SNAPSHOT.jar
cp -f blaze-bin/third_party/java_src/android_libs/glide/third_party/gif_decoder/libgif_decoder-src.jar \
   $TARGET_DIR/com/github/bumptech/glide/gifdecoder/SNAPSHOT/gifdecoder-SNAPSHOT-sources.jar

echo "This maven repository was synced to google3 CL $SYNCED_CL" > $TARGET_DIR/README.txt
popd
