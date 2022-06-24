#! /bin/sh 

for file in $(ls src/*.l22)
do
    bname=`basename $file .l22`
    expectedfile=expected/$bname.out
    execfile=src/$bname.exe
    outfile=src/$bname.out

    make -B $execfile 2> /dev/null 1> /dev/null
    $execfile | tr -d "\t\n\r" > $outfile
    printf "\n" >> $outfile

    echo $outfile
    diff --color $outfile $expectedfile
done
