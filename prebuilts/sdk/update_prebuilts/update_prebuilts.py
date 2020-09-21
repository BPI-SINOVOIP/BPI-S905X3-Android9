#!/usr/bin/python3

# This script is used to update platform SDK prebuilts, Support Library, and a variety of other
# prebuilt libraries used by Android's Makefile builds. For details on how to use this script,
# visit go/update-prebuilts.
import os, sys, getopt, zipfile, re
import argparse
import subprocess
from shutil import copyfile, rmtree, which
from distutils.version import LooseVersion
from functools import reduce

current_path = 'current'
system_path = 'system_current'
api_path = 'api'
system_api_path = 'system-api'
support_dir = os.path.join(current_path, 'support')
androidx_dir = os.path.join(current_path, 'androidx')
extras_dir = os.path.join(current_path, 'extras')
buildtools_dir = 'tools'
jetifier_dir = os.path.join(buildtools_dir, 'jetifier', 'jetifier-standalone')

temp_dir = os.path.join(os.getcwd(), "support_tmp")
os.chdir(os.path.dirname(os.path.dirname(os.path.realpath(sys.argv[0]))))
git_dir = os.getcwd()

# See go/fetch_artifact for details on this script.
FETCH_ARTIFACT = '/google/data/ro/projects/android/fetch_artifact'

