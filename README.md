petool
================================================================================

Tool to help rebuild, extend and patch 32-bit Windows applications.

### Tools

 - dump - show PE image sections
 - genlds - generate GNU ld linker script for recompilation
 - pe2obj - convert PE image into COFF object by stripping the DOS and NT header
 - patch - apply a patch set from patch section

Building
--------------------------------------------------------------------------------

### Requirements

 - GNU make
 - GNU cc

### Compiling

Type 'make' to build the tools on Linux. On Windows you might need to adjust
some environment variables to get a successful build. MingW gcc is expected and
the code should be C99 compliant.

Setting up
--------------------------------------------------------------------------------

First convert your PE image into an object file with **pe2obj** command. Also
generate a skeleton ld script with **genlds**.

You should be able to recompile the executable now by feeding the linker script
to ld.

Patch
--------------------------------------------------------------------------------

The **patch** command reads a PE image section for patch data. The format of the
patch data is as follows:

    <dword absolute address> <dword patch length> <patch data>

These binary blobs are right after each other to form a full patch set. The
patch command simply looks up the file offset in the PE image file based on
given absolute memory address and writes over the blob at that point.

Generating the patch set can be done with NASM for example. There are helper
macros that were previously called "annotations" that generate the patch section
automatically for you.

Rebuild
--------------------------------------------------------------------------------

With these tools you can add new sections to executable by "rebuilding" it using
GNU ld and any compiler that generates object files that GNU ld successfully can
read.

In theory, it's even possible to use gcc to add new sections with C or C++ based
code. It's up to the programmer's cleverness to set up the sections properly
with linker script and proper patches - however they are generated.
