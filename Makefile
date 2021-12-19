BINFLDR:= bin
SRCFLDR:= code
LOGFLDR:= logs
OBJFLDR:= inter

SRC:=     quadratic

CODE:=   $(SRCFLDR)/$(SRC).blr
TREE:=   $(OBJFLDR)/$(SRC).tree
ASM:=    $(OBJFLDR)/$(SRC).a
TARGET:= $(OBJFLDR)/$(SRC).exec

all: frontend backend asm cpu
	
compile: frontend backend asm

c-compile: backend asm

run: cpu

frontend:
	$(BINFLDR)/frontend.exe --src $(CODE) --dst $(TREE)
backend:
	$(BINFLDR)/backend.exe --src $(TREE) --dst $(ASM)
asm:
	$(BINFLDR)/asm.exe --src $(ASM) --dst $(TARGET)
cpu:
	$(BINFLDR)/cpu.exe --src $(TARGET)

t_cpu:
	$(BINFLDR)/cpu_test.exe --src $(TARGET)

clean:
	rm -rf $(OBJFLDR)/*.* $(LOGFLDR)/*.* $(LOGFLDR)/tree_dump/*.*