maven_to_make = {
    # Support Library
    'com.android.support:animated-vector-drawable': ['android-support-animatedvectordrawable', 'graphics/drawable'],
    'com.android.support:appcompat-v7': ['android-support-v7-appcompat', 'v7/appcompat'],
    'com.android.support:cardview-v7': ['android-support-v7-cardview', 'v7/cardview'],
    'com.android.support:collections': ['android-support-collections', 'collections', 'jar'],
    'com.android.support:customtabs': ['android-support-customtabs', 'customtabs'],
    'com.android.support:exifinterface': ['android-support-exifinterface', 'exifinterface'],
    'com.android.support:gridlayout-v7': ['android-support-v7-gridlayout', 'v7/gridlayout'],
    'com.android.support:leanback-v17': ['android-support-v17-leanback', 'v17/leanback'],
    'com.android.support:mediarouter-v7': ['android-support-v7-mediarouter', 'v7/mediarouter'],
    'com.android.support:palette-v7': ['android-support-v7-palette', 'v7/palette'],
    'com.android.support:percent': ['android-support-percent', 'percent'],
    'com.android.support:preference-leanback-v17': ['android-support-v17-preference-leanback', 'v17/preference-leanback'],
    'com.android.support:preference-v14': ['android-support-v14-preference', 'v14/preference'],
    'com.android.support:preference-v7': ['android-support-v7-preference', 'v7/preference'],
    'com.android.support:recommendation': ['android-support-recommendation', 'recommendation'],
    'com.android.support:recyclerview-v7': ['android-support-v7-recyclerview', 'v7/recyclerview'],
    'com.android.support:support-annotations': ['android-support-annotations', 'annotations', 'jar'],
    'com.android.support:support-compat': ['android-support-compat', 'compat'],
    'com.android.support:support-core-ui': ['android-support-core-ui', 'core-ui'],
    'com.android.support:support-core-utils': ['android-support-core-utils', 'core-utils'],
    'com.android.support:support-dynamic-animation': ['android-support-dynamic-animation', 'dynamic-animation'],
    'com.android.support:support-emoji-appcompat': ['android-support-emoji-appcompat', 'emoji-appcompat'],
    'com.android.support:support-emoji-bundled': ['android-support-emoji-bundled', 'emoji-bundled'],
    'com.android.support:support-emoji': ['android-support-emoji', 'emoji'],
    'com.android.support:support-fragment': ['android-support-fragment', 'fragment'],
    'com.android.support:support-media-compat': ['android-support-media-compat', 'media-compat'],
    'com.android.support:support-tv-provider': ['android-support-tv-provider', 'tv-provider'],
    'com.android.support:support-v13': ['android-support-v13', 'v13'],
    'com.android.support:support-v4': ['android-support-v4', 'v4'],
    'com.android.support:support-vector-drawable': ['android-support-vectordrawable', 'graphics/drawable'],
    'com.android.support:transition': ['android-support-transition', 'transition'],
    'com.android.support:wear': ['android-support-wear', 'wear'],

    # Support Library (28.0.0 splits + new modules)
    'com.android.support:heifwriter': ['android-support-heifwriter', 'heifwriter'],
    'com.android.support:webkit': ['android-support-webkit', 'webkit'],
    'com.android.support:customview': ['android-support-customview', 'customview'],
    'com.android.support:textclassifier': ['android-support-textclassifier', 'textclassifier'],
    'com.android.support:swiperefreshlayout': ['android-support-swiperefreshlayout', 'swiperefreshlayout'],
    'com.android.support:viewpager': ['android-support-viewpager', 'viewpager'],
    'com.android.support:coordinatorlayout': ['android-support-coordinatorlayout', 'coordinatorlayout'],
    'com.android.support:asynclayoutinflater': ['android-support-asynclayoutinflater', 'asynclayoutinflater'],
    'com.android.support:support-content': ['android-support-support-content', 'support-content'],
    'com.android.support:documentfile': ['android-support-documentfile', 'documentfile'],
    'com.android.support:drawerlayout': ['android-support-drawerlayout', 'drawerlayout'],
    'com.android.support:localbroadcastmanager': ['android-support-localbroadcastmanager', 'localbroadcastmanager'],
    'com.android.support:print': ['android-support-print', 'print'],
    'com.android.support:slidingpanelayout': ['android-support-slidingpanelayout', 'slidingpanelayout'],
    'com.android.support:interpolator': ['android-support-interpolator', 'interpolator'],
    'com.android.support:cursoradapter': ['android-support-cursoradapter', 'cursoradapter'],
    'com.android.support:loader': ['android-support-loader', 'loader'],
    'com.android.support:contentpaging': ['android-support-contentpaging', 'contentpaging'],
    'com.android.support:recyclerview-selection': ['android-support-recyclerview-selection', 'recyclerview-selection'],
    'com.android.support:car': ['android-support-car', 'car'],
    'com.android.support:slices-core': ['android-slices-core', 'slices-core'],
    'com.android.support:slices-view': ['android-slices-view', 'slices-view'],
    'com.android.support:slices-builders': ['android-slices-builders', 'slices-builders'],
    'com.android.support:versionedparcelable': ['android-versionedparcelable', 'versionedparcelable'],

    # Multidex
    'com.android.support:multidex': ['android-support-multidex', 'multidex/library'],
    'com.android.support:multidex-instrumentation': ['android-support-multidex-instrumentation', 'multidex/instrumentation'],

    # Constraint Layout
    'com.android.support.constraint:constraint-layout': ['android-support-constraint-layout', 'constraint-layout'],
    'com.android.support.constraint:constraint-layout-solver': ['android-support-constraint-layout-solver', 'constraint-layout-solver'],

    # Architecture Components
    'android.arch.core:runtime': ['android-arch-core-runtime', 'arch-core/runtime'],
    'android.arch.core:common': ['android-arch-core-common', 'arch-core/common'],
    'android.arch.lifecycle:common': ['android-arch-lifecycle-common', 'arch-lifecycle/common'],
    'android.arch.lifecycle:common-java8': ['android-arch-lifecycle-common-java8', 'arch-lifecycle/common-java8'],
    'android.arch.lifecycle:extensions': ['android-arch-lifecycle-extensions', 'arch-lifecycle/extensions'],
    'android.arch.lifecycle:livedata': ['android-arch-lifecycle-livedata', 'arch-lifecycle/livedata'],
    'android.arch.lifecycle:livedata-core': ['android-arch-lifecycle-livedata-core', 'arch-lifecycle/livedata-core'],
    'android.arch.lifecycle:process': ['android-arch-lifecycle-process', 'arch-lifecycle/process'],
    'android.arch.lifecycle:runtime': ['android-arch-lifecycle-runtime', 'arch-lifecycle/runtime'],
    'android.arch.lifecycle:service': ['android-arch-lifecycle-service', 'arch-lifecycle/service'],
    'android.arch.lifecycle:viewmodel': ['android-arch-lifecycle-viewmodel', 'arch-lifecycle/viewmodel'],
    'android.arch.paging:common': ['android-arch-paging-common', 'arch-paging/common'],
    'android.arch.paging:runtime': ['android-arch-paging-runtime', 'arch-paging/runtime'],
    'android.arch.persistence:db': ['android-arch-persistence-db', 'arch-persistence/db'],
    'android.arch.persistence:db-framework': ['android-arch-persistence-db-framework', 'arch-persistence/db-framework'],
    'android.arch.persistence.room:common': ['android-arch-room-common', 'arch-room/common'],
    'android.arch.persistence.room:migration': ['android-arch-room-migration', 'arch-room/migration'],
    'android.arch.persistence.room:runtime': ['android-arch-room-runtime', 'arch-room/runtime'],
    'android.arch.persistence.room:testing': ['android-arch-room-testing', 'arch-room/testing'],

    # AndroidX
    'androidx.slice:slice-builders': ['androidx.slice_slice-builders', 'androidx/slice/slice-builders'],
    'androidx.slice:slice-core': ['androidx.slice_slice-core', 'androidx/slice/slice-core'],
    'androidx.slice:slice-view': ['androidx.slice_slice-view', 'androidx/slice/slice-view'],
    'androidx.versionedparcelable:versionedparcelable': ['androidx.versionedparcelable_versionedparcelable', 'androidx/versionedparcelable'],
    'androidx.vectordrawable:vectordrawable-animated': ['androidx.vectordrawable_vectordrawable-animated', 'androidx/vectordrawable/vectordrawable-animated'],
    'androidx.annotation:annotation': ['androidx.annotation_annotation', 'androidx/annotation/annotation', 'jar'],
    'androidx.asynclayoutinflater:asynclayoutinflater': ['androidx.asynclayoutinflater_asynclayoutinflater', 'androidx/asynclayoutinflater/asynclayoutinflater'],
    'androidx.car:car': ['androidx.car_car', 'androidx/car/car'],
    'androidx.collection:collection': ['androidx.collection_collection', 'androidx/collection/collection', 'jar'],
    'androidx.core:core': ['androidx.core_core', 'androidx/core/core'],
    'androidx.contentpaging:contentpaging': ['androidx.contentpaging_contentpaging', 'androidx/contentpaging/contentpaging'],
    'androidx.coordinatorlayout:coordinatorlayout': ['androidx.coordinatorlayout_coordinatorlayout', 'androidx/coordinatorlayout/coordinatorlayout'],
    'androidx.legacy:legacy-support-core-ui': ['androidx.legacy_legacy-support-core-ui', 'androidx/legacy/legacy-support-core-ui'],
    'androidx.legacy:legacy-support-core-utils': ['androidx.legacy_legacy-support-core-utils', 'androidx/legacy/legacy-support-core-utils'],
    'androidx.cursoradapter:cursoradapter': ['androidx.cursoradapter_cursoradapter', 'androidx/cursoradapter/cursoradapter'],
    'androidx.browser:browser': ['androidx.browser_browser', 'androidx/browser/browser'],
    'androidx.customview:customview': ['androidx.customview_customview', 'androidx/customview/customview'],
    'androidx.documentfile:documentfile': ['androidx.documentfile_documentfile', 'androidx/documentfile/documentfile'],
    'androidx.drawerlayout:drawerlayout': ['androidx.drawerlayout_drawerlayout', 'androidx/drawerlayout/drawerlayout'],
    'androidx.dynamicanimation:dynamicanimation': ['androidx.dynamicanimation_dynamicanimation', 'androidx/dynamicanimation/dynamicanimation'],
    'androidx.emoji:emoji': ['androidx.emoji_emoji', 'androidx/emoji/emoji'],
    'androidx.emoji:emoji-appcompat': ['androidx.emoji_emoji-appcompat', 'androidx/emoji/emoji-appcompat'],
    'androidx.emoji:emoji-bundled': ['androidx.emoji_emoji-bundled', 'androidx/emoji/emoji-bundled'],
    'androidx.exifinterface:exifinterface': ['androidx.exifinterface_exifinterface', 'androidx/exifinterface/exifinterface'],
    'androidx.fragment:fragment': ['androidx.fragment_fragment', 'androidx/fragment/fragment'],
    'androidx.heifwriter:heifwriter': ['androidx.heifwriter_heifwriter', 'androidx/heifwriter/heifwriter'],
    'androidx.interpolator:interpolator': ['androidx.interpolator_interpolator', 'androidx/interpolator/interpolator'],
    'androidx.loader:loader': ['androidx.loader_loader', 'androidx/loader/loader'],
    'androidx.localbroadcastmanager:localbroadcastmanager': ['androidx.localbroadcastmanager_localbroadcastmanager', 'androidx/localbroadcastmanager/localbroadcastmanager'],
    'androidx.media:media': ['androidx.media_media', 'androidx/media/media'],
    'androidx.percentlayout:percentlayout': ['androidx.percentlayout_percentlayout', 'androidx/percentlayout/percentlayout'],
    'androidx.print:print': ['androidx.print_print', 'androidx/print/print'],
    'androidx.recommendation:recommendation': ['androidx.recommendation_recommendation', 'androidx/recommendation/recommendation'],
    'androidx.recyclerview:recyclerview-selection': ['androidx.recyclerview_recyclerview-selection', 'androidx/recyclerview/recyclerview-selection'],
    'androidx.slidingpanelayout:slidingpanelayout': ['androidx.slidingpanelayout_slidingpanelayout', 'androidx/slidingpanelayout/slidingpanelayout'],
    'androidx.swiperefreshlayout:swiperefreshlayout': ['androidx.swiperefreshlayout_swiperefreshlayout', 'androidx/swiperefreshlayout/swiperefreshlayout'],
    'androidx.textclassifier:textclassifier': ['androidx.textclassifier_textclassifier', 'androidx/textclassifier/textclassifier'],
    'androidx.transition:transition': ['androidx.transition_transition', 'androidx/transition/transition'],
    'androidx.tvprovider:tvprovider': ['androidx.tvprovider_tvprovider', 'androidx/tvprovider/tvprovider'],
    'androidx.legacy:legacy-support-v13': ['androidx.legacy_legacy-support-v13', 'androidx/legacy/legacy-support-v13'],
    'androidx.legacy:legacy-preference-v14': ['androidx.legacy_legacy-preference-v14', 'androidx/legacy/legacy-preference-v14'],
    'androidx.leanback:leanback': ['androidx.leanback_leanback', 'androidx/leanback/leanback'],
    'androidx.leanback:leanback-preference': ['androidx.leanback_leanback-preference', 'androidx/leanback/leanback-preference'],
    'androidx.legacy:legacy-support-v4': ['androidx.legacy_legacy-support-v4', 'androidx/legacy/legacy-support-v4'],
    'androidx.appcompat:appcompat': ['androidx.appcompat_appcompat', 'androidx/appcompat/appcompat'],
    'androidx.cardview:cardview': ['androidx.cardview_cardview', 'androidx/cardview/cardview'],
    'androidx.gridlayout:gridlayout': ['androidx.gridlayout_gridlayout', 'androidx/gridlayout/gridlayout'],
    'androidx.mediarouter:mediarouter': ['androidx.mediarouter_mediarouter', 'androidx/mediarouter/mediarouter'],
    'androidx.palette:palette': ['androidx.palette_palette', 'androidx/palette/palette'],
    'androidx.preference:preference': ['androidx.preference_preference', 'androidx/preference/preference'],
    'androidx.recyclerview:recyclerview': ['androidx.recyclerview_recyclerview', 'androidx/recyclerview/recyclerview'],
    'androidx.vectordrawable:vectordrawable': ['androidx.vectordrawable_vectordrawable', 'androidx/vectordrawable/vectordrawable'],
    'androidx.viewpager:viewpager': ['androidx.viewpager_viewpager', 'androidx/viewpager/viewpager'],
    'androidx.wear:wear': ['androidx.wear_wear', 'androidx/wear/wear'],
    'androidx.webkit:webkit': ['androidx.webkit_webkit', 'androidx/webkit/webkit'],

    # AndroidX for Multidex
    'androidx.multidex:multidex': ['androidx-multidex_multidex', 'androidx/multidex/multidex'],
    'androidx.multidex:multidex-instrumentation': ['androidx-multidex_multidex-instrumentation', 'androidx/multidex/multidex-instrumentation'],

    # AndroidX for Constraint Layout
    'androidx.constraintlayout:constraintlayout': ['androidx-constraintlayout_constraintlayout', 'androidx/constraintlayout/constraintlayout'],
    'androidx.constraintlayout:constraintlayout-solver': ['androidx-constraintlayout_constraintlayout-solver', 'androidx/constraintlayout/constraintlayout-solver'],

    # AndroidX for Architecture Components
    'androidx.arch.core:core-common': ['androidx.arch.core_core-common', 'androidx/arch/core/core-common'],
    'androidx.arch.core:core-runtime': ['androidx.arch.core_core-runtime', 'androidx/arch/core/core-runtime'],
    'androidx.lifecycle:lifecycle-common': ['androidx.lifecycle_lifecycle-common', 'androidx/lifecycle/lifecycle-common'],
    'androidx.lifecycle:lifecycle-common-java8': ['androidx.lifecycle_lifecycle-common-java8', 'androidx/lifecycle/lifecycle-common-java8'],
    'androidx.lifecycle:lifecycle-extensions': ['androidx.lifecycle_lifecycle-extensions', 'androidx/lifecycle/lifecycle-extensions'],
    'androidx.lifecycle:lifecycle-livedata': ['androidx.lifecycle_lifecycle-livedata', 'androidx/lifecycle/lifecycle-livedata'],
    'androidx.lifecycle:lifecycle-livedata-core': ['androidx.lifecycle_lifecycle-livedata-core', 'androidx/lifecycle/lifecycle-livedata-core'],
    'androidx.lifecycle:lifecycle-process': ['androidx.lifecycle_lifecycle-process', 'androidx/lifecycle/lifecycle-process'],
    'androidx.lifecycle:lifecycle-runtime': ['androidx.lifecycle_lifecycle-runtime', 'androidx/lifecycle/lifecycle-runtime'],
    'androidx.lifecycle:lifecycle-service': ['androidx.lifecycle_lifecycle-service', 'androidx/lifecycle/lifecycle-service'],
    'androidx.lifecycle:lifecycle-viewmodel': ['androidx.lifecycle_lifecycle-viewmodel', 'androidx/lifecycle/lifecycle-viewmodel'],
    'androidx.paging:paging-common': ['androidx.paging_paging-common', 'androidx/paging/paging-common'],
    'androidx.paging:paging-runtime': ['androidx.paging_paging-runtime', 'androidx/paging/paging-runtime'],
    'androidx.sqlite:sqlite': ['androidx.sqlite_sqlite', 'androidx/sqlite/sqlite'],
    'androidx.sqlite:sqlite-framework': ['androidx.sqlite_sqlite-framework', 'androidx/sqlite/sqlite-framework'],
    'androidx.room:room-common': ['androidx.room_room-common', 'androidx/room/room-common'],
    'androidx.room:room-migration': ['androidx.room_room-migration', 'androidx/room/room-migration'],
    'androidx.room:room-runtime': ['androidx.room_room-runtime', 'androidx/room/room-runtime'],
    'androidx.room:room-testing': ['androidx.room_room-testing', 'androidx/room/room-testing'],

    # Lifecycle
    # Missing dependencies:
    # - auto-common
    # - javapoet
    #'android.arch.lifecycle:compiler': ['android-arch-lifecycle-compiler', 'arch-lifecycle/compiler'],
    # Missing dependencies:
    # - reactive-streams
    #'android.arch.lifecycle:reactivestreams': ['android-arch-lifecycle-reactivestreams','arch-lifecycle/reactivestreams'],

    # Room
    # Missing dependencies:
    # - auto-common
    # - javapoet
    # - antlr4
    # - kotlin-metadata
    # - commons-codec
    #'android.arch.persistence.room:compiler': ['android-arch-room-compiler', 'arch-room/compiler'],
    # Missing dependencies:
    # - rxjava
    #'android.arch.persistence.room:rxjava2': ['android-arch-room-rxjava2', 'arch-room/rxjava2'],

    # Third-party dependencies
    'com.google.android:flexbox': ['flexbox', 'flexbox'],

    # Support Library Material Design Components
    'com.android.support:design': ['android-support-design', 'design'],
    'com.android.support:design-animation': ['android-support-design-animation', 'design-animation'],
    'com.android.support:design-bottomnavigation': ['android-support-design-bottomnavigation', 'design-bottomnavigation'],
    'com.android.support:design-bottomsheet': ['android-support-design-bottomsheet', 'design-bottomsheet'],
    'com.android.support:design-button': ['android-support-design-button', 'design-button'],
    'com.android.support:design-canvas': ['android-support-design-canvas', 'design-canvas'],
    'com.android.support:design-card': ['android-support-design-card', 'design-card'],
    'com.android.support:design-chip': ['android-support-design-chip', 'design-chip'],
    'com.android.support:design-circularreveal': ['android-support-design-circularreveal', 'design-circularreveal'],
    'com.android.support:design-circularreveal-cardview': ['android-support-design-circularreveal-cardview', 'design-circularreveal-cardview'],
    'com.android.support:design-circularreveal-coordinatorlayout': ['android-support-design-circularreveal-coordinatorlayout', 'design-circularreveal-coordinatorlayout'],
    'com.android.support:design-color': ['android-support-design-color', 'design-color'],
    'com.android.support:design-dialog': ['android-support-design-dialog', 'design-dialog'],
    'com.android.support:design-drawable': ['android-support-design-drawable', 'design-drawable'],
    'com.android.support:design-expandable': ['android-support-design-expandable', 'design-expandable'],
    'com.android.support:design-floatingactionbutton': ['android-support-design-floatingactionbutton', 'design-floatingactionbutton'],
    'com.android.support:design-internal': ['android-support-design-internal', 'design-internal'],
    'com.android.support:design-math': ['android-support-design-math', 'design-math'],
    'com.android.support:design-resources': ['android-support-design-resources', 'design-resources'],
    'com.android.support:design-ripple': ['android-support-design-ripple', 'design-ripple'],
    'com.android.support:design-snackbar': ['android-support-design-snackbar', 'design-snackbar'],
    'com.android.support:design-stateful': ['android-support-design-stateful', 'design-stateful'],
    'com.android.support:design-textfield': ['android-support-design-textfield', 'design-textfield'],
    'com.android.support:design-theme': ['android-support-design-theme', 'design-theme'],
    'com.android.support:design-transformation': ['android-support-design-transformation', 'design-transformation'],
    'com.android.support:design-typography': ['android-support-design-typography', 'design-typography'],
    'com.android.support:design-widget': ['android-support-design-widget', 'design-widget'],
    'com.android.support:design-navigation': ['android-support-design-navigation', 'design-navigation'],
    'com.android.support:design-tabs': ['android-support-design-tabs', 'design-tabs'],
    'com.android.support:design-bottomappbar': ['android-support-design-bottomappbar', 'design-bottomappbar'],
    'com.android.support:design-shape': ['android-support-design-shape', 'design-shape'],

    # Androidx Material Design Components
    'com.google.android.material:material': ['com.google.android.material_material', 'com/google/android/material/material'],

    # Intermediate-AndroidX Material Design Components
    'com.android.temp.support:design': ['androidx.design_design', 'com/android/temp/support/design/design'],
    'com.android.temp.support:design-animation': ['androidx.design_design-animation', 'com/android/temp/support/design/design-animation'],
    'com.android.temp.support:design-bottomnavigation': ['androidx.design_design-bottomnavigation', 'com/android/temp/support/design/design-bottomnavigation'],
    'com.android.temp.support:design-bottomsheet': ['androidx.design_design-bottomsheet', 'com/android/temp/support/design/design-bottomsheet'],
    'com.android.temp.support:design-button': ['androidx.design_design-button', 'com/android/temp/support/design/design-button'],
    'com.android.temp.support:design-canvas': ['androidx.design_design-canvas', 'com/android/temp/support/design/design-canvas'],
    'com.android.temp.support:design-card': ['androidx.design_design-card', 'com/android/temp/support/design/design-card'],
    'com.android.temp.support:design-chip': ['androidx.design_design-chip', 'com/android/temp/support/design/design-chip'],
    'com.android.temp.support:design-circularreveal': ['androidx.design_design-circularreveal', 'com/android/temp/support/design/design-circularreveal'],
    'com.android.temp.support:design-circularreveal-cardview': ['androidx.design_design-circularreveal-cardview', 'com/android/temp/support/design/design-circularreveal-cardview'],
    'com.android.temp.support:design-circularreveal-coordinatorlayout': ['androidx.design_design-circularreveal-coordinatorlayout', 'com/android/temp/support/design/design-circularreveal-coordinatorlayout'],
    'com.android.temp.support:design-color': ['androidx.design_design-color', 'com/android/temp/support/design/design-color'],
    'com.android.temp.support:design-dialog': ['androidx.design_design-dialog', 'com/android/temp/support/design/design-dialog'],
    'com.android.temp.support:design-drawable': ['androidx.design_design-drawable', 'com/android/temp/support/design/design-drawable'],
    'com.android.temp.support:design-expandable': ['androidx.design_design-expandable', 'com/android/temp/support/design/design-expandable'],
    'com.android.temp.support:design-floatingactionbutton': ['androidx.design_design-floatingactionbutton', 'com/android/temp/support/design/design-floatingactionbutton'],
    'com.android.temp.support:design-internal': ['androidx.design_design-internal', 'com/android/temp/support/design/design-internal'],
    'com.android.temp.support:design-math': ['androidx.design_design-math', 'com/android/temp/support/design/design-math'],
    'com.android.temp.support:design-resources': ['androidx.design_design-resources', 'com/android/temp/support/design/design-resources'],
    'com.android.temp.support:design-ripple': ['androidx.design_design-ripple', 'com/android/temp/support/design/design-ripple'],
    'com.android.temp.support:design-snackbar': ['androidx.design_design-snackbar', 'com/android/temp/support/design/design-snackbar'],
    'com.android.temp.support:design-stateful': ['androidx.design_design-stateful', 'com/android/temp/support/design/design-stateful'],
    'com.android.temp.support:design-textfield': ['androidx.design_design-textfield', 'com/android/temp/support/design/design-textfield'],
    'com.android.temp.support:design-theme': ['androidx.design_design-theme', 'com/android/temp/support/design/design-theme'],
    'com.android.temp.support:design-transformation': ['androidx.design_design-transformation', 'com/android/temp/support/design/design-transformation'],
    'com.android.temp.support:design-typography': ['androidx.design_design-typography', 'com/android/temp/support/design/design-typography'],
    'com.android.temp.support:design-widget': ['androidx.design_design-widget', 'com/android/temp/support/design/design-widget'],
    'com.android.temp.support:design-navigation': ['androidx.design_design-navigation', 'com/android/temp/support/design/design-navigation'],
    'com.android.temp.support:design-tabs': ['androidx.design_design-tabs', 'com/android/temp/support/design/design-tabs'],
    'com.android.temp.support:design-bottomappbar': ['androidx.design_design-bottomappbar', 'com/android/temp/support/design/design-bottomappbar'],
    'com.android.temp.support:design-shape': ['androidx.design_design-shape', 'com/android/temp/support/design/design-shape'],

}

