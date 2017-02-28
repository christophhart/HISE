; The MIT License (MIT)

; Copyright (c) 2016, Microsoft

; Permission is hereby granted, free of charge, to any person obtaining a copy
; of this software and associated documentation files (the "Software"), to deal
; in the Software without restriction, including without limitation the rights
; to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
; copies of the Software, and to permit persons to whom the Software is
; furnished to do so, subject to the following conditions:

; The above copyright notice and this permission notice shall be included in
; all copies or substantial portions of the Software.

; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
; IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
; FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
; AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
; LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
; OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
; THE SOFTWARE.

; Description:
; 
; Revision date:

.data
myMessage BYTE "MASM program example",0dh,0ah,0

.code
main PROC

    mov rcx, 16;
    mov rax, 1234567812345678h
    mov rbx, 2108765432108765h
    ; expect 2238767832348778

l0:
    cmp rax, rbx
    jae l1

    ; rbx is greater - shift its high bits into the low bits of rax
    shld rax,rbx, 4
    jmp l2

l1:

    ; rax is greater - rotate its high bits into the low bits of rax
    rol rax, 4
l2:

    ; both cases, shift the high order bits of rbx out of the way
    shl rbx, 4

    ;; decrement the loop counter and go back to the top if there are more fields
    dec rcx
    jnz l0
    ret


shld rax, rbx, 4
;movd xmm1, rax
;vprotq xmm0, xmm1, 4

add [rcx + 5], rax
add rax, [rcx + 5]
movd xmm0, rax
movsd xmm0, xmm1
; vmovq xmm0, rax       // Illegal instruction
ret

; Bit operations
bsf rax, r14
bsr r15, r13
bt eax, ecx
btc bx, dx
btr esi, edi
bts r8d, r12d

; Stosq
rep stosq

; Dec
dec al
dec ax
dec eax
dec rax
dec r12

dec byte ptr [r12 + 1234h]
dec word ptr [r13 + 1234h]
dec dword ptr [r14 + 1234h]
dec qword ptr [r15 + 1234h]

; Inc
inc r9b
inc r10w
inc r11d
inc r12

inc byte ptr [rax + 1234h]
inc word ptr [rbp + 1234h]
inc dword ptr [rsi + 1234h]
inc qword ptr [rdi + 1234h]

; Neg
neg rbx
neg qword ptr [rbp + 56h]

; Not
not bl
not word ptr [rbp + 56h]

; SIB reads
mov rax, [rsi + rcx * 8 + 1234h]
mov r15, [r14 + r13 * 8 + 1234h]
mov al, [rcx + r13 * 8 + 12h]
mov bx, [r15 + rax * 8 + 34h]

and rax, [rsi + rcx * 8 + 1234h]
and r15, [r14 + r13 * 8 + 1234h]
and al, [rcx + r13 * 8 + 12h]
and bx, [r15 + rax * 8 + 34h]

and rax, [rdi + rdx * 1 + 5678h]
and rax, [rdi + rdx * 2 + 5678h]
and rax, [rdi + rdx * 4 + 5678h]
and rax, [rdi + rdx * 8 + 5678h]


; SIB writes
mov [rsi + rcx * 8 + 1234h], rax
mov [r14 + r13 * 8 + 1234h], r15
mov [rcx + r13 * 8 + 12h], al 
mov [r15 + rax * 8 + 34h], bx

and [rsi + rcx * 8 + 1234h], rax
and [r14 + r13 * 8 + 1234h], r15
and [rcx + r13 * 8 + 12h], al
and [r15 + rax * 8 + 34h], bx

and [rdi + rdx * 1 + 5678h], rax
and [rdi + rdx * 2 + 5678h], rax
and [rdi + rdx * 4 + 5678h], rax
and [rdi + rdx * 8 + 5678h], rax


; Another special case
add r13, [r13]
mov r13, [r13]
mov rax, [rbp]
mov rax, [rbp + 12h]
mov rbx, [r13 + 34h]
mov rcx, [rbp + 1234h]

;
; Mod/RM special cases for RSP and R12 and [RBP] ==> [RBP + disp8]
;
sub rbx, [r12]
sub rdi, [r12 + 12h]
sub rbp, [r12 + 1234h]
sub r10, [r12 + 12345678h]

;
; Mod/RM case for R8..R15
;

;
; REX W bit. X bit.
;

