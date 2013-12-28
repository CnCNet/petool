; Suggested macros for petools with nasm
; see README.md for a general introduction

; Switch to the section used to store patches.  Only macros begining with "@"
; should be used here. Likewise such macros should not be used in normal code or
; data sections.
section .patch

; @CLEAR <start address> <repeated byte> <end address>
%macro @CLEAR 3
    dd %1                 ; patch address
    dd %3 - %1            ; length of the patch
    times (%3 - %1) db %2 ; repeated bytes
%endmacro

; @JMP <address of jump> <destination of jump>
; FIXME: do both near and far jumps
%macro @JMP 2
    dd %1                 ; patch address
    dd 5                  ; length of the patch
    db 0xE9               ; far jump
    dd %2 - %1 - 5        ; relative offset
%endmacro

; @CALL <address of call> <address of callee>
%macro @CALL 2
    dd %1                 ; patch addrss
    dd 5                  ; length of the patch
    db 0xE8               ; far call
    dd %2 - %1 - 5        ; relative offset
%endmacro


%comment
; an example:

section .pdata
    msg: db "Hello World",0

section .ptext
    some_func:
        mov eax, msg
        retn

    some_func2:
        retn

section .patch
    ; ld will resolve patch symbols
    @JMP 0x401000, some_func
    @CALL 0x402000, some_func2

%endcomment