# Always remove these files.
blacklist_files = [
    'annotations.zip',
    'public.txt',
    'R.txt',
    'AndroidManifest.xml',
    os.path.join('libs','noto-emoji-compat-java.jar')
]

artifact_pattern = re.compile(r"^(.+?)-(\d+\.\d+\.\d+(?:-\w+\d+)?(?:-[\d.]+)*)\.(jar|aar)$")


class MavenLibraryInfo:
    def __init__(self, key, group_id, artifact_id, version, dir, repo_dir, file):
        self.key = key
        self.group_id = group_id
        self.artifact_id = artifact_id
        self.version = version
        self.dir = dir
        self.repo_dir = repo_dir
        self.file = file


def print_e(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)


def touch(fname, times=None):
    with open(fname, 'a'):
        os.utime(fname, times)


def path(*path_parts):
    return reduce((lambda x, y: os.path.join(x, y)), path_parts)


def flatten(list):
    return reduce((lambda x, y: "%s %s" % (x, y)), list)


def rm(path):
    if os.path.isdir(path):
        rmtree(path)
    elif os.path.exists(path):
        os.remove(path)


def mv(src_path, dst_path):
    if os.path.exists(dst_path):
        rm(dst_path)
    if not os.path.exists(os.path.dirname(dst_path)):
        os.makedirs(os.path.dirname(dst_path))
    os.rename(src_path, dst_path)