;
; Group1 addressing mode permutations for a single opcode.
;

; direct-direct
add al, cl
add bx, dx
add esi, eax
add rax, rbx
add r8, r9
add rsp, r12

; direct-indirect with zero, byte, word, and double word offsets
add cl, byte ptr [rax]
add bl, byte ptr [rcx + 12h]
add r9b, byte ptr [rsi + 100h]
add r15b, byte ptr [rdi + 12345678h]

add dx, word ptr [rdx]
add cx, word ptr [rcx + 12h]
add r9w, word ptr [rsi + 1234h]
add r11w, word ptr [rdi + 12345678h]

add edx, dword ptr [rdx]
add ecx, dword ptr [rcx + 12h]
add r9d, dword ptr [rsi + 1234h]
add r11d, dword ptr [rdi + 12345678h]

add rdx, qword ptr [rdx]
add rcx, qword ptr [rcx + 12h]
add r9, qword ptr [rsi + 1234h]
add r11, qword ptr [rdi + 12345678h]


; indirect-direct with zero, byte, word, and double word offsets
add byte ptr [rax], cl
add byte ptr [rcx + 12h], bl
add byte ptr [rsi + 100h], r9b
add byte ptr [rdi + 12345678h], r15b

add word ptr [rdx], dx
add word ptr [rcx + 12h], cx
add word ptr [rsi + 1234h], r9w
add word ptr [rdi + 12345678h], r11w

add dword ptr [rdx], edx
add dword ptr [rcx + 12h], ecx
add dword ptr [rsi + 1234h], r9d
add dword ptr [rdi + 12345678h], r11d

add qword ptr [rdx], rdx
add qword ptr [rcx + 12h], rcx
add qword ptr [rsi + 1234h], r9
add qword ptr [rdi + 12345678h], r11

; direct-immediate register 0 case
add al, 34h
add ax, 56h
add ax, 5678h
add eax, 12h
add eax, 1234h
add eax, 12345678h
add rax, 12h
add rax, 1234h
add rax, 12345678h

; direct-immediate general purpose register case
add bl, 34h
add r13b, 34h
add cx, 56h
add dx, 5678h
add ebp, 12h
add ebp, 1234h
add ebp, 12345678h
add r12, 12h
add r12, 1234h
add r12, 12345678h

; Direct-immediate, different opcodes depending on
; whether sign extension is acceptable.
;
; The immediates that will be sign extended or will use the
; sign-extend opcode in cases when it makes no difference
; (when both source and target are 1-byte).
;
; The first two lines would correctly fail to compile in NativeJIT
; and are thus commented out. They would produce the value of
; FFFFFFFF80000000h unexpectedly since sign extension is unconditionally
; used for 32-bit immediates targeting 64-bit registers.
;
; add rax, 80000000h
; add rcx, 80000000h
add rax, -7fffffffh
add rcx, -7fffffffh
add cl, -7fh
add cl, 80h
add cx, -7fh
add ecx, -7fh
add rcx, -7fh

; The immediates that will not be sign extended.
add cx, 80h
add ecx, 80h
add rcx, 80h

;
; Verify various flavors of each Group1 opcode.
; These instructions excercise all different flavors
; which use different base opcode and extension. The
; generic Group1 encoding was already verified with
; the add instruction above.
;
and al, 11h
and eax, 11223344h
and dl, 11h
and edx, 11223344h
and edx, 11h
and byte ptr [rbx + 1], dl
and dword ptr [rcx + 4], edx
and dl, byte ptr [rbx + 1]
and edx, dword ptr [rcx + 4]

cmp al, 11h
cmp eax, 11223344h
cmp dl, 11h
cmp edx, 11223344h
cmp edx, 11h
cmp byte ptr [rbx + 1], dl
cmp dword ptr [rcx + 4], edx
cmp dl, byte ptr [rbx + 1]
cmp edx, dword ptr [rcx + 4]

or al, 11h
or eax, 11223344h
or dl, 11h
or edx, 11223344h
or edx, 11h
or byte ptr [rbx + 1], dl
or dword ptr [rcx + 4], edx
or dl, byte ptr [rbx + 1]
or edx, dword ptr [rcx + 4]

sub al, 11h
sub eax, 11223344h
sub dl, 11h
sub edx, 11223344h
sub edx, 11h
sub byte ptr [rbx + 1], dl
sub dword ptr [rcx + 4], edx
sub dl, byte ptr [rbx + 1]
sub edx, dword ptr [rcx + 4]

