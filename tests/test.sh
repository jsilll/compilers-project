#! /bin/sh 

make >/dev/null

for file in $(ls src/*.exe)
do
    bname=`basename $file .exe`
    expectedfile=expected/$bname.out
    outfile=src/$bname.out
    $file | tr -d " \t\n\r" > $outfile
    printf "\n" >> $outfile

    echo $outfile
    diff --color $outfile $expectedfile
done
