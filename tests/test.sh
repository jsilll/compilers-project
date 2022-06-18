#! /bin/sh 

make >/dev/null

for file in $(ls src/src)
do
    echo $file;
    diff expected/$file.out <(src/src/$file);
done