xor al, 11h
xor eax, 11223344h
xor dl, 11h
xor edx, 11223344h
xor edx, 11h
xor byte ptr [rbx + 1], dl
xor dword ptr [rcx + 4], edx
xor dl, byte ptr [rbx + 1]
xor edx, dword ptr [rcx + 4]

;
; Lea
;
lea rax, [rsi]
lea rax, [rsi + 12h]
lea rax, [rsi - 12h]
lea rax, [rsi + 1234h]
lea rax, [rsi - 1234h]
lea rax, [rsi + 12345678h]
lea rax, [rsi - 12345678h]
lea rbp, [r12]
lea rbp, [r12 + 87h]
lea rbp, [r12 - 789ABCDEh]
lea rbp, [rsp + 20h]
lea rsp, [rbp - 20h]

;
; Mov
;
mov al, cl
mov bx, dx
mov esi, eax
mov rax, rbx
mov r8, r9
mov rsp, r12

mov cl, [rax]
mov bl, [rcx + 12h]
mov r9b, [rsi + 100h]
mov r15b, [rdi + 12345678h]

mov dl, [rdx]
mov cx, [rcx + 12h]
mov r9w, [rsi + 1234h]
mov r11w, [rdi + 12345678h]

mov esp, [r9]
mov edx, [rcx + 12h]
mov esi, [rsi + 1234h]
mov r11d, [rdi + 12345678h]

mov rbx, [r12]
mov rdi, [rcx + 12h]
mov rbp, [rsi + 1234h]
mov r10, [rdi + 12345678h]

; mov r, imm - register 0 case
mov al, 0
mov al, 34h
mov ax, 56h
mov ax, 5678h
mov eax, 12h
mov eax, 1234h
mov eax, 12345678h
mov rax, 12h
mov rax, 1234h
mov rax, 12345678h
mov rax, 80000000h
mov rax, -1

; mov r, imm - general purpose register case
mov bl, 0
mov bl, 34h
mov r13b, 34h
mov cx, 56h
mov dx, 5678h
mov ebp, 12h
mov ebp, 1234h
mov ebp, 12345678h
mov r12, 12h
mov r12, 1234h
mov r12, 12345678h
mov r12, 80000000h
mov rbx, 1234567812345678h
mov rsp, 1234567812345678h
mov r12, 1234567812345678h
; Test for immediate T*
mov rax, 2234567812345678h


; mov [r + offset], r with zero, byte, word, and dword offsets
mov [rax], cl
mov [rcx + 12h], bl
mov [rsi + 100h], r9b
mov [rdi + 12345678h], r15b

mov [rdx], dl
mov [rcx + 12h], cx
mov [rsi + 1234h], r9w
mov [rdi + 12345678h], r11w

mov [r9], esp
mov [rcx + 12h], edx
mov [rsi + 1234h], esi
mov [rdi + 12345678h], r11d

mov [r12], rbx
mov [rcx + 12h], rdi
mov [rsi + 1234h], rbp
mov [rdi + 12345678h], r10


;
; Push/Pop
;
pop rax
pop rbp
pop r12
push rbx
push rbp
push r12

;
; Ret
;
ret

LL1:
    jz LL1
    jz near ptr LL1
    jz LL1
    jz near ptr LL1
;    jz far ptr LL1


;
; Imul
;

imul bx, cx
imul ebx, ecx
imul rbx, rcx

imul rax, rbx;
imul r8, r9;
imul ax, bx;

imul cx, [rcx + 12h]
imul r9w, [rsi + 1234h]
imul r11w, [rdi + 12345678h]

imul esp, [r9]
imul edx, [rcx + 12h]
imul esi, [rsi + 1234h]
imul r11d, [rdi + 12345678h]

imul rbx, [r12]
imul rdi, [rcx + 12h]
imul rbp, [rsi + 1234h]
imul r10, [rdi + 12345678h]

imul cx, 56h
imul cx, 80h
imul dx, 5678h
imul ebp, 12h
imul ebp, 1234h
imul ebp, 12345678h
imul r12, 12h
imul r12, 1234h
imul r12, 12345678h
imul r12, -1

call rax
call rsp
call rbp
call r12
call r13

mov rax, 07fffffffffffffffh

movd xmm1, rax
movd xmm1, rcx
movd xmm1, r8
movd xmm1, rbp
movd xmm1, r12

