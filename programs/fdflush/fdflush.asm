; Fixed disk flush - fdflush.com

org 0x100

mov ah, 0x0D   ; Reset disk (Flushes all file buffers)
int 0x21 

ret            ; return to DOS
