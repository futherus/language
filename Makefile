export CXX      ?= gcc
export CXXFLAGS ?=  -O2 -mavx -mavx2 -g -std=c++14 -fmax-errors=100 -Wall -Wextra  	    \
				-Weffc++ -Waggressive-loop-optimizations -Wc++0x-compat 	   					\
				-Wc++11-compat -Wc++14-compat -Wcast-align -Wcast-qual 	   					\
				-Wchar-subscripts -Wconditionally-supported -Wconversion        				\
				-Wctor-dtor-privacy -Wempty-body -Wfloat-equal 		   						\
				-Wformat-nonliteral -Wformat-security -Wformat-signedness       				\
				-Wformat=2 -Winline -Wlarger-than=8192 -Wlogical-op 	           				\
				-Wmissing-declarations -Wnon-virtual-dtor -Wopenmp-simd 	   					\
				-Woverloaded-virtual -Wpacked -Wpointer-arith -Wredundant-decls 				\
				-Wshadow -Wsign-conversion -Wsign-promo -Wstack-usage=8192      				\
				-Wstrict-null-sentinel -Wstrict-overflow=2 			   						\
				-Wsuggest-attribute=noreturn -Wsuggest-final-methods 	   					\
				-Wsuggest-final-types -Wsuggest-override -Wswitch-default 	   				\
				-Wswitch-enum -Wsync-nand -Wundef -Wunreachable-code -Wunused   				\
				-Wuseless-cast -Wvariadic-macros -Wno-literal-suffix 	   					\
				-Wno-missing-field-initializers -Wno-narrowing 	           					\
				-Wno-old-style-cast -Wno-varargs -fcheck-new 		   						\
				-fsized-deallocation -fstack-check -fstack-protector            				\
				-fstrict-overflow -flto-odr-type-merging 	   		   						\
				-fno-omit-frame-pointer                                         				\
				-fsanitize=address 	                                           				\
				-fsanitize=alignment                                            				\
				-fsanitize=bool                                                 				\
				-fsanitize=bounds                                               				\
				-fsanitize=enum                                                 				\
				-fsanitize=float-cast-overflow 	                           					\
				-fsanitize=float-divide-by-zero 			           							\
				-fsanitize=integer-divide-by-zero                               				\
				-fsanitize=leak 	                                           					\
				-fsanitize=nonnull-attribute                                    				\
				-fsanitize=null 	                                           					\
				-fsanitize=object-size                                          				\
				-fsanitize=return 		                                   					\
				-fsanitize=returns-nonnull-attribute                            				\
				-fsanitize=shift                                                				\
				-fsanitize=signed-integer-overflow                              				\
				-fsanitize=undefined                                            				\
				-fsanitize=unreachable                                          				\
				-fsanitize=vla-bound                                            				\
				-fsanitize=vptr                                                 				\
				-fPIE                                                           				\
				-lm -pie 					 

# not overwrite DESTDIR if recursive
export DESTDIR 	  	?= $(CURDIR)/bin
BACKEND_TARGET 		:= $(DESTDIR)/backend
ELF_BACKEND_TARGET 	:= $(DESTDIR)/elf_backend
FRONTEND_TARGET   	:= $(DESTDIR)/frontend
TRANSPILER_TARGET 	:= $(DESTDIR)/transpiler

export OBJDIR 	 := $(CURDIR)/obj
BACKEND_OBJ      := backend.o common.o Token.o Tree.o logs.o
ELF_BACKEND_OBJ  := elf_backend.o common.o Token.o Tree.o logs.o
FRONTEND_OBJ     := frontend.o common.o Token.o Tree.o logs.o
TRANSPILER_OBJ   := transpiler.o common.o Token.o Tree.o logs.o

#------------------------------------------------------------------------------
all: backend elf_backend frontend transpiler cpu assembler

elf_backend: common tree token logs | $(OBJDIR) $(DESTDIR)
	@ cd src/elf_backend && $(MAKE)
	@ echo ======== Linking $(notdir $@) ========
	@ $(CXX) $(addprefix $(OBJDIR)/, $(ELF_BACKEND_OBJ)) -o $(ELF_BACKEND_TARGET) $(CXXFLAGS)

backend: common tree token logs | $(OBJDIR) $(DESTDIR)
	@ cd src/backend && $(MAKE)
	@ echo ======== Linking $(notdir $@) ========
	@ $(CXX) $(addprefix $(OBJDIR)/, $(BACKEND_OBJ)) -o $(BACKEND_TARGET) $(CXXFLAGS)

frontend: common tree token logs | $(OBJDIR) $(DESTDIR)
	@ cd src/frontend && $(MAKE)
	@ echo ======== Linking $(notdir $@) ========
	@ $(CXX) $(addprefix $(OBJDIR)/, $(FRONTEND_OBJ)) -o $(FRONTEND_TARGET) $(CXXFLAGS)

transpiler: common tree token logs | $(OBJDIR) $(DESTDIR)
	@ cd src/transpiler && $(MAKE)
	@ echo ======== Linking $(notdir $@) ========
	@ $(CXX) $(addprefix $(OBJDIR)/, $(TRANSPILER_OBJ)) -o $(TRANSPILER_TARGET) $(CXXFLAGS)

cpu: | $(OBJDIR) $(DESTDIR)
	@ cd include/Processor && $(MAKE) cpu

assembler: | $(OBJDIR) $(DESTDIR)
	@ cd include/Processor && $(MAKE) assembler
	
clean:
	@ cd include/Processor && $(MAKE) clean
	rm -rf $(OBJDIR)

distclean:
	@ cd include/Processor && $(MAKE) distclean
	@ cd tests && $(MAKE) clean
	rm -rf $(OBJDIR) $(DESTDIR)

#------------------------------------------------------------------------------
common: | $(OBJDIR)
	@ cd src/common && $(MAKE)

token: | $(OBJDIR)
	@ cd src/token && $(MAKE)

tree: | $(OBJDIR)
	@ cd src/tree && $(MAKE)

logs: | $(OBJDIR)
	@ cd include/logs && $(MAKE)
	
$(OBJDIR):
	mkdir $(OBJDIR)

$(DESTDIR):
	mkdir $(DESTDIR)

.PHONY: elf_backend backend frontend transpiler cpu assembler clean distclean common token tree logs
