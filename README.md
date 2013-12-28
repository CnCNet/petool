petool
================================================================================

Tool to help rebuild, extend and patch 32-bit Windows applications.

### Tools

 - `dump`   - dump information about section of executable
 - `genlds` - generate GNU ld script for re-linking executable
 - `pe2obj` - convert PE executable into win32 object file
 - `patch`  - apply a patch set from the .patch section
 - `setdd`  - set any DataDirectory in PE header
 - `setvs`  - set VirtualSize for a section
 - `help`   - this information

Building
--------------------------------------------------------------------------------

### Requirements

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

First convert your PE image into an object file with `pe2obj` command. Also
generate a skeleton ld script with `genlds`.

You should be able to re-link the executable now by feeding the generated object
file and linker script to ld.

Patch
--------------------------------------------------------------------------------

The `patch` command reads a PE image section for patch data. The format of the
patch data is as follows:

    <dword absolute address> <dword patch length> <patch data>

These binary blobs are right after each other to form a full patch set. The
patch command simply looks up the file offset in the PE image file based on
given absolute memory address and writes over the blob at that point.

Generating the patch set can be done with NASM for example. There are helper
macros -- previously called "annotations" -- provided in `usage/nasm.inc` that
generate the patch section automatically for you.

Rebuild
--------------------------------------------------------------------------------

With these tools you can add new sections to executable by "rebuilding" it using
GNU ld and any compiler that generates object files that GNU ld successfully can
read.

In theory, additional sections can be created any way one wants, even from C or
C++ with gcc. It's up to the programmer's cleverness to set up the sections
properly with linker script and proper patches - however they are generated.
