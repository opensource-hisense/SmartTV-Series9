#!/usr/bin/perl
open memDumpFile, "<test_eyescan.bin";
open eyeFile, ">eye.txt";

my $tmpX = 0;
my $tmpY = 0;
while (<memDumpFile>){
	my $tmpLine = $_;
	
	#print "$tmpLine**match**\n";
	#Dump TRB: 0x3c3b000 0x0 0x0 0x1002c01
	$tmpLine=~/0x(\w+) (\w{4})(\w{4})/;
	my $field1 = $2;
	my $field2 = $3;
	$field1 = hex $field1;
	$field2 = hex $field2;
	$field1 = $field1;
	$field2 = $field2;
	print "[$field1] [$field2]\n";
	print eyeFile sprintf "%u,%u,", $field1, $field2;
	$tmpY = $tmpY+2;
	if($tmpY eq 128){
		$tmpY = 0;
		$tmpX = tmpX+1;
		print eyeFile "\n";
	}
}

