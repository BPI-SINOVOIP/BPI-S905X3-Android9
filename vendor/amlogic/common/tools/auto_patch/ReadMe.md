# commit process:

There have three steps:

1. generate patches those needed to patched.
  use command like `git format-patch`.
>Notice, every patch must contain Change-Id, like this

```
commit 8fae8012bcb4b7573a8475ea48debf9968adc9c9
Author: Shubang Lu <shubang@google.com>
Date:   Wed Aug 1 18:11:46 2018 -0700

    Forward volume keys when system audio mode off and property set

    Bug: 80296335
    Change-Id: I04b7cd0958c9300a76f6337ee891b5f4947484ad
```
> if not, and need exec commands to generate it as follows:

```
gitdir=$(git rev-parse --git-dir); scp -p -P 29418 user.name@aml-code-master.amlogic.com:hooks/commit-msg ${gitdir}/hooks/
  //you need replace you name to user.name, like ming.li

git commit --amend
```

2. patch name

modify patch name, like this, include path and sequence
    frameworks#base#0001.patch
    frameworks#base#0002.patch
    system#core#0001.patch


3. patch directory name.

       in the directory `vendor/amlogic/tools/auto_patch/`, create dir like this
   "Recovery", **Recovery"** is the module name..
   like "Recovery" module, and copy second step patches to this directory.
       if you wish your patch not applied on ATV version, you can named the directory to "Recovery_aosp_ui"; 
       if you wish your patch not applied on GTVS version, you can named the directory to "Recovery_gtvs_ui"; 
       if you wish your patch only applied on TV platform, you can named the directory to "Recovery_tv_platform"; 
