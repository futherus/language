# Bellang

Compiler for a Bellang (stays for "Belarusian language"), which was written as a part of study assignment. Supports cross compilation with few other [languages](#cross-compilation).

## Contents

* [Dependencies](#dependencies)
    - [ELF object files](#elf-object-files)
    - [Custom bytecode](#custom-bytecode)
    - [Logs](#logs)
* [Introduction](#introduction)
* [Cross compilation](#cross-compilation)
* [Build](#build)
* [Usage](#usage)
* [Language constructs and syntax](#language-constructs-and-syntax)
    - [Types](#types)
    - [Comments](#comments)
    - [Punctuators and operators](#punctuators-and-operators)
    - [Functions](#functions)
    - [Declaring directives (ELF only)](#declaring-directives-(elf-only))
    - [Embedded functions (Processor only)](#embedded-functions-(processor-only))
    - [Global variables](#global-variables)
    - [Local variables](#local-variables)
    - [Arrays](#arrays)
    - [Loop](#loop)
    - [Conditional](#conditional)
* [Code example](#code-example)
    - [Processor variant](#processor-variant)
    - [ELF variant](#elf-variant)
* [Grammar](#grammar)
* [ELF technical details](#elf-technical-details)
    - [ELF sections](#elf-sections)
    - [ELF header (Ehdr)](#elf-header-(ehdr))
    - [Section header (Shdr)](#section-header-(shdr))
    - [Program header (Phdr)](#program-header-(phdr))
    - [Shstrtab](#shstrtab)
    - [Text](#text)
    - [Data](#data)
    - [Symtab](#symtab)
    - [Strtab](#strtab)
    - [Relatab](#relatab)
    - [Further reading](#further-reading)
* [Instruction encoding](#instruction-encoding)

## Dependencies

### *ELF object files*

Compiling object files does not have any dependencies.

### *Custom bytecode*

This project uses [**Processor**](https://github.com/futherus/Processor) for executing programs compiled to bytecode.

*Processor* is a virtual machine which executes custom byte code. Byte code is compiled using *Assembler*, which is a part of **Processor** project.

### *Logs*

Logs contain graphical dumps of AST, which are created using [**graphviz**](https://graphviz.org/).

> Logs are not enabled by default, see `src/config.h` for details.

## Introduction

Project consists of 4 units:

* Frontend compiles source code to Abstract Syntax Tree (AST).

* Transpiler decompiles AST to source code.

* Backend compiles AST to the custom assembly code.

* Backend for ELF compiles AST to ELF object files.

> Custom assembly code is compiled using *Assembler* and executed using *Processor* (see [Custom bytecode](#custom-bytecode)).

Language uses custom standard for AST for [cross compilation](#cross-compilation).

## Cross compilation

Compiler supports cross compilation, which is based on common AST format.

> AST standard described in `tree_standard.md`.

Other languages that support cross compilation:

* [Assert language by d3phys](https://github.com/d3phys/assert-lang).
* [Joke language by k-kashapov](https://github.com/k-kashapov/lang).
* [Lukashenko language by kefirRzevo](https://github.com/kefirRzevo/Language).

## Build

Project should be built from source.

```
git clone https://github.com/futherus/Language.git
cd Language
make
```

All binaries will be placed in `Language/bin` folder.

To clean project tree after building run:
```
make clean
```

To remove all created files run:
```
make distclean
```

## Usage

Project contains 6 separate binaries with similar format of commandline arguments:

```
<path/to/binary> --src <path/to/input> --dst <path/to/output>
```

E.g.
```
./bin/frontend --src factorial.blr --dst factorial.tree
```

The only exception is `cpu` binary. It does not accept `--dst` option.

To run test compilation conveniently use examples from `Language/tests` folder.

```
# compile and run program for Processor
make compile
make cpu
```

```
# compile and run ELF program
make compile_elf
make run
```

```
# remove all created files
make clean
```

To compile other examples feel free to change `Makefile` in `tests` folder.

## Language constructs and syntax

Bellang constructively appears to be an oversimplified C language.
However, used keywords and few approaches are different.

### *Types*

Only `double` type is used in Bellang.

> :warning: **Warning**
>
> ELF compiler `double` support is not implemented yet, it uses `uint64_t` instead.


### *Comments*

Bellang supports comments with following syntax:
```
> this is comment <

>
    And this is also a comment.
<
```

### *Punctuators and operators*

`\\\\`, `////`

* Delimit function, conditional and loop body.

`нарэшце`

* Indicate the end of a statement.

`[]`

* Assignment subscription.

`зрушаны_на`

* Expression subscription.

`()`

* In a expression, indicate grouping.
* Function call operator.
* In a function definition, delimit the parameter list

`,`

* List separator in
    * function definition.
    * function call.

`не`

* Logical not operator.

`дадаць`

* Unary plus operator.
* Binary plus operator.

`за_недахопам`

* Unary minus operator.
* Binary minus operator.

`памножаны_на`

* Multiplication operator.

`падзелены_на`

* Division operator.

`апыняецца`

* Assignment operator.

`роўны_з`

* Equality operator.

`розьніцца_з`

* Inequality operator.

`большы_за`

* Greater-than operator

`драбнейшы_за`

* Less-than operator

`драбнейшы_ці_роўны_з`

* Less-than-or-equal-to operator.

`большы_ці_роўны_з`

* Greater-than-or-equal-to operator.

`і`

* Logical and operator.

`ці`

* Logical or operator.

`ў`

* Ariphmetic power operator.

### *Functions*

There is no function declaration (except [*declaring directives*](#declaring-directives)).
Functions are seen globally in every part of a program, even before definition.
Every function must have 'return' statement.
In Bellang `вышпурнуць` stays for `return`

Function definition has following form:

```
function-id function-parameter-list function-body
```
e.g.
```
сумма(параметр1, параметр2)
\\\\
    вышпурнуць параметр1 дадаць параметр2 нарэшце
////
```

Every program needs to have 'main' function.
This function does not accept parameters.
It cannot be called in program.
In Bellang it's name is `пачатак`.

```
пачатак()
\\\\
    вышпурнуць 0 нарэшце
////
```

### *Declaring directives (ELF only)*

When compiling to ELF object file you can declare external function, which will be linked later.

External functions can have arguments of `double` type and can return `double` value.
Compiler creates unmangled function name for C linkage.

Example:
```
@ foo(param1, param2) @
```

> :warning: **Warning**
>
> ELF compiler `double` support is not implemented yet, it uses `uint64_t` instead.

### *Embedded functions (Processor only)*

*Processor* does not support external functions. It has embedded functions instead.

They are called like normal functions, but they do not require to be defined.

```
пачатак()
\\\\
    адказаць(сінус(запытаць())) нарэшце
    вышпурнуць 0 нарэшце
////
```

In this example three embedded functions are called:
* `запытаць` gets input from stdin,
* `сінус` evaluates sine of input,
* `адказаць` writes the result to stdout.

List of embedded functions:

`сінус(arg)`

* Returns sine of it's argument.

`косінус(arg)`

* Returns cosine of it's argument.

`адказаць(arg)`

* Writes it's argument to stdout as a number. Returns 0.

`запытаць()`

* Gets number from stdin and returns it.

`надрукаваць(arr, arr_size)`

* Interprets array `arr` of size `arr_size` as square matrix of boolean values and prints it to stdout.
Returns 0.

`абсекчы(arg)`

* Returns integer part of a number.

### *Global variables*

Bellang supports global variables, but there are some differences with C.

Global variables:
* declared by first assignment in a file.
* cannot be reassigned in file scope.
* have global visibility.
* can be used in functions above variable declaration.
* can be initialized with number only.
* can be declared as constant using `непахісны` keyword.
* can be an array (see [*Arrays*](#arrays)).

E.g.

```
...

> GLOBAL_ARR wasn't declared before <

непахісны GLOBAL_ARR[19] апыняецца 42 нарэшце

...
```
will result in constructing array `GLOBAL_ARR` with 20 elements and assignment 42 to 19th element.

Example:
```
INDEX апыняецца 0 нарэшце

пачатак()
\\\\
    GLOBAL_ARR[5] апыняецца 1234 нарэшце
    INDEX апыняецца 3 нарэшце
    адказаць(GLOBAL_VAR зрушаны_на (INDEX дадаць 2)) нарэшце > write 1234 to stdout <

    адказаць(GLOBAL_VAR зрушаны_на 15) нарэшце               > write 42 to stdout   <

    вышпурнуць 0 нарэшце
////

> array of 16 elements, declared after it is used <
GLOBAL_ARR[15] апыняецца 42 нарэшце
```

### *Local variables*

Bellang supports local variables, but there are some differences with C.

Global variables:
* declared by first assignment in a function scope.
* appear in scope only after first assignment.
* can be declared as constant using `непахісны` keyword.
* can be an array (see [*Arrays*](#arrays)).

Example:
```
пачатак()
\\\\
    local_a апыняецца 27 дадаць 15 нарэшце
    local_b[15] апыняецца 5 памножаны_на local_a нарэшце

    вышпурнуць foo() нарэщце
////

foo()
\\\\
    local_b апыняецца 3 нарэшце

    вышпурнуць local_b памножаны на 5 нарэшце
////
```

### *Arrays*

Bellang supports arrays, however it handles them not like C.

Arrays are declared on first assignment, as any variable. To specify that variable is an array use brackets `[]` with number of elements in them (e.g. `array[20]`).

The statement
```
...
> arr wasn't declared before <

arr[17] = 42 нарэшце

...
```
will result in constructing array of size **18** and assigning 42 to it's 17 element.

If `arr` was declared before, assignment occurs to 17 element of already constructed array. Compiler does not provide bounds-checking and will not expand existing array in case of out-of-bounds assignment.

### *Loop*

There is only one type of loops -- 'while' loop.
In Bellang `пакуль` stays for `while`.

Example:

```
...

iter апыняецца 0

пакуль(iter драбнейшы_за 10)
\\\\
    iter апыняецца iter дадаць 1 нарэшце
////

...
```

### *Conditional*

There is only one type of conditional -- 'if' statement.
In C if statement consists of 'if' branch and optional 'else' branch.
Bellang uses the same form, with `калі` staying for `if` and `інакш` staying for `else`.

Example:

```
...
> var was declared before <

калі(var драбнейшы_за 10)
\\\\
    var апыняецца var дадаць 1 нарэшце
////
інакш
\\\\
    var апыняецца 42 нарэшце
////

калі(var розьніцца_з 42)
\\\\
    var апыняецца var памножаны_на 2
////
...
```

## Code example

This is examples of program that gets number from standard input and prints it's factorial to standard output.

#### Processor variant

```
пачатак()
\\\\
    адказаць(фактарыял(запытаць())) нарэшце

    вышпурнуць 0 нарэшце
////

фактарыял(лічба)
\\\\
    калі(лічба драбнейшы_ці_роўны_з 1)
    \\\\
        вышпурнуць 1 нарэшце
    ////

    вышпурнуць лічба памножаны_на фактарыял(лічба за_недахопам 1) нарэшце
////
```

#### ELF variant

```
> External functions to print number and get number <
@ putnum(val) @
@ getnum() @

пачатак()
\\\\
    putnum(factorial(getnum())) нарэшце
    вышпурнуць 0 нарэшце
////

factorial (лічба)
\\\\
    калі(лічба драбнейшы_ці_роўны_з 1)
    \\\\
        вышпурнуць 1 нарэшце
    ////

    вышпурнуць лічба памножаны_на factorial(лічба за_недахопам 1) нарэшце
////
```

## Grammar

Frontend uses recursive descent parser.

> Grammar with Bellang syntax can be found in `src/frontend/parser.cpp`.

```
Primary ::=
    '!' Primary           |
    '+' Primary           |
    '-' Primary           |
    '(' Expression ')'    |
    TYPE_NUMBER           |
    TYPE_ID               | // Variable     
    TYPE_ID '>>>' Primary | // Array access 
    TYPE_ID  '(' {Expression {',' Expression}*}? ')' | // Function call
    TYPE_EMB '(' {Expression {',' Expression}?}? ')'   // Embedded-function call

Power      ::= Primary {'^' Primary}?
Term       ::= Power {['/' '*'] Power}*
Ariphmetic ::= Term {['+' '-'] Term}*
Boolean    ::= Ariphmetic {['==' '>' '<' '<=' '>=', '!='] Ariphmetic}*
Expression ::= Boolean {['||' '&&'] Boolean}*
    
Conditional   ::= 'if'    '(' Expression ')' '{' {Statement}+ '}' {'else' '{' {Statement}+ '}'}?
Cycle         ::= 'while' '(' Expression ')' '{' {Statement}+ '}'
Terminational ::= 'return' Expression ';'
Assign        ::= {'const'}? TYPE_ID {'[' Expression ']'}? '=' Expression ';'
    
Statement ::=
    Conditional   |
    Cycle         |
    Terminational |
    Assign        |
    Expression ';'

Define ::= TYPE_ID '(' {TYPE_ID {',' TYPE_ID}*}? ')' '{' {Statement}+ '}'

Directive ::= '@' TYPE_ID '(' {TYPE_ID {',' TYPE_ID}*}? ')' '@'

General ::= {Define | Assign | Directive}+
```

## ELF technical details

### *ELF sections*

Generally, the main idea of ELF format is to divide program into sections, create **section headers** for storing information about sections' position, size, connection between them and create **program headers** for storing information about program loading for execution. However, when creating ELF object file **program headers** are not needed.

### *ELF header (Ehdr)*

The first place where any software that analyses ELF file will look at. Consists of ELF identifier bytes, information about machine, amount and position of section and program headers and so on.

### *Section header (Shdr)*

Section headers are stored as an array of structures at displacement specified in Ehdr.

Section header contains information about section:
* sh_name[4] - offset of string in [shstrtab](#shstrtab), containing name of the section.
* sh_type[4] - section type (e.g. `PROGBITS`, `STRTAB`, ...).
* sh_flags[8] - information about section rights in executable (e.g. **A**llocate, **R**ead, **W**rite, e**X**ecute).
* sh_addr[8] - section address during execution.
* sh_offset[8] - section offset in file.
* sh_size[8] - size of section in bytes.
* sh_link[4] - index of section header of somehow linked section (e.g. index of strtab in symtab section header).
* sh_info[4] - field with information that is interpreted depending on section type.
* sh_addralign[8] - required alignment.
* sh_entsize[8] - if section has element structure this field stores size of these elements (e.g. [symtab](#symtab)).

### *Program header (Phdr)*

Program headers are stored as an array of structures at displacement specified in Ehdr.

> These headers are not used nor created in object file.

Program header contains information about loading parts (segments) of program:
* p_type[4] - segment type (e.g. `LOAD`, `INTERP`, ...)
* p_flags[4] - segment permissions (e.g. **R**ead, **W**rite, **E**xecute).
* p_offset[8] - segment offset in file.
* p_vaddr[8] - virtual address of section.
* p_paddr[8] - physical address of section.
* p_filesz[8] - amount of bytes in segment.
* p_memsz[8] - size of segment in memory (e.g. can be bigger than filesz for bss section).
* p_align[8] - required alignment.

### *Shstrtab*

Section header string table (shstrtab) contains names of sections as null-terminated strings.

### *Text*

Text section contains encoded instructions, jumptables, etc.

### *Data*

Data section contains mutable static structures (e.g. global variables).

### *Symtab*

Symbol table (symtab) contains information about global and external symbols that program uses:
* st_name[4] - offset of string in [strtab](#strtab), containing name of the symbol.
* st_info[1] - type and bind attributes.
* st_other[1] - symbol visibility.
* st_shndx[2] - index of corresponding section header.
* st_value[8] - address or value of symbol (depends on symbol type).
* st_size[8] - corresponding size (depends on symbol type).

> st_info[8:4] - bind attributes (`Local`, `Global`, `Weak`).
> 
> st_info[4:0] - type (object, function, section, file).

> st_other[3:0] - visibility.

### *Strtab*

String table (strtab) contains null-terminated strings which are used as names' storage for other sections (e.g. [symtab](#symtab)).

### *Relatab*

Relocation table (rel**a**tab (**a** goes for explicit addend)) contains relocations that has to be done during static or dynamic linking:
* r_offset[8] - address to apply relocation to.
* r_info[8] - symbol index in [symtab](#symtab) and relocation type.
* r_addend[8] - addend to value which will be inserted.

> r_offset[8] is relative to section beginning.

> When resolving `call` relocation we must set displacement relative to beginning of next instruction (RIP), but r_offset points to the beginning of memory for this displacement. Therefore, we must set `-4` (size of displacement value) addend for every `call` to fix this.

### *Further reading*

* [Step-by-step explanation of ELF format](http://blog.k3170makan.com/p/series.html).
* [Good reference for ELF object files](https://refspecs.linuxbase.org/elf/gabi4+/contents.html).
* [Picture from Wikipedia by Ange Albertini](https://en.wikipedia.org/wiki/Executable_and_Linkable_Format#/media/File:ELF_Executable_and_Linkable_Format_diagram_by_Ange_Albertini.png).

## Instructions encoding

Coming soon...