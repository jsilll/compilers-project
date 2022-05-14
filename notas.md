ficheiro:
- [x] file_node.h

declaration: (nao sei quantas sao precisas? )
- [x] variable_declaration_node.h (só é preciso uma, para a declaracao com tipo e com 'var', porque o tipo
    é inferido em compile time)

programa-principal:
- [x] main_program_node.h 

bloco:
- [x] block_node.h (DUVIDA: quando é que devemos usar este nó nos outros nodes ???)

instrução:
- [x] cdk::expression_node.h (já está implementado na lib)
- [x] writeln_instruction_node.h (TODO: passar a expression para expressions, DUVIDA: fazemos isto já para esta entrega?)
- [x] write_instruction_node.h (TODO: passar a expression para expressions, DUVIDA: fazemos isto já para esta entrega?)
- [x] again_instruction_node.h
- [x] stop_instruction_node.h 
- [x] return_expression_node.h 
- [x] if_node.h (já estava feito)
- [x] while_node.h (já estava feito)
- [x] block_node.h 

expressões:
- [x] cdk::sequence_node.h (já está implementado na lib)

funcão:
- [x] function_declaration_node.h 

tipo-de-funcao:
- type_of_function_node.h (DUVIDA: é preciso definir isto ??)
- function_call.h (DUVIDA: é preciso definir isto ??)

misc:
- DUVIDA: devemos remover o read_node.h ??
- DUVIDA: todas as variaveis cdk::basic_node _block; passar a block_node _block (para isso é preciso meter o parser a reconhecer blocos primeiro)