def detect_artifacts(maven_repo_dirs):
    maven_lib_info = {}

    # Find the latest revision for each artifact, remove others
    for repo_dir in maven_repo_dirs:
        for root, dirs, files in os.walk(repo_dir):
            for file in files:
                if file[-4:] == ".pom":
                    # Read the POM (hack hack hack).
                    group_id = ''
                    artifact_id = ''
                    version = ''
                    file = os.path.join(root, file)
                    with open(file) as pom_file:
                        for line in pom_file:
                            if line[:11] == '  <groupId>':
                                group_id = line[11:-11]
                            elif line[:14] == '  <artifactId>':
                                artifact_id = line[14:-14]
                            elif line[:11] == '  <version>':
                                version = line[11:-11]
                    if group_id == '' or artifact_id == '' or version == '':
                        print_e('Failed to find Maven artifact data in ' + file)
                        continue

                    # Locate the artifact.
                    artifact_file = file[:-4]
                    if os.path.exists(artifact_file + '.jar'):
                        artifact_file = artifact_file + '.jar'
                    elif os.path.exists(artifact_file + '.aar'):
                        artifact_file = artifact_file + '.aar'
                    else:
                        print_e('Failed to find artifact for ' + artifact_file)
                        continue

                    # Make relative to root.
                    artifact_file = artifact_file[len(root) + 1:]

                    # Find the mapping.
                    group_artifact = group_id + ':' + artifact_id
                    if group_artifact in maven_to_make:
                        key = group_artifact
                    elif artifact_id in maven_to_make:
                        key = artifact_id
                    else:
                        print_e('Failed to find artifact mapping for ' + group_artifact)
                        continue

                    # Store the latest version.
                    version = LooseVersion(version)
                    if key not in maven_lib_info \
                            or version > maven_lib_info[key].version:
                        maven_lib_info[key] = MavenLibraryInfo(key, group_id, artifact_id, version,
                                                               root, repo_dir, artifact_file)

    return maven_lib_info