movd xmm0, rcx
movd xmm1, rcx
movd xmm2, rcx
movd xmm5, rcx
movd xmm12, rcx

movd xmm1, eax
movd xmm1, ecx
movd xmm1, r8d
movd xmm1, ebp
movd xmm1, r12d

movd xmm0, ecx
movd xmm1, ecx
movd xmm2, ecx
movd xmm5, ecx
movd xmm12, ecx

; movss
movss xmm1, xmm2
movss xmm0, xmm12
movss xmm5, xmm12
movss xmm5, xmm3
movss xmm13, xmm5
movss xmm0, xmm15

movss xmm0, dword ptr [r12]
movss xmm4, dword ptr [rcx + 12h]
movss xmm5, dword ptr [rsi + 1234h]
movss xmm12, dword ptr [rdi + 12345678h]

movss dword ptr [r12], xmm0
movss dword ptr [rcx + 12h], xmm4
movss dword ptr [rsi + 1234h], xmm5
movss dword ptr [rdi + 12345678h], xmm12

; movsd
movsd xmm1, xmm2
movsd xmm0, xmm12
movsd xmm5, xmm12
movsd xmm5, xmm3
movsd xmm13, xmm5
movsd xmm0, xmm15

movsd xmm0, qword ptr [r12]
movsd xmm4, qword ptr [rcx + 12h]
movsd xmm5, qword ptr [rsi + 1234h]
movsd xmm12, qword ptr [rdi + 12345678h]

movsd qword ptr [r12], xmm0
movsd qword ptr [rcx + 12h], xmm4
movsd qword ptr [rsi + 1234h], xmm5
movsd qword ptr [rdi + 12345678h], xmm12

addsd xmm1, xmm2
addsd xmm0, xmm12
mulsd xmm5, xmm12
mulsd xmm5, xmm3
subsd xmm13, xmm5
subsd xmm0, xmm15

addsd xmm0, qword ptr [r12]
addsd xmm4, qword ptr [rcx + 12h]
mulsd xmm5, qword ptr [rsi + 1234h]
subsd xmm12, qword ptr [rdi + 12345678h]

addss xmm1, xmm2
addss xmm0, xmm12
mulss xmm5, xmm12
mulss xmm5, xmm3
subss xmm13, xmm5
subss xmm0, xmm15

addss xmm0, dword ptr [r12]
addss xmm4, dword ptr [rcx + 12h]
mulss xmm5, dword ptr [rsi + 1234h]
subss xmm12, dword ptr [rdi + 12345678h]

;
; MovZX
;

; 1 byte to 2, 4 and 8.
movzx bx, bl
movzx bx, r12b
movzx r9w, dl
movzx bx, byte ptr [rcx + 12h]
movzx bx, byte ptr [r9 + 34h]

movzx ebx, bl
movzx ebx, r12b
movzx r9d, dl
movzx ebx, byte ptr [rcx + 12h]
movzx ebx, byte ptr [r9 + 34h]

movzx rbx, bl
movzx rbx, r12b
movzx r9, dl
movzx rbx, byte ptr [rcx + 12h]
movzx rbx, byte ptr [r9 + 34h]

; 2 bytes to 4 and 8
movzx ebx, bx
movzx ebx, r12w
movzx r9d, dx
movzx ebx, word ptr [rcx + 12h]
movzx ebx, word ptr [r9 + 34h]

movzx rbx, bx
movzx rbx, r12w
movzx r9, dx
movzx rbx, word ptr [rcx + 12h]
movzx rbx, word ptr [r9 + 34h]

; 4 bytes to 8, implemented in terms of mov
mov ebx, ebx
mov ebx, r12d
mov r9d, edx
mov ebx, dword ptr [rcx + 12h]
mov ebx, dword ptr [r9 + 34h]

;
; MovSX
;

; 1 byte to 2, 4 and 8.
movsx bx, bl
movsx bx, r12b
movsx r9w, dl
movsx bx, byte ptr [rcx + 12h]
movsx bx, byte ptr [r9 + 34h]

movsx ebx, bl
movsx ebx, r12b
movsx r9d, dl
movsx ebx, byte ptr [rcx + 12h]
movsx ebx, byte ptr [r9 + 34h]

movsx rbx, bl
movsx rbx, r12b
movsx r9, dl
movsx rbx, byte ptr [rcx + 12h]
movsx rbx, byte ptr [r9 + 34h]

