format PE64 console
entry _start
section '.idata' data readable import
include "win64a.inc"
library kernel32, 'kernel32.dll', \
        msvcrt,   'msvcrt.dll'
import kernel32, ExitProcess, 'ExitProcess'
import msvcrt, printf, 'printf', \
               malloc, 'malloc', \
               free,   'free'

section '.text' code readable executable

