nasm-patcher
================================================================================

Toolkit to patch 32-bit Windows applications using NASM assembler.

### Tools

 - petool - Edit, add, remove, import, and export sections from PE images
 - linker - Compile and "link" assembly patches using NASM

Building
--------------------------------------------------------------------------------

### Requirements

 - make
 - MinGW-w64 gcc

### Instructions

Type 'make' to build the tools on Linux. On Windows you might need to adjust
the _CC_ variable in Makefile to use your native compiling gcc.

Usage
--------------------------------------------------------------------------------

The pattern you should be using is first expanding the image with **petool** to
add new sections with more than enough space for your new code and data to be
written to. Take a note of the virtual address of every new section as that is
required for your assembled code and data to work.

Remember to set the proper flags for your new sections!

Next, open up your source files corresponding to each new section and add the
following line at the beginning:

    [ORG <address>]

where `<address>` is replaced by the address you got from **petool**.

Now you are ready to compile your code. Use **linker** against each source file
and it should say it PATCHed your newly created section with the code that is in
your source file. If **linker** said nothing, your source file did not contain
any compilable instructions.

This doesn't yet do anything interesting than compile your new code into a new
section. The next topic is dedicated to special **linker** "annotations" that
allow you to hook the original program to your code.

While this documentation is quite exhaustive regarding the tools, knowing the
NASM assembler is very essential as it has powerful macro expansion and many
other useful features like C-style structure definitions.

Get creative! Virtual sections can be "mapped" with structures so you will get
named global variables. Use data section to store constant strings and blobs
instead of polluting your code section with them. Align your functions with the
ALIGN keyword in NASM.

Linker annotations
--------------------------------------------------------------------------------

While all of the tools are straightforward to use, the linker program has a
special annotation syntax that allows you to add small modification to the
target executable you are linking your assembly against.

These annotations are essential for your code to be executed as with them you
can write JMPs and CALLs directly over the original code of the executable.

All annotations start with the @ symbol and take one or more arguments separated
by spaces. Required arguments are in `<>` and optional in `[]`.

Keywords _from_, _to_ and _address_ are absolute memory addresses. Global labels
in your own assembly code will be replaced by their absolute address while
linking which allows you to use them usually as the _to_ address of many
annotations.

### Clearing

    @CLEAR <from> <byte> <to>

Clearing is extremely important if you want to keep your target program code
tidy. `<byte>` is an 8-bit number being the byte that is written. `<to>` is
not inclusive.

When you write a JMP or anything else over the original code that would leave
some instructions broken or a dead code block, consider CLEARing the area before
writing the JMP. It ensures when you or someone else is following the code in
a disassembler or a debugger that they will not get confused by sudden far
jumps which have broken instructions just after them.

Example: `@CLEAR 0x410000 0x90 0x410005 ; NOP 5 bytes starting from 0x410000`

### Jumps

    @JMP <from> <to>

Jumps are your best friend. You can jump from the middle of any function to your
new section to do custom processing. A jump overwrites the instructions at
`<from>` with either 5 bytes far jump or 2 byte near jump depending how far
apart it is from `<to>`.

As you probably know, when you jump out, you also need to jump back when you have
finished with your modifications. Remember to end your label with an absolute
JMP instruction back to original code, NASM will properly calculate the offset
for you.

Example: `@JMP 0x410000 doMagic ; Do a (far) jump from 0x410000 to label doMagic`

### Calls

    @CALL <address> <to>

Sometimes you need to replace a single function call instead of the called
function itself (then you would write a JMP at the beginning of it).

The CALL annotation writes a 5 byte CALL at `<address>` to `<to>`.

Example: `@CALL 0x410000 doMagic ; Make a call from 0x410000 to label doMagic`

### Replacing constants

    @SETB <address> [<byte1> [<byte2> ...]]  
    @SETW <address> [<word1> [<word2> ...]]  
    @SETD <address> [<dword1> [<dword2> ...]]  
    @SETQ <address> [<qword1> [<qword2> ...]]  
    @SETF <address> [<float1> [<float2> ...]]  
    @SETDF <address> [<double1> [<double2> ...]]  

The target executable might have some constants that you may want to change.
Maybe a PUSH instruction has the wrong value or some local variable has a bad
initialization. You're here to fix bugs, right?

Set family of annotations write the number of arguments amount of raw data at
the target `<address>`. The number of arguments is not limited. Please keep in
mind that the type of the command determines the size of the write.

Example: `@SETD 0x410000 1 2 3 ; Write three DWORDs to 0x410000 (12 bytes)`  
Example: `@SETF 0x410000 1.234 ; Write a float to 0x410000`

SETB also accepts character literals enclosed in single quotes which helps
writing over internal strings.

Example: `@SETB 0x410000 'H' 'e' 'l' 'l' 'o' 0x00 ; Note the \0 delimiter`