def transform_maven_repos(maven_repo_dirs, transformed_dir, extract_res=True, include_static_deps=True):
    cwd = os.getcwd()

    # Use a temporary working directory.
    maven_lib_info = detect_artifacts(maven_repo_dirs)
    working_dir = temp_dir

    if not maven_lib_info:
        print_e('Failed to detect artifacts')
        return False

    # extract some files (for example, AndroidManifest.xml) from any relevant artifacts
    for info in maven_lib_info.values():
        transform_maven_lib(working_dir, info, extract_res)

    # generate a single Android.bp that specifies to use all of the above artifacts
    makefile = os.path.join(working_dir, 'Android.bp')
    with open(makefile, 'w') as f:
        args = ["pom2bp", "-sdk-version", "current"]
        if include_static_deps:
            args.append("-static-deps")
        rewriteNames = sorted([name for name in maven_to_make if ":" in name] + [name for name in maven_to_make if ":" not in name])
        args.extend(["-rewrite=^" + name + "$=" + maven_to_make[name][0] for name in rewriteNames])
        args.extend(["-extra-deps=android-support-car=prebuilt-android.car-stubs"])
        # these depend on GSON which is not in AOSP
        args.extend(["-exclude=androidx.room_room-migration",
                     "-exclude=androidx.room_room-testing",
                     "-exclude=android-arch-room-migration",
                     "-exclude=android-arch-room-testing"])
        args.extend(["."])
        subprocess.check_call(args, stdout=f, cwd=working_dir)

    # Replace the old directory.
    output_dir = os.path.join(cwd, transformed_dir)
    mv(working_dir, output_dir)
    return True

