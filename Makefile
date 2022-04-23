BINFLDR:= bin
SRCFLDR:= code
LOGFLDR:= logs
OBJFLDR:= inter

SRC:=     quadratic

CODE:=   $(SRCFLDR)/$(SRC).blr
TREE:=   $(OBJFLDR)/$(SRC).tree
ASM:=    $(OBJFLDR)/$(SRC).a
TARGET:= $(OBJFLDR)/$(SRC).exec
TRANSP:= $(SRCFLDR)/$(SRC)_T.blr


all: frontend backend asm cpu
	
compile: frontend backend asm

run: cpu

t-compile: detransp c-compile

c-compile: backend asm

frontend:
	$(BINFLDR)/frontend.exe --src $(CODE) --dst $(TREE)
backend:
	$(BINFLDR)/backend.exe --src $(TREE) --dst $(ASM)
asm:
	$(BINFLDR)/asm.exe --src $(ASM) --dst $(TARGET)
cpu:
	$(BINFLDR)/cpu.exe --src $(TARGET) < $(SRCFLDR)/input.txt

transp:
	$(BINFLDR)/transpiler.exe --src $(TREE) --dst $(TRANSP)
detransp:
	$(BINFLDR)/frontend.exe --src $(TRANSP) --dst $(TREE)

clean:
	rm -rf $(OBJFLDR)/*.* $(LOGFLDR)/*.* $(LOGFLDR)/tree_dump/*.*