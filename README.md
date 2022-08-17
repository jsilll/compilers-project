Compilers Project 2021-2022
===
This compiler is aimed at the [L22 language](https://web.tecnico.ulisboa.pt/~david.matos/w/pt/index.php/Compiladores/Projecto_de_Compiladores/Projecto_2021-2022/Manual_de_Refer%C3%AAncia_da_Linguagem_L22).

Building Instructions
===

- Install [CDK](https://web.tecnico.ulisboa.pt/~david.matos/w/pt/index.php/Compiladores/Projecto_de_Compiladores/Material_de_Apoio_ao_Desenvolvimento)
```
cd cdk/
make install
```
- Install [RTS](https://web.tecnico.ulisboa.pt/~david.matos/w/pt/index.php/Compiladores/Projecto_de_Compiladores/Material_de_Apoio_ao_Desenvolvimento)
```
cd rts/
make install
```
- Build L22 Compiler
```
cd compilers-project/
make
```
Compiling a Single File
```
cd compilers-project/tests/
make <file_name>.exe
```

Running Tests
===
```
cd compilers-project/tests/
./test.sh
```
