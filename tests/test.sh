#! /bin/sh 

make >/dev/null

for file in $(ls src/*.exe)
do
    bname=`basename $file .exe`
    expectedfile=expected/$bname.out
    outfile=src/$bname.out
    $file > $outfile
    
    echo $outfile;
    diff $expectedfile $outfile

done