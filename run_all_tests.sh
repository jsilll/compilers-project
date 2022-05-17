if ! [ -f l22 ];
then
    make
fi

pushd tests/

for input_file in *.l22 ; do
    output_file="expected/`basename ${input_file%.l22}.out`"
    if [ -f $output_file ]; 
    then
        ../l22 --target asm $input_file > /dev/null
        asm_file=${input_file%.l22}.asm
        yasm -felf32 $asm_file > /dev/null
        bin_file=${input_file%.l22}.o
        ld -melf_i386 -o $bin_file $asm_file -lrts > /dev/null
        # ./$bin_file
    fi
done

popd