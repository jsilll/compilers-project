if ! [ -f l22 ];
then
    make
fi


for input_file in tests/*.l22 ; do
    output_file="tests/expected/`basename ${input_file%.l22}.out`"
    if [ -f $output_file ]; 
    then
        ./l22 --target asm $input_file
        asm_file=${input_file%.l22}.asm
        yasm -felf32 $asm_file
        bin_file=${input_file%.l22}
        ld -melf_i386 -o $bin_file $asm_file -lrts
        ./$bin_file
    fi
done