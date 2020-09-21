#!/usr/bin/perl -w

use File::Spec;
my $path_curf = File::Spec->rel2abs(__FILE__);
my ($vol, $dirs, $file) = File::Spec->splitpath($path_curf);
print "C Dir = ", $dirs,"\n";

my $name;
my $dpath;
$name = `whoami`;
chomp $name;

use POSIX;
my $time=strftime("%Y%m%d_%H%M%S",localtime());
$ddir = "/tmp/".$name;
$output = `mkdir $ddir`;
$dpath = "$ddir"."/".$time.".diff";
print "dpath:", $dpath, "\n";

my $FILE;
my $FILE_O;
open($FILE, '<&STDIN');
open ($FILE_O,">",$dpath);
while (<$FILE>) {
	chomp;
	my $line = $_;
	print $FILE_O "$line";
	print $FILE_O "\n";
}
close $FILE;
close $FILE_O;

system("python $dirs/check_patch.py $dpath");
$exitcode = $?;
system("rm $dpath");
if($exitcode == 0)
{
	exit 0;
}
else
{
	exit 1;
}


