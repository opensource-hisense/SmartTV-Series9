#!/usr/bin/perl
open memDumpFile, "<test_eyescan.bin";
open eyeFile, ">eye.txt";

my $tmpX = 0;
my $tmpY = 0;
while (<memDumpFile>){
	my $tmpLine = $_;
	
	#print "$tmpLine**match**\n";
	$tmpLine=~/0x(\w+) (\w{8})/;
	my $field1 = $2;
	$field1 = hex $field1;
	$field1 = $field1;
	print "[$field1]\n";
	print eyeFile sprintf "%u,", $field1;
	$tmpY = $tmpY+1;
	if($tmpY eq 128){
		$tmpY = 0;
		$tmpX = tmpX+1;
		print eyeFile "\n";
	}
}