# moves <artifact_info> (of type MavenLibraryInfo) into the appropriate part of <working_dir> , and possibly unpacks any necessary included files
def transform_maven_lib(working_dir, artifact_info, extract_res):
    # Move library into working dir
    new_dir = os.path.normpath(os.path.join(working_dir, os.path.relpath(artifact_info.dir, artifact_info.repo_dir)))
    mv(artifact_info.dir, new_dir)

    for dirpath, dirs, files in os.walk(new_dir):
        for f in files:
            if '-sources.jar' in f:
                os.remove(os.path.join(dirpath, f))

    matcher = artifact_pattern.match(artifact_info.file)
    maven_lib_name = artifact_info.key
    maven_lib_vers = matcher.group(2)
    maven_lib_type = artifact_info.file[-3:]

    make_lib_name = maven_to_make[artifact_info.key][0]
    make_dir_name = maven_to_make[artifact_info.key][1]

    artifact_file = os.path.join(new_dir, artifact_info.file)

    if maven_lib_type == "aar":
        if extract_res:
            target_dir = os.path.join(working_dir, make_dir_name)
            if not os.path.exists(target_dir):
                os.makedirs(target_dir)

            process_aar(artifact_file, target_dir)

        with zipfile.ZipFile(artifact_file) as zip:
            manifests_dir = os.path.join(working_dir, "manifests")
            zip.extract("AndroidManifest.xml", os.path.join(manifests_dir, make_lib_name))

    print(maven_lib_vers, ":", maven_lib_name, "->", make_lib_name)


def process_aar(artifact_file, target_dir):
    # Extract AAR file to target_dir.
    with zipfile.ZipFile(artifact_file) as zip:
        zip.extractall(target_dir)

    # Remove classes.jar
    classes_jar = os.path.join(target_dir, "classes.jar")
    if os.path.exists(classes_jar):
        os.remove(classes_jar)

    # Remove or preserve empty dirs.
    for root, dirs, files in os.walk(target_dir):
        for dir in dirs:
            dir_path = os.path.join(root, dir)
            if not os.listdir(dir_path):
                os.rmdir(dir_path)

    # Remove top-level cruft.
    for file in blacklist_files:
        file_path = os.path.join(target_dir, file)
        if os.path.exists(file_path):
            os.remove(file_path)


def fetch_artifact(target, build_id, artifact_path):
    print('Fetching %s from %s...' % (artifact_path, target))
    fetch_cmd = [FETCH_ARTIFACT, '--bid', str(build_id), '--target', target, artifact_path]
    try:
        subprocess.check_output(fetch_cmd, stderr=subprocess.STDOUT)
    except subprocess.CalledProcessError:
        print_e('FAIL: Unable to retrieve %s artifact for build ID %s' % (artifact_path, build_id))
        print_e('Please make sure you are authenticated for build server access!')
        return None
    return artifact_path


def extract_artifact(artifact_path):
    # Unzip the repo archive into a separate directory.
    repo_dir = os.path.basename(artifact_path)[:-4]
    with zipfile.ZipFile(artifact_path) as zipFile:
        zipFile.extractall(repo_dir)
    return repo_dir


def fetch_and_extract(target, build_id, file, artifact_path=None):
    if not artifact_path:
        artifact_path = fetch_artifact(target, build_id, file)
    if not artifact_path:
        return None
    return extract_artifact(artifact_path)


def update_support(target, build_id, local_file):
    if build_id:
        repo_file = 'top-of-tree-m2repository-dejetified-%s.zip' % build_id.fs_id
        repo_dir = fetch_and_extract(target, build_id.url_id, repo_file, None)
    else:
        repo_dir = fetch_and_extract(target, None, None, local_file)
    if not repo_dir:
        print_e('Failed to extract Support Library repository')
        return False

    # Transform the repo archive into a Makefile-compatible format.
    return transform_maven_repos([repo_dir], support_dir, extract_res=True)


def update_androidx(target, build_id, local_file):
    if build_id:
        repo_file = 'top-of-tree-m2repository-%s.zip' % build_id.fs_id
        repo_dir = fetch_and_extract(target, build_id.url_id, repo_file, None)
    else:
        repo_dir = fetch_and_extract(target, None, None, local_file)
    if not repo_dir:
        print_e('Failed to extract AndroidX repository')
        return False

    # Transform the repo archive into a Makefile-compatible format.
    return transform_maven_repos([repo_dir], androidx_dir, extract_res=False)


def update_jetifier(target, build_id):
    repo_file = 'jetifier-standalone.zip'
    repo_dir = fetch_and_extract(target, build_id.url_id, repo_file)
    if not repo_dir:
        print_e('Failed to extract Jetifier')
        return False

    rm(jetifier_dir)
    mv(os.path.join(repo_dir, 'jetifier-standalone'), jetifier_dir)
    os.chmod(os.path.join(jetifier_dir, 'bin', 'jetifier-standalone'), 0o755)
    return True


def update_constraint(target, build_id):
    layout_dir = fetch_and_extract(target, build_id.url_id,
                                   'com.android.support.constraint-constraint-layout-%s.zip' % build_id.fs_id)
    solver_dir = fetch_and_extract(target, build_id.url_id,
                                   'com.android.support.constraint-constraint-layout-solver-%s.zip' % build_id.fs_id)
    if not layout_dir or not solver_dir:
        print_e('Failed to extract Constraint Layout repositories')
        return False

    # Passing False here is an inelegant solution, but it means we can replace
    # the top-level directory without worrying about other child directories.
    return transform_maven_repos([layout_dir, solver_dir],
                                os.path.join(extras_dir, 'constraint-layout'), extract_res=False)