; 2 bytes to 4 and 8
movsx ebx, bx
movsx ebx, r12w
movsx r9d, dx
movsx ebx, word ptr [rcx + 12h]
movsx ebx, word ptr [r9 + 34h]

movsx rbx, bx
movsx rbx, r12w
movsx r9, dx
movsx rbx, word ptr [rcx + 12h]
movsx rbx, word ptr [r9 + 34h]

; 4 bytes to 8
movsxd rbx, ebx
movsxd rbx, r12d
movsxd r9, edx
movsxd rbx, dword ptr [rcx + 12h]
movsxd rbx, dword ptr [r9 + 34h]

;
; Aligned 128-bit floating point move: movaps and movapd
;

movaps xmm1, xmm1
movaps xmm2, xmm9
movaps xmm2, dword ptr [rcx + 20h]
movaps xmm2, dword ptr [r9 + 200h]
movaps dword ptr [rcx + 20h], xmm2
movaps dword ptr [r9 + 20h], xmm2
movaps dword ptr [r9 + 200h], xmm11

movapd xmm1, xmm1
movapd xmm2, xmm9
movapd xmm2, qword ptr [rcx + 20h]
movapd xmm2, qword ptr [r9 + 200h]
movapd qword ptr [rcx + 20h], xmm2
movapd qword ptr [r9 + 20h], xmm2
movapd qword ptr [r9 + 200h], xmm11

;
; CvtSI2SD/CvtSI2SS
;

cvtsi2ss xmm1, eax
cvtsi2ss xmm1, rax
cvtsi2ss xmm9, rbx
cvtsi2ss xmm1, r8
cvtsi2ss xmm1, dword ptr [rcx + 12h]
cvtsi2ss xmm1, dword ptr [r9 + 34h]
cvtsi2ss xmm1, qword ptr [rcx + 56h]

cvtsi2sd xmm1, eax
cvtsi2sd xmm1, rax
cvtsi2sd xmm9, rbx
cvtsi2sd xmm1, r8
cvtsi2sd xmm1, dword ptr [rcx + 12h]
cvtsi2sd xmm1, dword ptr [r9 + 34h]
cvtsi2sd xmm1, qword ptr [rcx + 56h]

;
; CvtTSD2SI/CvtTSS2SI
;

cvttss2si eax, xmm1
cvttss2si rax, xmm1
cvttss2si rbx, xmm9
cvttss2si r8, xmm1
cvttss2si ebx, dword ptr [rcx + 12h]
cvttss2si ebx, dword ptr [r9 + 34h]
cvttss2si rbx, dword ptr [rcx + 56h]

cvttsd2si eax, xmm1
cvttsd2si rax, xmm1
cvttsd2si rbx, xmm9
cvttsd2si r8, xmm1
cvttsd2si ebx, qword ptr [rcx + 12h]
cvttsd2si ebx, qword ptr [r9 + 34h]
cvttsd2si rbx, qword ptr [rcx + 56h]

;
; Conversion, float - cvtss2sd and cvtsd2ss
;

cvtss2sd xmm1, xmm1
cvtss2sd xmm2, xmm9
cvtss2sd xmm2, dword ptr [rcx + 20h]
cvtss2sd xmm2, dword ptr [r9 + 200h]

cvtsd2ss xmm1, xmm1
cvtsd2ss xmm2, xmm9
cvtsd2ss xmm2, qword ptr [rcx + 20h]
cvtsd2ss xmm2, qword ptr [r9 + 200h]

;
; Floating point comparison, comiss and comisd.
;

comiss xmm1, xmm1
comiss xmm2, xmm9
comiss xmm2, dword ptr [rcx + 20h]
comiss xmm2, dword ptr [r9 + 200h]

comisd xmm1, xmm1
comisd xmm2, xmm9
comisd xmm2, qword ptr [rcx + 20h]
comisd xmm2, qword ptr [r9 + 200h]

;
; Shift/rotate
;

rol al, cl
shl ebx, cl
shr r12, cl

rol rax, 3
shl bl, 4
shr r12d, 5

shld ax, bx, 11
shld edx, esi, 24
shld r12, rbp, 43
shld rbp, r12, 43

shld ax, bx, cl
shld edx, esi, cl
shld r12, rbp, cl
shld rbp, r12, cl

main ENDP

END
