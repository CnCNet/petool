petool
================================================================================

Tool to help rebuild and patch 32-bit Windows applications.

### Tools

 - `dump`   - dump information about section of executable
 - `genlds` - generate GNU ld script for re-linking executable
 - `pe2obj` - convert PE executable into win32 object file
 - `patch`  - apply a patch set from the .patch section
 - `setdd`  - set any DataDirectory in PE header
 - `setvs`  - set VirtualSize for a section
 - `export` - export section data as raw binary
 - `import` - dump the import table as assembly
 - `re2obj` - convert the resource section into COFF object
 - `genmak` - generate project Makefile

### Note on GNU binutils

You need `GNU binutils` *2.25.51-20150607* or at least commit `9ac47a4` to
successfully do everything without assertion errors. Fixes are expected to
land in *2.26* when it is released.

All stable releases before 2.26 suffer from missing absolute reloc support
for COFF object files which means you can't jump to or call absolute memory
addresses in `GNU as` assembly and it includes inline assembly with `gcc`.

You can workaround this by using a different assembler with ELF output format 
(e.g. `nasm -f elf`).

The 2.25 stable release suffers from a regression that makes relinking resource
sections impossible if the original binary with the `.rsrc` section is included
in the linker script.

You can workaround this by removing the resource section from the original
binary before using it in the process.

Building
--------------------------------------------------------------------------------

### Build Requirements

 - GNU make
 - GNU cc

### Compiling

Type 'make' to build the tools on Linux. On Windows you might need to adjust
some environment variables to get a successful build. The developers test with
GCC on Linux and Windows (via MinGW), but the code aims to be strictly
C99-compliant so any C99-supporting platform should suffice.

`petool` uses fixed-with numeric types wherever possible so building on both 64-
and 32-bit architectures is supported. Note however that the code currently
supports working with 32-bit portable executable.

Setting up
--------------------------------------------------------------------------------

Generate a ld script with `genlds` and make manual modifications as needed.
`petool` tries its best to create a re-linkable layout for any given executable.
For technical reasons, embedded .bss inside .data is not supported but is
instead unwound to separate `.bss` after `.data`. You can optionally use `setdd`
command to expand `.data` to its original size.

The default linker script uses existing `.idata` section if such is in the
original executable, otherwise it adds it as a new section and expects imports
to be rebuilt from the assembly generated by `import` command. You can
optionally use `setdd` to use the original import section and skip rebuilding by
removing it from the linker script.

You should be able to re-link the executable now by feeding the generated object
file and linker script to ld.

Finally, generate a Makefile with `genmak`. It should work as-is with your
linker script and be able to rebuild the executable without any modifications
including resources from the `.rsrc` section. This default Makefile assumes you
don't want to change the imports so it sets the old import table address back.

Hooking and patching
--------------------------------------------------------------------------------

Generating the patch set can be done with `GNU as` for example. There are helper
macros provided in `patch.s` that generate the patch section automatically for
you. If you prefer another assembler you can port the macros.

Below are examples how these macros are used in practice with `GNU as`.

### Jump

    memljmp <from> <to>

Jumps are your best friend. You can jump from the middle of any function to your
new code to do custom processing. A jump overwrites the instructions at
_from_ with 5 byte far jump that is converted to a relative jump to _to_.

As you probably know, when you jump out, you also need to jump back when you have
finished with your modifications. Remember to end your label with an absolute
jump instruction back to original code.

Example: `memljmp 0x410000 doMagic /* Do a (far) jump from 0x410000 to label doMagic */`

### Call

    memcall <from> <to>

Sometimes you need to replace a single function call instead of the called
function itself (then you would write a jump beginning of it).

The memcall macro writes a 5 byte CALL instruction at _from_ to _to_.

Example: `memcall 0x410000 doMagic /* Make a call from 0x410000 to label doMagic */`

### Clear

    memset <from> <byte> <to>

Clearing is extremely important if you want to keep your target program code
tidy. _byte_ is an 8-bit number being the byte that is written. _to_ is
not inclusive.

When you make a `memjmp` or anything else over the original code that would leave
some instructions broken or a dead code block, consider clearing the area before
writing the jump. It ensures when you or someone else is following the code in
a disassembler or a debugger that they will not get confused by sudden far
jumps which have broken instructions just after them.

Example: `memset 0x410000 0x90 0x410005 /* NOP 5 bytes starting from 0x410000 */`

### Data

    memcpy <address> "<inst>"

The target executable might have some constants that you may want to change.
Maybe a PUSH instruction has the wrong value or some local variable has a bad
initialization.

`memcpy` macro will write the assembled instructions directly to the specified
_address_ in the executable. You can use labels to local symbols as well.

Example: `memcpy 0x410000 ".long 1; .long 2; .long 3" /* Write three DWORDs to 0x410000 (12 bytes) */`

### Existing symbols in original executable

    .global _printf
    .equ _printf, 0x4B8EE0

When you need to refer to existing symbols inside the executable, you can export
global symbols from assembly source. Symbols can be any named memory address:
function, data, uninitialized variable. As long as you define them global, you can
use them anywhere. Remember C decorations if you're referring to such symbols.

Compiling new code
--------------------------------------------------------------------------------

You can use any compilable language that can produce an COFF or ELF object or
whatever your build of `GNU binutils` supports. `GNU as` and `GNU cc` are the
most compatible tools.

What's important is that whatever external symbols you may need can be exported
for the linker somehow and if you use any external dlls you re-create the
import table.

When mixing already compiled executable with new code, you need to make sure of
the calling convention of your functions and compiler can be made compatible
with everything else.

Patch section
--------------------------------------------------------------------------------

The `patch` command reads a PE image section for patch data. The format of the
patch data is as follows:

    <dword absolute address> <dword patch length> <patch data>

These binary blobs are right after each other to form a full patch set. The
patch command simply looks up the file offset in the PE image file based on
given absolute memory address and writes over the blob at that point.

After the patch is applied, you should remove the patch section with GNU strip
as it is not needed in the final product.

Rebuild
--------------------------------------------------------------------------------

With these tools you can add new sections to executable by "rebuilding" it using
GNU ld and any compiler that generates object files that GNU ld successfully can
read.

In theory, additional sections can be created any way one wants, even from C or
C++ with gcc. It's up to the programmer's cleverness to set up the sections
properly with linker script and proper patches - however they are generated.