def update_constraint_x(local_file):
    repo_dir = extract_artifact(local_file)
    if not repo_dir:
        print_e('Failed to extract Constraint Layout X')
        return False
    return transform_maven_repos([repo_dir], os.path.join(extras_dir, 'constraint-layout-x'), extract_res=False)


def update_design(file):
    design_dir = extract_artifact(file)
    if not design_dir:
        print_e('Failed to extract Design Library repositories')
        return False

    # Don't bother extracting resources -- this should only be used with AAPT2.
    return transform_maven_repos([design_dir],
                                os.path.join(extras_dir, 'material-design'), extract_res=False)


def update_material(file):
    design_dir = extract_artifact(file)
    if not design_dir:
        print_e('Failed to extract intermediate-AndroidX Design Library repositories')
        return False

    # Don't bother extracting resources -- this should only be used with AAPT2.
    return transform_maven_repos([design_dir],
                                 os.path.join(extras_dir, 'material-design-x'), extract_res=False)


def extract_to(zip_file, paths, filename, parent_path):
    zip_path = next(filter(lambda path: filename in path, paths))
    src_path = zip_file.extract(zip_path)
    dst_path = path(parent_path, filename)
    mv(src_path, dst_path)


def fetch_framework_artifacts(target, build_id, target_path, is_current_sdk):
    platform = 'darwin' if 'mac' in target else 'linux'
    artifact_path = fetch_artifact(
        target, build_id.url_id, 'sdk-repo-%s-platforms-%s.zip' % (platform, build_id.fs_id))
    if not artifact_path:
        return False

    with zipfile.ZipFile(artifact_path) as zipFile:
        paths = zipFile.namelist()

        filenames = ['android.jar', 'uiautomator.jar',  'framework.aidl',
            'optional/android.test.base.jar', 'optional/android.test.mock.jar',
            'optional/android.test.runner.jar']

        for filename in filenames:
            extract_to(zipFile, paths, filename, target_path)

        if is_current_sdk:
            # There's no system version of framework.aidl, so use the public one.
            extract_to(zipFile, paths, 'framework.aidl', system_path)

            # We don't keep historical artifacts for these.
            artifact_path = fetch_artifact(target, build_id.fs_id, 'core.current.stubs.jar')
            if not artifact_path:
                return False
            mv(artifact_path, path(current_path, 'core.jar'))

    return True


def update_sdk_repo(target, build_id):
    return fetch_framework_artifacts(target, build_id, current_path, is_current_sdk = True)


def update_system(target, build_id):
    artifact_path = fetch_artifact(target, build_id.url_id, 'android_system.jar')
    if not artifact_path:
        return False

    mv(artifact_path, path(system_path, 'android.jar'))

    artifact_path = fetch_artifact(target, build_id.url_id, 'android.test.mock.stubs_system.jar')
    if not artifact_path:
        return False

    mv(artifact_path, path(system_path, 'optional/android.test.mock.jar'))

    return True



def finalize_sdk(target, build_id, sdk_version):
    target_finalize_dir = "%d" % sdk_version
    target_finalize_system_dir = "system_%d" % sdk_version

    artifact_to_path = {
      'android_system.jar': path(target_finalize_system_dir, 'android.jar'),
      'public_api.txt': path(api_path, "%d.txt" % sdk_version),
      'system-api.txt': path(system_api_path, "%d.txt" % sdk_version),
    }

    for artifact, target_path in artifact_to_path.items():
        artifact_path = fetch_artifact(target, build_id.url_id, artifact)
        if not artifact_path:
            return False
        mv(artifact_path, target_path)

    if not fetch_framework_artifacts(target, build_id, target_finalize_dir, is_current_sdk = False):
      return False

    copyfile(path(target_finalize_dir, 'framework.aidl'),
             path(target_finalize_system_dir, 'framework.aidl'))
    return True


def update_buildtools(target, arch, build_id):
    artifact_path = fetch_and_extract(target, build_id.url_id,
                                   "sdk-repo-%s-build-tools-%s.zip" % (arch, build_id.fs_id))
    if not artifact_path:
        return False

    top_level_dir = os.listdir(artifact_path)[0]
    src_path = os.path.join(artifact_path, top_level_dir)
    dst_path = path(buildtools_dir, arch)
    mv(src_path, dst_path)

    # Move all top-level files to /bin and make them executable
    bin_path = path(dst_path, 'bin')
    top_level_files = filter(lambda e: os.path.isfile(path(dst_path, e)), os.listdir(dst_path))
    for file in top_level_files:
        src_file = path(dst_path, file)
        dst_file = path(bin_path, file)
        mv(src_file, dst_file)
        os.chmod(dst_file, 0o755)

    # Remove renderscript
    rm(path(dst_path, 'renderscript'))

    return True


def append(text, more_text):
    if text:
        return "%s, %s" % (text, more_text)
    return more_text


class buildId(object):
  def __init__(self, url_id, fs_id):
    # id when used in build server urls
    self.url_id = url_id
    # id when used in build commands
    self.fs_id = fs_id

def getBuildId(args):
  # must be in the format 12345 or P12345
  source = args.source
  number_text = source[:]
  presubmit = False
  if number_text.startswith("P"):
    presubmit = True
    number_text = number_text[1:]
  if not number_text.isnumeric():
    return None
  url_id = source
  fs_id = url_id
  if presubmit:
    fs_id = "0"
  args.file = False
  return buildId(url_id, fs_id)

def getFile(args):
  source = args.source
  if not source.isnumeric():
    return args.source
  return None


def script_relative(rel_path):
    return os.path.join(script_dir, rel_path)


def uncommittedChangesExist():
    try:
        # Make sure we don't overwrite any pending changes.
        diffCommand = "cd " + git_dir + " && git diff --quiet"
        subprocess.check_call(diffCommand, shell=True)
        subprocess.check_call(diffCommand + " --cached", shell=True)
        return False
    except subprocess.CalledProcessError:
        return True


rm(temp_dir)
parser = argparse.ArgumentParser(
    description=('Update current prebuilts'))
parser.add_argument(
    'source',
    help='Build server build ID or local Maven ZIP file')
parser.add_argument(
    '-d', '--design', action="store_true",
    help='If specified, updates only the Design Library')
