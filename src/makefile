#Makefile asm
EXEC = ../bin/prog

result : $(EXEC).o utils.o
	gcc -o ../bin/result.out $(EXEC).o utils.o -nostartfiles -no-pie

$(EXEC).o : $(EXEC).asm
	nasm -f elf64 -o $(EXEC).o $(EXEC).asm

utils.o : utils.asm
	nasm -f elf64 -o utils.o utils.asm