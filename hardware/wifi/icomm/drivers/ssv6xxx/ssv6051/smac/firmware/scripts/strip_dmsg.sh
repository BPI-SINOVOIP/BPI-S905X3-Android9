#!/bin/bash
tag_do="=> "
tag_err="error! "
msg_exit="program exit!"

# directory & file
dir_top=$(cd .. && pwd)
dir_sim=$dir_top/sim
dir_img=$dir_top/soc/image
fname=ssv6200-uart
fbin=$dir_img/$fname.bin
fmap=$dir_img/$fname.map
fout=$dir_img/$fname.dmsg
ftmp=$fmap.tmp
fold=$fbin.old


sec_name=".dmsg"
# section addr in digital & hex
sec_addr_d=-1
declare -i sec_addr_h
sec_size=-1
# flag
# 0: not start
# 1: ready to read sec_addr
# 2: ready to read sec_size
# 3: done
flag=0

function halt() {
	echo "program halt!"
	exit 1
}

echo
echo "       < dmsg stripping tool >          "
echo "----------------------------------------"
echo "date :" $(date)
echo "dir_top :" $dir_top
echo "fin  :" $fbin
echo "fout :" $fout
echo "ftmp :" $ftmp
echo "fold :" $fold 
echo "      (original .bin before stripping)"
echo

if [ -e $fold ]; then
	echo $tag_do "rm previous fold -> ok!"
	$(rm $fold)
fi
if [ -e $ftmp ]; then
	echo $tag_do "rm previous ftmp -> ok!"
	$(rm $ftmp)
fi
if [ -e $fout ]; then
	echo $tag_do "rm previous fout -> ok!"
	$(rm $fout)
fi
if [ ! -e $fmap ]; then
	echo $tag_err $fmap "does not exist!"
	halt;
fi

$(cat $fmap | grep '.dmsg' > $ftmp)
echo 
echo $tag_do "gen ftmp -> ok!"

for token in $(cat $ftmp)
do 
	if [ $flag == 1 ]; then
		# sec_addr_h=$(printf %08x $token)
		sec_addr_h=$token
		sec_addr_d=$(printf %d $token)
		flag=2
		continue
	fi
	if [ $flag == 2 ]; then
		sec_size=$(printf %d $token)
		flag=3
		break
	fi
	if [ $flag == 0 ] && [ $token == $sec_name ]; then
		flag=1
		echo $tag_do "find section [" $sec_name "] -> ok!"
	fi
done

if [ $flag == 0 ]; then
	echo $tag_do "find section [" $sec_name "] -> fail!"
	halt;
fi

echo "addr(d) =" $sec_addr_d
echo "addr(h) =" $(printf 0x%08x $sec_addr_h)
echo "size    =" $sec_size
echo 

echo $tag_do "dmsg stripping -> start ..."
$(mv $fbin $fold)
$(dd if=$fold of=$fout bs=1 skip=$sec_addr_d count=$sec_size)
$(dd if=$fold of=$fbin bs=1 count=$sec_addr_d)
#$(rm $fold)

b0=$(printf %02x $(($sec_addr_h % 0x100)))
b1=$(printf %02x $(($sec_addr_h / 0x100   % 0x100)))
b2=$(printf %02x $(($sec_addr_h / 0x10000 % 0x100)))
b3=$(printf %02x $(($sec_addr_h / 0x1000000)))
echo -n -e \\x$b0\\x$b1\\x$b2\\x$b3 >> $fout
echo
echo "append 4 bytes addr (0x"$b3$b2$b1$b0") to fout ..."
echo $tag_do "dmsg stripping -> ok!"

$(rm $ftmp)
echo
echo $tag_do "rm ftmp -> ok!"

echo $tag_do "ls -l " $dir_img && ls -l $dir_img
echo

echo $tag_do "copy .dmsg & .bin files ..."
cp -v $fbin $fout $dir_sim/MSVC/cabrio-sim/cabrio-sim
echo
cp -v $fbin $fout $dir_sim/MSVC/cabrio-sim/cabrio-sim/image
echo
cp -v $fbin $fout $dir_sim/Linux
echo

echo
echo "goodbye!!"
exit 0