parser.add_argument(
    '-m', '--material', action="store_true",
    help='If specified, updates only the intermediate-AndroidX Design Library')
parser.add_argument(
    '-c', '--constraint', action="store_true",
    help='If specified, updates only Constraint Layout')
parser.add_argument(
    '--constraint_x', action="store_true",
    help='If specified, updates Constraint Layout X')
parser.add_argument(
    '-j', '--jetifier', action="store_true",
    help='If specified, updates only Jetifier')
parser.add_argument(
    '-p', '--platform', action="store_true",
    help='If specified, updates only the Android Platform')
parser.add_argument(
    '-f', '--finalize_sdk', type=int,
    help='If specified, imports the source build as the specified finalized SDK version')
parser.add_argument(
    '-b', '--buildtools', action="store_true",
    help='If specified, updates only the Build Tools')
parser.add_argument(
    '--stx', action="store_true",
    help='If specified, updates Support Library and Androidx (that is, all artifacts built from frameworks/support)')
parser.add_argument(
    '--commit-first', action="store_true",
    help='If specified, then if uncommited changes exist, commit before continuing')
args = parser.parse_args()
if args.stx:
    args.support = args.androidx = True
else:
    args.support = args.androidx = False
args.file = True
if not args.source:
    parser.error("You must specify a build ID or local Maven ZIP file")
    sys.exit(1)
if not (args.support or args.platform or args.constraint or args.buildtools \
                or args.design or args.jetifier or args.androidx or args.material \
                or args.finalize_sdk or args.constraint_x):
    parser.error("You must specify at least one target to update")
    sys.exit(1)
if (args.support or args.constraint or args.constraint_x or args.design or args.material or args.androidx) \
        and which('pom2bp') is None:
    parser.error("Cannot find pom2bp in path; please run lunch to set up build environment")
    sys.exit(1)

if uncommittedChangesExist():
    if args.commit_first:
        subprocess.check_call("cd " + git_dir + " && git add -u", shell=True)
        subprocess.check_call("cd " + git_dir + " && git commit -m 'save working state'", shell=True)

if uncommittedChangesExist():
    print_e('FAIL: There are uncommitted changes here. Please commit or stash before continuing, because %s will run "git reset --hard" if execution fails' % os.path.basename(__file__))
    sys.exit(1)

try:
    components = None
    if args.constraint:
        if update_constraint('studio', getBuildId(args)):
            components = append(components, 'Constraint Layout')
        else:
            print_e('Failed to update Constraint Layout, aborting...')
            sys.exit(1)
    if args.constraint_x:
        if update_constraint_x(getFile(args)):
            components = append(components, 'Constraint Layout X')
        else:
            print_e('Failed to update Constraint Layout X, aborting...')
            sys.exit(1)
    if args.support:
        if update_support('support_library', getBuildId(args), getFile(args)):
            components = append(components, 'Support Library')
        else:
            print_e('Failed to update Support Library, aborting...')
            sys.exit(1)
    if args.androidx:
        if update_androidx('support_library', \
                           getBuildId(args), getFile(args)):
            components = append(components, 'AndroidX')
        else:
            print_e('Failed to update AndroidX, aborting...')
            sys.exit(1)
    if args.jetifier:
        if update_jetifier('support_library', getBuildId(args)):
            components = append(components, 'Jetifier')
        else:
            print_e('Failed to update Jetifier, aborting...')
            sys.exit(1)
    if args.platform  or args.finalize_sdk:
        if update_sdk_repo('sdk_phone_armv7-sdk_mac', getBuildId(args)) \
                and update_system('sdk_phone_armv7-sdk_mac', getBuildId(args)):
            components = append(components, 'platform SDK')
        else:
            print_e('Failed to update platform SDK, aborting...')
            sys.exit(1)
    if args.finalize_sdk:
        n = args.finalize_sdk
        if finalize_sdk('sdk_phone_armv7-sdk_mac', getBuildId(args), n):
            # We commit the finalized dir separately from the current sdk update.
            msg = "Import final sdk version %d from build %s" % (n, getBuildId(args).url_id)
            dirs = ["%d" % n, 'system_%d' % n, api_path, system_api_path]
            subprocess.check_call(['git', 'add'] + dirs)
            subprocess.check_call(['git', 'commit', '-m', msg])
        else:
            print_e('Failed to finalize SDK %d, aborting...' % n)
            sys.exit(1)
    if args.design:
        if update_design(getFile(args)):
            components = append(components, 'Design Library')
        else:
            print_e('Failed to update Design Library, aborting...')
            sys.exit(1)
    if args.material:
        if update_material(getFile(args)):
            components = append(components, 'intermediate-AndroidX Design Library')
        else:
            print_e('Failed to update intermediate-AndroidX Design Library, aborting...')
            sys.exit(1)
    if args.buildtools:
        if update_buildtools('sdk_phone_armv7-sdk_mac', 'darwin', getBuildId(args)) \
                and update_buildtools('sdk_phone_x86_64-sdk', 'linux', getBuildId(args)) \
                and update_buildtools('sdk_phone_armv7-win_sdk', 'windows', getBuildId(args)):
            components = append(components, 'build tools')
        else:
            print_e('Failed to update build tools, aborting...')
            sys.exit(1)



    subprocess.check_call(['git', 'add', current_path, system_path, buildtools_dir])
    if not args.source.isnumeric():
        src_msg = "local Maven ZIP"
    else:
        src_msg = "build %s" % (getBuildId(args).url_id)
    msg = "Import %s from %s\n\n%s" % (components, src_msg, flatten(sys.argv))
    subprocess.check_call(['git', 'commit', '-m', msg])
    if args.finalize_sdk:
        print('NOTE: Created two commits:')
        subprocess.check_call(['git', 'log', '-2', '--oneline'])
    print('Remember to test this change before uploading it to Gerrit!')

finally:
    # Revert all stray files, including the downloaded zip.
    try:
        with open(os.devnull, 'w') as bitbucket:
            subprocess.check_call(['git', 'add', '-Af', '.'], stdout=bitbucket)
            subprocess.check_call(
                ['git', 'commit', '-m', 'COMMIT TO REVERT - RESET ME!!!', '--allow-empty'], stdout=bitbucket)
            subprocess.check_call(['git', 'reset', '--hard', 'HEAD~1'], stdout=bitbucket)
    except subprocess.CalledProcessError:
        print_e('ERROR: Failed cleaning up, manual cleanup required!!!')
