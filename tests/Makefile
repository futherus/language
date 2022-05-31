# Folder with compiler binaries
BIN:= ../bin

SRCFLDR  := code
LOGFLDR  := logs
OBJFLDR  := inter
DESTFLDR := out

# Uncomment this for using perf profiling
PERF := # perf stat -g
# Variable providing input for executed program
INPUT := # < $(SRCFLDR)/input.txt

#--------------------------------- ELF setup ----------------------------------
# Name of source file without extension
ELF_SRC 	:= elf_factorial

ELF_LIB 	:= lib.cpp

ELF_CODE 	:= $(SRCFLDR)/$(ELF_SRC).blr
ELF_TREE 	:= $(OBJFLDR)/$(ELF_SRC).tree
ELF_OBJECT	:= $(OBJFLDR)/$(ELF_SRC).o
ELF_TARGET	:= $(DESTFLDR)/$(ELF_SRC)

#------------------------------ Processor setup -------------------------------
# Name of source file without extension
SRC			:= factorial

CODE 		:= $(SRCFLDR)/$(SRC).blr
TREE		:= $(OBJFLDR)/$(SRC).tree
ASM			:= $(OBJFLDR)/$(SRC).a
TARGET		:= $(DESTFLDR)/$(SRC).exec
TRANSP		:= $(SRCFLDR)/$(SRC)_T.blr

#------------------------------------------------------------------------------
# Compile ELF executable
elf_compile: elf_frontend elf_backend gcc

# Run ELF executable
elf_run:
	$(PERF) $(ELF_TARGET) $(INPUT)

# Compile source code to AST
elf_frontend: | $(OBJFLDR) $(LOGFLDR)
	$(PERF) $(BIN)/frontend --src $(ELF_CODE) --dst $(ELF_TREE)

# Generate object file from AST
elf_backend: | $(OBJFLDR) $(LOGFLDR)
	$(PERF) $(BIN)/elf_backend --src $(ELF_TREE) --dst $(ELF_OBJECT)

# Compile external functions and link object files to executable
gcc: | $(OBJFLDR) $(DESTFLDR)
	gcc -c $(SRCFLDR)/$(ELF_LIB) -o $(OBJFLDR)/$(ELF_LIB:.cpp=.o)
	gcc $(OBJFLDR)/$(ELF_LIB:.cpp=.o) $(ELF_OBJECT) -o $(ELF_TARGET)

#------------------------------------------------------------------------------
# Compile program as Processor bytecode
compile: frontend backend asm

# Run program on Processor
run: | $(OBJFLDR) $(LOGFLDR)
	$(PERF) $(BIN)/cpu --src $(TARGET) $(INPUT)

# Compile source code to AST
frontend: | $(OBJFLDR) $(LOGFLDR)
	$(PERF) $(BIN)/frontend --src $(CODE) --dst $(TREE)

# Generate Assembly code from AST
backend: | $(DESTFLDR) $(OBJFLDR) $(LOGFLDR)
	$(PERF) $(BIN)/backend --src $(TREE) --dst $(ASM)

# Assemble program to Processor bytecode
asm: | $(OBJFLDR) $(LOGFLDR)
	$(BIN)/assembler --src $(ASM) --dst $(TARGET)

# Transpile AST to source code
transp: | $(OBJFLDR) $(LOGFLDR)
	$(PERF) $(BIN)/transpiler --src $(TREE) --dst $(TRANSP)

# Compile transpiled source code to AST
detransp: | $(OBJFLDR) $(LOGFLDR)
	$(BIN)/frontend --src $(TRANSP) --dst $(TREE)

#------------------------------------------------------------------------------
clean:
	rm -rf $(OBJFLDR) $(LOGFLDR) $(DESTFLDR)

$(LOGFLDR):
	mkdir $@
$(OBJFLDR):
	mkdir $@
$(DESTFLDR):
	mkdir $@

.PHONY: compile_elf compile frontend backend elf_backend gcc asm cpu transp detransp clean
