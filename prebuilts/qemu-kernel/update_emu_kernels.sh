#!/bin/bash
set -e

manual_mode=false
version=3.18

while getopts "mv:" opt; do
    case $opt in
	m) manual_mode=true
	    ;;
	v) version=$OPTARG
	   ;;
	?) echo "Usage: $0 [-m] [-v version]"
	   echo "   -m: manually specify build numbers"
	   echo "   -v: specify kernel version [default 3.10]"
	   exit 1
	   ;;
    esac
done

if [[ "$version" != "3.10" && "$version" != "3.18" ]]
then
	echo "kernel version must be 3.10 or 3.18"
	exit 1
fi

abcmd_lkgb='/google/data/ro/projects/android/ab lkgb --target kernel --branch'
fetch_artifact='/google/data/ro/projects/android/fetch_artifact --request_timeout_secs 60 --target kernel'

branch_prefix='kernel-n-dev-android-goldfish-'

# kernel_img[branch]="build_server_output local_file_name"
declare -A kernel_img

kernel_img[3.10-arm]="zImage arm/3.10/kernel-qemu"
kernel_img[3.10-arm64]="Image arm64/3.10/kernel-qemu"
kernel_img[3.10-mips]="vmlinux mips/3.10/kernel-qemu"
kernel_img[3.10-mips64]="vmlinux mips64/3.10/kernel-qemu"
kernel_img[3.10-x86]="bzImage x86/3.10/kernel-qemu"
kernel_img[3.10-x86_64]="bzImage x86_64/3.10/kernel-qemu"
kernel_img[3.10-x86_64-qemu1]="bzImage x86_64/kernel-qemu"
kernel_img[3.18-arm]="zImage arm/3.18/kernel-qemu2"
kernel_img[3.18-arm64]="Image arm64/3.18/kernel-qemu2"
kernel_img[3.18-mips]="vmlinux mips/3.18/kernel-qemu2"
kernel_img[3.18-mips64]="vmlinux mips64/3.18/kernel-qemu2"
kernel_img[3.18-x86]="bzImage x86/3.18/kernel-qemu2"
kernel_img[3.18-x86_64]="bzImage x86_64/3.18/kernel-qemu2"

printf "Upgrade emulator kernels $version\n\n" > emu_kernel.commitmsg

for key in "${!kernel_img[@]}"
do
	if [[ $key != $version* ]]
	then
		continue
	fi

	branch=$branch_prefix$key

	# Find the latest build by searching for highest build number since
	# build server doesn't provide the "latest" link.
	build=`$abcmd_lkgb $branch | cut -d' ' -f3 | head -n 1`

	if $manual_mode
	then
		read -p "Enter build number for $branch: [$build]" input
		build="${input:-$build}"
	fi

	echo Fetching build $build from branch $branch

	# file_info[0] - kernel image on build server
	# file_info[1] - kernel image in local tree
	file_info=(${kernel_img[$key]})

	$fetch_artifact --bid $build ${file_info[0]} ${file_info[1]}

	git add ${file_info[1]}

	printf "$branch - build: $build\n" >> emu_kernel.commitmsg
done

last_commit=`git log | \
	sed -rn "s/.*Upgrade $version kernel images to ([a-z0-9]+).*/\1/p" | \
	head -n 1`

if [ ! -d goldfish_cache ]
then
	mkdir goldfish_cache
	git clone https://android.googlesource.com/kernel/goldfish goldfish_cache
fi

pushd goldfish_cache

git fetch origin

git checkout remotes/origin/android-goldfish-$version
tot_commit=`git log --oneline -1 | cut -d' ' -f1`
printf "\nUpgrade $version kernel images to ${tot_commit}\n" >> ../emu_kernel.commitmsg

line_count=`git log --oneline HEAD...${last_commit} | wc -l`
if [ "$line_count" -gt "6" ]
then
	git log --oneline --no-decorate -3 >> ../emu_kernel.commitmsg
	echo "..." >> ../emu_kernel.commitmsg
	git log --oneline --no-decorate HEAD...${last_commit} | tail -n 3 >> ../emu_kernel.commitmsg
else
	git log --oneline --no-decorate HEAD...${last_commit} >> ../emu_kernel.commitmsg
fi

popd

git commit -t emu_kernel.commitmsg

rm emu_kernel.commitmsg

