BIN:= ../bin

SRCFLDR  := code
LOGFLDR  := logs
OBJFLDR  := inter
DESTFLDR := out

PERF	:= perf stat -g
INPUT   := # < $(SRCFLDR)/input.txt

#------------------------------------------------------------------------------
ELF_SRC 	:= elf_factorial
ELF_LIB 	:= lib.cpp

ELF_CODE 	:= $(SRCFLDR)/$(ELF_SRC).blr

ELF_TREE 	:= $(OBJFLDR)/$(ELF_SRC).tree
ELF_OBJECT	:= $(OBJFLDR)/$(ELF_SRC).o
ELF_TARGET	:= $(DESTFLDR)/$(ELF_SRC)

#------------------------------------------------------------------------------
SRC			:= factorial

CODE 		:= $(SRCFLDR)/$(SRC).blr

TREE		:= $(OBJFLDR)/$(SRC).tree
ASM			:= $(OBJFLDR)/$(SRC).a
TARGET		:= $(DESTFLDR)/$(SRC).exec
TRANSP		:= $(SRCFLDR)/$(SRC)_T.blr

#------------------------------------------------------------------------------
compile_elf: elf_frontend elf_backend gcc

compile: frontend backend asm

#------------------------------------------------------------------------------
run:
	$(PERF) $(ELF_TARGET) $(INPUT)

elf_frontend: | $(OBJFLDR) $(LOGFLDR)
	$(PERF) $(BIN)/frontend --src $(ELF_CODE) --dst $(ELF_TREE)

elf_backend: | $(OBJFLDR) $(LOGFLDR)
	$(PERF) $(BIN)/elf_backend --src $(ELF_TREE) --dst $(ELF_OBJECT)

gcc: | $(OBJFLDR) $(DESTFLDR)
	gcc -c $(SRCFLDR)/$(ELF_LIB) -o $(OBJFLDR)/$(ELF_LIB:.cpp=.o)
	gcc $(OBJFLDR)/$(ELF_LIB:.cpp=.o) $(ELF_OBJECT) -o $(ELF_TARGET)

#------------------------------------------------------------------------------
cpu: | $(OBJFLDR) $(LOGFLDR)
	$(PERF) $(BIN)/cpu --src $(TARGET) $(INPUT)

frontend: | $(OBJFLDR) $(LOGFLDR)
	$(PERF) $(BIN)/frontend --src $(CODE) --dst $(TREE)

backend: | $(DESTFLDR) $(OBJFLDR) $(LOGFLDR)
	$(PERF) $(BIN)/backend --src $(TREE) --dst $(ASM)

asm: | $(OBJFLDR) $(LOGFLDR)
	$(BIN)/assembler --src $(ASM) --dst $(TARGET)

transp: | $(OBJFLDR) $(LOGFLDR)
	$(PERF) $(BIN)/transpiler --src $(TREE) --dst $(TRANSP)

detransp: | $(OBJFLDR) $(LOGFLDR)
	$(BIN)/frontend --src $(TRANSP) --dst $(TREE)

clean:
	rm -rf $(OBJFLDR) $(LOGFLDR) $(DESTFLDR)

#------------------------------------------------------------------------------
$(LOGFLDR):
	mkdir $@
$(OBJFLDR):
	mkdir $@
$(DESTFLDR):
	mkdir $@

.PHONY: compile_elf compile frontend backend elf_backend gcc asm cpu transp detransp clean
