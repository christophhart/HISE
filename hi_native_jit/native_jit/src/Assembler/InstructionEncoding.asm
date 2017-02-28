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

.code
instructions PROC


; Int <-> Int

; Source: rax, target: rax
mov al, 1
mov ax, 1
mov eax, 1
mov rax, 1

mov al, al
mov ax, ax
mov eax, eax
mov rax, rax

mov al, byte ptr [rax - 4]
mov al, byte ptr [rax + 4]
mov ax, word ptr [rax + 4]
mov eax, dword ptr [rax + 4]
mov rax, qword ptr [rax + 4]

mov byte ptr [rax - 4], al
mov byte ptr [rax + 4], al
mov word ptr [rax + 4], ax
mov dword ptr [rax + 4], eax
mov qword ptr [rax + 4], rax

movzx ax, al
movzx ax, byte ptr [rax + 4]

movzx eax, al
movzx eax, ax
movzx eax, byte ptr [rax + 4]
movzx eax, word ptr [rax + 4]

movzx rax, al
movzx rax, ax
movzx rax, byte ptr [rax + 4]
movzx rax, word ptr [rax + 4]


; Source: rax, target: rcx
mov cl, 1
mov cx, 1
mov ecx, 1
mov rcx, 1

mov cl, al
mov cx, ax
mov ecx, eax
mov rcx, rax

mov cl, byte ptr [rax - 4]
mov cl, byte ptr [rax + 4]
mov cx, word ptr [rax + 4]
mov ecx, dword ptr [rax + 4]
mov rcx, qword ptr [rax + 4]

mov byte ptr [rcx - 4], al
mov byte ptr [rcx + 4], al
mov word ptr [rcx + 4], ax
mov dword ptr [rcx + 4], eax
mov qword ptr [rcx + 4], rax

movzx cx, al
movzx cx, byte ptr [rax + 4]

movzx ecx, al
movzx ecx, ax
movzx ecx, byte ptr [rax + 4]
movzx ecx, word ptr [rax + 4]

movzx rcx, al
movzx rcx, ax
movzx rcx, byte ptr [rax + 4]
movzx rcx, word ptr [rax + 4]


; Source: rax, target: rdx
mov dl, 1
mov dx, 1
mov edx, 1
mov rdx, 1

mov dl, al
mov dx, ax
mov edx, eax
mov rdx, rax

mov dl, byte ptr [rax - 4]
mov dl, byte ptr [rax + 4]
mov dx, word ptr [rax + 4]
mov edx, dword ptr [rax + 4]
mov rdx, qword ptr [rax + 4]

mov byte ptr [rdx - 4], al
mov byte ptr [rdx + 4], al
mov word ptr [rdx + 4], ax
mov dword ptr [rdx + 4], eax
mov qword ptr [rdx + 4], rax

movzx dx, al
movzx dx, byte ptr [rax + 4]

movzx edx, al
movzx edx, ax
movzx edx, byte ptr [rax + 4]
movzx edx, word ptr [rax + 4]

movzx rdx, al
movzx rdx, ax
movzx rdx, byte ptr [rax + 4]
movzx rdx, word ptr [rax + 4]


; Source: rax, target: rbx
mov bl, 1
mov bx, 1
mov ebx, 1
mov rbx, 1

mov bl, al
mov bx, ax
mov ebx, eax
mov rbx, rax

mov bl, byte ptr [rax - 4]
mov bl, byte ptr [rax + 4]
mov bx, word ptr [rax + 4]
mov ebx, dword ptr [rax + 4]
mov rbx, qword ptr [rax + 4]

mov byte ptr [rbx - 4], al
mov byte ptr [rbx + 4], al
mov word ptr [rbx + 4], ax
mov dword ptr [rbx + 4], eax
mov qword ptr [rbx + 4], rax

movzx bx, al
movzx bx, byte ptr [rax + 4]

movzx ebx, al
movzx ebx, ax
movzx ebx, byte ptr [rax + 4]
movzx ebx, word ptr [rax + 4]

movzx rbx, al
movzx rbx, ax
movzx rbx, byte ptr [rax + 4]
movzx rbx, word ptr [rax + 4]


; Source: rax, target: rsp
mov spl, 1
mov sp, 1
mov esp, 1
mov rsp, 1

mov spl, al
mov sp, ax
mov esp, eax
mov rsp, rax

mov spl, byte ptr [rax - 4]
mov spl, byte ptr [rax + 4]
mov sp, word ptr [rax + 4]
mov esp, dword ptr [rax + 4]
mov rsp, qword ptr [rax + 4]

mov byte ptr [rsp - 4], al
mov byte ptr [rsp + 4], al
mov word ptr [rsp + 4], ax
mov dword ptr [rsp + 4], eax
mov qword ptr [rsp + 4], rax

movzx sp, al
movzx sp, byte ptr [rax + 4]

movzx esp, al
movzx esp, ax
movzx esp, byte ptr [rax + 4]
movzx esp, word ptr [rax + 4]

movzx rsp, al
movzx rsp, ax
movzx rsp, byte ptr [rax + 4]
movzx rsp, word ptr [rax + 4]


; Source: rax, target: rbp
mov bpl, 1
mov bp, 1
mov ebp, 1
mov rbp, 1

mov bpl, al
mov bp, ax
mov ebp, eax
mov rbp, rax

mov bpl, byte ptr [rax - 4]
mov bpl, byte ptr [rax + 4]
mov bp, word ptr [rax + 4]
mov ebp, dword ptr [rax + 4]
mov rbp, qword ptr [rax + 4]

mov byte ptr [rbp - 4], al
mov byte ptr [rbp + 4], al
mov word ptr [rbp + 4], ax
mov dword ptr [rbp + 4], eax
mov qword ptr [rbp + 4], rax

movzx bp, al
movzx bp, byte ptr [rax + 4]

movzx ebp, al
movzx ebp, ax
movzx ebp, byte ptr [rax + 4]
movzx ebp, word ptr [rax + 4]

movzx rbp, al
movzx rbp, ax
movzx rbp, byte ptr [rax + 4]
movzx rbp, word ptr [rax + 4]


; Source: rax, target: rsi
mov dil, 1
mov si, 1
mov esi, 1
mov rsi, 1

mov dil, al
mov si, ax
mov esi, eax
mov rsi, rax

mov dil, byte ptr [rax - 4]
mov dil, byte ptr [rax + 4]
mov si, word ptr [rax + 4]
mov esi, dword ptr [rax + 4]
mov rsi, qword ptr [rax + 4]

mov byte ptr [rsi - 4], al
mov byte ptr [rsi + 4], al
mov word ptr [rsi + 4], ax
mov dword ptr [rsi + 4], eax
mov qword ptr [rsi + 4], rax

movzx si, al
movzx si, byte ptr [rax + 4]

movzx esi, al
movzx esi, ax
movzx esi, byte ptr [rax + 4]
movzx esi, word ptr [rax + 4]

movzx rsi, al
movzx rsi, ax
movzx rsi, byte ptr [rax + 4]
movzx rsi, word ptr [rax + 4]


; Source: rax, target: rdi
mov sil, 1
mov di, 1
mov edi, 1
mov rdi, 1

mov sil, al
mov di, ax
mov edi, eax
mov rdi, rax

mov sil, byte ptr [rax - 4]
mov sil, byte ptr [rax + 4]
mov di, word ptr [rax + 4]
mov edi, dword ptr [rax + 4]
mov rdi, qword ptr [rax + 4]

mov byte ptr [rdi - 4], al
mov byte ptr [rdi + 4], al
mov word ptr [rdi + 4], ax
mov dword ptr [rdi + 4], eax
mov qword ptr [rdi + 4], rax

movzx di, al
movzx di, byte ptr [rax + 4]

movzx edi, al
movzx edi, ax
movzx edi, byte ptr [rax + 4]
movzx edi, word ptr [rax + 4]

movzx rdi, al
movzx rdi, ax
movzx rdi, byte ptr [rax + 4]
movzx rdi, word ptr [rax + 4]


; Source: rax, target: r8
mov r8b, 1
mov r8w, 1
mov r8d, 1
mov r8, 1

mov r8b, al
mov r8w, ax
mov r8d, eax
mov r8, rax

mov r8b, byte ptr [rax - 4]
mov r8b, byte ptr [rax + 4]
mov r8w, word ptr [rax + 4]
mov r8d, dword ptr [rax + 4]
mov r8, qword ptr [rax + 4]

mov byte ptr [r8 - 4], al
mov byte ptr [r8 + 4], al
mov word ptr [r8 + 4], ax
mov dword ptr [r8 + 4], eax
mov qword ptr [r8 + 4], rax

movzx r8w, al
movzx r8w, byte ptr [rax + 4]

movzx r8d, al
movzx r8d, ax
movzx r8d, byte ptr [rax + 4]
movzx r8d, word ptr [rax + 4]

movzx r8, al
movzx r8, ax
movzx r8, byte ptr [rax + 4]
movzx r8, word ptr [rax + 4]


; Source: rax, target: r9
mov r9b, 1
mov r9w, 1
mov r9d, 1
mov r9, 1

mov r9b, al
mov r9w, ax
mov r9d, eax
mov r9, rax

mov r9b, byte ptr [rax - 4]
mov r9b, byte ptr [rax + 4]
mov r9w, word ptr [rax + 4]
mov r9d, dword ptr [rax + 4]
mov r9, qword ptr [rax + 4]

mov byte ptr [r9 - 4], al
mov byte ptr [r9 + 4], al
mov word ptr [r9 + 4], ax
mov dword ptr [r9 + 4], eax
mov qword ptr [r9 + 4], rax

movzx r9w, al
movzx r9w, byte ptr [rax + 4]

movzx r9d, al
movzx r9d, ax
movzx r9d, byte ptr [rax + 4]
movzx r9d, word ptr [rax + 4]

movzx r9, al
movzx r9, ax
movzx r9, byte ptr [rax + 4]
movzx r9, word ptr [rax + 4]


; Source: rax, target: r10
mov r10b, 1
mov r10w, 1
mov r10d, 1
mov r10, 1

mov r10b, al
mov r10w, ax
mov r10d, eax
mov r10, rax

mov r10b, byte ptr [rax - 4]
mov r10b, byte ptr [rax + 4]
mov r10w, word ptr [rax + 4]
mov r10d, dword ptr [rax + 4]
mov r10, qword ptr [rax + 4]

mov byte ptr [r10 - 4], al
mov byte ptr [r10 + 4], al
mov word ptr [r10 + 4], ax
mov dword ptr [r10 + 4], eax
mov qword ptr [r10 + 4], rax

movzx r10w, al
movzx r10w, byte ptr [rax + 4]

movzx r10d, al
movzx r10d, ax
movzx r10d, byte ptr [rax + 4]
movzx r10d, word ptr [rax + 4]

movzx r10, al
movzx r10, ax
movzx r10, byte ptr [rax + 4]
movzx r10, word ptr [rax + 4]


; Source: rax, target: r11
mov r11b, 1
mov r11w, 1
mov r11d, 1
mov r11, 1

mov r11b, al
mov r11w, ax
mov r11d, eax
mov r11, rax

mov r11b, byte ptr [rax - 4]
mov r11b, byte ptr [rax + 4]
mov r11w, word ptr [rax + 4]
mov r11d, dword ptr [rax + 4]
mov r11, qword ptr [rax + 4]

mov byte ptr [r11 - 4], al
mov byte ptr [r11 + 4], al
mov word ptr [r11 + 4], ax
mov dword ptr [r11 + 4], eax
mov qword ptr [r11 + 4], rax

movzx r11w, al
movzx r11w, byte ptr [rax + 4]

movzx r11d, al
movzx r11d, ax
movzx r11d, byte ptr [rax + 4]
movzx r11d, word ptr [rax + 4]

movzx r11, al
movzx r11, ax
movzx r11, byte ptr [rax + 4]
movzx r11, word ptr [rax + 4]


; Source: rax, target: r12
mov r12b, 1
mov r12w, 1
mov r12d, 1
mov r12, 1

mov r12b, al
mov r12w, ax
mov r12d, eax
mov r12, rax

mov r12b, byte ptr [rax - 4]
mov r12b, byte ptr [rax + 4]
mov r12w, word ptr [rax + 4]
mov r12d, dword ptr [rax + 4]
mov r12, qword ptr [rax + 4]

mov byte ptr [r12 - 4], al
mov byte ptr [r12 + 4], al
mov word ptr [r12 + 4], ax
mov dword ptr [r12 + 4], eax
mov qword ptr [r12 + 4], rax

movzx r12w, al
movzx r12w, byte ptr [rax + 4]

movzx r12d, al
movzx r12d, ax
movzx r12d, byte ptr [rax + 4]
movzx r12d, word ptr [rax + 4]

movzx r12, al
movzx r12, ax
movzx r12, byte ptr [rax + 4]
movzx r12, word ptr [rax + 4]


; Source: rax, target: r13
mov r13b, 1
mov r13w, 1
mov r13d, 1
mov r13, 1

mov r13b, al
mov r13w, ax
mov r13d, eax
mov r13, rax

mov r13b, byte ptr [rax - 4]
mov r13b, byte ptr [rax + 4]
mov r13w, word ptr [rax + 4]
mov r13d, dword ptr [rax + 4]
mov r13, qword ptr [rax + 4]

mov byte ptr [r13 - 4], al
mov byte ptr [r13 + 4], al
mov word ptr [r13 + 4], ax
mov dword ptr [r13 + 4], eax
mov qword ptr [r13 + 4], rax

movzx r13w, al
movzx r13w, byte ptr [rax + 4]

movzx r13d, al
movzx r13d, ax
movzx r13d, byte ptr [rax + 4]
movzx r13d, word ptr [rax + 4]

movzx r13, al
movzx r13, ax
movzx r13, byte ptr [rax + 4]
movzx r13, word ptr [rax + 4]


; Source: rax, target: r14
mov r14b, 1
mov r14w, 1
mov r14d, 1
mov r14, 1

mov r14b, al
mov r14w, ax
mov r14d, eax
mov r14, rax

mov r14b, byte ptr [rax - 4]
mov r14b, byte ptr [rax + 4]
mov r14w, word ptr [rax + 4]
mov r14d, dword ptr [rax + 4]
mov r14, qword ptr [rax + 4]

mov byte ptr [r14 - 4], al
mov byte ptr [r14 + 4], al
mov word ptr [r14 + 4], ax
mov dword ptr [r14 + 4], eax
mov qword ptr [r14 + 4], rax

movzx r14w, al
movzx r14w, byte ptr [rax + 4]

movzx r14d, al
movzx r14d, ax
movzx r14d, byte ptr [rax + 4]
movzx r14d, word ptr [rax + 4]

movzx r14, al
movzx r14, ax
movzx r14, byte ptr [rax + 4]
movzx r14, word ptr [rax + 4]


; Source: rax, target: r15
mov r15b, 1
mov r15w, 1
mov r15d, 1
mov r15, 1

mov r15b, al
mov r15w, ax
mov r15d, eax
mov r15, rax

mov r15b, byte ptr [rax - 4]
mov r15b, byte ptr [rax + 4]
mov r15w, word ptr [rax + 4]
mov r15d, dword ptr [rax + 4]
mov r15, qword ptr [rax + 4]

mov byte ptr [r15 - 4], al
mov byte ptr [r15 + 4], al
mov word ptr [r15 + 4], ax
mov dword ptr [r15 + 4], eax
mov qword ptr [r15 + 4], rax

movzx r15w, al
movzx r15w, byte ptr [rax + 4]

movzx r15d, al
movzx r15d, ax
movzx r15d, byte ptr [rax + 4]
movzx r15d, word ptr [rax + 4]

movzx r15, al
movzx r15, ax
movzx r15, byte ptr [rax + 4]
movzx r15, word ptr [rax + 4]


; Source: rcx, target: rax
mov al, 1
mov ax, 1
mov eax, 1
mov rax, 1

mov al, cl
mov ax, cx
mov eax, ecx
mov rax, rcx

mov al, byte ptr [rcx - 4]
mov al, byte ptr [rcx + 4]
mov ax, word ptr [rcx + 4]
mov eax, dword ptr [rcx + 4]
mov rax, qword ptr [rcx + 4]

mov byte ptr [rax - 4], cl
mov byte ptr [rax + 4], cl
mov word ptr [rax + 4], cx
mov dword ptr [rax + 4], ecx
mov qword ptr [rax + 4], rcx

movzx ax, cl
movzx ax, byte ptr [rcx + 4]

movzx eax, cl
movzx eax, cx
movzx eax, byte ptr [rcx + 4]
movzx eax, word ptr [rcx + 4]

movzx rax, cl
movzx rax, cx
movzx rax, byte ptr [rcx + 4]
movzx rax, word ptr [rcx + 4]


; Source: rcx, target: rcx
mov cl, 1
mov cx, 1
mov ecx, 1
mov rcx, 1

mov cl, cl
mov cx, cx
mov ecx, ecx
mov rcx, rcx

mov cl, byte ptr [rcx - 4]
mov cl, byte ptr [rcx + 4]
mov cx, word ptr [rcx + 4]
mov ecx, dword ptr [rcx + 4]
mov rcx, qword ptr [rcx + 4]

mov byte ptr [rcx - 4], cl
mov byte ptr [rcx + 4], cl
mov word ptr [rcx + 4], cx
mov dword ptr [rcx + 4], ecx
mov qword ptr [rcx + 4], rcx

movzx cx, cl
movzx cx, byte ptr [rcx + 4]

movzx ecx, cl
movzx ecx, cx
movzx ecx, byte ptr [rcx + 4]
movzx ecx, word ptr [rcx + 4]

movzx rcx, cl
movzx rcx, cx
movzx rcx, byte ptr [rcx + 4]
movzx rcx, word ptr [rcx + 4]


; Source: rcx, target: rdx
mov dl, 1
mov dx, 1
mov edx, 1
mov rdx, 1

mov dl, cl
mov dx, cx
mov edx, ecx
mov rdx, rcx

mov dl, byte ptr [rcx - 4]
mov dl, byte ptr [rcx + 4]
mov dx, word ptr [rcx + 4]
mov edx, dword ptr [rcx + 4]
mov rdx, qword ptr [rcx + 4]

mov byte ptr [rdx - 4], cl
mov byte ptr [rdx + 4], cl
mov word ptr [rdx + 4], cx
mov dword ptr [rdx + 4], ecx
mov qword ptr [rdx + 4], rcx

movzx dx, cl
movzx dx, byte ptr [rcx + 4]

movzx edx, cl
movzx edx, cx
movzx edx, byte ptr [rcx + 4]
movzx edx, word ptr [rcx + 4]

movzx rdx, cl
movzx rdx, cx
movzx rdx, byte ptr [rcx + 4]
movzx rdx, word ptr [rcx + 4]


; Source: rcx, target: rbx
mov bl, 1
mov bx, 1
mov ebx, 1
mov rbx, 1

mov bl, cl
mov bx, cx
mov ebx, ecx
mov rbx, rcx

mov bl, byte ptr [rcx - 4]
mov bl, byte ptr [rcx + 4]
mov bx, word ptr [rcx + 4]
mov ebx, dword ptr [rcx + 4]
mov rbx, qword ptr [rcx + 4]

mov byte ptr [rbx - 4], cl
mov byte ptr [rbx + 4], cl
mov word ptr [rbx + 4], cx
mov dword ptr [rbx + 4], ecx
mov qword ptr [rbx + 4], rcx

movzx bx, cl
movzx bx, byte ptr [rcx + 4]

movzx ebx, cl
movzx ebx, cx
movzx ebx, byte ptr [rcx + 4]
movzx ebx, word ptr [rcx + 4]

movzx rbx, cl
movzx rbx, cx
movzx rbx, byte ptr [rcx + 4]
movzx rbx, word ptr [rcx + 4]


; Source: rcx, target: rsp
mov spl, 1
mov sp, 1
mov esp, 1
mov rsp, 1

mov spl, cl
mov sp, cx
mov esp, ecx
mov rsp, rcx

mov spl, byte ptr [rcx - 4]
mov spl, byte ptr [rcx + 4]
mov sp, word ptr [rcx + 4]
mov esp, dword ptr [rcx + 4]
mov rsp, qword ptr [rcx + 4]

mov byte ptr [rsp - 4], cl
mov byte ptr [rsp + 4], cl
mov word ptr [rsp + 4], cx
mov dword ptr [rsp + 4], ecx
mov qword ptr [rsp + 4], rcx

movzx sp, cl
movzx sp, byte ptr [rcx + 4]

movzx esp, cl
movzx esp, cx
movzx esp, byte ptr [rcx + 4]
movzx esp, word ptr [rcx + 4]

movzx rsp, cl
movzx rsp, cx
movzx rsp, byte ptr [rcx + 4]
movzx rsp, word ptr [rcx + 4]


; Source: rcx, target: rbp
mov bpl, 1
mov bp, 1
mov ebp, 1
mov rbp, 1

mov bpl, cl
mov bp, cx
mov ebp, ecx
mov rbp, rcx

mov bpl, byte ptr [rcx - 4]
mov bpl, byte ptr [rcx + 4]
mov bp, word ptr [rcx + 4]
mov ebp, dword ptr [rcx + 4]
mov rbp, qword ptr [rcx + 4]

mov byte ptr [rbp - 4], cl
mov byte ptr [rbp + 4], cl
mov word ptr [rbp + 4], cx
mov dword ptr [rbp + 4], ecx
mov qword ptr [rbp + 4], rcx

movzx bp, cl
movzx bp, byte ptr [rcx + 4]

movzx ebp, cl
movzx ebp, cx
movzx ebp, byte ptr [rcx + 4]
movzx ebp, word ptr [rcx + 4]

movzx rbp, cl
movzx rbp, cx
movzx rbp, byte ptr [rcx + 4]
movzx rbp, word ptr [rcx + 4]


; Source: rcx, target: rsi
mov dil, 1
mov si, 1
mov esi, 1
mov rsi, 1

mov dil, cl
mov si, cx
mov esi, ecx
mov rsi, rcx

mov dil, byte ptr [rcx - 4]
mov dil, byte ptr [rcx + 4]
mov si, word ptr [rcx + 4]
mov esi, dword ptr [rcx + 4]
mov rsi, qword ptr [rcx + 4]

mov byte ptr [rsi - 4], cl
mov byte ptr [rsi + 4], cl
mov word ptr [rsi + 4], cx
mov dword ptr [rsi + 4], ecx
mov qword ptr [rsi + 4], rcx

movzx si, cl
movzx si, byte ptr [rcx + 4]

movzx esi, cl
movzx esi, cx
movzx esi, byte ptr [rcx + 4]
movzx esi, word ptr [rcx + 4]

movzx rsi, cl
movzx rsi, cx
movzx rsi, byte ptr [rcx + 4]
movzx rsi, word ptr [rcx + 4]


; Source: rcx, target: rdi
mov sil, 1
mov di, 1
mov edi, 1
mov rdi, 1

mov sil, cl
mov di, cx
mov edi, ecx
mov rdi, rcx

mov sil, byte ptr [rcx - 4]
mov sil, byte ptr [rcx + 4]
mov di, word ptr [rcx + 4]
mov edi, dword ptr [rcx + 4]
mov rdi, qword ptr [rcx + 4]

mov byte ptr [rdi - 4], cl
mov byte ptr [rdi + 4], cl
mov word ptr [rdi + 4], cx
mov dword ptr [rdi + 4], ecx
mov qword ptr [rdi + 4], rcx

movzx di, cl
movzx di, byte ptr [rcx + 4]

movzx edi, cl
movzx edi, cx
movzx edi, byte ptr [rcx + 4]
movzx edi, word ptr [rcx + 4]

movzx rdi, cl
movzx rdi, cx
movzx rdi, byte ptr [rcx + 4]
movzx rdi, word ptr [rcx + 4]


; Source: rcx, target: r8
mov r8b, 1
mov r8w, 1
mov r8d, 1
mov r8, 1

mov r8b, cl
mov r8w, cx
mov r8d, ecx
mov r8, rcx

mov r8b, byte ptr [rcx - 4]
mov r8b, byte ptr [rcx + 4]
mov r8w, word ptr [rcx + 4]
mov r8d, dword ptr [rcx + 4]
mov r8, qword ptr [rcx + 4]

mov byte ptr [r8 - 4], cl
mov byte ptr [r8 + 4], cl
mov word ptr [r8 + 4], cx
mov dword ptr [r8 + 4], ecx
mov qword ptr [r8 + 4], rcx

movzx r8w, cl
movzx r8w, byte ptr [rcx + 4]

movzx r8d, cl
movzx r8d, cx
movzx r8d, byte ptr [rcx + 4]
movzx r8d, word ptr [rcx + 4]

movzx r8, cl
movzx r8, cx
movzx r8, byte ptr [rcx + 4]
movzx r8, word ptr [rcx + 4]


; Source: rcx, target: r9
mov r9b, 1
mov r9w, 1
mov r9d, 1
mov r9, 1

mov r9b, cl
mov r9w, cx
mov r9d, ecx
mov r9, rcx

mov r9b, byte ptr [rcx - 4]
mov r9b, byte ptr [rcx + 4]
mov r9w, word ptr [rcx + 4]
mov r9d, dword ptr [rcx + 4]
mov r9, qword ptr [rcx + 4]

mov byte ptr [r9 - 4], cl
mov byte ptr [r9 + 4], cl
mov word ptr [r9 + 4], cx
mov dword ptr [r9 + 4], ecx
mov qword ptr [r9 + 4], rcx

movzx r9w, cl
movzx r9w, byte ptr [rcx + 4]

movzx r9d, cl
movzx r9d, cx
movzx r9d, byte ptr [rcx + 4]
movzx r9d, word ptr [rcx + 4]

movzx r9, cl
movzx r9, cx
movzx r9, byte ptr [rcx + 4]
movzx r9, word ptr [rcx + 4]


; Source: rcx, target: r10
mov r10b, 1
mov r10w, 1
mov r10d, 1
mov r10, 1

mov r10b, cl
mov r10w, cx
mov r10d, ecx
mov r10, rcx

mov r10b, byte ptr [rcx - 4]
mov r10b, byte ptr [rcx + 4]
mov r10w, word ptr [rcx + 4]
mov r10d, dword ptr [rcx + 4]
mov r10, qword ptr [rcx + 4]

mov byte ptr [r10 - 4], cl
mov byte ptr [r10 + 4], cl
mov word ptr [r10 + 4], cx
mov dword ptr [r10 + 4], ecx
mov qword ptr [r10 + 4], rcx

movzx r10w, cl
movzx r10w, byte ptr [rcx + 4]

movzx r10d, cl
movzx r10d, cx
movzx r10d, byte ptr [rcx + 4]
movzx r10d, word ptr [rcx + 4]

movzx r10, cl
movzx r10, cx
movzx r10, byte ptr [rcx + 4]
movzx r10, word ptr [rcx + 4]


; Source: rcx, target: r11
mov r11b, 1
mov r11w, 1
mov r11d, 1
mov r11, 1

mov r11b, cl
mov r11w, cx
mov r11d, ecx
mov r11, rcx

mov r11b, byte ptr [rcx - 4]
mov r11b, byte ptr [rcx + 4]
mov r11w, word ptr [rcx + 4]
mov r11d, dword ptr [rcx + 4]
mov r11, qword ptr [rcx + 4]

mov byte ptr [r11 - 4], cl
mov byte ptr [r11 + 4], cl
mov word ptr [r11 + 4], cx
mov dword ptr [r11 + 4], ecx
mov qword ptr [r11 + 4], rcx

movzx r11w, cl
movzx r11w, byte ptr [rcx + 4]

movzx r11d, cl
movzx r11d, cx
movzx r11d, byte ptr [rcx + 4]
movzx r11d, word ptr [rcx + 4]

movzx r11, cl
movzx r11, cx
movzx r11, byte ptr [rcx + 4]
movzx r11, word ptr [rcx + 4]


; Source: rcx, target: r12
mov r12b, 1
mov r12w, 1
mov r12d, 1
mov r12, 1

mov r12b, cl
mov r12w, cx
mov r12d, ecx
mov r12, rcx

mov r12b, byte ptr [rcx - 4]
mov r12b, byte ptr [rcx + 4]
mov r12w, word ptr [rcx + 4]
mov r12d, dword ptr [rcx + 4]
mov r12, qword ptr [rcx + 4]

mov byte ptr [r12 - 4], cl
mov byte ptr [r12 + 4], cl
mov word ptr [r12 + 4], cx
mov dword ptr [r12 + 4], ecx
mov qword ptr [r12 + 4], rcx

movzx r12w, cl
movzx r12w, byte ptr [rcx + 4]

movzx r12d, cl
movzx r12d, cx
movzx r12d, byte ptr [rcx + 4]
movzx r12d, word ptr [rcx + 4]

movzx r12, cl
movzx r12, cx
movzx r12, byte ptr [rcx + 4]
movzx r12, word ptr [rcx + 4]


; Source: rcx, target: r13
mov r13b, 1
mov r13w, 1
mov r13d, 1
mov r13, 1

mov r13b, cl
mov r13w, cx
mov r13d, ecx
mov r13, rcx

mov r13b, byte ptr [rcx - 4]
mov r13b, byte ptr [rcx + 4]
mov r13w, word ptr [rcx + 4]
mov r13d, dword ptr [rcx + 4]
mov r13, qword ptr [rcx + 4]

mov byte ptr [r13 - 4], cl
mov byte ptr [r13 + 4], cl
mov word ptr [r13 + 4], cx
mov dword ptr [r13 + 4], ecx
mov qword ptr [r13 + 4], rcx

movzx r13w, cl
movzx r13w, byte ptr [rcx + 4]

movzx r13d, cl
movzx r13d, cx
movzx r13d, byte ptr [rcx + 4]
movzx r13d, word ptr [rcx + 4]

movzx r13, cl
movzx r13, cx
movzx r13, byte ptr [rcx + 4]
movzx r13, word ptr [rcx + 4]


; Source: rcx, target: r14
mov r14b, 1
mov r14w, 1
mov r14d, 1
mov r14, 1

mov r14b, cl
mov r14w, cx
mov r14d, ecx
mov r14, rcx

mov r14b, byte ptr [rcx - 4]
mov r14b, byte ptr [rcx + 4]
mov r14w, word ptr [rcx + 4]
mov r14d, dword ptr [rcx + 4]
mov r14, qword ptr [rcx + 4]

mov byte ptr [r14 - 4], cl
mov byte ptr [r14 + 4], cl
mov word ptr [r14 + 4], cx
mov dword ptr [r14 + 4], ecx
mov qword ptr [r14 + 4], rcx

movzx r14w, cl
movzx r14w, byte ptr [rcx + 4]

movzx r14d, cl
movzx r14d, cx
movzx r14d, byte ptr [rcx + 4]
movzx r14d, word ptr [rcx + 4]

movzx r14, cl
movzx r14, cx
movzx r14, byte ptr [rcx + 4]
movzx r14, word ptr [rcx + 4]


; Source: rcx, target: r15
mov r15b, 1
mov r15w, 1
mov r15d, 1
mov r15, 1

mov r15b, cl
mov r15w, cx
mov r15d, ecx
mov r15, rcx

mov r15b, byte ptr [rcx - 4]
mov r15b, byte ptr [rcx + 4]
mov r15w, word ptr [rcx + 4]
mov r15d, dword ptr [rcx + 4]
mov r15, qword ptr [rcx + 4]

mov byte ptr [r15 - 4], cl
mov byte ptr [r15 + 4], cl
mov word ptr [r15 + 4], cx
mov dword ptr [r15 + 4], ecx
mov qword ptr [r15 + 4], rcx

movzx r15w, cl
movzx r15w, byte ptr [rcx + 4]

movzx r15d, cl
movzx r15d, cx
movzx r15d, byte ptr [rcx + 4]
movzx r15d, word ptr [rcx + 4]

movzx r15, cl
movzx r15, cx
movzx r15, byte ptr [rcx + 4]
movzx r15, word ptr [rcx + 4]


; Source: rdx, target: rax
mov al, 1
mov ax, 1
mov eax, 1
mov rax, 1

mov al, dl
mov ax, dx
mov eax, edx
mov rax, rdx

mov al, byte ptr [rdx - 4]
mov al, byte ptr [rdx + 4]
mov ax, word ptr [rdx + 4]
mov eax, dword ptr [rdx + 4]
mov rax, qword ptr [rdx + 4]

mov byte ptr [rax - 4], dl
mov byte ptr [rax + 4], dl
mov word ptr [rax + 4], dx
mov dword ptr [rax + 4], edx
mov qword ptr [rax + 4], rdx

movzx ax, dl
movzx ax, byte ptr [rdx + 4]

movzx eax, dl
movzx eax, dx
movzx eax, byte ptr [rdx + 4]
movzx eax, word ptr [rdx + 4]

movzx rax, dl
movzx rax, dx
movzx rax, byte ptr [rdx + 4]
movzx rax, word ptr [rdx + 4]


; Source: rdx, target: rcx
mov cl, 1
mov cx, 1
mov ecx, 1
mov rcx, 1

mov cl, dl
mov cx, dx
mov ecx, edx
mov rcx, rdx

mov cl, byte ptr [rdx - 4]
mov cl, byte ptr [rdx + 4]
mov cx, word ptr [rdx + 4]
mov ecx, dword ptr [rdx + 4]
mov rcx, qword ptr [rdx + 4]

mov byte ptr [rcx - 4], dl
mov byte ptr [rcx + 4], dl
mov word ptr [rcx + 4], dx
mov dword ptr [rcx + 4], edx
mov qword ptr [rcx + 4], rdx

movzx cx, dl
movzx cx, byte ptr [rdx + 4]

movzx ecx, dl
movzx ecx, dx
movzx ecx, byte ptr [rdx + 4]
movzx ecx, word ptr [rdx + 4]

movzx rcx, dl
movzx rcx, dx
movzx rcx, byte ptr [rdx + 4]
movzx rcx, word ptr [rdx + 4]


; Source: rdx, target: rdx
mov dl, 1
mov dx, 1
mov edx, 1
mov rdx, 1

mov dl, dl
mov dx, dx
mov edx, edx
mov rdx, rdx

mov dl, byte ptr [rdx - 4]
mov dl, byte ptr [rdx + 4]
mov dx, word ptr [rdx + 4]
mov edx, dword ptr [rdx + 4]
mov rdx, qword ptr [rdx + 4]

mov byte ptr [rdx - 4], dl
mov byte ptr [rdx + 4], dl
mov word ptr [rdx + 4], dx
mov dword ptr [rdx + 4], edx
mov qword ptr [rdx + 4], rdx

movzx dx, dl
movzx dx, byte ptr [rdx + 4]

movzx edx, dl
movzx edx, dx
movzx edx, byte ptr [rdx + 4]
movzx edx, word ptr [rdx + 4]

movzx rdx, dl
movzx rdx, dx
movzx rdx, byte ptr [rdx + 4]
movzx rdx, word ptr [rdx + 4]


; Source: rdx, target: rbx
mov bl, 1
mov bx, 1
mov ebx, 1
mov rbx, 1

mov bl, dl
mov bx, dx
mov ebx, edx
mov rbx, rdx

mov bl, byte ptr [rdx - 4]
mov bl, byte ptr [rdx + 4]
mov bx, word ptr [rdx + 4]
mov ebx, dword ptr [rdx + 4]
mov rbx, qword ptr [rdx + 4]

mov byte ptr [rbx - 4], dl
mov byte ptr [rbx + 4], dl
mov word ptr [rbx + 4], dx
mov dword ptr [rbx + 4], edx
mov qword ptr [rbx + 4], rdx

movzx bx, dl
movzx bx, byte ptr [rdx + 4]

movzx ebx, dl
movzx ebx, dx
movzx ebx, byte ptr [rdx + 4]
movzx ebx, word ptr [rdx + 4]

movzx rbx, dl
movzx rbx, dx
movzx rbx, byte ptr [rdx + 4]
movzx rbx, word ptr [rdx + 4]


; Source: rdx, target: rsp
mov spl, 1
mov sp, 1
mov esp, 1
mov rsp, 1

mov spl, dl
mov sp, dx
mov esp, edx
mov rsp, rdx

mov spl, byte ptr [rdx - 4]
mov spl, byte ptr [rdx + 4]
mov sp, word ptr [rdx + 4]
mov esp, dword ptr [rdx + 4]
mov rsp, qword ptr [rdx + 4]

mov byte ptr [rsp - 4], dl
mov byte ptr [rsp + 4], dl
mov word ptr [rsp + 4], dx
mov dword ptr [rsp + 4], edx
mov qword ptr [rsp + 4], rdx

movzx sp, dl
movzx sp, byte ptr [rdx + 4]

movzx esp, dl
movzx esp, dx
movzx esp, byte ptr [rdx + 4]
movzx esp, word ptr [rdx + 4]

movzx rsp, dl
movzx rsp, dx
movzx rsp, byte ptr [rdx + 4]
movzx rsp, word ptr [rdx + 4]


; Source: rdx, target: rbp
mov bpl, 1
mov bp, 1
mov ebp, 1
mov rbp, 1

mov bpl, dl
mov bp, dx
mov ebp, edx
mov rbp, rdx

mov bpl, byte ptr [rdx - 4]
mov bpl, byte ptr [rdx + 4]
mov bp, word ptr [rdx + 4]
mov ebp, dword ptr [rdx + 4]
mov rbp, qword ptr [rdx + 4]

mov byte ptr [rbp - 4], dl
mov byte ptr [rbp + 4], dl
mov word ptr [rbp + 4], dx
mov dword ptr [rbp + 4], edx
mov qword ptr [rbp + 4], rdx

movzx bp, dl
movzx bp, byte ptr [rdx + 4]

movzx ebp, dl
movzx ebp, dx
movzx ebp, byte ptr [rdx + 4]
movzx ebp, word ptr [rdx + 4]

movzx rbp, dl
movzx rbp, dx
movzx rbp, byte ptr [rdx + 4]
movzx rbp, word ptr [rdx + 4]


; Source: rdx, target: rsi
mov dil, 1
mov si, 1
mov esi, 1
mov rsi, 1

mov dil, dl
mov si, dx
mov esi, edx
mov rsi, rdx

mov dil, byte ptr [rdx - 4]
mov dil, byte ptr [rdx + 4]
mov si, word ptr [rdx + 4]
mov esi, dword ptr [rdx + 4]
mov rsi, qword ptr [rdx + 4]

mov byte ptr [rsi - 4], dl
mov byte ptr [rsi + 4], dl
mov word ptr [rsi + 4], dx
mov dword ptr [rsi + 4], edx
mov qword ptr [rsi + 4], rdx

movzx si, dl
movzx si, byte ptr [rdx + 4]

movzx esi, dl
movzx esi, dx
movzx esi, byte ptr [rdx + 4]
movzx esi, word ptr [rdx + 4]

movzx rsi, dl
movzx rsi, dx
movzx rsi, byte ptr [rdx + 4]
movzx rsi, word ptr [rdx + 4]


; Source: rdx, target: rdi
mov sil, 1
mov di, 1
mov edi, 1
mov rdi, 1

mov sil, dl
mov di, dx
mov edi, edx
mov rdi, rdx

mov sil, byte ptr [rdx - 4]
mov sil, byte ptr [rdx + 4]
mov di, word ptr [rdx + 4]
mov edi, dword ptr [rdx + 4]
mov rdi, qword ptr [rdx + 4]

mov byte ptr [rdi - 4], dl
mov byte ptr [rdi + 4], dl
mov word ptr [rdi + 4], dx
mov dword ptr [rdi + 4], edx
mov qword ptr [rdi + 4], rdx

movzx di, dl
movzx di, byte ptr [rdx + 4]

movzx edi, dl
movzx edi, dx
movzx edi, byte ptr [rdx + 4]
movzx edi, word ptr [rdx + 4]

movzx rdi, dl
movzx rdi, dx
movzx rdi, byte ptr [rdx + 4]
movzx rdi, word ptr [rdx + 4]


; Source: rdx, target: r8
mov r8b, 1
mov r8w, 1
mov r8d, 1
mov r8, 1

mov r8b, dl
mov r8w, dx
mov r8d, edx
mov r8, rdx

mov r8b, byte ptr [rdx - 4]
mov r8b, byte ptr [rdx + 4]
mov r8w, word ptr [rdx + 4]
mov r8d, dword ptr [rdx + 4]
mov r8, qword ptr [rdx + 4]

mov byte ptr [r8 - 4], dl
mov byte ptr [r8 + 4], dl
mov word ptr [r8 + 4], dx
mov dword ptr [r8 + 4], edx
mov qword ptr [r8 + 4], rdx

movzx r8w, dl
movzx r8w, byte ptr [rdx + 4]

movzx r8d, dl
movzx r8d, dx
movzx r8d, byte ptr [rdx + 4]
movzx r8d, word ptr [rdx + 4]

movzx r8, dl
movzx r8, dx
movzx r8, byte ptr [rdx + 4]
movzx r8, word ptr [rdx + 4]


; Source: rdx, target: r9
mov r9b, 1
mov r9w, 1
mov r9d, 1
mov r9, 1

mov r9b, dl
mov r9w, dx
mov r9d, edx
mov r9, rdx

mov r9b, byte ptr [rdx - 4]
mov r9b, byte ptr [rdx + 4]
mov r9w, word ptr [rdx + 4]
mov r9d, dword ptr [rdx + 4]
mov r9, qword ptr [rdx + 4]

mov byte ptr [r9 - 4], dl
mov byte ptr [r9 + 4], dl
mov word ptr [r9 + 4], dx
mov dword ptr [r9 + 4], edx
mov qword ptr [r9 + 4], rdx

movzx r9w, dl
movzx r9w, byte ptr [rdx + 4]

movzx r9d, dl
movzx r9d, dx
movzx r9d, byte ptr [rdx + 4]
movzx r9d, word ptr [rdx + 4]

movzx r9, dl
movzx r9, dx
movzx r9, byte ptr [rdx + 4]
movzx r9, word ptr [rdx + 4]


; Source: rdx, target: r10
mov r10b, 1
mov r10w, 1
mov r10d, 1
mov r10, 1

mov r10b, dl
mov r10w, dx
mov r10d, edx
mov r10, rdx

mov r10b, byte ptr [rdx - 4]
mov r10b, byte ptr [rdx + 4]
mov r10w, word ptr [rdx + 4]
mov r10d, dword ptr [rdx + 4]
mov r10, qword ptr [rdx + 4]

mov byte ptr [r10 - 4], dl
mov byte ptr [r10 + 4], dl
mov word ptr [r10 + 4], dx
mov dword ptr [r10 + 4], edx
mov qword ptr [r10 + 4], rdx

movzx r10w, dl
movzx r10w, byte ptr [rdx + 4]

movzx r10d, dl
movzx r10d, dx
movzx r10d, byte ptr [rdx + 4]
movzx r10d, word ptr [rdx + 4]

movzx r10, dl
movzx r10, dx
movzx r10, byte ptr [rdx + 4]
movzx r10, word ptr [rdx + 4]


; Source: rdx, target: r11
mov r11b, 1
mov r11w, 1
mov r11d, 1
mov r11, 1

mov r11b, dl
mov r11w, dx
mov r11d, edx
mov r11, rdx

mov r11b, byte ptr [rdx - 4]
mov r11b, byte ptr [rdx + 4]
mov r11w, word ptr [rdx + 4]
mov r11d, dword ptr [rdx + 4]
mov r11, qword ptr [rdx + 4]

mov byte ptr [r11 - 4], dl
mov byte ptr [r11 + 4], dl
mov word ptr [r11 + 4], dx
mov dword ptr [r11 + 4], edx
mov qword ptr [r11 + 4], rdx

movzx r11w, dl
movzx r11w, byte ptr [rdx + 4]

movzx r11d, dl
movzx r11d, dx
movzx r11d, byte ptr [rdx + 4]
movzx r11d, word ptr [rdx + 4]

movzx r11, dl
movzx r11, dx
movzx r11, byte ptr [rdx + 4]
movzx r11, word ptr [rdx + 4]


; Source: rdx, target: r12
mov r12b, 1
mov r12w, 1
mov r12d, 1
mov r12, 1

mov r12b, dl
mov r12w, dx
mov r12d, edx
mov r12, rdx

mov r12b, byte ptr [rdx - 4]
mov r12b, byte ptr [rdx + 4]
mov r12w, word ptr [rdx + 4]
mov r12d, dword ptr [rdx + 4]
mov r12, qword ptr [rdx + 4]

mov byte ptr [r12 - 4], dl
mov byte ptr [r12 + 4], dl
mov word ptr [r12 + 4], dx
mov dword ptr [r12 + 4], edx
mov qword ptr [r12 + 4], rdx

movzx r12w, dl
movzx r12w, byte ptr [rdx + 4]

movzx r12d, dl
movzx r12d, dx
movzx r12d, byte ptr [rdx + 4]
movzx r12d, word ptr [rdx + 4]

movzx r12, dl
movzx r12, dx
movzx r12, byte ptr [rdx + 4]
movzx r12, word ptr [rdx + 4]


; Source: rdx, target: r13
mov r13b, 1
mov r13w, 1
mov r13d, 1
mov r13, 1

mov r13b, dl
mov r13w, dx
mov r13d, edx
mov r13, rdx

mov r13b, byte ptr [rdx - 4]
mov r13b, byte ptr [rdx + 4]
mov r13w, word ptr [rdx + 4]
mov r13d, dword ptr [rdx + 4]
mov r13, qword ptr [rdx + 4]

mov byte ptr [r13 - 4], dl
mov byte ptr [r13 + 4], dl
mov word ptr [r13 + 4], dx
mov dword ptr [r13 + 4], edx
mov qword ptr [r13 + 4], rdx

movzx r13w, dl
movzx r13w, byte ptr [rdx + 4]

movzx r13d, dl
movzx r13d, dx
movzx r13d, byte ptr [rdx + 4]
movzx r13d, word ptr [rdx + 4]

movzx r13, dl
movzx r13, dx
movzx r13, byte ptr [rdx + 4]
movzx r13, word ptr [rdx + 4]


; Source: rdx, target: r14
mov r14b, 1
mov r14w, 1
mov r14d, 1
mov r14, 1

mov r14b, dl
mov r14w, dx
mov r14d, edx
mov r14, rdx

mov r14b, byte ptr [rdx - 4]
mov r14b, byte ptr [rdx + 4]
mov r14w, word ptr [rdx + 4]
mov r14d, dword ptr [rdx + 4]
mov r14, qword ptr [rdx + 4]

mov byte ptr [r14 - 4], dl
mov byte ptr [r14 + 4], dl
mov word ptr [r14 + 4], dx
mov dword ptr [r14 + 4], edx
mov qword ptr [r14 + 4], rdx

movzx r14w, dl
movzx r14w, byte ptr [rdx + 4]

movzx r14d, dl
movzx r14d, dx
movzx r14d, byte ptr [rdx + 4]
movzx r14d, word ptr [rdx + 4]

movzx r14, dl
movzx r14, dx
movzx r14, byte ptr [rdx + 4]
movzx r14, word ptr [rdx + 4]


; Source: rdx, target: r15
mov r15b, 1
mov r15w, 1
mov r15d, 1
mov r15, 1

mov r15b, dl
mov r15w, dx
mov r15d, edx
mov r15, rdx

mov r15b, byte ptr [rdx - 4]
mov r15b, byte ptr [rdx + 4]
mov r15w, word ptr [rdx + 4]
mov r15d, dword ptr [rdx + 4]
mov r15, qword ptr [rdx + 4]

mov byte ptr [r15 - 4], dl
mov byte ptr [r15 + 4], dl
mov word ptr [r15 + 4], dx
mov dword ptr [r15 + 4], edx
mov qword ptr [r15 + 4], rdx

movzx r15w, dl
movzx r15w, byte ptr [rdx + 4]

movzx r15d, dl
movzx r15d, dx
movzx r15d, byte ptr [rdx + 4]
movzx r15d, word ptr [rdx + 4]

movzx r15, dl
movzx r15, dx
movzx r15, byte ptr [rdx + 4]
movzx r15, word ptr [rdx + 4]


; Source: rbx, target: rax
mov al, 1
mov ax, 1
mov eax, 1
mov rax, 1

mov al, bl
mov ax, bx
mov eax, ebx
mov rax, rbx

mov al, byte ptr [rbx - 4]
mov al, byte ptr [rbx + 4]
mov ax, word ptr [rbx + 4]
mov eax, dword ptr [rbx + 4]
mov rax, qword ptr [rbx + 4]

mov byte ptr [rax - 4], bl
mov byte ptr [rax + 4], bl
mov word ptr [rax + 4], bx
mov dword ptr [rax + 4], ebx
mov qword ptr [rax + 4], rbx

movzx ax, bl
movzx ax, byte ptr [rbx + 4]

movzx eax, bl
movzx eax, bx
movzx eax, byte ptr [rbx + 4]
movzx eax, word ptr [rbx + 4]

movzx rax, bl
movzx rax, bx
movzx rax, byte ptr [rbx + 4]
movzx rax, word ptr [rbx + 4]


; Source: rbx, target: rcx
mov cl, 1
mov cx, 1
mov ecx, 1
mov rcx, 1

mov cl, bl
mov cx, bx
mov ecx, ebx
mov rcx, rbx

mov cl, byte ptr [rbx - 4]
mov cl, byte ptr [rbx + 4]
mov cx, word ptr [rbx + 4]
mov ecx, dword ptr [rbx + 4]
mov rcx, qword ptr [rbx + 4]

mov byte ptr [rcx - 4], bl
mov byte ptr [rcx + 4], bl
mov word ptr [rcx + 4], bx
mov dword ptr [rcx + 4], ebx
mov qword ptr [rcx + 4], rbx

movzx cx, bl
movzx cx, byte ptr [rbx + 4]

movzx ecx, bl
movzx ecx, bx
movzx ecx, byte ptr [rbx + 4]
movzx ecx, word ptr [rbx + 4]

movzx rcx, bl
movzx rcx, bx
movzx rcx, byte ptr [rbx + 4]
movzx rcx, word ptr [rbx + 4]


; Source: rbx, target: rdx
mov dl, 1
mov dx, 1
mov edx, 1
mov rdx, 1

mov dl, bl
mov dx, bx
mov edx, ebx
mov rdx, rbx

mov dl, byte ptr [rbx - 4]
mov dl, byte ptr [rbx + 4]
mov dx, word ptr [rbx + 4]
mov edx, dword ptr [rbx + 4]
mov rdx, qword ptr [rbx + 4]

mov byte ptr [rdx - 4], bl
mov byte ptr [rdx + 4], bl
mov word ptr [rdx + 4], bx
mov dword ptr [rdx + 4], ebx
mov qword ptr [rdx + 4], rbx

movzx dx, bl
movzx dx, byte ptr [rbx + 4]

movzx edx, bl
movzx edx, bx
movzx edx, byte ptr [rbx + 4]
movzx edx, word ptr [rbx + 4]

movzx rdx, bl
movzx rdx, bx
movzx rdx, byte ptr [rbx + 4]
movzx rdx, word ptr [rbx + 4]


; Source: rbx, target: rbx
mov bl, 1
mov bx, 1
mov ebx, 1
mov rbx, 1

mov bl, bl
mov bx, bx
mov ebx, ebx
mov rbx, rbx

mov bl, byte ptr [rbx - 4]
mov bl, byte ptr [rbx + 4]
mov bx, word ptr [rbx + 4]
mov ebx, dword ptr [rbx + 4]
mov rbx, qword ptr [rbx + 4]

mov byte ptr [rbx - 4], bl
mov byte ptr [rbx + 4], bl
mov word ptr [rbx + 4], bx
mov dword ptr [rbx + 4], ebx
mov qword ptr [rbx + 4], rbx

movzx bx, bl
movzx bx, byte ptr [rbx + 4]

movzx ebx, bl
movzx ebx, bx
movzx ebx, byte ptr [rbx + 4]
movzx ebx, word ptr [rbx + 4]

movzx rbx, bl
movzx rbx, bx
movzx rbx, byte ptr [rbx + 4]
movzx rbx, word ptr [rbx + 4]


; Source: rbx, target: rsp
mov spl, 1
mov sp, 1
mov esp, 1
mov rsp, 1

mov spl, bl
mov sp, bx
mov esp, ebx
mov rsp, rbx

mov spl, byte ptr [rbx - 4]
mov spl, byte ptr [rbx + 4]
mov sp, word ptr [rbx + 4]
mov esp, dword ptr [rbx + 4]
mov rsp, qword ptr [rbx + 4]

mov byte ptr [rsp - 4], bl
mov byte ptr [rsp + 4], bl
mov word ptr [rsp + 4], bx
mov dword ptr [rsp + 4], ebx
mov qword ptr [rsp + 4], rbx

movzx sp, bl
movzx sp, byte ptr [rbx + 4]

movzx esp, bl
movzx esp, bx
movzx esp, byte ptr [rbx + 4]
movzx esp, word ptr [rbx + 4]

movzx rsp, bl
movzx rsp, bx
movzx rsp, byte ptr [rbx + 4]
movzx rsp, word ptr [rbx + 4]


; Source: rbx, target: rbp
mov bpl, 1
mov bp, 1
mov ebp, 1
mov rbp, 1

mov bpl, bl
mov bp, bx
mov ebp, ebx
mov rbp, rbx

mov bpl, byte ptr [rbx - 4]
mov bpl, byte ptr [rbx + 4]
mov bp, word ptr [rbx + 4]
mov ebp, dword ptr [rbx + 4]
mov rbp, qword ptr [rbx + 4]

mov byte ptr [rbp - 4], bl
mov byte ptr [rbp + 4], bl
mov word ptr [rbp + 4], bx
mov dword ptr [rbp + 4], ebx
mov qword ptr [rbp + 4], rbx

movzx bp, bl
movzx bp, byte ptr [rbx + 4]

movzx ebp, bl
movzx ebp, bx
movzx ebp, byte ptr [rbx + 4]
movzx ebp, word ptr [rbx + 4]

movzx rbp, bl
movzx rbp, bx
movzx rbp, byte ptr [rbx + 4]
movzx rbp, word ptr [rbx + 4]


; Source: rbx, target: rsi
mov dil, 1
mov si, 1
mov esi, 1
mov rsi, 1

mov dil, bl
mov si, bx
mov esi, ebx
mov rsi, rbx

mov dil, byte ptr [rbx - 4]
mov dil, byte ptr [rbx + 4]
mov si, word ptr [rbx + 4]
mov esi, dword ptr [rbx + 4]
mov rsi, qword ptr [rbx + 4]

mov byte ptr [rsi - 4], bl
mov byte ptr [rsi + 4], bl
mov word ptr [rsi + 4], bx
mov dword ptr [rsi + 4], ebx
mov qword ptr [rsi + 4], rbx

movzx si, bl
movzx si, byte ptr [rbx + 4]

movzx esi, bl
movzx esi, bx
movzx esi, byte ptr [rbx + 4]
movzx esi, word ptr [rbx + 4]

movzx rsi, bl
movzx rsi, bx
movzx rsi, byte ptr [rbx + 4]
movzx rsi, word ptr [rbx + 4]


; Source: rbx, target: rdi
mov sil, 1
mov di, 1
mov edi, 1
mov rdi, 1

mov sil, bl
mov di, bx
mov edi, ebx
mov rdi, rbx

mov sil, byte ptr [rbx - 4]
mov sil, byte ptr [rbx + 4]
mov di, word ptr [rbx + 4]
mov edi, dword ptr [rbx + 4]
mov rdi, qword ptr [rbx + 4]

mov byte ptr [rdi - 4], bl
mov byte ptr [rdi + 4], bl
mov word ptr [rdi + 4], bx
mov dword ptr [rdi + 4], ebx
mov qword ptr [rdi + 4], rbx

movzx di, bl
movzx di, byte ptr [rbx + 4]

movzx edi, bl
movzx edi, bx
movzx edi, byte ptr [rbx + 4]
movzx edi, word ptr [rbx + 4]

movzx rdi, bl
movzx rdi, bx
movzx rdi, byte ptr [rbx + 4]
movzx rdi, word ptr [rbx + 4]


; Source: rbx, target: r8
mov r8b, 1
mov r8w, 1
mov r8d, 1
mov r8, 1

mov r8b, bl
mov r8w, bx
mov r8d, ebx
mov r8, rbx

mov r8b, byte ptr [rbx - 4]
mov r8b, byte ptr [rbx + 4]
mov r8w, word ptr [rbx + 4]
mov r8d, dword ptr [rbx + 4]
mov r8, qword ptr [rbx + 4]

mov byte ptr [r8 - 4], bl
mov byte ptr [r8 + 4], bl
mov word ptr [r8 + 4], bx
mov dword ptr [r8 + 4], ebx
mov qword ptr [r8 + 4], rbx

movzx r8w, bl
movzx r8w, byte ptr [rbx + 4]

movzx r8d, bl
movzx r8d, bx
movzx r8d, byte ptr [rbx + 4]
movzx r8d, word ptr [rbx + 4]

movzx r8, bl
movzx r8, bx
movzx r8, byte ptr [rbx + 4]
movzx r8, word ptr [rbx + 4]


; Source: rbx, target: r9
mov r9b, 1
mov r9w, 1
mov r9d, 1
mov r9, 1

mov r9b, bl
mov r9w, bx
mov r9d, ebx
mov r9, rbx

mov r9b, byte ptr [rbx - 4]
mov r9b, byte ptr [rbx + 4]
mov r9w, word ptr [rbx + 4]
mov r9d, dword ptr [rbx + 4]
mov r9, qword ptr [rbx + 4]

mov byte ptr [r9 - 4], bl
mov byte ptr [r9 + 4], bl
mov word ptr [r9 + 4], bx
mov dword ptr [r9 + 4], ebx
mov qword ptr [r9 + 4], rbx

movzx r9w, bl
movzx r9w, byte ptr [rbx + 4]

movzx r9d, bl
movzx r9d, bx
movzx r9d, byte ptr [rbx + 4]
movzx r9d, word ptr [rbx + 4]

movzx r9, bl
movzx r9, bx
movzx r9, byte ptr [rbx + 4]
movzx r9, word ptr [rbx + 4]


; Source: rbx, target: r10
mov r10b, 1
mov r10w, 1
mov r10d, 1
mov r10, 1

mov r10b, bl
mov r10w, bx
mov r10d, ebx
mov r10, rbx

mov r10b, byte ptr [rbx - 4]
mov r10b, byte ptr [rbx + 4]
mov r10w, word ptr [rbx + 4]
mov r10d, dword ptr [rbx + 4]
mov r10, qword ptr [rbx + 4]

mov byte ptr [r10 - 4], bl
mov byte ptr [r10 + 4], bl
mov word ptr [r10 + 4], bx
mov dword ptr [r10 + 4], ebx
mov qword ptr [r10 + 4], rbx

movzx r10w, bl
movzx r10w, byte ptr [rbx + 4]

movzx r10d, bl
movzx r10d, bx
movzx r10d, byte ptr [rbx + 4]
movzx r10d, word ptr [rbx + 4]

movzx r10, bl
movzx r10, bx
movzx r10, byte ptr [rbx + 4]
movzx r10, word ptr [rbx + 4]


; Source: rbx, target: r11
mov r11b, 1
mov r11w, 1
mov r11d, 1
mov r11, 1

mov r11b, bl
mov r11w, bx
mov r11d, ebx
mov r11, rbx

mov r11b, byte ptr [rbx - 4]
mov r11b, byte ptr [rbx + 4]
mov r11w, word ptr [rbx + 4]
mov r11d, dword ptr [rbx + 4]
mov r11, qword ptr [rbx + 4]

mov byte ptr [r11 - 4], bl
mov byte ptr [r11 + 4], bl
mov word ptr [r11 + 4], bx
mov dword ptr [r11 + 4], ebx
mov qword ptr [r11 + 4], rbx

movzx r11w, bl
movzx r11w, byte ptr [rbx + 4]

movzx r11d, bl
movzx r11d, bx
movzx r11d, byte ptr [rbx + 4]
movzx r11d, word ptr [rbx + 4]

movzx r11, bl
movzx r11, bx
movzx r11, byte ptr [rbx + 4]
movzx r11, word ptr [rbx + 4]


; Source: rbx, target: r12
mov r12b, 1
mov r12w, 1
mov r12d, 1
mov r12, 1

mov r12b, bl
mov r12w, bx
mov r12d, ebx
mov r12, rbx

mov r12b, byte ptr [rbx - 4]
mov r12b, byte ptr [rbx + 4]
mov r12w, word ptr [rbx + 4]
mov r12d, dword ptr [rbx + 4]
mov r12, qword ptr [rbx + 4]

mov byte ptr [r12 - 4], bl
mov byte ptr [r12 + 4], bl
mov word ptr [r12 + 4], bx
mov dword ptr [r12 + 4], ebx
mov qword ptr [r12 + 4], rbx

movzx r12w, bl
movzx r12w, byte ptr [rbx + 4]

movzx r12d, bl
movzx r12d, bx
movzx r12d, byte ptr [rbx + 4]
movzx r12d, word ptr [rbx + 4]

movzx r12, bl
movzx r12, bx
movzx r12, byte ptr [rbx + 4]
movzx r12, word ptr [rbx + 4]


; Source: rbx, target: r13
mov r13b, 1
mov r13w, 1
mov r13d, 1
mov r13, 1

mov r13b, bl
mov r13w, bx
mov r13d, ebx
mov r13, rbx

mov r13b, byte ptr [rbx - 4]
mov r13b, byte ptr [rbx + 4]
mov r13w, word ptr [rbx + 4]
mov r13d, dword ptr [rbx + 4]
mov r13, qword ptr [rbx + 4]

mov byte ptr [r13 - 4], bl
mov byte ptr [r13 + 4], bl
mov word ptr [r13 + 4], bx
mov dword ptr [r13 + 4], ebx
mov qword ptr [r13 + 4], rbx

movzx r13w, bl
movzx r13w, byte ptr [rbx + 4]

movzx r13d, bl
movzx r13d, bx
movzx r13d, byte ptr [rbx + 4]
movzx r13d, word ptr [rbx + 4]

movzx r13, bl
movzx r13, bx
movzx r13, byte ptr [rbx + 4]
movzx r13, word ptr [rbx + 4]


; Source: rbx, target: r14
mov r14b, 1
mov r14w, 1
mov r14d, 1
mov r14, 1

mov r14b, bl
mov r14w, bx
mov r14d, ebx
mov r14, rbx

mov r14b, byte ptr [rbx - 4]
mov r14b, byte ptr [rbx + 4]
mov r14w, word ptr [rbx + 4]
mov r14d, dword ptr [rbx + 4]
mov r14, qword ptr [rbx + 4]

mov byte ptr [r14 - 4], bl
mov byte ptr [r14 + 4], bl
mov word ptr [r14 + 4], bx
mov dword ptr [r14 + 4], ebx
mov qword ptr [r14 + 4], rbx

movzx r14w, bl
movzx r14w, byte ptr [rbx + 4]

movzx r14d, bl
movzx r14d, bx
movzx r14d, byte ptr [rbx + 4]
movzx r14d, word ptr [rbx + 4]

movzx r14, bl
movzx r14, bx
movzx r14, byte ptr [rbx + 4]
movzx r14, word ptr [rbx + 4]


; Source: rbx, target: r15
mov r15b, 1
mov r15w, 1
mov r15d, 1
mov r15, 1

mov r15b, bl
mov r15w, bx
mov r15d, ebx
mov r15, rbx

mov r15b, byte ptr [rbx - 4]
mov r15b, byte ptr [rbx + 4]
mov r15w, word ptr [rbx + 4]
mov r15d, dword ptr [rbx + 4]
mov r15, qword ptr [rbx + 4]

mov byte ptr [r15 - 4], bl
mov byte ptr [r15 + 4], bl
mov word ptr [r15 + 4], bx
mov dword ptr [r15 + 4], ebx
mov qword ptr [r15 + 4], rbx

movzx r15w, bl
movzx r15w, byte ptr [rbx + 4]

movzx r15d, bl
movzx r15d, bx
movzx r15d, byte ptr [rbx + 4]
movzx r15d, word ptr [rbx + 4]

movzx r15, bl
movzx r15, bx
movzx r15, byte ptr [rbx + 4]
movzx r15, word ptr [rbx + 4]


; Source: rsp, target: rax
mov al, 1
mov ax, 1
mov eax, 1
mov rax, 1

mov al, spl
mov ax, sp
mov eax, esp
mov rax, rsp

mov al, byte ptr [rsp - 4]
mov al, byte ptr [rsp + 4]
mov ax, word ptr [rsp + 4]
mov eax, dword ptr [rsp + 4]
mov rax, qword ptr [rsp + 4]

mov byte ptr [rax - 4], spl
mov byte ptr [rax + 4], spl
mov word ptr [rax + 4], sp
mov dword ptr [rax + 4], esp
mov qword ptr [rax + 4], rsp

movzx ax, spl
movzx ax, byte ptr [rsp + 4]

movzx eax, spl
movzx eax, sp
movzx eax, byte ptr [rsp + 4]
movzx eax, word ptr [rsp + 4]

movzx rax, spl
movzx rax, sp
movzx rax, byte ptr [rsp + 4]
movzx rax, word ptr [rsp + 4]


; Source: rsp, target: rcx
mov cl, 1
mov cx, 1
mov ecx, 1
mov rcx, 1

mov cl, spl
mov cx, sp
mov ecx, esp
mov rcx, rsp

mov cl, byte ptr [rsp - 4]
mov cl, byte ptr [rsp + 4]
mov cx, word ptr [rsp + 4]
mov ecx, dword ptr [rsp + 4]
mov rcx, qword ptr [rsp + 4]

mov byte ptr [rcx - 4], spl
mov byte ptr [rcx + 4], spl
mov word ptr [rcx + 4], sp
mov dword ptr [rcx + 4], esp
mov qword ptr [rcx + 4], rsp

movzx cx, spl
movzx cx, byte ptr [rsp + 4]

movzx ecx, spl
movzx ecx, sp
movzx ecx, byte ptr [rsp + 4]
movzx ecx, word ptr [rsp + 4]

movzx rcx, spl
movzx rcx, sp
movzx rcx, byte ptr [rsp + 4]
movzx rcx, word ptr [rsp + 4]


; Source: rsp, target: rdx
mov dl, 1
mov dx, 1
mov edx, 1
mov rdx, 1

mov dl, spl
mov dx, sp
mov edx, esp
mov rdx, rsp

mov dl, byte ptr [rsp - 4]
mov dl, byte ptr [rsp + 4]
mov dx, word ptr [rsp + 4]
mov edx, dword ptr [rsp + 4]
mov rdx, qword ptr [rsp + 4]

mov byte ptr [rdx - 4], spl
mov byte ptr [rdx + 4], spl
mov word ptr [rdx + 4], sp
mov dword ptr [rdx + 4], esp
mov qword ptr [rdx + 4], rsp

movzx dx, spl
movzx dx, byte ptr [rsp + 4]

movzx edx, spl
movzx edx, sp
movzx edx, byte ptr [rsp + 4]
movzx edx, word ptr [rsp + 4]

movzx rdx, spl
movzx rdx, sp
movzx rdx, byte ptr [rsp + 4]
movzx rdx, word ptr [rsp + 4]


; Source: rsp, target: rbx
mov bl, 1
mov bx, 1
mov ebx, 1
mov rbx, 1

mov bl, spl
mov bx, sp
mov ebx, esp
mov rbx, rsp

mov bl, byte ptr [rsp - 4]
mov bl, byte ptr [rsp + 4]
mov bx, word ptr [rsp + 4]
mov ebx, dword ptr [rsp + 4]
mov rbx, qword ptr [rsp + 4]

mov byte ptr [rbx - 4], spl
mov byte ptr [rbx + 4], spl
mov word ptr [rbx + 4], sp
mov dword ptr [rbx + 4], esp
mov qword ptr [rbx + 4], rsp

movzx bx, spl
movzx bx, byte ptr [rsp + 4]

movzx ebx, spl
movzx ebx, sp
movzx ebx, byte ptr [rsp + 4]
movzx ebx, word ptr [rsp + 4]

movzx rbx, spl
movzx rbx, sp
movzx rbx, byte ptr [rsp + 4]
movzx rbx, word ptr [rsp + 4]


; Source: rsp, target: rsp
mov spl, 1
mov sp, 1
mov esp, 1
mov rsp, 1

mov spl, spl
mov sp, sp
mov esp, esp
mov rsp, rsp

mov spl, byte ptr [rsp - 4]
mov spl, byte ptr [rsp + 4]
mov sp, word ptr [rsp + 4]
mov esp, dword ptr [rsp + 4]
mov rsp, qword ptr [rsp + 4]

mov byte ptr [rsp - 4], spl
mov byte ptr [rsp + 4], spl
mov word ptr [rsp + 4], sp
mov dword ptr [rsp + 4], esp
mov qword ptr [rsp + 4], rsp

movzx sp, spl
movzx sp, byte ptr [rsp + 4]

movzx esp, spl
movzx esp, sp
movzx esp, byte ptr [rsp + 4]
movzx esp, word ptr [rsp + 4]

movzx rsp, spl
movzx rsp, sp
movzx rsp, byte ptr [rsp + 4]
movzx rsp, word ptr [rsp + 4]


; Source: rsp, target: rbp
mov bpl, 1
mov bp, 1
mov ebp, 1
mov rbp, 1

mov bpl, spl
mov bp, sp
mov ebp, esp
mov rbp, rsp

mov bpl, byte ptr [rsp - 4]
mov bpl, byte ptr [rsp + 4]
mov bp, word ptr [rsp + 4]
mov ebp, dword ptr [rsp + 4]
mov rbp, qword ptr [rsp + 4]

mov byte ptr [rbp - 4], spl
mov byte ptr [rbp + 4], spl
mov word ptr [rbp + 4], sp
mov dword ptr [rbp + 4], esp
mov qword ptr [rbp + 4], rsp

movzx bp, spl
movzx bp, byte ptr [rsp + 4]

movzx ebp, spl
movzx ebp, sp
movzx ebp, byte ptr [rsp + 4]
movzx ebp, word ptr [rsp + 4]

movzx rbp, spl
movzx rbp, sp
movzx rbp, byte ptr [rsp + 4]
movzx rbp, word ptr [rsp + 4]


; Source: rsp, target: rsi
mov dil, 1
mov si, 1
mov esi, 1
mov rsi, 1

mov dil, spl
mov si, sp
mov esi, esp
mov rsi, rsp

mov dil, byte ptr [rsp - 4]
mov dil, byte ptr [rsp + 4]
mov si, word ptr [rsp + 4]
mov esi, dword ptr [rsp + 4]
mov rsi, qword ptr [rsp + 4]

mov byte ptr [rsi - 4], spl
mov byte ptr [rsi + 4], spl
mov word ptr [rsi + 4], sp
mov dword ptr [rsi + 4], esp
mov qword ptr [rsi + 4], rsp

movzx si, spl
movzx si, byte ptr [rsp + 4]

movzx esi, spl
movzx esi, sp
movzx esi, byte ptr [rsp + 4]
movzx esi, word ptr [rsp + 4]

movzx rsi, spl
movzx rsi, sp
movzx rsi, byte ptr [rsp + 4]
movzx rsi, word ptr [rsp + 4]


; Source: rsp, target: rdi
mov sil, 1
mov di, 1
mov edi, 1
mov rdi, 1

mov sil, spl
mov di, sp
mov edi, esp
mov rdi, rsp

mov sil, byte ptr [rsp - 4]
mov sil, byte ptr [rsp + 4]
mov di, word ptr [rsp + 4]
mov edi, dword ptr [rsp + 4]
mov rdi, qword ptr [rsp + 4]

mov byte ptr [rdi - 4], spl
mov byte ptr [rdi + 4], spl
mov word ptr [rdi + 4], sp
mov dword ptr [rdi + 4], esp
mov qword ptr [rdi + 4], rsp

movzx di, spl
movzx di, byte ptr [rsp + 4]

movzx edi, spl
movzx edi, sp
movzx edi, byte ptr [rsp + 4]
movzx edi, word ptr [rsp + 4]

movzx rdi, spl
movzx rdi, sp
movzx rdi, byte ptr [rsp + 4]
movzx rdi, word ptr [rsp + 4]


; Source: rsp, target: r8
mov r8b, 1
mov r8w, 1
mov r8d, 1
mov r8, 1

mov r8b, spl
mov r8w, sp
mov r8d, esp
mov r8, rsp

mov r8b, byte ptr [rsp - 4]
mov r8b, byte ptr [rsp + 4]
mov r8w, word ptr [rsp + 4]
mov r8d, dword ptr [rsp + 4]
mov r8, qword ptr [rsp + 4]

mov byte ptr [r8 - 4], spl
mov byte ptr [r8 + 4], spl
mov word ptr [r8 + 4], sp
mov dword ptr [r8 + 4], esp
mov qword ptr [r8 + 4], rsp

movzx r8w, spl
movzx r8w, byte ptr [rsp + 4]

movzx r8d, spl
movzx r8d, sp
movzx r8d, byte ptr [rsp + 4]
movzx r8d, word ptr [rsp + 4]

movzx r8, spl
movzx r8, sp
movzx r8, byte ptr [rsp + 4]
movzx r8, word ptr [rsp + 4]


; Source: rsp, target: r9
mov r9b, 1
mov r9w, 1
mov r9d, 1
mov r9, 1

mov r9b, spl
mov r9w, sp
mov r9d, esp
mov r9, rsp

mov r9b, byte ptr [rsp - 4]
mov r9b, byte ptr [rsp + 4]
mov r9w, word ptr [rsp + 4]
mov r9d, dword ptr [rsp + 4]
mov r9, qword ptr [rsp + 4]

mov byte ptr [r9 - 4], spl
mov byte ptr [r9 + 4], spl
mov word ptr [r9 + 4], sp
mov dword ptr [r9 + 4], esp
mov qword ptr [r9 + 4], rsp

movzx r9w, spl
movzx r9w, byte ptr [rsp + 4]

movzx r9d, spl
movzx r9d, sp
movzx r9d, byte ptr [rsp + 4]
movzx r9d, word ptr [rsp + 4]

movzx r9, spl
movzx r9, sp
movzx r9, byte ptr [rsp + 4]
movzx r9, word ptr [rsp + 4]


; Source: rsp, target: r10
mov r10b, 1
mov r10w, 1
mov r10d, 1
mov r10, 1

mov r10b, spl
mov r10w, sp
mov r10d, esp
mov r10, rsp

mov r10b, byte ptr [rsp - 4]
mov r10b, byte ptr [rsp + 4]
mov r10w, word ptr [rsp + 4]
mov r10d, dword ptr [rsp + 4]
mov r10, qword ptr [rsp + 4]

mov byte ptr [r10 - 4], spl
mov byte ptr [r10 + 4], spl
mov word ptr [r10 + 4], sp
mov dword ptr [r10 + 4], esp
mov qword ptr [r10 + 4], rsp

movzx r10w, spl
movzx r10w, byte ptr [rsp + 4]

movzx r10d, spl
movzx r10d, sp
movzx r10d, byte ptr [rsp + 4]
movzx r10d, word ptr [rsp + 4]

movzx r10, spl
movzx r10, sp
movzx r10, byte ptr [rsp + 4]
movzx r10, word ptr [rsp + 4]


; Source: rsp, target: r11
mov r11b, 1
mov r11w, 1
mov r11d, 1
mov r11, 1

mov r11b, spl
mov r11w, sp
mov r11d, esp
mov r11, rsp

mov r11b, byte ptr [rsp - 4]
mov r11b, byte ptr [rsp + 4]
mov r11w, word ptr [rsp + 4]
mov r11d, dword ptr [rsp + 4]
mov r11, qword ptr [rsp + 4]

mov byte ptr [r11 - 4], spl
mov byte ptr [r11 + 4], spl
mov word ptr [r11 + 4], sp
mov dword ptr [r11 + 4], esp
mov qword ptr [r11 + 4], rsp

movzx r11w, spl
movzx r11w, byte ptr [rsp + 4]

movzx r11d, spl
movzx r11d, sp
movzx r11d, byte ptr [rsp + 4]
movzx r11d, word ptr [rsp + 4]

movzx r11, spl
movzx r11, sp
movzx r11, byte ptr [rsp + 4]
movzx r11, word ptr [rsp + 4]


; Source: rsp, target: r12
mov r12b, 1
mov r12w, 1
mov r12d, 1
mov r12, 1

mov r12b, spl
mov r12w, sp
mov r12d, esp
mov r12, rsp

mov r12b, byte ptr [rsp - 4]
mov r12b, byte ptr [rsp + 4]
mov r12w, word ptr [rsp + 4]
mov r12d, dword ptr [rsp + 4]
mov r12, qword ptr [rsp + 4]

mov byte ptr [r12 - 4], spl
mov byte ptr [r12 + 4], spl
mov word ptr [r12 + 4], sp
mov dword ptr [r12 + 4], esp
mov qword ptr [r12 + 4], rsp

movzx r12w, spl
movzx r12w, byte ptr [rsp + 4]

movzx r12d, spl
movzx r12d, sp
movzx r12d, byte ptr [rsp + 4]
movzx r12d, word ptr [rsp + 4]

movzx r12, spl
movzx r12, sp
movzx r12, byte ptr [rsp + 4]
movzx r12, word ptr [rsp + 4]


; Source: rsp, target: r13
mov r13b, 1
mov r13w, 1
mov r13d, 1
mov r13, 1

mov r13b, spl
mov r13w, sp
mov r13d, esp
mov r13, rsp

mov r13b, byte ptr [rsp - 4]
mov r13b, byte ptr [rsp + 4]
mov r13w, word ptr [rsp + 4]
mov r13d, dword ptr [rsp + 4]
mov r13, qword ptr [rsp + 4]

mov byte ptr [r13 - 4], spl
mov byte ptr [r13 + 4], spl
mov word ptr [r13 + 4], sp
mov dword ptr [r13 + 4], esp
mov qword ptr [r13 + 4], rsp

movzx r13w, spl
movzx r13w, byte ptr [rsp + 4]

movzx r13d, spl
movzx r13d, sp
movzx r13d, byte ptr [rsp + 4]
movzx r13d, word ptr [rsp + 4]

movzx r13, spl
movzx r13, sp
movzx r13, byte ptr [rsp + 4]
movzx r13, word ptr [rsp + 4]


; Source: rsp, target: r14
mov r14b, 1
mov r14w, 1
mov r14d, 1
mov r14, 1

mov r14b, spl
mov r14w, sp
mov r14d, esp
mov r14, rsp

mov r14b, byte ptr [rsp - 4]
mov r14b, byte ptr [rsp + 4]
mov r14w, word ptr [rsp + 4]
mov r14d, dword ptr [rsp + 4]
mov r14, qword ptr [rsp + 4]

mov byte ptr [r14 - 4], spl
mov byte ptr [r14 + 4], spl
mov word ptr [r14 + 4], sp
mov dword ptr [r14 + 4], esp
mov qword ptr [r14 + 4], rsp

movzx r14w, spl
movzx r14w, byte ptr [rsp + 4]

movzx r14d, spl
movzx r14d, sp
movzx r14d, byte ptr [rsp + 4]
movzx r14d, word ptr [rsp + 4]

movzx r14, spl
movzx r14, sp
movzx r14, byte ptr [rsp + 4]
movzx r14, word ptr [rsp + 4]


; Source: rsp, target: r15
mov r15b, 1
mov r15w, 1
mov r15d, 1
mov r15, 1

mov r15b, spl
mov r15w, sp
mov r15d, esp
mov r15, rsp

mov r15b, byte ptr [rsp - 4]
mov r15b, byte ptr [rsp + 4]
mov r15w, word ptr [rsp + 4]
mov r15d, dword ptr [rsp + 4]
mov r15, qword ptr [rsp + 4]

mov byte ptr [r15 - 4], spl
mov byte ptr [r15 + 4], spl
mov word ptr [r15 + 4], sp
mov dword ptr [r15 + 4], esp
mov qword ptr [r15 + 4], rsp

movzx r15w, spl
movzx r15w, byte ptr [rsp + 4]

movzx r15d, spl
movzx r15d, sp
movzx r15d, byte ptr [rsp + 4]
movzx r15d, word ptr [rsp + 4]

movzx r15, spl
movzx r15, sp
movzx r15, byte ptr [rsp + 4]
movzx r15, word ptr [rsp + 4]


; Source: rbp, target: rax
mov al, 1
mov ax, 1
mov eax, 1
mov rax, 1

mov al, bpl
mov ax, bp
mov eax, ebp
mov rax, rbp

mov al, byte ptr [rbp - 4]
mov al, byte ptr [rbp + 4]
mov ax, word ptr [rbp + 4]
mov eax, dword ptr [rbp + 4]
mov rax, qword ptr [rbp + 4]

mov byte ptr [rax - 4], bpl
mov byte ptr [rax + 4], bpl
mov word ptr [rax + 4], bp
mov dword ptr [rax + 4], ebp
mov qword ptr [rax + 4], rbp

movzx ax, bpl
movzx ax, byte ptr [rbp + 4]

movzx eax, bpl
movzx eax, bp
movzx eax, byte ptr [rbp + 4]
movzx eax, word ptr [rbp + 4]

movzx rax, bpl
movzx rax, bp
movzx rax, byte ptr [rbp + 4]
movzx rax, word ptr [rbp + 4]


; Source: rbp, target: rcx
mov cl, 1
mov cx, 1
mov ecx, 1
mov rcx, 1

mov cl, bpl
mov cx, bp
mov ecx, ebp
mov rcx, rbp

mov cl, byte ptr [rbp - 4]
mov cl, byte ptr [rbp + 4]
mov cx, word ptr [rbp + 4]
mov ecx, dword ptr [rbp + 4]
mov rcx, qword ptr [rbp + 4]

mov byte ptr [rcx - 4], bpl
mov byte ptr [rcx + 4], bpl
mov word ptr [rcx + 4], bp
mov dword ptr [rcx + 4], ebp
mov qword ptr [rcx + 4], rbp

movzx cx, bpl
movzx cx, byte ptr [rbp + 4]

movzx ecx, bpl
movzx ecx, bp
movzx ecx, byte ptr [rbp + 4]
movzx ecx, word ptr [rbp + 4]

movzx rcx, bpl
movzx rcx, bp
movzx rcx, byte ptr [rbp + 4]
movzx rcx, word ptr [rbp + 4]


; Source: rbp, target: rdx
mov dl, 1
mov dx, 1
mov edx, 1
mov rdx, 1

mov dl, bpl
mov dx, bp
mov edx, ebp
mov rdx, rbp

mov dl, byte ptr [rbp - 4]
mov dl, byte ptr [rbp + 4]
mov dx, word ptr [rbp + 4]
mov edx, dword ptr [rbp + 4]
mov rdx, qword ptr [rbp + 4]

mov byte ptr [rdx - 4], bpl
mov byte ptr [rdx + 4], bpl
mov word ptr [rdx + 4], bp
mov dword ptr [rdx + 4], ebp
mov qword ptr [rdx + 4], rbp

movzx dx, bpl
movzx dx, byte ptr [rbp + 4]

movzx edx, bpl
movzx edx, bp
movzx edx, byte ptr [rbp + 4]
movzx edx, word ptr [rbp + 4]

movzx rdx, bpl
movzx rdx, bp
movzx rdx, byte ptr [rbp + 4]
movzx rdx, word ptr [rbp + 4]


; Source: rbp, target: rbx
mov bl, 1
mov bx, 1
mov ebx, 1
mov rbx, 1

mov bl, bpl
mov bx, bp
mov ebx, ebp
mov rbx, rbp

mov bl, byte ptr [rbp - 4]
mov bl, byte ptr [rbp + 4]
mov bx, word ptr [rbp + 4]
mov ebx, dword ptr [rbp + 4]
mov rbx, qword ptr [rbp + 4]

mov byte ptr [rbx - 4], bpl
mov byte ptr [rbx + 4], bpl
mov word ptr [rbx + 4], bp
mov dword ptr [rbx + 4], ebp
mov qword ptr [rbx + 4], rbp

movzx bx, bpl
movzx bx, byte ptr [rbp + 4]

movzx ebx, bpl
movzx ebx, bp
movzx ebx, byte ptr [rbp + 4]
movzx ebx, word ptr [rbp + 4]

movzx rbx, bpl
movzx rbx, bp
movzx rbx, byte ptr [rbp + 4]
movzx rbx, word ptr [rbp + 4]


; Source: rbp, target: rsp
mov spl, 1
mov sp, 1
mov esp, 1
mov rsp, 1

mov spl, bpl
mov sp, bp
mov esp, ebp
mov rsp, rbp

mov spl, byte ptr [rbp - 4]
mov spl, byte ptr [rbp + 4]
mov sp, word ptr [rbp + 4]
mov esp, dword ptr [rbp + 4]
mov rsp, qword ptr [rbp + 4]

mov byte ptr [rsp - 4], bpl
mov byte ptr [rsp + 4], bpl
mov word ptr [rsp + 4], bp
mov dword ptr [rsp + 4], ebp
mov qword ptr [rsp + 4], rbp

movzx sp, bpl
movzx sp, byte ptr [rbp + 4]

movzx esp, bpl
movzx esp, bp
movzx esp, byte ptr [rbp + 4]
movzx esp, word ptr [rbp + 4]

movzx rsp, bpl
movzx rsp, bp
movzx rsp, byte ptr [rbp + 4]
movzx rsp, word ptr [rbp + 4]


; Source: rbp, target: rbp
mov bpl, 1
mov bp, 1
mov ebp, 1
mov rbp, 1

mov bpl, bpl
mov bp, bp
mov ebp, ebp
mov rbp, rbp

mov bpl, byte ptr [rbp - 4]
mov bpl, byte ptr [rbp + 4]
mov bp, word ptr [rbp + 4]
mov ebp, dword ptr [rbp + 4]
mov rbp, qword ptr [rbp + 4]

mov byte ptr [rbp - 4], bpl
mov byte ptr [rbp + 4], bpl
mov word ptr [rbp + 4], bp
mov dword ptr [rbp + 4], ebp
mov qword ptr [rbp + 4], rbp

movzx bp, bpl
movzx bp, byte ptr [rbp + 4]

movzx ebp, bpl
movzx ebp, bp
movzx ebp, byte ptr [rbp + 4]
movzx ebp, word ptr [rbp + 4]

movzx rbp, bpl
movzx rbp, bp
movzx rbp, byte ptr [rbp + 4]
movzx rbp, word ptr [rbp + 4]


; Source: rbp, target: rsi
mov dil, 1
mov si, 1
mov esi, 1
mov rsi, 1

mov dil, bpl
mov si, bp
mov esi, ebp
mov rsi, rbp

mov dil, byte ptr [rbp - 4]
mov dil, byte ptr [rbp + 4]
mov si, word ptr [rbp + 4]
mov esi, dword ptr [rbp + 4]
mov rsi, qword ptr [rbp + 4]

mov byte ptr [rsi - 4], bpl
mov byte ptr [rsi + 4], bpl
mov word ptr [rsi + 4], bp
mov dword ptr [rsi + 4], ebp
mov qword ptr [rsi + 4], rbp

movzx si, bpl
movzx si, byte ptr [rbp + 4]

movzx esi, bpl
movzx esi, bp
movzx esi, byte ptr [rbp + 4]
movzx esi, word ptr [rbp + 4]

movzx rsi, bpl
movzx rsi, bp
movzx rsi, byte ptr [rbp + 4]
movzx rsi, word ptr [rbp + 4]


; Source: rbp, target: rdi
mov sil, 1
mov di, 1
mov edi, 1
mov rdi, 1

mov sil, bpl
mov di, bp
mov edi, ebp
mov rdi, rbp

mov sil, byte ptr [rbp - 4]
mov sil, byte ptr [rbp + 4]
mov di, word ptr [rbp + 4]
mov edi, dword ptr [rbp + 4]
mov rdi, qword ptr [rbp + 4]

mov byte ptr [rdi - 4], bpl
mov byte ptr [rdi + 4], bpl
mov word ptr [rdi + 4], bp
mov dword ptr [rdi + 4], ebp
mov qword ptr [rdi + 4], rbp

movzx di, bpl
movzx di, byte ptr [rbp + 4]

movzx edi, bpl
movzx edi, bp
movzx edi, byte ptr [rbp + 4]
movzx edi, word ptr [rbp + 4]

movzx rdi, bpl
movzx rdi, bp
movzx rdi, byte ptr [rbp + 4]
movzx rdi, word ptr [rbp + 4]


; Source: rbp, target: r8
mov r8b, 1
mov r8w, 1
mov r8d, 1
mov r8, 1

mov r8b, bpl
mov r8w, bp
mov r8d, ebp
mov r8, rbp

mov r8b, byte ptr [rbp - 4]
mov r8b, byte ptr [rbp + 4]
mov r8w, word ptr [rbp + 4]
mov r8d, dword ptr [rbp + 4]
mov r8, qword ptr [rbp + 4]

mov byte ptr [r8 - 4], bpl
mov byte ptr [r8 + 4], bpl
mov word ptr [r8 + 4], bp
mov dword ptr [r8 + 4], ebp
mov qword ptr [r8 + 4], rbp

movzx r8w, bpl
movzx r8w, byte ptr [rbp + 4]

movzx r8d, bpl
movzx r8d, bp
movzx r8d, byte ptr [rbp + 4]
movzx r8d, word ptr [rbp + 4]

movzx r8, bpl
movzx r8, bp
movzx r8, byte ptr [rbp + 4]
movzx r8, word ptr [rbp + 4]


; Source: rbp, target: r9
mov r9b, 1
mov r9w, 1
mov r9d, 1
mov r9, 1

mov r9b, bpl
mov r9w, bp
mov r9d, ebp
mov r9, rbp

mov r9b, byte ptr [rbp - 4]
mov r9b, byte ptr [rbp + 4]
mov r9w, word ptr [rbp + 4]
mov r9d, dword ptr [rbp + 4]
mov r9, qword ptr [rbp + 4]

mov byte ptr [r9 - 4], bpl
mov byte ptr [r9 + 4], bpl
mov word ptr [r9 + 4], bp
mov dword ptr [r9 + 4], ebp
mov qword ptr [r9 + 4], rbp

movzx r9w, bpl
movzx r9w, byte ptr [rbp + 4]

movzx r9d, bpl
movzx r9d, bp
movzx r9d, byte ptr [rbp + 4]
movzx r9d, word ptr [rbp + 4]

movzx r9, bpl
movzx r9, bp
movzx r9, byte ptr [rbp + 4]
movzx r9, word ptr [rbp + 4]


; Source: rbp, target: r10
mov r10b, 1
mov r10w, 1
mov r10d, 1
mov r10, 1

mov r10b, bpl
mov r10w, bp
mov r10d, ebp
mov r10, rbp

mov r10b, byte ptr [rbp - 4]
mov r10b, byte ptr [rbp + 4]
mov r10w, word ptr [rbp + 4]
mov r10d, dword ptr [rbp + 4]
mov r10, qword ptr [rbp + 4]

mov byte ptr [r10 - 4], bpl
mov byte ptr [r10 + 4], bpl
mov word ptr [r10 + 4], bp
mov dword ptr [r10 + 4], ebp
mov qword ptr [r10 + 4], rbp

movzx r10w, bpl
movzx r10w, byte ptr [rbp + 4]

movzx r10d, bpl
movzx r10d, bp
movzx r10d, byte ptr [rbp + 4]
movzx r10d, word ptr [rbp + 4]

movzx r10, bpl
movzx r10, bp
movzx r10, byte ptr [rbp + 4]
movzx r10, word ptr [rbp + 4]


; Source: rbp, target: r11
mov r11b, 1
mov r11w, 1
mov r11d, 1
mov r11, 1

mov r11b, bpl
mov r11w, bp
mov r11d, ebp
mov r11, rbp

mov r11b, byte ptr [rbp - 4]
mov r11b, byte ptr [rbp + 4]
mov r11w, word ptr [rbp + 4]
mov r11d, dword ptr [rbp + 4]
mov r11, qword ptr [rbp + 4]

mov byte ptr [r11 - 4], bpl
mov byte ptr [r11 + 4], bpl
mov word ptr [r11 + 4], bp
mov dword ptr [r11 + 4], ebp
mov qword ptr [r11 + 4], rbp

movzx r11w, bpl
movzx r11w, byte ptr [rbp + 4]

movzx r11d, bpl
movzx r11d, bp
movzx r11d, byte ptr [rbp + 4]
movzx r11d, word ptr [rbp + 4]

movzx r11, bpl
movzx r11, bp
movzx r11, byte ptr [rbp + 4]
movzx r11, word ptr [rbp + 4]


; Source: rbp, target: r12
mov r12b, 1
mov r12w, 1
mov r12d, 1
mov r12, 1

mov r12b, bpl
mov r12w, bp
mov r12d, ebp
mov r12, rbp

mov r12b, byte ptr [rbp - 4]
mov r12b, byte ptr [rbp + 4]
mov r12w, word ptr [rbp + 4]
mov r12d, dword ptr [rbp + 4]
mov r12, qword ptr [rbp + 4]

mov byte ptr [r12 - 4], bpl
mov byte ptr [r12 + 4], bpl
mov word ptr [r12 + 4], bp
mov dword ptr [r12 + 4], ebp
mov qword ptr [r12 + 4], rbp

movzx r12w, bpl
movzx r12w, byte ptr [rbp + 4]

movzx r12d, bpl
movzx r12d, bp
movzx r12d, byte ptr [rbp + 4]
movzx r12d, word ptr [rbp + 4]

movzx r12, bpl
movzx r12, bp
movzx r12, byte ptr [rbp + 4]
movzx r12, word ptr [rbp + 4]


; Source: rbp, target: r13
mov r13b, 1
mov r13w, 1
mov r13d, 1
mov r13, 1

mov r13b, bpl
mov r13w, bp
mov r13d, ebp
mov r13, rbp

mov r13b, byte ptr [rbp - 4]
mov r13b, byte ptr [rbp + 4]
mov r13w, word ptr [rbp + 4]
mov r13d, dword ptr [rbp + 4]
mov r13, qword ptr [rbp + 4]

mov byte ptr [r13 - 4], bpl
mov byte ptr [r13 + 4], bpl
mov word ptr [r13 + 4], bp
mov dword ptr [r13 + 4], ebp
mov qword ptr [r13 + 4], rbp

movzx r13w, bpl
movzx r13w, byte ptr [rbp + 4]

movzx r13d, bpl
movzx r13d, bp
movzx r13d, byte ptr [rbp + 4]
movzx r13d, word ptr [rbp + 4]

movzx r13, bpl
movzx r13, bp
movzx r13, byte ptr [rbp + 4]
movzx r13, word ptr [rbp + 4]


; Source: rbp, target: r14
mov r14b, 1
mov r14w, 1
mov r14d, 1
mov r14, 1

mov r14b, bpl
mov r14w, bp
mov r14d, ebp
mov r14, rbp

mov r14b, byte ptr [rbp - 4]
mov r14b, byte ptr [rbp + 4]
mov r14w, word ptr [rbp + 4]
mov r14d, dword ptr [rbp + 4]
mov r14, qword ptr [rbp + 4]

mov byte ptr [r14 - 4], bpl
mov byte ptr [r14 + 4], bpl
mov word ptr [r14 + 4], bp
mov dword ptr [r14 + 4], ebp
mov qword ptr [r14 + 4], rbp

movzx r14w, bpl
movzx r14w, byte ptr [rbp + 4]

movzx r14d, bpl
movzx r14d, bp
movzx r14d, byte ptr [rbp + 4]
movzx r14d, word ptr [rbp + 4]

movzx r14, bpl
movzx r14, bp
movzx r14, byte ptr [rbp + 4]
movzx r14, word ptr [rbp + 4]


; Source: rbp, target: r15
mov r15b, 1
mov r15w, 1
mov r15d, 1
mov r15, 1

mov r15b, bpl
mov r15w, bp
mov r15d, ebp
mov r15, rbp

mov r15b, byte ptr [rbp - 4]
mov r15b, byte ptr [rbp + 4]
mov r15w, word ptr [rbp + 4]
mov r15d, dword ptr [rbp + 4]
mov r15, qword ptr [rbp + 4]

mov byte ptr [r15 - 4], bpl
mov byte ptr [r15 + 4], bpl
mov word ptr [r15 + 4], bp
mov dword ptr [r15 + 4], ebp
mov qword ptr [r15 + 4], rbp

movzx r15w, bpl
movzx r15w, byte ptr [rbp + 4]

movzx r15d, bpl
movzx r15d, bp
movzx r15d, byte ptr [rbp + 4]
movzx r15d, word ptr [rbp + 4]

movzx r15, bpl
movzx r15, bp
movzx r15, byte ptr [rbp + 4]
movzx r15, word ptr [rbp + 4]


; Source: rsi, target: rax
mov al, 1
mov ax, 1
mov eax, 1
mov rax, 1

mov al, dil
mov ax, si
mov eax, esi
mov rax, rsi

mov al, byte ptr [rsi - 4]
mov al, byte ptr [rsi + 4]
mov ax, word ptr [rsi + 4]
mov eax, dword ptr [rsi + 4]
mov rax, qword ptr [rsi + 4]

mov byte ptr [rax - 4], dil
mov byte ptr [rax + 4], dil
mov word ptr [rax + 4], si
mov dword ptr [rax + 4], esi
mov qword ptr [rax + 4], rsi

movzx ax, dil
movzx ax, byte ptr [rsi + 4]

movzx eax, dil
movzx eax, si
movzx eax, byte ptr [rsi + 4]
movzx eax, word ptr [rsi + 4]

movzx rax, dil
movzx rax, si
movzx rax, byte ptr [rsi + 4]
movzx rax, word ptr [rsi + 4]


; Source: rsi, target: rcx
mov cl, 1
mov cx, 1
mov ecx, 1
mov rcx, 1

mov cl, dil
mov cx, si
mov ecx, esi
mov rcx, rsi

mov cl, byte ptr [rsi - 4]
mov cl, byte ptr [rsi + 4]
mov cx, word ptr [rsi + 4]
mov ecx, dword ptr [rsi + 4]
mov rcx, qword ptr [rsi + 4]

mov byte ptr [rcx - 4], dil
mov byte ptr [rcx + 4], dil
mov word ptr [rcx + 4], si
mov dword ptr [rcx + 4], esi
mov qword ptr [rcx + 4], rsi

movzx cx, dil
movzx cx, byte ptr [rsi + 4]

movzx ecx, dil
movzx ecx, si
movzx ecx, byte ptr [rsi + 4]
movzx ecx, word ptr [rsi + 4]

movzx rcx, dil
movzx rcx, si
movzx rcx, byte ptr [rsi + 4]
movzx rcx, word ptr [rsi + 4]


; Source: rsi, target: rdx
mov dl, 1
mov dx, 1
mov edx, 1
mov rdx, 1

mov dl, dil
mov dx, si
mov edx, esi
mov rdx, rsi

mov dl, byte ptr [rsi - 4]
mov dl, byte ptr [rsi + 4]
mov dx, word ptr [rsi + 4]
mov edx, dword ptr [rsi + 4]
mov rdx, qword ptr [rsi + 4]

mov byte ptr [rdx - 4], dil
mov byte ptr [rdx + 4], dil
mov word ptr [rdx + 4], si
mov dword ptr [rdx + 4], esi
mov qword ptr [rdx + 4], rsi

movzx dx, dil
movzx dx, byte ptr [rsi + 4]

movzx edx, dil
movzx edx, si
movzx edx, byte ptr [rsi + 4]
movzx edx, word ptr [rsi + 4]

movzx rdx, dil
movzx rdx, si
movzx rdx, byte ptr [rsi + 4]
movzx rdx, word ptr [rsi + 4]


; Source: rsi, target: rbx
mov bl, 1
mov bx, 1
mov ebx, 1
mov rbx, 1

mov bl, dil
mov bx, si
mov ebx, esi
mov rbx, rsi

mov bl, byte ptr [rsi - 4]
mov bl, byte ptr [rsi + 4]
mov bx, word ptr [rsi + 4]
mov ebx, dword ptr [rsi + 4]
mov rbx, qword ptr [rsi + 4]

mov byte ptr [rbx - 4], dil
mov byte ptr [rbx + 4], dil
mov word ptr [rbx + 4], si
mov dword ptr [rbx + 4], esi
mov qword ptr [rbx + 4], rsi

movzx bx, dil
movzx bx, byte ptr [rsi + 4]

movzx ebx, dil
movzx ebx, si
movzx ebx, byte ptr [rsi + 4]
movzx ebx, word ptr [rsi + 4]

movzx rbx, dil
movzx rbx, si
movzx rbx, byte ptr [rsi + 4]
movzx rbx, word ptr [rsi + 4]


; Source: rsi, target: rsp
mov spl, 1
mov sp, 1
mov esp, 1
mov rsp, 1

mov spl, dil
mov sp, si
mov esp, esi
mov rsp, rsi

mov spl, byte ptr [rsi - 4]
mov spl, byte ptr [rsi + 4]
mov sp, word ptr [rsi + 4]
mov esp, dword ptr [rsi + 4]
mov rsp, qword ptr [rsi + 4]

mov byte ptr [rsp - 4], dil
mov byte ptr [rsp + 4], dil
mov word ptr [rsp + 4], si
mov dword ptr [rsp + 4], esi
mov qword ptr [rsp + 4], rsi

movzx sp, dil
movzx sp, byte ptr [rsi + 4]

movzx esp, dil
movzx esp, si
movzx esp, byte ptr [rsi + 4]
movzx esp, word ptr [rsi + 4]

movzx rsp, dil
movzx rsp, si
movzx rsp, byte ptr [rsi + 4]
movzx rsp, word ptr [rsi + 4]


; Source: rsi, target: rbp
mov bpl, 1
mov bp, 1
mov ebp, 1
mov rbp, 1

mov bpl, dil
mov bp, si
mov ebp, esi
mov rbp, rsi

mov bpl, byte ptr [rsi - 4]
mov bpl, byte ptr [rsi + 4]
mov bp, word ptr [rsi + 4]
mov ebp, dword ptr [rsi + 4]
mov rbp, qword ptr [rsi + 4]

mov byte ptr [rbp - 4], dil
mov byte ptr [rbp + 4], dil
mov word ptr [rbp + 4], si
mov dword ptr [rbp + 4], esi
mov qword ptr [rbp + 4], rsi

movzx bp, dil
movzx bp, byte ptr [rsi + 4]

movzx ebp, dil
movzx ebp, si
movzx ebp, byte ptr [rsi + 4]
movzx ebp, word ptr [rsi + 4]

movzx rbp, dil
movzx rbp, si
movzx rbp, byte ptr [rsi + 4]
movzx rbp, word ptr [rsi + 4]


; Source: rsi, target: rsi
mov dil, 1
mov si, 1
mov esi, 1
mov rsi, 1

mov dil, dil
mov si, si
mov esi, esi
mov rsi, rsi

mov dil, byte ptr [rsi - 4]
mov dil, byte ptr [rsi + 4]
mov si, word ptr [rsi + 4]
mov esi, dword ptr [rsi + 4]
mov rsi, qword ptr [rsi + 4]

mov byte ptr [rsi - 4], dil
mov byte ptr [rsi + 4], dil
mov word ptr [rsi + 4], si
mov dword ptr [rsi + 4], esi
mov qword ptr [rsi + 4], rsi

movzx si, dil
movzx si, byte ptr [rsi + 4]

movzx esi, dil
movzx esi, si
movzx esi, byte ptr [rsi + 4]
movzx esi, word ptr [rsi + 4]

movzx rsi, dil
movzx rsi, si
movzx rsi, byte ptr [rsi + 4]
movzx rsi, word ptr [rsi + 4]


; Source: rsi, target: rdi
mov sil, 1
mov di, 1
mov edi, 1
mov rdi, 1

mov sil, dil
mov di, si
mov edi, esi
mov rdi, rsi

mov sil, byte ptr [rsi - 4]
mov sil, byte ptr [rsi + 4]
mov di, word ptr [rsi + 4]
mov edi, dword ptr [rsi + 4]
mov rdi, qword ptr [rsi + 4]

mov byte ptr [rdi - 4], dil
mov byte ptr [rdi + 4], dil
mov word ptr [rdi + 4], si
mov dword ptr [rdi + 4], esi
mov qword ptr [rdi + 4], rsi

movzx di, dil
movzx di, byte ptr [rsi + 4]

movzx edi, dil
movzx edi, si
movzx edi, byte ptr [rsi + 4]
movzx edi, word ptr [rsi + 4]

movzx rdi, dil
movzx rdi, si
movzx rdi, byte ptr [rsi + 4]
movzx rdi, word ptr [rsi + 4]


; Source: rsi, target: r8
mov r8b, 1
mov r8w, 1
mov r8d, 1
mov r8, 1

mov r8b, dil
mov r8w, si
mov r8d, esi
mov r8, rsi

mov r8b, byte ptr [rsi - 4]
mov r8b, byte ptr [rsi + 4]
mov r8w, word ptr [rsi + 4]
mov r8d, dword ptr [rsi + 4]
mov r8, qword ptr [rsi + 4]

mov byte ptr [r8 - 4], dil
mov byte ptr [r8 + 4], dil
mov word ptr [r8 + 4], si
mov dword ptr [r8 + 4], esi
mov qword ptr [r8 + 4], rsi

movzx r8w, dil
movzx r8w, byte ptr [rsi + 4]

movzx r8d, dil
movzx r8d, si
movzx r8d, byte ptr [rsi + 4]
movzx r8d, word ptr [rsi + 4]

movzx r8, dil
movzx r8, si
movzx r8, byte ptr [rsi + 4]
movzx r8, word ptr [rsi + 4]


; Source: rsi, target: r9
mov r9b, 1
mov r9w, 1
mov r9d, 1
mov r9, 1

mov r9b, dil
mov r9w, si
mov r9d, esi
mov r9, rsi

mov r9b, byte ptr [rsi - 4]
mov r9b, byte ptr [rsi + 4]
mov r9w, word ptr [rsi + 4]
mov r9d, dword ptr [rsi + 4]
mov r9, qword ptr [rsi + 4]

mov byte ptr [r9 - 4], dil
mov byte ptr [r9 + 4], dil
mov word ptr [r9 + 4], si
mov dword ptr [r9 + 4], esi
mov qword ptr [r9 + 4], rsi

movzx r9w, dil
movzx r9w, byte ptr [rsi + 4]

movzx r9d, dil
movzx r9d, si
movzx r9d, byte ptr [rsi + 4]
movzx r9d, word ptr [rsi + 4]

movzx r9, dil
movzx r9, si
movzx r9, byte ptr [rsi + 4]
movzx r9, word ptr [rsi + 4]


; Source: rsi, target: r10
mov r10b, 1
mov r10w, 1
mov r10d, 1
mov r10, 1

mov r10b, dil
mov r10w, si
mov r10d, esi
mov r10, rsi

mov r10b, byte ptr [rsi - 4]
mov r10b, byte ptr [rsi + 4]
mov r10w, word ptr [rsi + 4]
mov r10d, dword ptr [rsi + 4]
mov r10, qword ptr [rsi + 4]

mov byte ptr [r10 - 4], dil
mov byte ptr [r10 + 4], dil
mov word ptr [r10 + 4], si
mov dword ptr [r10 + 4], esi
mov qword ptr [r10 + 4], rsi

movzx r10w, dil
movzx r10w, byte ptr [rsi + 4]

movzx r10d, dil
movzx r10d, si
movzx r10d, byte ptr [rsi + 4]
movzx r10d, word ptr [rsi + 4]

movzx r10, dil
movzx r10, si
movzx r10, byte ptr [rsi + 4]
movzx r10, word ptr [rsi + 4]


; Source: rsi, target: r11
mov r11b, 1
mov r11w, 1
mov r11d, 1
mov r11, 1

mov r11b, dil
mov r11w, si
mov r11d, esi
mov r11, rsi

mov r11b, byte ptr [rsi - 4]
mov r11b, byte ptr [rsi + 4]
mov r11w, word ptr [rsi + 4]
mov r11d, dword ptr [rsi + 4]
mov r11, qword ptr [rsi + 4]

mov byte ptr [r11 - 4], dil
mov byte ptr [r11 + 4], dil
mov word ptr [r11 + 4], si
mov dword ptr [r11 + 4], esi
mov qword ptr [r11 + 4], rsi

movzx r11w, dil
movzx r11w, byte ptr [rsi + 4]

movzx r11d, dil
movzx r11d, si
movzx r11d, byte ptr [rsi + 4]
movzx r11d, word ptr [rsi + 4]

movzx r11, dil
movzx r11, si
movzx r11, byte ptr [rsi + 4]
movzx r11, word ptr [rsi + 4]


; Source: rsi, target: r12
mov r12b, 1
mov r12w, 1
mov r12d, 1
mov r12, 1

mov r12b, dil
mov r12w, si
mov r12d, esi
mov r12, rsi

mov r12b, byte ptr [rsi - 4]
mov r12b, byte ptr [rsi + 4]
mov r12w, word ptr [rsi + 4]
mov r12d, dword ptr [rsi + 4]
mov r12, qword ptr [rsi + 4]

mov byte ptr [r12 - 4], dil
mov byte ptr [r12 + 4], dil
mov word ptr [r12 + 4], si
mov dword ptr [r12 + 4], esi
mov qword ptr [r12 + 4], rsi

movzx r12w, dil
movzx r12w, byte ptr [rsi + 4]

movzx r12d, dil
movzx r12d, si
movzx r12d, byte ptr [rsi + 4]
movzx r12d, word ptr [rsi + 4]

movzx r12, dil
movzx r12, si
movzx r12, byte ptr [rsi + 4]
movzx r12, word ptr [rsi + 4]


; Source: rsi, target: r13
mov r13b, 1
mov r13w, 1
mov r13d, 1
mov r13, 1

mov r13b, dil
mov r13w, si
mov r13d, esi
mov r13, rsi

mov r13b, byte ptr [rsi - 4]
mov r13b, byte ptr [rsi + 4]
mov r13w, word ptr [rsi + 4]
mov r13d, dword ptr [rsi + 4]
mov r13, qword ptr [rsi + 4]

mov byte ptr [r13 - 4], dil
mov byte ptr [r13 + 4], dil
mov word ptr [r13 + 4], si
mov dword ptr [r13 + 4], esi
mov qword ptr [r13 + 4], rsi

movzx r13w, dil
movzx r13w, byte ptr [rsi + 4]

movzx r13d, dil
movzx r13d, si
movzx r13d, byte ptr [rsi + 4]
movzx r13d, word ptr [rsi + 4]

movzx r13, dil
movzx r13, si
movzx r13, byte ptr [rsi + 4]
movzx r13, word ptr [rsi + 4]


; Source: rsi, target: r14
mov r14b, 1
mov r14w, 1
mov r14d, 1
mov r14, 1

mov r14b, dil
mov r14w, si
mov r14d, esi
mov r14, rsi

mov r14b, byte ptr [rsi - 4]
mov r14b, byte ptr [rsi + 4]
mov r14w, word ptr [rsi + 4]
mov r14d, dword ptr [rsi + 4]
mov r14, qword ptr [rsi + 4]

mov byte ptr [r14 - 4], dil
mov byte ptr [r14 + 4], dil
mov word ptr [r14 + 4], si
mov dword ptr [r14 + 4], esi
mov qword ptr [r14 + 4], rsi

movzx r14w, dil
movzx r14w, byte ptr [rsi + 4]

movzx r14d, dil
movzx r14d, si
movzx r14d, byte ptr [rsi + 4]
movzx r14d, word ptr [rsi + 4]

movzx r14, dil
movzx r14, si
movzx r14, byte ptr [rsi + 4]
movzx r14, word ptr [rsi + 4]


; Source: rsi, target: r15
mov r15b, 1
mov r15w, 1
mov r15d, 1
mov r15, 1

mov r15b, dil
mov r15w, si
mov r15d, esi
mov r15, rsi

mov r15b, byte ptr [rsi - 4]
mov r15b, byte ptr [rsi + 4]
mov r15w, word ptr [rsi + 4]
mov r15d, dword ptr [rsi + 4]
mov r15, qword ptr [rsi + 4]

mov byte ptr [r15 - 4], dil
mov byte ptr [r15 + 4], dil
mov word ptr [r15 + 4], si
mov dword ptr [r15 + 4], esi
mov qword ptr [r15 + 4], rsi

movzx r15w, dil
movzx r15w, byte ptr [rsi + 4]

movzx r15d, dil
movzx r15d, si
movzx r15d, byte ptr [rsi + 4]
movzx r15d, word ptr [rsi + 4]

movzx r15, dil
movzx r15, si
movzx r15, byte ptr [rsi + 4]
movzx r15, word ptr [rsi + 4]


; Source: rdi, target: rax
mov al, 1
mov ax, 1
mov eax, 1
mov rax, 1

mov al, sil
mov ax, di
mov eax, edi
mov rax, rdi

mov al, byte ptr [rdi - 4]
mov al, byte ptr [rdi + 4]
mov ax, word ptr [rdi + 4]
mov eax, dword ptr [rdi + 4]
mov rax, qword ptr [rdi + 4]

mov byte ptr [rax - 4], sil
mov byte ptr [rax + 4], sil
mov word ptr [rax + 4], di
mov dword ptr [rax + 4], edi
mov qword ptr [rax + 4], rdi

movzx ax, sil
movzx ax, byte ptr [rdi + 4]

movzx eax, sil
movzx eax, di
movzx eax, byte ptr [rdi + 4]
movzx eax, word ptr [rdi + 4]

movzx rax, sil
movzx rax, di
movzx rax, byte ptr [rdi + 4]
movzx rax, word ptr [rdi + 4]


; Source: rdi, target: rcx
mov cl, 1
mov cx, 1
mov ecx, 1
mov rcx, 1

mov cl, sil
mov cx, di
mov ecx, edi
mov rcx, rdi

mov cl, byte ptr [rdi - 4]
mov cl, byte ptr [rdi + 4]
mov cx, word ptr [rdi + 4]
mov ecx, dword ptr [rdi + 4]
mov rcx, qword ptr [rdi + 4]

mov byte ptr [rcx - 4], sil
mov byte ptr [rcx + 4], sil
mov word ptr [rcx + 4], di
mov dword ptr [rcx + 4], edi
mov qword ptr [rcx + 4], rdi

movzx cx, sil
movzx cx, byte ptr [rdi + 4]

movzx ecx, sil
movzx ecx, di
movzx ecx, byte ptr [rdi + 4]
movzx ecx, word ptr [rdi + 4]

movzx rcx, sil
movzx rcx, di
movzx rcx, byte ptr [rdi + 4]
movzx rcx, word ptr [rdi + 4]


; Source: rdi, target: rdx
mov dl, 1
mov dx, 1
mov edx, 1
mov rdx, 1

mov dl, sil
mov dx, di
mov edx, edi
mov rdx, rdi

mov dl, byte ptr [rdi - 4]
mov dl, byte ptr [rdi + 4]
mov dx, word ptr [rdi + 4]
mov edx, dword ptr [rdi + 4]
mov rdx, qword ptr [rdi + 4]

mov byte ptr [rdx - 4], sil
mov byte ptr [rdx + 4], sil
mov word ptr [rdx + 4], di
mov dword ptr [rdx + 4], edi
mov qword ptr [rdx + 4], rdi

movzx dx, sil
movzx dx, byte ptr [rdi + 4]

movzx edx, sil
movzx edx, di
movzx edx, byte ptr [rdi + 4]
movzx edx, word ptr [rdi + 4]

movzx rdx, sil
movzx rdx, di
movzx rdx, byte ptr [rdi + 4]
movzx rdx, word ptr [rdi + 4]


; Source: rdi, target: rbx
mov bl, 1
mov bx, 1
mov ebx, 1
mov rbx, 1

mov bl, sil
mov bx, di
mov ebx, edi
mov rbx, rdi

mov bl, byte ptr [rdi - 4]
mov bl, byte ptr [rdi + 4]
mov bx, word ptr [rdi + 4]
mov ebx, dword ptr [rdi + 4]
mov rbx, qword ptr [rdi + 4]

mov byte ptr [rbx - 4], sil
mov byte ptr [rbx + 4], sil
mov word ptr [rbx + 4], di
mov dword ptr [rbx + 4], edi
mov qword ptr [rbx + 4], rdi

movzx bx, sil
movzx bx, byte ptr [rdi + 4]

movzx ebx, sil
movzx ebx, di
movzx ebx, byte ptr [rdi + 4]
movzx ebx, word ptr [rdi + 4]

movzx rbx, sil
movzx rbx, di
movzx rbx, byte ptr [rdi + 4]
movzx rbx, word ptr [rdi + 4]


; Source: rdi, target: rsp
mov spl, 1
mov sp, 1
mov esp, 1
mov rsp, 1

mov spl, sil
mov sp, di
mov esp, edi
mov rsp, rdi

mov spl, byte ptr [rdi - 4]
mov spl, byte ptr [rdi + 4]
mov sp, word ptr [rdi + 4]
mov esp, dword ptr [rdi + 4]
mov rsp, qword ptr [rdi + 4]

mov byte ptr [rsp - 4], sil
mov byte ptr [rsp + 4], sil
mov word ptr [rsp + 4], di
mov dword ptr [rsp + 4], edi
mov qword ptr [rsp + 4], rdi

movzx sp, sil
movzx sp, byte ptr [rdi + 4]

movzx esp, sil
movzx esp, di
movzx esp, byte ptr [rdi + 4]
movzx esp, word ptr [rdi + 4]

movzx rsp, sil
movzx rsp, di
movzx rsp, byte ptr [rdi + 4]
movzx rsp, word ptr [rdi + 4]


; Source: rdi, target: rbp
mov bpl, 1
mov bp, 1
mov ebp, 1
mov rbp, 1

mov bpl, sil
mov bp, di
mov ebp, edi
mov rbp, rdi

mov bpl, byte ptr [rdi - 4]
mov bpl, byte ptr [rdi + 4]
mov bp, word ptr [rdi + 4]
mov ebp, dword ptr [rdi + 4]
mov rbp, qword ptr [rdi + 4]

mov byte ptr [rbp - 4], sil
mov byte ptr [rbp + 4], sil
mov word ptr [rbp + 4], di
mov dword ptr [rbp + 4], edi
mov qword ptr [rbp + 4], rdi

movzx bp, sil
movzx bp, byte ptr [rdi + 4]

movzx ebp, sil
movzx ebp, di
movzx ebp, byte ptr [rdi + 4]
movzx ebp, word ptr [rdi + 4]

movzx rbp, sil
movzx rbp, di
movzx rbp, byte ptr [rdi + 4]
movzx rbp, word ptr [rdi + 4]


; Source: rdi, target: rsi
mov dil, 1
mov si, 1
mov esi, 1
mov rsi, 1

mov dil, sil
mov si, di
mov esi, edi
mov rsi, rdi

mov dil, byte ptr [rdi - 4]
mov dil, byte ptr [rdi + 4]
mov si, word ptr [rdi + 4]
mov esi, dword ptr [rdi + 4]
mov rsi, qword ptr [rdi + 4]

mov byte ptr [rsi - 4], sil
mov byte ptr [rsi + 4], sil
mov word ptr [rsi + 4], di
mov dword ptr [rsi + 4], edi
mov qword ptr [rsi + 4], rdi

movzx si, sil
movzx si, byte ptr [rdi + 4]

movzx esi, sil
movzx esi, di
movzx esi, byte ptr [rdi + 4]
movzx esi, word ptr [rdi + 4]

movzx rsi, sil
movzx rsi, di
movzx rsi, byte ptr [rdi + 4]
movzx rsi, word ptr [rdi + 4]


; Source: rdi, target: rdi
mov sil, 1
mov di, 1
mov edi, 1
mov rdi, 1

mov sil, sil
mov di, di
mov edi, edi
mov rdi, rdi

mov sil, byte ptr [rdi - 4]
mov sil, byte ptr [rdi + 4]
mov di, word ptr [rdi + 4]
mov edi, dword ptr [rdi + 4]
mov rdi, qword ptr [rdi + 4]

mov byte ptr [rdi - 4], sil
mov byte ptr [rdi + 4], sil
mov word ptr [rdi + 4], di
mov dword ptr [rdi + 4], edi
mov qword ptr [rdi + 4], rdi

movzx di, sil
movzx di, byte ptr [rdi + 4]

movzx edi, sil
movzx edi, di
movzx edi, byte ptr [rdi + 4]
movzx edi, word ptr [rdi + 4]

movzx rdi, sil
movzx rdi, di
movzx rdi, byte ptr [rdi + 4]
movzx rdi, word ptr [rdi + 4]


; Source: rdi, target: r8
mov r8b, 1
mov r8w, 1
mov r8d, 1
mov r8, 1

mov r8b, sil
mov r8w, di
mov r8d, edi
mov r8, rdi

mov r8b, byte ptr [rdi - 4]
mov r8b, byte ptr [rdi + 4]
mov r8w, word ptr [rdi + 4]
mov r8d, dword ptr [rdi + 4]
mov r8, qword ptr [rdi + 4]

mov byte ptr [r8 - 4], sil
mov byte ptr [r8 + 4], sil
mov word ptr [r8 + 4], di
mov dword ptr [r8 + 4], edi
mov qword ptr [r8 + 4], rdi

movzx r8w, sil
movzx r8w, byte ptr [rdi + 4]

movzx r8d, sil
movzx r8d, di
movzx r8d, byte ptr [rdi + 4]
movzx r8d, word ptr [rdi + 4]

movzx r8, sil
movzx r8, di
movzx r8, byte ptr [rdi + 4]
movzx r8, word ptr [rdi + 4]


; Source: rdi, target: r9
mov r9b, 1
mov r9w, 1
mov r9d, 1
mov r9, 1

mov r9b, sil
mov r9w, di
mov r9d, edi
mov r9, rdi

mov r9b, byte ptr [rdi - 4]
mov r9b, byte ptr [rdi + 4]
mov r9w, word ptr [rdi + 4]
mov r9d, dword ptr [rdi + 4]
mov r9, qword ptr [rdi + 4]

mov byte ptr [r9 - 4], sil
mov byte ptr [r9 + 4], sil
mov word ptr [r9 + 4], di
mov dword ptr [r9 + 4], edi
mov qword ptr [r9 + 4], rdi

movzx r9w, sil
movzx r9w, byte ptr [rdi + 4]

movzx r9d, sil
movzx r9d, di
movzx r9d, byte ptr [rdi + 4]
movzx r9d, word ptr [rdi + 4]

movzx r9, sil
movzx r9, di
movzx r9, byte ptr [rdi + 4]
movzx r9, word ptr [rdi + 4]


; Source: rdi, target: r10
mov r10b, 1
mov r10w, 1
mov r10d, 1
mov r10, 1

mov r10b, sil
mov r10w, di
mov r10d, edi
mov r10, rdi

mov r10b, byte ptr [rdi - 4]
mov r10b, byte ptr [rdi + 4]
mov r10w, word ptr [rdi + 4]
mov r10d, dword ptr [rdi + 4]
mov r10, qword ptr [rdi + 4]

mov byte ptr [r10 - 4], sil
mov byte ptr [r10 + 4], sil
mov word ptr [r10 + 4], di
mov dword ptr [r10 + 4], edi
mov qword ptr [r10 + 4], rdi

movzx r10w, sil
movzx r10w, byte ptr [rdi + 4]

movzx r10d, sil
movzx r10d, di
movzx r10d, byte ptr [rdi + 4]
movzx r10d, word ptr [rdi + 4]

movzx r10, sil
movzx r10, di
movzx r10, byte ptr [rdi + 4]
movzx r10, word ptr [rdi + 4]


; Source: rdi, target: r11
mov r11b, 1
mov r11w, 1
mov r11d, 1
mov r11, 1

mov r11b, sil
mov r11w, di
mov r11d, edi
mov r11, rdi

mov r11b, byte ptr [rdi - 4]
mov r11b, byte ptr [rdi + 4]
mov r11w, word ptr [rdi + 4]
mov r11d, dword ptr [rdi + 4]
mov r11, qword ptr [rdi + 4]

mov byte ptr [r11 - 4], sil
mov byte ptr [r11 + 4], sil
mov word ptr [r11 + 4], di
mov dword ptr [r11 + 4], edi
mov qword ptr [r11 + 4], rdi

movzx r11w, sil
movzx r11w, byte ptr [rdi + 4]

movzx r11d, sil
movzx r11d, di
movzx r11d, byte ptr [rdi + 4]
movzx r11d, word ptr [rdi + 4]

movzx r11, sil
movzx r11, di
movzx r11, byte ptr [rdi + 4]
movzx r11, word ptr [rdi + 4]


; Source: rdi, target: r12
mov r12b, 1
mov r12w, 1
mov r12d, 1
mov r12, 1

mov r12b, sil
mov r12w, di
mov r12d, edi
mov r12, rdi

mov r12b, byte ptr [rdi - 4]
mov r12b, byte ptr [rdi + 4]
mov r12w, word ptr [rdi + 4]
mov r12d, dword ptr [rdi + 4]
mov r12, qword ptr [rdi + 4]

mov byte ptr [r12 - 4], sil
mov byte ptr [r12 + 4], sil
mov word ptr [r12 + 4], di
mov dword ptr [r12 + 4], edi
mov qword ptr [r12 + 4], rdi

movzx r12w, sil
movzx r12w, byte ptr [rdi + 4]

movzx r12d, sil
movzx r12d, di
movzx r12d, byte ptr [rdi + 4]
movzx r12d, word ptr [rdi + 4]

movzx r12, sil
movzx r12, di
movzx r12, byte ptr [rdi + 4]
movzx r12, word ptr [rdi + 4]


; Source: rdi, target: r13
mov r13b, 1
mov r13w, 1
mov r13d, 1
mov r13, 1

mov r13b, sil
mov r13w, di
mov r13d, edi
mov r13, rdi

mov r13b, byte ptr [rdi - 4]
mov r13b, byte ptr [rdi + 4]
mov r13w, word ptr [rdi + 4]
mov r13d, dword ptr [rdi + 4]
mov r13, qword ptr [rdi + 4]

mov byte ptr [r13 - 4], sil
mov byte ptr [r13 + 4], sil
mov word ptr [r13 + 4], di
mov dword ptr [r13 + 4], edi
mov qword ptr [r13 + 4], rdi

movzx r13w, sil
movzx r13w, byte ptr [rdi + 4]

movzx r13d, sil
movzx r13d, di
movzx r13d, byte ptr [rdi + 4]
movzx r13d, word ptr [rdi + 4]

movzx r13, sil
movzx r13, di
movzx r13, byte ptr [rdi + 4]
movzx r13, word ptr [rdi + 4]


; Source: rdi, target: r14
mov r14b, 1
mov r14w, 1
mov r14d, 1
mov r14, 1

mov r14b, sil
mov r14w, di
mov r14d, edi
mov r14, rdi

mov r14b, byte ptr [rdi - 4]
mov r14b, byte ptr [rdi + 4]
mov r14w, word ptr [rdi + 4]
mov r14d, dword ptr [rdi + 4]
mov r14, qword ptr [rdi + 4]

mov byte ptr [r14 - 4], sil
mov byte ptr [r14 + 4], sil
mov word ptr [r14 + 4], di
mov dword ptr [r14 + 4], edi
mov qword ptr [r14 + 4], rdi

movzx r14w, sil
movzx r14w, byte ptr [rdi + 4]

movzx r14d, sil
movzx r14d, di
movzx r14d, byte ptr [rdi + 4]
movzx r14d, word ptr [rdi + 4]

movzx r14, sil
movzx r14, di
movzx r14, byte ptr [rdi + 4]
movzx r14, word ptr [rdi + 4]


; Source: rdi, target: r15
mov r15b, 1
mov r15w, 1
mov r15d, 1
mov r15, 1

mov r15b, sil
mov r15w, di
mov r15d, edi
mov r15, rdi

mov r15b, byte ptr [rdi - 4]
mov r15b, byte ptr [rdi + 4]
mov r15w, word ptr [rdi + 4]
mov r15d, dword ptr [rdi + 4]
mov r15, qword ptr [rdi + 4]

mov byte ptr [r15 - 4], sil
mov byte ptr [r15 + 4], sil
mov word ptr [r15 + 4], di
mov dword ptr [r15 + 4], edi
mov qword ptr [r15 + 4], rdi

movzx r15w, sil
movzx r15w, byte ptr [rdi + 4]

movzx r15d, sil
movzx r15d, di
movzx r15d, byte ptr [rdi + 4]
movzx r15d, word ptr [rdi + 4]

movzx r15, sil
movzx r15, di
movzx r15, byte ptr [rdi + 4]
movzx r15, word ptr [rdi + 4]


; Source: r8, target: rax
mov al, 1
mov ax, 1
mov eax, 1
mov rax, 1

mov al, r8b
mov ax, r8w
mov eax, r8d
mov rax, r8

mov al, byte ptr [r8 - 4]
mov al, byte ptr [r8 + 4]
mov ax, word ptr [r8 + 4]
mov eax, dword ptr [r8 + 4]
mov rax, qword ptr [r8 + 4]

mov byte ptr [rax - 4], r8b
mov byte ptr [rax + 4], r8b
mov word ptr [rax + 4], r8w
mov dword ptr [rax + 4], r8d
mov qword ptr [rax + 4], r8

movzx ax, r8b
movzx ax, byte ptr [r8 + 4]

movzx eax, r8b
movzx eax, r8w
movzx eax, byte ptr [r8 + 4]
movzx eax, word ptr [r8 + 4]

movzx rax, r8b
movzx rax, r8w
movzx rax, byte ptr [r8 + 4]
movzx rax, word ptr [r8 + 4]


; Source: r8, target: rcx
mov cl, 1
mov cx, 1
mov ecx, 1
mov rcx, 1

mov cl, r8b
mov cx, r8w
mov ecx, r8d
mov rcx, r8

mov cl, byte ptr [r8 - 4]
mov cl, byte ptr [r8 + 4]
mov cx, word ptr [r8 + 4]
mov ecx, dword ptr [r8 + 4]
mov rcx, qword ptr [r8 + 4]

mov byte ptr [rcx - 4], r8b
mov byte ptr [rcx + 4], r8b
mov word ptr [rcx + 4], r8w
mov dword ptr [rcx + 4], r8d
mov qword ptr [rcx + 4], r8

movzx cx, r8b
movzx cx, byte ptr [r8 + 4]

movzx ecx, r8b
movzx ecx, r8w
movzx ecx, byte ptr [r8 + 4]
movzx ecx, word ptr [r8 + 4]

movzx rcx, r8b
movzx rcx, r8w
movzx rcx, byte ptr [r8 + 4]
movzx rcx, word ptr [r8 + 4]


; Source: r8, target: rdx
mov dl, 1
mov dx, 1
mov edx, 1
mov rdx, 1

mov dl, r8b
mov dx, r8w
mov edx, r8d
mov rdx, r8

mov dl, byte ptr [r8 - 4]
mov dl, byte ptr [r8 + 4]
mov dx, word ptr [r8 + 4]
mov edx, dword ptr [r8 + 4]
mov rdx, qword ptr [r8 + 4]

mov byte ptr [rdx - 4], r8b
mov byte ptr [rdx + 4], r8b
mov word ptr [rdx + 4], r8w
mov dword ptr [rdx + 4], r8d
mov qword ptr [rdx + 4], r8

movzx dx, r8b
movzx dx, byte ptr [r8 + 4]

movzx edx, r8b
movzx edx, r8w
movzx edx, byte ptr [r8 + 4]
movzx edx, word ptr [r8 + 4]

movzx rdx, r8b
movzx rdx, r8w
movzx rdx, byte ptr [r8 + 4]
movzx rdx, word ptr [r8 + 4]


; Source: r8, target: rbx
mov bl, 1
mov bx, 1
mov ebx, 1
mov rbx, 1

mov bl, r8b
mov bx, r8w
mov ebx, r8d
mov rbx, r8

mov bl, byte ptr [r8 - 4]
mov bl, byte ptr [r8 + 4]
mov bx, word ptr [r8 + 4]
mov ebx, dword ptr [r8 + 4]
mov rbx, qword ptr [r8 + 4]

mov byte ptr [rbx - 4], r8b
mov byte ptr [rbx + 4], r8b
mov word ptr [rbx + 4], r8w
mov dword ptr [rbx + 4], r8d
mov qword ptr [rbx + 4], r8

movzx bx, r8b
movzx bx, byte ptr [r8 + 4]

movzx ebx, r8b
movzx ebx, r8w
movzx ebx, byte ptr [r8 + 4]
movzx ebx, word ptr [r8 + 4]

movzx rbx, r8b
movzx rbx, r8w
movzx rbx, byte ptr [r8 + 4]
movzx rbx, word ptr [r8 + 4]


; Source: r8, target: rsp
mov spl, 1
mov sp, 1
mov esp, 1
mov rsp, 1

mov spl, r8b
mov sp, r8w
mov esp, r8d
mov rsp, r8

mov spl, byte ptr [r8 - 4]
mov spl, byte ptr [r8 + 4]
mov sp, word ptr [r8 + 4]
mov esp, dword ptr [r8 + 4]
mov rsp, qword ptr [r8 + 4]

mov byte ptr [rsp - 4], r8b
mov byte ptr [rsp + 4], r8b
mov word ptr [rsp + 4], r8w
mov dword ptr [rsp + 4], r8d
mov qword ptr [rsp + 4], r8

movzx sp, r8b
movzx sp, byte ptr [r8 + 4]

movzx esp, r8b
movzx esp, r8w
movzx esp, byte ptr [r8 + 4]
movzx esp, word ptr [r8 + 4]

movzx rsp, r8b
movzx rsp, r8w
movzx rsp, byte ptr [r8 + 4]
movzx rsp, word ptr [r8 + 4]


; Source: r8, target: rbp
mov bpl, 1
mov bp, 1
mov ebp, 1
mov rbp, 1

mov bpl, r8b
mov bp, r8w
mov ebp, r8d
mov rbp, r8

mov bpl, byte ptr [r8 - 4]
mov bpl, byte ptr [r8 + 4]
mov bp, word ptr [r8 + 4]
mov ebp, dword ptr [r8 + 4]
mov rbp, qword ptr [r8 + 4]

mov byte ptr [rbp - 4], r8b
mov byte ptr [rbp + 4], r8b
mov word ptr [rbp + 4], r8w
mov dword ptr [rbp + 4], r8d
mov qword ptr [rbp + 4], r8

movzx bp, r8b
movzx bp, byte ptr [r8 + 4]

movzx ebp, r8b
movzx ebp, r8w
movzx ebp, byte ptr [r8 + 4]
movzx ebp, word ptr [r8 + 4]

movzx rbp, r8b
movzx rbp, r8w
movzx rbp, byte ptr [r8 + 4]
movzx rbp, word ptr [r8 + 4]


; Source: r8, target: rsi
mov dil, 1
mov si, 1
mov esi, 1
mov rsi, 1

mov dil, r8b
mov si, r8w
mov esi, r8d
mov rsi, r8

mov dil, byte ptr [r8 - 4]
mov dil, byte ptr [r8 + 4]
mov si, word ptr [r8 + 4]
mov esi, dword ptr [r8 + 4]
mov rsi, qword ptr [r8 + 4]

mov byte ptr [rsi - 4], r8b
mov byte ptr [rsi + 4], r8b
mov word ptr [rsi + 4], r8w
mov dword ptr [rsi + 4], r8d
mov qword ptr [rsi + 4], r8

movzx si, r8b
movzx si, byte ptr [r8 + 4]

movzx esi, r8b
movzx esi, r8w
movzx esi, byte ptr [r8 + 4]
movzx esi, word ptr [r8 + 4]

movzx rsi, r8b
movzx rsi, r8w
movzx rsi, byte ptr [r8 + 4]
movzx rsi, word ptr [r8 + 4]


; Source: r8, target: rdi
mov sil, 1
mov di, 1
mov edi, 1
mov rdi, 1

mov sil, r8b
mov di, r8w
mov edi, r8d
mov rdi, r8

mov sil, byte ptr [r8 - 4]
mov sil, byte ptr [r8 + 4]
mov di, word ptr [r8 + 4]
mov edi, dword ptr [r8 + 4]
mov rdi, qword ptr [r8 + 4]

mov byte ptr [rdi - 4], r8b
mov byte ptr [rdi + 4], r8b
mov word ptr [rdi + 4], r8w
mov dword ptr [rdi + 4], r8d
mov qword ptr [rdi + 4], r8

movzx di, r8b
movzx di, byte ptr [r8 + 4]

movzx edi, r8b
movzx edi, r8w
movzx edi, byte ptr [r8 + 4]
movzx edi, word ptr [r8 + 4]

movzx rdi, r8b
movzx rdi, r8w
movzx rdi, byte ptr [r8 + 4]
movzx rdi, word ptr [r8 + 4]


; Source: r8, target: r8
mov r8b, 1
mov r8w, 1
mov r8d, 1
mov r8, 1

mov r8b, r8b
mov r8w, r8w
mov r8d, r8d
mov r8, r8

mov r8b, byte ptr [r8 - 4]
mov r8b, byte ptr [r8 + 4]
mov r8w, word ptr [r8 + 4]
mov r8d, dword ptr [r8 + 4]
mov r8, qword ptr [r8 + 4]

mov byte ptr [r8 - 4], r8b
mov byte ptr [r8 + 4], r8b
mov word ptr [r8 + 4], r8w
mov dword ptr [r8 + 4], r8d
mov qword ptr [r8 + 4], r8

movzx r8w, r8b
movzx r8w, byte ptr [r8 + 4]

movzx r8d, r8b
movzx r8d, r8w
movzx r8d, byte ptr [r8 + 4]
movzx r8d, word ptr [r8 + 4]

movzx r8, r8b
movzx r8, r8w
movzx r8, byte ptr [r8 + 4]
movzx r8, word ptr [r8 + 4]


; Source: r8, target: r9
mov r9b, 1
mov r9w, 1
mov r9d, 1
mov r9, 1

mov r9b, r8b
mov r9w, r8w
mov r9d, r8d
mov r9, r8

mov r9b, byte ptr [r8 - 4]
mov r9b, byte ptr [r8 + 4]
mov r9w, word ptr [r8 + 4]
mov r9d, dword ptr [r8 + 4]
mov r9, qword ptr [r8 + 4]

mov byte ptr [r9 - 4], r8b
mov byte ptr [r9 + 4], r8b
mov word ptr [r9 + 4], r8w
mov dword ptr [r9 + 4], r8d
mov qword ptr [r9 + 4], r8

movzx r9w, r8b
movzx r9w, byte ptr [r8 + 4]

movzx r9d, r8b
movzx r9d, r8w
movzx r9d, byte ptr [r8 + 4]
movzx r9d, word ptr [r8 + 4]

movzx r9, r8b
movzx r9, r8w
movzx r9, byte ptr [r8 + 4]
movzx r9, word ptr [r8 + 4]


; Source: r8, target: r10
mov r10b, 1
mov r10w, 1
mov r10d, 1
mov r10, 1

mov r10b, r8b
mov r10w, r8w
mov r10d, r8d
mov r10, r8

mov r10b, byte ptr [r8 - 4]
mov r10b, byte ptr [r8 + 4]
mov r10w, word ptr [r8 + 4]
mov r10d, dword ptr [r8 + 4]
mov r10, qword ptr [r8 + 4]

mov byte ptr [r10 - 4], r8b
mov byte ptr [r10 + 4], r8b
mov word ptr [r10 + 4], r8w
mov dword ptr [r10 + 4], r8d
mov qword ptr [r10 + 4], r8

movzx r10w, r8b
movzx r10w, byte ptr [r8 + 4]

movzx r10d, r8b
movzx r10d, r8w
movzx r10d, byte ptr [r8 + 4]
movzx r10d, word ptr [r8 + 4]

movzx r10, r8b
movzx r10, r8w
movzx r10, byte ptr [r8 + 4]
movzx r10, word ptr [r8 + 4]


; Source: r8, target: r11
mov r11b, 1
mov r11w, 1
mov r11d, 1
mov r11, 1

mov r11b, r8b
mov r11w, r8w
mov r11d, r8d
mov r11, r8

mov r11b, byte ptr [r8 - 4]
mov r11b, byte ptr [r8 + 4]
mov r11w, word ptr [r8 + 4]
mov r11d, dword ptr [r8 + 4]
mov r11, qword ptr [r8 + 4]

mov byte ptr [r11 - 4], r8b
mov byte ptr [r11 + 4], r8b
mov word ptr [r11 + 4], r8w
mov dword ptr [r11 + 4], r8d
mov qword ptr [r11 + 4], r8

movzx r11w, r8b
movzx r11w, byte ptr [r8 + 4]

movzx r11d, r8b
movzx r11d, r8w
movzx r11d, byte ptr [r8 + 4]
movzx r11d, word ptr [r8 + 4]

movzx r11, r8b
movzx r11, r8w
movzx r11, byte ptr [r8 + 4]
movzx r11, word ptr [r8 + 4]


; Source: r8, target: r12
mov r12b, 1
mov r12w, 1
mov r12d, 1
mov r12, 1

mov r12b, r8b
mov r12w, r8w
mov r12d, r8d
mov r12, r8

mov r12b, byte ptr [r8 - 4]
mov r12b, byte ptr [r8 + 4]
mov r12w, word ptr [r8 + 4]
mov r12d, dword ptr [r8 + 4]
mov r12, qword ptr [r8 + 4]

mov byte ptr [r12 - 4], r8b
mov byte ptr [r12 + 4], r8b
mov word ptr [r12 + 4], r8w
mov dword ptr [r12 + 4], r8d
mov qword ptr [r12 + 4], r8

movzx r12w, r8b
movzx r12w, byte ptr [r8 + 4]

movzx r12d, r8b
movzx r12d, r8w
movzx r12d, byte ptr [r8 + 4]
movzx r12d, word ptr [r8 + 4]

movzx r12, r8b
movzx r12, r8w
movzx r12, byte ptr [r8 + 4]
movzx r12, word ptr [r8 + 4]


; Source: r8, target: r13
mov r13b, 1
mov r13w, 1
mov r13d, 1
mov r13, 1

mov r13b, r8b
mov r13w, r8w
mov r13d, r8d
mov r13, r8

mov r13b, byte ptr [r8 - 4]
mov r13b, byte ptr [r8 + 4]
mov r13w, word ptr [r8 + 4]
mov r13d, dword ptr [r8 + 4]
mov r13, qword ptr [r8 + 4]

mov byte ptr [r13 - 4], r8b
mov byte ptr [r13 + 4], r8b
mov word ptr [r13 + 4], r8w
mov dword ptr [r13 + 4], r8d
mov qword ptr [r13 + 4], r8

movzx r13w, r8b
movzx r13w, byte ptr [r8 + 4]

movzx r13d, r8b
movzx r13d, r8w
movzx r13d, byte ptr [r8 + 4]
movzx r13d, word ptr [r8 + 4]

movzx r13, r8b
movzx r13, r8w
movzx r13, byte ptr [r8 + 4]
movzx r13, word ptr [r8 + 4]


; Source: r8, target: r14
mov r14b, 1
mov r14w, 1
mov r14d, 1
mov r14, 1

mov r14b, r8b
mov r14w, r8w
mov r14d, r8d
mov r14, r8

mov r14b, byte ptr [r8 - 4]
mov r14b, byte ptr [r8 + 4]
mov r14w, word ptr [r8 + 4]
mov r14d, dword ptr [r8 + 4]
mov r14, qword ptr [r8 + 4]

mov byte ptr [r14 - 4], r8b
mov byte ptr [r14 + 4], r8b
mov word ptr [r14 + 4], r8w
mov dword ptr [r14 + 4], r8d
mov qword ptr [r14 + 4], r8

movzx r14w, r8b
movzx r14w, byte ptr [r8 + 4]

movzx r14d, r8b
movzx r14d, r8w
movzx r14d, byte ptr [r8 + 4]
movzx r14d, word ptr [r8 + 4]

movzx r14, r8b
movzx r14, r8w
movzx r14, byte ptr [r8 + 4]
movzx r14, word ptr [r8 + 4]


; Source: r8, target: r15
mov r15b, 1
mov r15w, 1
mov r15d, 1
mov r15, 1

mov r15b, r8b
mov r15w, r8w
mov r15d, r8d
mov r15, r8

mov r15b, byte ptr [r8 - 4]
mov r15b, byte ptr [r8 + 4]
mov r15w, word ptr [r8 + 4]
mov r15d, dword ptr [r8 + 4]
mov r15, qword ptr [r8 + 4]

mov byte ptr [r15 - 4], r8b
mov byte ptr [r15 + 4], r8b
mov word ptr [r15 + 4], r8w
mov dword ptr [r15 + 4], r8d
mov qword ptr [r15 + 4], r8

movzx r15w, r8b
movzx r15w, byte ptr [r8 + 4]

movzx r15d, r8b
movzx r15d, r8w
movzx r15d, byte ptr [r8 + 4]
movzx r15d, word ptr [r8 + 4]

movzx r15, r8b
movzx r15, r8w
movzx r15, byte ptr [r8 + 4]
movzx r15, word ptr [r8 + 4]


; Source: r9, target: rax
mov al, 1
mov ax, 1
mov eax, 1
mov rax, 1

mov al, r9b
mov ax, r9w
mov eax, r9d
mov rax, r9

mov al, byte ptr [r9 - 4]
mov al, byte ptr [r9 + 4]
mov ax, word ptr [r9 + 4]
mov eax, dword ptr [r9 + 4]
mov rax, qword ptr [r9 + 4]

mov byte ptr [rax - 4], r9b
mov byte ptr [rax + 4], r9b
mov word ptr [rax + 4], r9w
mov dword ptr [rax + 4], r9d
mov qword ptr [rax + 4], r9

movzx ax, r9b
movzx ax, byte ptr [r9 + 4]

movzx eax, r9b
movzx eax, r9w
movzx eax, byte ptr [r9 + 4]
movzx eax, word ptr [r9 + 4]

movzx rax, r9b
movzx rax, r9w
movzx rax, byte ptr [r9 + 4]
movzx rax, word ptr [r9 + 4]


; Source: r9, target: rcx
mov cl, 1
mov cx, 1
mov ecx, 1
mov rcx, 1

mov cl, r9b
mov cx, r9w
mov ecx, r9d
mov rcx, r9

mov cl, byte ptr [r9 - 4]
mov cl, byte ptr [r9 + 4]
mov cx, word ptr [r9 + 4]
mov ecx, dword ptr [r9 + 4]
mov rcx, qword ptr [r9 + 4]

mov byte ptr [rcx - 4], r9b
mov byte ptr [rcx + 4], r9b
mov word ptr [rcx + 4], r9w
mov dword ptr [rcx + 4], r9d
mov qword ptr [rcx + 4], r9

movzx cx, r9b
movzx cx, byte ptr [r9 + 4]

movzx ecx, r9b
movzx ecx, r9w
movzx ecx, byte ptr [r9 + 4]
movzx ecx, word ptr [r9 + 4]

movzx rcx, r9b
movzx rcx, r9w
movzx rcx, byte ptr [r9 + 4]
movzx rcx, word ptr [r9 + 4]


; Source: r9, target: rdx
mov dl, 1
mov dx, 1
mov edx, 1
mov rdx, 1

mov dl, r9b
mov dx, r9w
mov edx, r9d
mov rdx, r9

mov dl, byte ptr [r9 - 4]
mov dl, byte ptr [r9 + 4]
mov dx, word ptr [r9 + 4]
mov edx, dword ptr [r9 + 4]
mov rdx, qword ptr [r9 + 4]

mov byte ptr [rdx - 4], r9b
mov byte ptr [rdx + 4], r9b
mov word ptr [rdx + 4], r9w
mov dword ptr [rdx + 4], r9d
mov qword ptr [rdx + 4], r9

movzx dx, r9b
movzx dx, byte ptr [r9 + 4]

movzx edx, r9b
movzx edx, r9w
movzx edx, byte ptr [r9 + 4]
movzx edx, word ptr [r9 + 4]

movzx rdx, r9b
movzx rdx, r9w
movzx rdx, byte ptr [r9 + 4]
movzx rdx, word ptr [r9 + 4]


; Source: r9, target: rbx
mov bl, 1
mov bx, 1
mov ebx, 1
mov rbx, 1

mov bl, r9b
mov bx, r9w
mov ebx, r9d
mov rbx, r9

mov bl, byte ptr [r9 - 4]
mov bl, byte ptr [r9 + 4]
mov bx, word ptr [r9 + 4]
mov ebx, dword ptr [r9 + 4]
mov rbx, qword ptr [r9 + 4]

mov byte ptr [rbx - 4], r9b
mov byte ptr [rbx + 4], r9b
mov word ptr [rbx + 4], r9w
mov dword ptr [rbx + 4], r9d
mov qword ptr [rbx + 4], r9

movzx bx, r9b
movzx bx, byte ptr [r9 + 4]

movzx ebx, r9b
movzx ebx, r9w
movzx ebx, byte ptr [r9 + 4]
movzx ebx, word ptr [r9 + 4]

movzx rbx, r9b
movzx rbx, r9w
movzx rbx, byte ptr [r9 + 4]
movzx rbx, word ptr [r9 + 4]


; Source: r9, target: rsp
mov spl, 1
mov sp, 1
mov esp, 1
mov rsp, 1

mov spl, r9b
mov sp, r9w
mov esp, r9d
mov rsp, r9

mov spl, byte ptr [r9 - 4]
mov spl, byte ptr [r9 + 4]
mov sp, word ptr [r9 + 4]
mov esp, dword ptr [r9 + 4]
mov rsp, qword ptr [r9 + 4]

mov byte ptr [rsp - 4], r9b
mov byte ptr [rsp + 4], r9b
mov word ptr [rsp + 4], r9w
mov dword ptr [rsp + 4], r9d
mov qword ptr [rsp + 4], r9

movzx sp, r9b
movzx sp, byte ptr [r9 + 4]

movzx esp, r9b
movzx esp, r9w
movzx esp, byte ptr [r9 + 4]
movzx esp, word ptr [r9 + 4]

movzx rsp, r9b
movzx rsp, r9w
movzx rsp, byte ptr [r9 + 4]
movzx rsp, word ptr [r9 + 4]


; Source: r9, target: rbp
mov bpl, 1
mov bp, 1
mov ebp, 1
mov rbp, 1

mov bpl, r9b
mov bp, r9w
mov ebp, r9d
mov rbp, r9

mov bpl, byte ptr [r9 - 4]
mov bpl, byte ptr [r9 + 4]
mov bp, word ptr [r9 + 4]
mov ebp, dword ptr [r9 + 4]
mov rbp, qword ptr [r9 + 4]

mov byte ptr [rbp - 4], r9b
mov byte ptr [rbp + 4], r9b
mov word ptr [rbp + 4], r9w
mov dword ptr [rbp + 4], r9d
mov qword ptr [rbp + 4], r9

movzx bp, r9b
movzx bp, byte ptr [r9 + 4]

movzx ebp, r9b
movzx ebp, r9w
movzx ebp, byte ptr [r9 + 4]
movzx ebp, word ptr [r9 + 4]

movzx rbp, r9b
movzx rbp, r9w
movzx rbp, byte ptr [r9 + 4]
movzx rbp, word ptr [r9 + 4]


; Source: r9, target: rsi
mov dil, 1
mov si, 1
mov esi, 1
mov rsi, 1

mov dil, r9b
mov si, r9w
mov esi, r9d
mov rsi, r9

mov dil, byte ptr [r9 - 4]
mov dil, byte ptr [r9 + 4]
mov si, word ptr [r9 + 4]
mov esi, dword ptr [r9 + 4]
mov rsi, qword ptr [r9 + 4]

mov byte ptr [rsi - 4], r9b
mov byte ptr [rsi + 4], r9b
mov word ptr [rsi + 4], r9w
mov dword ptr [rsi + 4], r9d
mov qword ptr [rsi + 4], r9

movzx si, r9b
movzx si, byte ptr [r9 + 4]

movzx esi, r9b
movzx esi, r9w
movzx esi, byte ptr [r9 + 4]
movzx esi, word ptr [r9 + 4]

movzx rsi, r9b
movzx rsi, r9w
movzx rsi, byte ptr [r9 + 4]
movzx rsi, word ptr [r9 + 4]


; Source: r9, target: rdi
mov sil, 1
mov di, 1
mov edi, 1
mov rdi, 1

mov sil, r9b
mov di, r9w
mov edi, r9d
mov rdi, r9

mov sil, byte ptr [r9 - 4]
mov sil, byte ptr [r9 + 4]
mov di, word ptr [r9 + 4]
mov edi, dword ptr [r9 + 4]
mov rdi, qword ptr [r9 + 4]

mov byte ptr [rdi - 4], r9b
mov byte ptr [rdi + 4], r9b
mov word ptr [rdi + 4], r9w
mov dword ptr [rdi + 4], r9d
mov qword ptr [rdi + 4], r9

movzx di, r9b
movzx di, byte ptr [r9 + 4]

movzx edi, r9b
movzx edi, r9w
movzx edi, byte ptr [r9 + 4]
movzx edi, word ptr [r9 + 4]

movzx rdi, r9b
movzx rdi, r9w
movzx rdi, byte ptr [r9 + 4]
movzx rdi, word ptr [r9 + 4]


; Source: r9, target: r8
mov r8b, 1
mov r8w, 1
mov r8d, 1
mov r8, 1

mov r8b, r9b
mov r8w, r9w
mov r8d, r9d
mov r8, r9

mov r8b, byte ptr [r9 - 4]
mov r8b, byte ptr [r9 + 4]
mov r8w, word ptr [r9 + 4]
mov r8d, dword ptr [r9 + 4]
mov r8, qword ptr [r9 + 4]

mov byte ptr [r8 - 4], r9b
mov byte ptr [r8 + 4], r9b
mov word ptr [r8 + 4], r9w
mov dword ptr [r8 + 4], r9d
mov qword ptr [r8 + 4], r9

movzx r8w, r9b
movzx r8w, byte ptr [r9 + 4]

movzx r8d, r9b
movzx r8d, r9w
movzx r8d, byte ptr [r9 + 4]
movzx r8d, word ptr [r9 + 4]

movzx r8, r9b
movzx r8, r9w
movzx r8, byte ptr [r9 + 4]
movzx r8, word ptr [r9 + 4]


; Source: r9, target: r9
mov r9b, 1
mov r9w, 1
mov r9d, 1
mov r9, 1

mov r9b, r9b
mov r9w, r9w
mov r9d, r9d
mov r9, r9

mov r9b, byte ptr [r9 - 4]
mov r9b, byte ptr [r9 + 4]
mov r9w, word ptr [r9 + 4]
mov r9d, dword ptr [r9 + 4]
mov r9, qword ptr [r9 + 4]

mov byte ptr [r9 - 4], r9b
mov byte ptr [r9 + 4], r9b
mov word ptr [r9 + 4], r9w
mov dword ptr [r9 + 4], r9d
mov qword ptr [r9 + 4], r9

movzx r9w, r9b
movzx r9w, byte ptr [r9 + 4]

movzx r9d, r9b
movzx r9d, r9w
movzx r9d, byte ptr [r9 + 4]
movzx r9d, word ptr [r9 + 4]

movzx r9, r9b
movzx r9, r9w
movzx r9, byte ptr [r9 + 4]
movzx r9, word ptr [r9 + 4]


; Source: r9, target: r10
mov r10b, 1
mov r10w, 1
mov r10d, 1
mov r10, 1

mov r10b, r9b
mov r10w, r9w
mov r10d, r9d
mov r10, r9

mov r10b, byte ptr [r9 - 4]
mov r10b, byte ptr [r9 + 4]
mov r10w, word ptr [r9 + 4]
mov r10d, dword ptr [r9 + 4]
mov r10, qword ptr [r9 + 4]

mov byte ptr [r10 - 4], r9b
mov byte ptr [r10 + 4], r9b
mov word ptr [r10 + 4], r9w
mov dword ptr [r10 + 4], r9d
mov qword ptr [r10 + 4], r9

movzx r10w, r9b
movzx r10w, byte ptr [r9 + 4]

movzx r10d, r9b
movzx r10d, r9w
movzx r10d, byte ptr [r9 + 4]
movzx r10d, word ptr [r9 + 4]

movzx r10, r9b
movzx r10, r9w
movzx r10, byte ptr [r9 + 4]
movzx r10, word ptr [r9 + 4]


; Source: r9, target: r11
mov r11b, 1
mov r11w, 1
mov r11d, 1
mov r11, 1

mov r11b, r9b
mov r11w, r9w
mov r11d, r9d
mov r11, r9

mov r11b, byte ptr [r9 - 4]
mov r11b, byte ptr [r9 + 4]
mov r11w, word ptr [r9 + 4]
mov r11d, dword ptr [r9 + 4]
mov r11, qword ptr [r9 + 4]

mov byte ptr [r11 - 4], r9b
mov byte ptr [r11 + 4], r9b
mov word ptr [r11 + 4], r9w
mov dword ptr [r11 + 4], r9d
mov qword ptr [r11 + 4], r9

movzx r11w, r9b
movzx r11w, byte ptr [r9 + 4]

movzx r11d, r9b
movzx r11d, r9w
movzx r11d, byte ptr [r9 + 4]
movzx r11d, word ptr [r9 + 4]

movzx r11, r9b
movzx r11, r9w
movzx r11, byte ptr [r9 + 4]
movzx r11, word ptr [r9 + 4]


; Source: r9, target: r12
mov r12b, 1
mov r12w, 1
mov r12d, 1
mov r12, 1

mov r12b, r9b
mov r12w, r9w
mov r12d, r9d
mov r12, r9

mov r12b, byte ptr [r9 - 4]
mov r12b, byte ptr [r9 + 4]
mov r12w, word ptr [r9 + 4]
mov r12d, dword ptr [r9 + 4]
mov r12, qword ptr [r9 + 4]

mov byte ptr [r12 - 4], r9b
mov byte ptr [r12 + 4], r9b
mov word ptr [r12 + 4], r9w
mov dword ptr [r12 + 4], r9d
mov qword ptr [r12 + 4], r9

movzx r12w, r9b
movzx r12w, byte ptr [r9 + 4]

movzx r12d, r9b
movzx r12d, r9w
movzx r12d, byte ptr [r9 + 4]
movzx r12d, word ptr [r9 + 4]

movzx r12, r9b
movzx r12, r9w
movzx r12, byte ptr [r9 + 4]
movzx r12, word ptr [r9 + 4]


; Source: r9, target: r13
mov r13b, 1
mov r13w, 1
mov r13d, 1
mov r13, 1

mov r13b, r9b
mov r13w, r9w
mov r13d, r9d
mov r13, r9

mov r13b, byte ptr [r9 - 4]
mov r13b, byte ptr [r9 + 4]
mov r13w, word ptr [r9 + 4]
mov r13d, dword ptr [r9 + 4]
mov r13, qword ptr [r9 + 4]

mov byte ptr [r13 - 4], r9b
mov byte ptr [r13 + 4], r9b
mov word ptr [r13 + 4], r9w
mov dword ptr [r13 + 4], r9d
mov qword ptr [r13 + 4], r9

movzx r13w, r9b
movzx r13w, byte ptr [r9 + 4]

movzx r13d, r9b
movzx r13d, r9w
movzx r13d, byte ptr [r9 + 4]
movzx r13d, word ptr [r9 + 4]

movzx r13, r9b
movzx r13, r9w
movzx r13, byte ptr [r9 + 4]
movzx r13, word ptr [r9 + 4]


; Source: r9, target: r14
mov r14b, 1
mov r14w, 1
mov r14d, 1
mov r14, 1

mov r14b, r9b
mov r14w, r9w
mov r14d, r9d
mov r14, r9

mov r14b, byte ptr [r9 - 4]
mov r14b, byte ptr [r9 + 4]
mov r14w, word ptr [r9 + 4]
mov r14d, dword ptr [r9 + 4]
mov r14, qword ptr [r9 + 4]

mov byte ptr [r14 - 4], r9b
mov byte ptr [r14 + 4], r9b
mov word ptr [r14 + 4], r9w
mov dword ptr [r14 + 4], r9d
mov qword ptr [r14 + 4], r9

movzx r14w, r9b
movzx r14w, byte ptr [r9 + 4]

movzx r14d, r9b
movzx r14d, r9w
movzx r14d, byte ptr [r9 + 4]
movzx r14d, word ptr [r9 + 4]

movzx r14, r9b
movzx r14, r9w
movzx r14, byte ptr [r9 + 4]
movzx r14, word ptr [r9 + 4]


; Source: r9, target: r15
mov r15b, 1
mov r15w, 1
mov r15d, 1
mov r15, 1

mov r15b, r9b
mov r15w, r9w
mov r15d, r9d
mov r15, r9

mov r15b, byte ptr [r9 - 4]
mov r15b, byte ptr [r9 + 4]
mov r15w, word ptr [r9 + 4]
mov r15d, dword ptr [r9 + 4]
mov r15, qword ptr [r9 + 4]

mov byte ptr [r15 - 4], r9b
mov byte ptr [r15 + 4], r9b
mov word ptr [r15 + 4], r9w
mov dword ptr [r15 + 4], r9d
mov qword ptr [r15 + 4], r9

movzx r15w, r9b
movzx r15w, byte ptr [r9 + 4]

movzx r15d, r9b
movzx r15d, r9w
movzx r15d, byte ptr [r9 + 4]
movzx r15d, word ptr [r9 + 4]

movzx r15, r9b
movzx r15, r9w
movzx r15, byte ptr [r9 + 4]
movzx r15, word ptr [r9 + 4]


; Source: r10, target: rax
mov al, 1
mov ax, 1
mov eax, 1
mov rax, 1

mov al, r10b
mov ax, r10w
mov eax, r10d
mov rax, r10

mov al, byte ptr [r10 - 4]
mov al, byte ptr [r10 + 4]
mov ax, word ptr [r10 + 4]
mov eax, dword ptr [r10 + 4]
mov rax, qword ptr [r10 + 4]

mov byte ptr [rax - 4], r10b
mov byte ptr [rax + 4], r10b
mov word ptr [rax + 4], r10w
mov dword ptr [rax + 4], r10d
mov qword ptr [rax + 4], r10

movzx ax, r10b
movzx ax, byte ptr [r10 + 4]

movzx eax, r10b
movzx eax, r10w
movzx eax, byte ptr [r10 + 4]
movzx eax, word ptr [r10 + 4]

movzx rax, r10b
movzx rax, r10w
movzx rax, byte ptr [r10 + 4]
movzx rax, word ptr [r10 + 4]


; Source: r10, target: rcx
mov cl, 1
mov cx, 1
mov ecx, 1
mov rcx, 1

mov cl, r10b
mov cx, r10w
mov ecx, r10d
mov rcx, r10

mov cl, byte ptr [r10 - 4]
mov cl, byte ptr [r10 + 4]
mov cx, word ptr [r10 + 4]
mov ecx, dword ptr [r10 + 4]
mov rcx, qword ptr [r10 + 4]

mov byte ptr [rcx - 4], r10b
mov byte ptr [rcx + 4], r10b
mov word ptr [rcx + 4], r10w
mov dword ptr [rcx + 4], r10d
mov qword ptr [rcx + 4], r10

movzx cx, r10b
movzx cx, byte ptr [r10 + 4]

movzx ecx, r10b
movzx ecx, r10w
movzx ecx, byte ptr [r10 + 4]
movzx ecx, word ptr [r10 + 4]

movzx rcx, r10b
movzx rcx, r10w
movzx rcx, byte ptr [r10 + 4]
movzx rcx, word ptr [r10 + 4]


; Source: r10, target: rdx
mov dl, 1
mov dx, 1
mov edx, 1
mov rdx, 1

mov dl, r10b
mov dx, r10w
mov edx, r10d
mov rdx, r10

mov dl, byte ptr [r10 - 4]
mov dl, byte ptr [r10 + 4]
mov dx, word ptr [r10 + 4]
mov edx, dword ptr [r10 + 4]
mov rdx, qword ptr [r10 + 4]

mov byte ptr [rdx - 4], r10b
mov byte ptr [rdx + 4], r10b
mov word ptr [rdx + 4], r10w
mov dword ptr [rdx + 4], r10d
mov qword ptr [rdx + 4], r10

movzx dx, r10b
movzx dx, byte ptr [r10 + 4]

movzx edx, r10b
movzx edx, r10w
movzx edx, byte ptr [r10 + 4]
movzx edx, word ptr [r10 + 4]

movzx rdx, r10b
movzx rdx, r10w
movzx rdx, byte ptr [r10 + 4]
movzx rdx, word ptr [r10 + 4]


; Source: r10, target: rbx
mov bl, 1
mov bx, 1
mov ebx, 1
mov rbx, 1

mov bl, r10b
mov bx, r10w
mov ebx, r10d
mov rbx, r10

mov bl, byte ptr [r10 - 4]
mov bl, byte ptr [r10 + 4]
mov bx, word ptr [r10 + 4]
mov ebx, dword ptr [r10 + 4]
mov rbx, qword ptr [r10 + 4]

mov byte ptr [rbx - 4], r10b
mov byte ptr [rbx + 4], r10b
mov word ptr [rbx + 4], r10w
mov dword ptr [rbx + 4], r10d
mov qword ptr [rbx + 4], r10

movzx bx, r10b
movzx bx, byte ptr [r10 + 4]

movzx ebx, r10b
movzx ebx, r10w
movzx ebx, byte ptr [r10 + 4]
movzx ebx, word ptr [r10 + 4]

movzx rbx, r10b
movzx rbx, r10w
movzx rbx, byte ptr [r10 + 4]
movzx rbx, word ptr [r10 + 4]


; Source: r10, target: rsp
mov spl, 1
mov sp, 1
mov esp, 1
mov rsp, 1

mov spl, r10b
mov sp, r10w
mov esp, r10d
mov rsp, r10

mov spl, byte ptr [r10 - 4]
mov spl, byte ptr [r10 + 4]
mov sp, word ptr [r10 + 4]
mov esp, dword ptr [r10 + 4]
mov rsp, qword ptr [r10 + 4]

mov byte ptr [rsp - 4], r10b
mov byte ptr [rsp + 4], r10b
mov word ptr [rsp + 4], r10w
mov dword ptr [rsp + 4], r10d
mov qword ptr [rsp + 4], r10

movzx sp, r10b
movzx sp, byte ptr [r10 + 4]

movzx esp, r10b
movzx esp, r10w
movzx esp, byte ptr [r10 + 4]
movzx esp, word ptr [r10 + 4]

movzx rsp, r10b
movzx rsp, r10w
movzx rsp, byte ptr [r10 + 4]
movzx rsp, word ptr [r10 + 4]


; Source: r10, target: rbp
mov bpl, 1
mov bp, 1
mov ebp, 1
mov rbp, 1

mov bpl, r10b
mov bp, r10w
mov ebp, r10d
mov rbp, r10

mov bpl, byte ptr [r10 - 4]
mov bpl, byte ptr [r10 + 4]
mov bp, word ptr [r10 + 4]
mov ebp, dword ptr [r10 + 4]
mov rbp, qword ptr [r10 + 4]

mov byte ptr [rbp - 4], r10b
mov byte ptr [rbp + 4], r10b
mov word ptr [rbp + 4], r10w
mov dword ptr [rbp + 4], r10d
mov qword ptr [rbp + 4], r10

movzx bp, r10b
movzx bp, byte ptr [r10 + 4]

movzx ebp, r10b
movzx ebp, r10w
movzx ebp, byte ptr [r10 + 4]
movzx ebp, word ptr [r10 + 4]

movzx rbp, r10b
movzx rbp, r10w
movzx rbp, byte ptr [r10 + 4]
movzx rbp, word ptr [r10 + 4]


; Source: r10, target: rsi
mov dil, 1
mov si, 1
mov esi, 1
mov rsi, 1

mov dil, r10b
mov si, r10w
mov esi, r10d
mov rsi, r10

mov dil, byte ptr [r10 - 4]
mov dil, byte ptr [r10 + 4]
mov si, word ptr [r10 + 4]
mov esi, dword ptr [r10 + 4]
mov rsi, qword ptr [r10 + 4]

mov byte ptr [rsi - 4], r10b
mov byte ptr [rsi + 4], r10b
mov word ptr [rsi + 4], r10w
mov dword ptr [rsi + 4], r10d
mov qword ptr [rsi + 4], r10

movzx si, r10b
movzx si, byte ptr [r10 + 4]

movzx esi, r10b
movzx esi, r10w
movzx esi, byte ptr [r10 + 4]
movzx esi, word ptr [r10 + 4]

movzx rsi, r10b
movzx rsi, r10w
movzx rsi, byte ptr [r10 + 4]
movzx rsi, word ptr [r10 + 4]


; Source: r10, target: rdi
mov sil, 1
mov di, 1
mov edi, 1
mov rdi, 1

mov sil, r10b
mov di, r10w
mov edi, r10d
mov rdi, r10

mov sil, byte ptr [r10 - 4]
mov sil, byte ptr [r10 + 4]
mov di, word ptr [r10 + 4]
mov edi, dword ptr [r10 + 4]
mov rdi, qword ptr [r10 + 4]

mov byte ptr [rdi - 4], r10b
mov byte ptr [rdi + 4], r10b
mov word ptr [rdi + 4], r10w
mov dword ptr [rdi + 4], r10d
mov qword ptr [rdi + 4], r10

movzx di, r10b
movzx di, byte ptr [r10 + 4]

movzx edi, r10b
movzx edi, r10w
movzx edi, byte ptr [r10 + 4]
movzx edi, word ptr [r10 + 4]

movzx rdi, r10b
movzx rdi, r10w
movzx rdi, byte ptr [r10 + 4]
movzx rdi, word ptr [r10 + 4]


; Source: r10, target: r8
mov r8b, 1
mov r8w, 1
mov r8d, 1
mov r8, 1

mov r8b, r10b
mov r8w, r10w
mov r8d, r10d
mov r8, r10

mov r8b, byte ptr [r10 - 4]
mov r8b, byte ptr [r10 + 4]
mov r8w, word ptr [r10 + 4]
mov r8d, dword ptr [r10 + 4]
mov r8, qword ptr [r10 + 4]

mov byte ptr [r8 - 4], r10b
mov byte ptr [r8 + 4], r10b
mov word ptr [r8 + 4], r10w
mov dword ptr [r8 + 4], r10d
mov qword ptr [r8 + 4], r10

movzx r8w, r10b
movzx r8w, byte ptr [r10 + 4]

movzx r8d, r10b
movzx r8d, r10w
movzx r8d, byte ptr [r10 + 4]
movzx r8d, word ptr [r10 + 4]

movzx r8, r10b
movzx r8, r10w
movzx r8, byte ptr [r10 + 4]
movzx r8, word ptr [r10 + 4]


; Source: r10, target: r9
mov r9b, 1
mov r9w, 1
mov r9d, 1
mov r9, 1

mov r9b, r10b
mov r9w, r10w
mov r9d, r10d
mov r9, r10

mov r9b, byte ptr [r10 - 4]
mov r9b, byte ptr [r10 + 4]
mov r9w, word ptr [r10 + 4]
mov r9d, dword ptr [r10 + 4]
mov r9, qword ptr [r10 + 4]

mov byte ptr [r9 - 4], r10b
mov byte ptr [r9 + 4], r10b
mov word ptr [r9 + 4], r10w
mov dword ptr [r9 + 4], r10d
mov qword ptr [r9 + 4], r10

movzx r9w, r10b
movzx r9w, byte ptr [r10 + 4]

movzx r9d, r10b
movzx r9d, r10w
movzx r9d, byte ptr [r10 + 4]
movzx r9d, word ptr [r10 + 4]

movzx r9, r10b
movzx r9, r10w
movzx r9, byte ptr [r10 + 4]
movzx r9, word ptr [r10 + 4]


; Source: r10, target: r10
mov r10b, 1
mov r10w, 1
mov r10d, 1
mov r10, 1

mov r10b, r10b
mov r10w, r10w
mov r10d, r10d
mov r10, r10

mov r10b, byte ptr [r10 - 4]
mov r10b, byte ptr [r10 + 4]
mov r10w, word ptr [r10 + 4]
mov r10d, dword ptr [r10 + 4]
mov r10, qword ptr [r10 + 4]

mov byte ptr [r10 - 4], r10b
mov byte ptr [r10 + 4], r10b
mov word ptr [r10 + 4], r10w
mov dword ptr [r10 + 4], r10d
mov qword ptr [r10 + 4], r10

movzx r10w, r10b
movzx r10w, byte ptr [r10 + 4]

movzx r10d, r10b
movzx r10d, r10w
movzx r10d, byte ptr [r10 + 4]
movzx r10d, word ptr [r10 + 4]

movzx r10, r10b
movzx r10, r10w
movzx r10, byte ptr [r10 + 4]
movzx r10, word ptr [r10 + 4]


; Source: r10, target: r11
mov r11b, 1
mov r11w, 1
mov r11d, 1
mov r11, 1

mov r11b, r10b
mov r11w, r10w
mov r11d, r10d
mov r11, r10

mov r11b, byte ptr [r10 - 4]
mov r11b, byte ptr [r10 + 4]
mov r11w, word ptr [r10 + 4]
mov r11d, dword ptr [r10 + 4]
mov r11, qword ptr [r10 + 4]

mov byte ptr [r11 - 4], r10b
mov byte ptr [r11 + 4], r10b
mov word ptr [r11 + 4], r10w
mov dword ptr [r11 + 4], r10d
mov qword ptr [r11 + 4], r10

movzx r11w, r10b
movzx r11w, byte ptr [r10 + 4]

movzx r11d, r10b
movzx r11d, r10w
movzx r11d, byte ptr [r10 + 4]
movzx r11d, word ptr [r10 + 4]

movzx r11, r10b
movzx r11, r10w
movzx r11, byte ptr [r10 + 4]
movzx r11, word ptr [r10 + 4]


; Source: r10, target: r12
mov r12b, 1
mov r12w, 1
mov r12d, 1
mov r12, 1

mov r12b, r10b
mov r12w, r10w
mov r12d, r10d
mov r12, r10

mov r12b, byte ptr [r10 - 4]
mov r12b, byte ptr [r10 + 4]
mov r12w, word ptr [r10 + 4]
mov r12d, dword ptr [r10 + 4]
mov r12, qword ptr [r10 + 4]

mov byte ptr [r12 - 4], r10b
mov byte ptr [r12 + 4], r10b
mov word ptr [r12 + 4], r10w
mov dword ptr [r12 + 4], r10d
mov qword ptr [r12 + 4], r10

movzx r12w, r10b
movzx r12w, byte ptr [r10 + 4]

movzx r12d, r10b
movzx r12d, r10w
movzx r12d, byte ptr [r10 + 4]
movzx r12d, word ptr [r10 + 4]

movzx r12, r10b
movzx r12, r10w
movzx r12, byte ptr [r10 + 4]
movzx r12, word ptr [r10 + 4]


; Source: r10, target: r13
mov r13b, 1
mov r13w, 1
mov r13d, 1
mov r13, 1

mov r13b, r10b
mov r13w, r10w
mov r13d, r10d
mov r13, r10

mov r13b, byte ptr [r10 - 4]
mov r13b, byte ptr [r10 + 4]
mov r13w, word ptr [r10 + 4]
mov r13d, dword ptr [r10 + 4]
mov r13, qword ptr [r10 + 4]

mov byte ptr [r13 - 4], r10b
mov byte ptr [r13 + 4], r10b
mov word ptr [r13 + 4], r10w
mov dword ptr [r13 + 4], r10d
mov qword ptr [r13 + 4], r10

movzx r13w, r10b
movzx r13w, byte ptr [r10 + 4]

movzx r13d, r10b
movzx r13d, r10w
movzx r13d, byte ptr [r10 + 4]
movzx r13d, word ptr [r10 + 4]

movzx r13, r10b
movzx r13, r10w
movzx r13, byte ptr [r10 + 4]
movzx r13, word ptr [r10 + 4]


; Source: r10, target: r14
mov r14b, 1
mov r14w, 1
mov r14d, 1
mov r14, 1

mov r14b, r10b
mov r14w, r10w
mov r14d, r10d
mov r14, r10

mov r14b, byte ptr [r10 - 4]
mov r14b, byte ptr [r10 + 4]
mov r14w, word ptr [r10 + 4]
mov r14d, dword ptr [r10 + 4]
mov r14, qword ptr [r10 + 4]

mov byte ptr [r14 - 4], r10b
mov byte ptr [r14 + 4], r10b
mov word ptr [r14 + 4], r10w
mov dword ptr [r14 + 4], r10d
mov qword ptr [r14 + 4], r10

movzx r14w, r10b
movzx r14w, byte ptr [r10 + 4]

movzx r14d, r10b
movzx r14d, r10w
movzx r14d, byte ptr [r10 + 4]
movzx r14d, word ptr [r10 + 4]

movzx r14, r10b
movzx r14, r10w
movzx r14, byte ptr [r10 + 4]
movzx r14, word ptr [r10 + 4]


; Source: r10, target: r15
mov r15b, 1
mov r15w, 1
mov r15d, 1
mov r15, 1

mov r15b, r10b
mov r15w, r10w
mov r15d, r10d
mov r15, r10

mov r15b, byte ptr [r10 - 4]
mov r15b, byte ptr [r10 + 4]
mov r15w, word ptr [r10 + 4]
mov r15d, dword ptr [r10 + 4]
mov r15, qword ptr [r10 + 4]

mov byte ptr [r15 - 4], r10b
mov byte ptr [r15 + 4], r10b
mov word ptr [r15 + 4], r10w
mov dword ptr [r15 + 4], r10d
mov qword ptr [r15 + 4], r10

movzx r15w, r10b
movzx r15w, byte ptr [r10 + 4]

movzx r15d, r10b
movzx r15d, r10w
movzx r15d, byte ptr [r10 + 4]
movzx r15d, word ptr [r10 + 4]

movzx r15, r10b
movzx r15, r10w
movzx r15, byte ptr [r10 + 4]
movzx r15, word ptr [r10 + 4]


; Source: r11, target: rax
mov al, 1
mov ax, 1
mov eax, 1
mov rax, 1

mov al, r11b
mov ax, r11w
mov eax, r11d
mov rax, r11

mov al, byte ptr [r11 - 4]
mov al, byte ptr [r11 + 4]
mov ax, word ptr [r11 + 4]
mov eax, dword ptr [r11 + 4]
mov rax, qword ptr [r11 + 4]

mov byte ptr [rax - 4], r11b
mov byte ptr [rax + 4], r11b
mov word ptr [rax + 4], r11w
mov dword ptr [rax + 4], r11d
mov qword ptr [rax + 4], r11

movzx ax, r11b
movzx ax, byte ptr [r11 + 4]

movzx eax, r11b
movzx eax, r11w
movzx eax, byte ptr [r11 + 4]
movzx eax, word ptr [r11 + 4]

movzx rax, r11b
movzx rax, r11w
movzx rax, byte ptr [r11 + 4]
movzx rax, word ptr [r11 + 4]


; Source: r11, target: rcx
mov cl, 1
mov cx, 1
mov ecx, 1
mov rcx, 1

mov cl, r11b
mov cx, r11w
mov ecx, r11d
mov rcx, r11

mov cl, byte ptr [r11 - 4]
mov cl, byte ptr [r11 + 4]
mov cx, word ptr [r11 + 4]
mov ecx, dword ptr [r11 + 4]
mov rcx, qword ptr [r11 + 4]

mov byte ptr [rcx - 4], r11b
mov byte ptr [rcx + 4], r11b
mov word ptr [rcx + 4], r11w
mov dword ptr [rcx + 4], r11d
mov qword ptr [rcx + 4], r11

movzx cx, r11b
movzx cx, byte ptr [r11 + 4]

movzx ecx, r11b
movzx ecx, r11w
movzx ecx, byte ptr [r11 + 4]
movzx ecx, word ptr [r11 + 4]

movzx rcx, r11b
movzx rcx, r11w
movzx rcx, byte ptr [r11 + 4]
movzx rcx, word ptr [r11 + 4]


; Source: r11, target: rdx
mov dl, 1
mov dx, 1
mov edx, 1
mov rdx, 1

mov dl, r11b
mov dx, r11w
mov edx, r11d
mov rdx, r11

mov dl, byte ptr [r11 - 4]
mov dl, byte ptr [r11 + 4]
mov dx, word ptr [r11 + 4]
mov edx, dword ptr [r11 + 4]
mov rdx, qword ptr [r11 + 4]

mov byte ptr [rdx - 4], r11b
mov byte ptr [rdx + 4], r11b
mov word ptr [rdx + 4], r11w
mov dword ptr [rdx + 4], r11d
mov qword ptr [rdx + 4], r11

movzx dx, r11b
movzx dx, byte ptr [r11 + 4]

movzx edx, r11b
movzx edx, r11w
movzx edx, byte ptr [r11 + 4]
movzx edx, word ptr [r11 + 4]

movzx rdx, r11b
movzx rdx, r11w
movzx rdx, byte ptr [r11 + 4]
movzx rdx, word ptr [r11 + 4]


; Source: r11, target: rbx
mov bl, 1
mov bx, 1
mov ebx, 1
mov rbx, 1

mov bl, r11b
mov bx, r11w
mov ebx, r11d
mov rbx, r11

mov bl, byte ptr [r11 - 4]
mov bl, byte ptr [r11 + 4]
mov bx, word ptr [r11 + 4]
mov ebx, dword ptr [r11 + 4]
mov rbx, qword ptr [r11 + 4]

mov byte ptr [rbx - 4], r11b
mov byte ptr [rbx + 4], r11b
mov word ptr [rbx + 4], r11w
mov dword ptr [rbx + 4], r11d
mov qword ptr [rbx + 4], r11

movzx bx, r11b
movzx bx, byte ptr [r11 + 4]

movzx ebx, r11b
movzx ebx, r11w
movzx ebx, byte ptr [r11 + 4]
movzx ebx, word ptr [r11 + 4]

movzx rbx, r11b
movzx rbx, r11w
movzx rbx, byte ptr [r11 + 4]
movzx rbx, word ptr [r11 + 4]


; Source: r11, target: rsp
mov spl, 1
mov sp, 1
mov esp, 1
mov rsp, 1

mov spl, r11b
mov sp, r11w
mov esp, r11d
mov rsp, r11

mov spl, byte ptr [r11 - 4]
mov spl, byte ptr [r11 + 4]
mov sp, word ptr [r11 + 4]
mov esp, dword ptr [r11 + 4]
mov rsp, qword ptr [r11 + 4]

mov byte ptr [rsp - 4], r11b
mov byte ptr [rsp + 4], r11b
mov word ptr [rsp + 4], r11w
mov dword ptr [rsp + 4], r11d
mov qword ptr [rsp + 4], r11

movzx sp, r11b
movzx sp, byte ptr [r11 + 4]

movzx esp, r11b
movzx esp, r11w
movzx esp, byte ptr [r11 + 4]
movzx esp, word ptr [r11 + 4]

movzx rsp, r11b
movzx rsp, r11w
movzx rsp, byte ptr [r11 + 4]
movzx rsp, word ptr [r11 + 4]


; Source: r11, target: rbp
mov bpl, 1
mov bp, 1
mov ebp, 1
mov rbp, 1

mov bpl, r11b
mov bp, r11w
mov ebp, r11d
mov rbp, r11

mov bpl, byte ptr [r11 - 4]
mov bpl, byte ptr [r11 + 4]
mov bp, word ptr [r11 + 4]
mov ebp, dword ptr [r11 + 4]
mov rbp, qword ptr [r11 + 4]

mov byte ptr [rbp - 4], r11b
mov byte ptr [rbp + 4], r11b
mov word ptr [rbp + 4], r11w
mov dword ptr [rbp + 4], r11d
mov qword ptr [rbp + 4], r11

movzx bp, r11b
movzx bp, byte ptr [r11 + 4]

movzx ebp, r11b
movzx ebp, r11w
movzx ebp, byte ptr [r11 + 4]
movzx ebp, word ptr [r11 + 4]

movzx rbp, r11b
movzx rbp, r11w
movzx rbp, byte ptr [r11 + 4]
movzx rbp, word ptr [r11 + 4]


; Source: r11, target: rsi
mov dil, 1
mov si, 1
mov esi, 1
mov rsi, 1

mov dil, r11b
mov si, r11w
mov esi, r11d
mov rsi, r11

mov dil, byte ptr [r11 - 4]
mov dil, byte ptr [r11 + 4]
mov si, word ptr [r11 + 4]
mov esi, dword ptr [r11 + 4]
mov rsi, qword ptr [r11 + 4]

mov byte ptr [rsi - 4], r11b
mov byte ptr [rsi + 4], r11b
mov word ptr [rsi + 4], r11w
mov dword ptr [rsi + 4], r11d
mov qword ptr [rsi + 4], r11

movzx si, r11b
movzx si, byte ptr [r11 + 4]

movzx esi, r11b
movzx esi, r11w
movzx esi, byte ptr [r11 + 4]
movzx esi, word ptr [r11 + 4]

movzx rsi, r11b
movzx rsi, r11w
movzx rsi, byte ptr [r11 + 4]
movzx rsi, word ptr [r11 + 4]


; Source: r11, target: rdi
mov sil, 1
mov di, 1
mov edi, 1
mov rdi, 1

mov sil, r11b
mov di, r11w
mov edi, r11d
mov rdi, r11

mov sil, byte ptr [r11 - 4]
mov sil, byte ptr [r11 + 4]
mov di, word ptr [r11 + 4]
mov edi, dword ptr [r11 + 4]
mov rdi, qword ptr [r11 + 4]

mov byte ptr [rdi - 4], r11b
mov byte ptr [rdi + 4], r11b
mov word ptr [rdi + 4], r11w
mov dword ptr [rdi + 4], r11d
mov qword ptr [rdi + 4], r11

movzx di, r11b
movzx di, byte ptr [r11 + 4]

movzx edi, r11b
movzx edi, r11w
movzx edi, byte ptr [r11 + 4]
movzx edi, word ptr [r11 + 4]

movzx rdi, r11b
movzx rdi, r11w
movzx rdi, byte ptr [r11 + 4]
movzx rdi, word ptr [r11 + 4]


; Source: r11, target: r8
mov r8b, 1
mov r8w, 1
mov r8d, 1
mov r8, 1

mov r8b, r11b
mov r8w, r11w
mov r8d, r11d
mov r8, r11

mov r8b, byte ptr [r11 - 4]
mov r8b, byte ptr [r11 + 4]
mov r8w, word ptr [r11 + 4]
mov r8d, dword ptr [r11 + 4]
mov r8, qword ptr [r11 + 4]

mov byte ptr [r8 - 4], r11b
mov byte ptr [r8 + 4], r11b
mov word ptr [r8 + 4], r11w
mov dword ptr [r8 + 4], r11d
mov qword ptr [r8 + 4], r11

movzx r8w, r11b
movzx r8w, byte ptr [r11 + 4]

movzx r8d, r11b
movzx r8d, r11w
movzx r8d, byte ptr [r11 + 4]
movzx r8d, word ptr [r11 + 4]

movzx r8, r11b
movzx r8, r11w
movzx r8, byte ptr [r11 + 4]
movzx r8, word ptr [r11 + 4]


; Source: r11, target: r9
mov r9b, 1
mov r9w, 1
mov r9d, 1
mov r9, 1

mov r9b, r11b
mov r9w, r11w
mov r9d, r11d
mov r9, r11

mov r9b, byte ptr [r11 - 4]
mov r9b, byte ptr [r11 + 4]
mov r9w, word ptr [r11 + 4]
mov r9d, dword ptr [r11 + 4]
mov r9, qword ptr [r11 + 4]

mov byte ptr [r9 - 4], r11b
mov byte ptr [r9 + 4], r11b
mov word ptr [r9 + 4], r11w
mov dword ptr [r9 + 4], r11d
mov qword ptr [r9 + 4], r11

movzx r9w, r11b
movzx r9w, byte ptr [r11 + 4]

movzx r9d, r11b
movzx r9d, r11w
movzx r9d, byte ptr [r11 + 4]
movzx r9d, word ptr [r11 + 4]

movzx r9, r11b
movzx r9, r11w
movzx r9, byte ptr [r11 + 4]
movzx r9, word ptr [r11 + 4]


; Source: r11, target: r10
mov r10b, 1
mov r10w, 1
mov r10d, 1
mov r10, 1

mov r10b, r11b
mov r10w, r11w
mov r10d, r11d
mov r10, r11

mov r10b, byte ptr [r11 - 4]
mov r10b, byte ptr [r11 + 4]
mov r10w, word ptr [r11 + 4]
mov r10d, dword ptr [r11 + 4]
mov r10, qword ptr [r11 + 4]

mov byte ptr [r10 - 4], r11b
mov byte ptr [r10 + 4], r11b
mov word ptr [r10 + 4], r11w
mov dword ptr [r10 + 4], r11d
mov qword ptr [r10 + 4], r11

movzx r10w, r11b
movzx r10w, byte ptr [r11 + 4]

movzx r10d, r11b
movzx r10d, r11w
movzx r10d, byte ptr [r11 + 4]
movzx r10d, word ptr [r11 + 4]

movzx r10, r11b
movzx r10, r11w
movzx r10, byte ptr [r11 + 4]
movzx r10, word ptr [r11 + 4]


; Source: r11, target: r11
mov r11b, 1
mov r11w, 1
mov r11d, 1
mov r11, 1

mov r11b, r11b
mov r11w, r11w
mov r11d, r11d
mov r11, r11

mov r11b, byte ptr [r11 - 4]
mov r11b, byte ptr [r11 + 4]
mov r11w, word ptr [r11 + 4]
mov r11d, dword ptr [r11 + 4]
mov r11, qword ptr [r11 + 4]

mov byte ptr [r11 - 4], r11b
mov byte ptr [r11 + 4], r11b
mov word ptr [r11 + 4], r11w
mov dword ptr [r11 + 4], r11d
mov qword ptr [r11 + 4], r11

movzx r11w, r11b
movzx r11w, byte ptr [r11 + 4]

movzx r11d, r11b
movzx r11d, r11w
movzx r11d, byte ptr [r11 + 4]
movzx r11d, word ptr [r11 + 4]

movzx r11, r11b
movzx r11, r11w
movzx r11, byte ptr [r11 + 4]
movzx r11, word ptr [r11 + 4]


; Source: r11, target: r12
mov r12b, 1
mov r12w, 1
mov r12d, 1
mov r12, 1

mov r12b, r11b
mov r12w, r11w
mov r12d, r11d
mov r12, r11

mov r12b, byte ptr [r11 - 4]
mov r12b, byte ptr [r11 + 4]
mov r12w, word ptr [r11 + 4]
mov r12d, dword ptr [r11 + 4]
mov r12, qword ptr [r11 + 4]

mov byte ptr [r12 - 4], r11b
mov byte ptr [r12 + 4], r11b
mov word ptr [r12 + 4], r11w
mov dword ptr [r12 + 4], r11d
mov qword ptr [r12 + 4], r11

movzx r12w, r11b
movzx r12w, byte ptr [r11 + 4]

movzx r12d, r11b
movzx r12d, r11w
movzx r12d, byte ptr [r11 + 4]
movzx r12d, word ptr [r11 + 4]

movzx r12, r11b
movzx r12, r11w
movzx r12, byte ptr [r11 + 4]
movzx r12, word ptr [r11 + 4]


; Source: r11, target: r13
mov r13b, 1
mov r13w, 1
mov r13d, 1
mov r13, 1

mov r13b, r11b
mov r13w, r11w
mov r13d, r11d
mov r13, r11

mov r13b, byte ptr [r11 - 4]
mov r13b, byte ptr [r11 + 4]
mov r13w, word ptr [r11 + 4]
mov r13d, dword ptr [r11 + 4]
mov r13, qword ptr [r11 + 4]

mov byte ptr [r13 - 4], r11b
mov byte ptr [r13 + 4], r11b
mov word ptr [r13 + 4], r11w
mov dword ptr [r13 + 4], r11d
mov qword ptr [r13 + 4], r11

movzx r13w, r11b
movzx r13w, byte ptr [r11 + 4]

movzx r13d, r11b
movzx r13d, r11w
movzx r13d, byte ptr [r11 + 4]
movzx r13d, word ptr [r11 + 4]

movzx r13, r11b
movzx r13, r11w
movzx r13, byte ptr [r11 + 4]
movzx r13, word ptr [r11 + 4]


; Source: r11, target: r14
mov r14b, 1
mov r14w, 1
mov r14d, 1
mov r14, 1

mov r14b, r11b
mov r14w, r11w
mov r14d, r11d
mov r14, r11

mov r14b, byte ptr [r11 - 4]
mov r14b, byte ptr [r11 + 4]
mov r14w, word ptr [r11 + 4]
mov r14d, dword ptr [r11 + 4]
mov r14, qword ptr [r11 + 4]

mov byte ptr [r14 - 4], r11b
mov byte ptr [r14 + 4], r11b
mov word ptr [r14 + 4], r11w
mov dword ptr [r14 + 4], r11d
mov qword ptr [r14 + 4], r11

movzx r14w, r11b
movzx r14w, byte ptr [r11 + 4]

movzx r14d, r11b
movzx r14d, r11w
movzx r14d, byte ptr [r11 + 4]
movzx r14d, word ptr [r11 + 4]

movzx r14, r11b
movzx r14, r11w
movzx r14, byte ptr [r11 + 4]
movzx r14, word ptr [r11 + 4]


; Source: r11, target: r15
mov r15b, 1
mov r15w, 1
mov r15d, 1
mov r15, 1

mov r15b, r11b
mov r15w, r11w
mov r15d, r11d
mov r15, r11

mov r15b, byte ptr [r11 - 4]
mov r15b, byte ptr [r11 + 4]
mov r15w, word ptr [r11 + 4]
mov r15d, dword ptr [r11 + 4]
mov r15, qword ptr [r11 + 4]

mov byte ptr [r15 - 4], r11b
mov byte ptr [r15 + 4], r11b
mov word ptr [r15 + 4], r11w
mov dword ptr [r15 + 4], r11d
mov qword ptr [r15 + 4], r11

movzx r15w, r11b
movzx r15w, byte ptr [r11 + 4]

movzx r15d, r11b
movzx r15d, r11w
movzx r15d, byte ptr [r11 + 4]
movzx r15d, word ptr [r11 + 4]

movzx r15, r11b
movzx r15, r11w
movzx r15, byte ptr [r11 + 4]
movzx r15, word ptr [r11 + 4]


; Source: r12, target: rax
mov al, 1
mov ax, 1
mov eax, 1
mov rax, 1

mov al, r12b
mov ax, r12w
mov eax, r12d
mov rax, r12

mov al, byte ptr [r12 - 4]
mov al, byte ptr [r12 + 4]
mov ax, word ptr [r12 + 4]
mov eax, dword ptr [r12 + 4]
mov rax, qword ptr [r12 + 4]

mov byte ptr [rax - 4], r12b
mov byte ptr [rax + 4], r12b
mov word ptr [rax + 4], r12w
mov dword ptr [rax + 4], r12d
mov qword ptr [rax + 4], r12

movzx ax, r12b
movzx ax, byte ptr [r12 + 4]

movzx eax, r12b
movzx eax, r12w
movzx eax, byte ptr [r12 + 4]
movzx eax, word ptr [r12 + 4]

movzx rax, r12b
movzx rax, r12w
movzx rax, byte ptr [r12 + 4]
movzx rax, word ptr [r12 + 4]


; Source: r12, target: rcx
mov cl, 1
mov cx, 1
mov ecx, 1
mov rcx, 1

mov cl, r12b
mov cx, r12w
mov ecx, r12d
mov rcx, r12

mov cl, byte ptr [r12 - 4]
mov cl, byte ptr [r12 + 4]
mov cx, word ptr [r12 + 4]
mov ecx, dword ptr [r12 + 4]
mov rcx, qword ptr [r12 + 4]

mov byte ptr [rcx - 4], r12b
mov byte ptr [rcx + 4], r12b
mov word ptr [rcx + 4], r12w
mov dword ptr [rcx + 4], r12d
mov qword ptr [rcx + 4], r12

movzx cx, r12b
movzx cx, byte ptr [r12 + 4]

movzx ecx, r12b
movzx ecx, r12w
movzx ecx, byte ptr [r12 + 4]
movzx ecx, word ptr [r12 + 4]

movzx rcx, r12b
movzx rcx, r12w
movzx rcx, byte ptr [r12 + 4]
movzx rcx, word ptr [r12 + 4]


; Source: r12, target: rdx
mov dl, 1
mov dx, 1
mov edx, 1
mov rdx, 1

mov dl, r12b
mov dx, r12w
mov edx, r12d
mov rdx, r12

mov dl, byte ptr [r12 - 4]
mov dl, byte ptr [r12 + 4]
mov dx, word ptr [r12 + 4]
mov edx, dword ptr [r12 + 4]
mov rdx, qword ptr [r12 + 4]

mov byte ptr [rdx - 4], r12b
mov byte ptr [rdx + 4], r12b
mov word ptr [rdx + 4], r12w
mov dword ptr [rdx + 4], r12d
mov qword ptr [rdx + 4], r12

movzx dx, r12b
movzx dx, byte ptr [r12 + 4]

movzx edx, r12b
movzx edx, r12w
movzx edx, byte ptr [r12 + 4]
movzx edx, word ptr [r12 + 4]

movzx rdx, r12b
movzx rdx, r12w
movzx rdx, byte ptr [r12 + 4]
movzx rdx, word ptr [r12 + 4]


; Source: r12, target: rbx
mov bl, 1
mov bx, 1
mov ebx, 1
mov rbx, 1

mov bl, r12b
mov bx, r12w
mov ebx, r12d
mov rbx, r12

mov bl, byte ptr [r12 - 4]
mov bl, byte ptr [r12 + 4]
mov bx, word ptr [r12 + 4]
mov ebx, dword ptr [r12 + 4]
mov rbx, qword ptr [r12 + 4]

mov byte ptr [rbx - 4], r12b
mov byte ptr [rbx + 4], r12b
mov word ptr [rbx + 4], r12w
mov dword ptr [rbx + 4], r12d
mov qword ptr [rbx + 4], r12

movzx bx, r12b
movzx bx, byte ptr [r12 + 4]

movzx ebx, r12b
movzx ebx, r12w
movzx ebx, byte ptr [r12 + 4]
movzx ebx, word ptr [r12 + 4]

movzx rbx, r12b
movzx rbx, r12w
movzx rbx, byte ptr [r12 + 4]
movzx rbx, word ptr [r12 + 4]


; Source: r12, target: rsp
mov spl, 1
mov sp, 1
mov esp, 1
mov rsp, 1

mov spl, r12b
mov sp, r12w
mov esp, r12d
mov rsp, r12

mov spl, byte ptr [r12 - 4]
mov spl, byte ptr [r12 + 4]
mov sp, word ptr [r12 + 4]
mov esp, dword ptr [r12 + 4]
mov rsp, qword ptr [r12 + 4]

mov byte ptr [rsp - 4], r12b
mov byte ptr [rsp + 4], r12b
mov word ptr [rsp + 4], r12w
mov dword ptr [rsp + 4], r12d
mov qword ptr [rsp + 4], r12

movzx sp, r12b
movzx sp, byte ptr [r12 + 4]

movzx esp, r12b
movzx esp, r12w
movzx esp, byte ptr [r12 + 4]
movzx esp, word ptr [r12 + 4]

movzx rsp, r12b
movzx rsp, r12w
movzx rsp, byte ptr [r12 + 4]
movzx rsp, word ptr [r12 + 4]


; Source: r12, target: rbp
mov bpl, 1
mov bp, 1
mov ebp, 1
mov rbp, 1

mov bpl, r12b
mov bp, r12w
mov ebp, r12d
mov rbp, r12

mov bpl, byte ptr [r12 - 4]
mov bpl, byte ptr [r12 + 4]
mov bp, word ptr [r12 + 4]
mov ebp, dword ptr [r12 + 4]
mov rbp, qword ptr [r12 + 4]

mov byte ptr [rbp - 4], r12b
mov byte ptr [rbp + 4], r12b
mov word ptr [rbp + 4], r12w
mov dword ptr [rbp + 4], r12d
mov qword ptr [rbp + 4], r12

movzx bp, r12b
movzx bp, byte ptr [r12 + 4]

movzx ebp, r12b
movzx ebp, r12w
movzx ebp, byte ptr [r12 + 4]
movzx ebp, word ptr [r12 + 4]

movzx rbp, r12b
movzx rbp, r12w
movzx rbp, byte ptr [r12 + 4]
movzx rbp, word ptr [r12 + 4]


; Source: r12, target: rsi
mov dil, 1
mov si, 1
mov esi, 1
mov rsi, 1

mov dil, r12b
mov si, r12w
mov esi, r12d
mov rsi, r12

mov dil, byte ptr [r12 - 4]
mov dil, byte ptr [r12 + 4]
mov si, word ptr [r12 + 4]
mov esi, dword ptr [r12 + 4]
mov rsi, qword ptr [r12 + 4]

mov byte ptr [rsi - 4], r12b
mov byte ptr [rsi + 4], r12b
mov word ptr [rsi + 4], r12w
mov dword ptr [rsi + 4], r12d
mov qword ptr [rsi + 4], r12

movzx si, r12b
movzx si, byte ptr [r12 + 4]

movzx esi, r12b
movzx esi, r12w
movzx esi, byte ptr [r12 + 4]
movzx esi, word ptr [r12 + 4]

movzx rsi, r12b
movzx rsi, r12w
movzx rsi, byte ptr [r12 + 4]
movzx rsi, word ptr [r12 + 4]


; Source: r12, target: rdi
mov sil, 1
mov di, 1
mov edi, 1
mov rdi, 1

mov sil, r12b
mov di, r12w
mov edi, r12d
mov rdi, r12

mov sil, byte ptr [r12 - 4]
mov sil, byte ptr [r12 + 4]
mov di, word ptr [r12 + 4]
mov edi, dword ptr [r12 + 4]
mov rdi, qword ptr [r12 + 4]

mov byte ptr [rdi - 4], r12b
mov byte ptr [rdi + 4], r12b
mov word ptr [rdi + 4], r12w
mov dword ptr [rdi + 4], r12d
mov qword ptr [rdi + 4], r12

movzx di, r12b
movzx di, byte ptr [r12 + 4]

movzx edi, r12b
movzx edi, r12w
movzx edi, byte ptr [r12 + 4]
movzx edi, word ptr [r12 + 4]

movzx rdi, r12b
movzx rdi, r12w
movzx rdi, byte ptr [r12 + 4]
movzx rdi, word ptr [r12 + 4]


; Source: r12, target: r8
mov r8b, 1
mov r8w, 1
mov r8d, 1
mov r8, 1

mov r8b, r12b
mov r8w, r12w
mov r8d, r12d
mov r8, r12

mov r8b, byte ptr [r12 - 4]
mov r8b, byte ptr [r12 + 4]
mov r8w, word ptr [r12 + 4]
mov r8d, dword ptr [r12 + 4]
mov r8, qword ptr [r12 + 4]

mov byte ptr [r8 - 4], r12b
mov byte ptr [r8 + 4], r12b
mov word ptr [r8 + 4], r12w
mov dword ptr [r8 + 4], r12d
mov qword ptr [r8 + 4], r12

movzx r8w, r12b
movzx r8w, byte ptr [r12 + 4]

movzx r8d, r12b
movzx r8d, r12w
movzx r8d, byte ptr [r12 + 4]
movzx r8d, word ptr [r12 + 4]

movzx r8, r12b
movzx r8, r12w
movzx r8, byte ptr [r12 + 4]
movzx r8, word ptr [r12 + 4]


; Source: r12, target: r9
mov r9b, 1
mov r9w, 1
mov r9d, 1
mov r9, 1

mov r9b, r12b
mov r9w, r12w
mov r9d, r12d
mov r9, r12

mov r9b, byte ptr [r12 - 4]
mov r9b, byte ptr [r12 + 4]
mov r9w, word ptr [r12 + 4]
mov r9d, dword ptr [r12 + 4]
mov r9, qword ptr [r12 + 4]

mov byte ptr [r9 - 4], r12b
mov byte ptr [r9 + 4], r12b
mov word ptr [r9 + 4], r12w
mov dword ptr [r9 + 4], r12d
mov qword ptr [r9 + 4], r12

movzx r9w, r12b
movzx r9w, byte ptr [r12 + 4]

movzx r9d, r12b
movzx r9d, r12w
movzx r9d, byte ptr [r12 + 4]
movzx r9d, word ptr [r12 + 4]

movzx r9, r12b
movzx r9, r12w
movzx r9, byte ptr [r12 + 4]
movzx r9, word ptr [r12 + 4]


; Source: r12, target: r10
mov r10b, 1
mov r10w, 1
mov r10d, 1
mov r10, 1

mov r10b, r12b
mov r10w, r12w
mov r10d, r12d
mov r10, r12

mov r10b, byte ptr [r12 - 4]
mov r10b, byte ptr [r12 + 4]
mov r10w, word ptr [r12 + 4]
mov r10d, dword ptr [r12 + 4]
mov r10, qword ptr [r12 + 4]

mov byte ptr [r10 - 4], r12b
mov byte ptr [r10 + 4], r12b
mov word ptr [r10 + 4], r12w
mov dword ptr [r10 + 4], r12d
mov qword ptr [r10 + 4], r12

movzx r10w, r12b
movzx r10w, byte ptr [r12 + 4]

movzx r10d, r12b
movzx r10d, r12w
movzx r10d, byte ptr [r12 + 4]
movzx r10d, word ptr [r12 + 4]

movzx r10, r12b
movzx r10, r12w
movzx r10, byte ptr [r12 + 4]
movzx r10, word ptr [r12 + 4]


; Source: r12, target: r11
mov r11b, 1
mov r11w, 1
mov r11d, 1
mov r11, 1

mov r11b, r12b
mov r11w, r12w
mov r11d, r12d
mov r11, r12

mov r11b, byte ptr [r12 - 4]
mov r11b, byte ptr [r12 + 4]
mov r11w, word ptr [r12 + 4]
mov r11d, dword ptr [r12 + 4]
mov r11, qword ptr [r12 + 4]

mov byte ptr [r11 - 4], r12b
mov byte ptr [r11 + 4], r12b
mov word ptr [r11 + 4], r12w
mov dword ptr [r11 + 4], r12d
mov qword ptr [r11 + 4], r12

movzx r11w, r12b
movzx r11w, byte ptr [r12 + 4]

movzx r11d, r12b
movzx r11d, r12w
movzx r11d, byte ptr [r12 + 4]
movzx r11d, word ptr [r12 + 4]

movzx r11, r12b
movzx r11, r12w
movzx r11, byte ptr [r12 + 4]
movzx r11, word ptr [r12 + 4]


; Source: r12, target: r12
mov r12b, 1
mov r12w, 1
mov r12d, 1
mov r12, 1

mov r12b, r12b
mov r12w, r12w
mov r12d, r12d
mov r12, r12

mov r12b, byte ptr [r12 - 4]
mov r12b, byte ptr [r12 + 4]
mov r12w, word ptr [r12 + 4]
mov r12d, dword ptr [r12 + 4]
mov r12, qword ptr [r12 + 4]

mov byte ptr [r12 - 4], r12b
mov byte ptr [r12 + 4], r12b
mov word ptr [r12 + 4], r12w
mov dword ptr [r12 + 4], r12d
mov qword ptr [r12 + 4], r12

movzx r12w, r12b
movzx r12w, byte ptr [r12 + 4]

movzx r12d, r12b
movzx r12d, r12w
movzx r12d, byte ptr [r12 + 4]
movzx r12d, word ptr [r12 + 4]

movzx r12, r12b
movzx r12, r12w
movzx r12, byte ptr [r12 + 4]
movzx r12, word ptr [r12 + 4]


; Source: r12, target: r13
mov r13b, 1
mov r13w, 1
mov r13d, 1
mov r13, 1

mov r13b, r12b
mov r13w, r12w
mov r13d, r12d
mov r13, r12

mov r13b, byte ptr [r12 - 4]
mov r13b, byte ptr [r12 + 4]
mov r13w, word ptr [r12 + 4]
mov r13d, dword ptr [r12 + 4]
mov r13, qword ptr [r12 + 4]

mov byte ptr [r13 - 4], r12b
mov byte ptr [r13 + 4], r12b
mov word ptr [r13 + 4], r12w
mov dword ptr [r13 + 4], r12d
mov qword ptr [r13 + 4], r12

movzx r13w, r12b
movzx r13w, byte ptr [r12 + 4]

movzx r13d, r12b
movzx r13d, r12w
movzx r13d, byte ptr [r12 + 4]
movzx r13d, word ptr [r12 + 4]

movzx r13, r12b
movzx r13, r12w
movzx r13, byte ptr [r12 + 4]
movzx r13, word ptr [r12 + 4]


; Source: r12, target: r14
mov r14b, 1
mov r14w, 1
mov r14d, 1
mov r14, 1

mov r14b, r12b
mov r14w, r12w
mov r14d, r12d
mov r14, r12

mov r14b, byte ptr [r12 - 4]
mov r14b, byte ptr [r12 + 4]
mov r14w, word ptr [r12 + 4]
mov r14d, dword ptr [r12 + 4]
mov r14, qword ptr [r12 + 4]

mov byte ptr [r14 - 4], r12b
mov byte ptr [r14 + 4], r12b
mov word ptr [r14 + 4], r12w
mov dword ptr [r14 + 4], r12d
mov qword ptr [r14 + 4], r12

movzx r14w, r12b
movzx r14w, byte ptr [r12 + 4]

movzx r14d, r12b
movzx r14d, r12w
movzx r14d, byte ptr [r12 + 4]
movzx r14d, word ptr [r12 + 4]

movzx r14, r12b
movzx r14, r12w
movzx r14, byte ptr [r12 + 4]
movzx r14, word ptr [r12 + 4]


; Source: r12, target: r15
mov r15b, 1
mov r15w, 1
mov r15d, 1
mov r15, 1

mov r15b, r12b
mov r15w, r12w
mov r15d, r12d
mov r15, r12

mov r15b, byte ptr [r12 - 4]
mov r15b, byte ptr [r12 + 4]
mov r15w, word ptr [r12 + 4]
mov r15d, dword ptr [r12 + 4]
mov r15, qword ptr [r12 + 4]

mov byte ptr [r15 - 4], r12b
mov byte ptr [r15 + 4], r12b
mov word ptr [r15 + 4], r12w
mov dword ptr [r15 + 4], r12d
mov qword ptr [r15 + 4], r12

movzx r15w, r12b
movzx r15w, byte ptr [r12 + 4]

movzx r15d, r12b
movzx r15d, r12w
movzx r15d, byte ptr [r12 + 4]
movzx r15d, word ptr [r12 + 4]

movzx r15, r12b
movzx r15, r12w
movzx r15, byte ptr [r12 + 4]
movzx r15, word ptr [r12 + 4]


; Source: r13, target: rax
mov al, 1
mov ax, 1
mov eax, 1
mov rax, 1

mov al, r13b
mov ax, r13w
mov eax, r13d
mov rax, r13

mov al, byte ptr [r13 - 4]
mov al, byte ptr [r13 + 4]
mov ax, word ptr [r13 + 4]
mov eax, dword ptr [r13 + 4]
mov rax, qword ptr [r13 + 4]

mov byte ptr [rax - 4], r13b
mov byte ptr [rax + 4], r13b
mov word ptr [rax + 4], r13w
mov dword ptr [rax + 4], r13d
mov qword ptr [rax + 4], r13

movzx ax, r13b
movzx ax, byte ptr [r13 + 4]

movzx eax, r13b
movzx eax, r13w
movzx eax, byte ptr [r13 + 4]
movzx eax, word ptr [r13 + 4]

movzx rax, r13b
movzx rax, r13w
movzx rax, byte ptr [r13 + 4]
movzx rax, word ptr [r13 + 4]


; Source: r13, target: rcx
mov cl, 1
mov cx, 1
mov ecx, 1
mov rcx, 1

mov cl, r13b
mov cx, r13w
mov ecx, r13d
mov rcx, r13

mov cl, byte ptr [r13 - 4]
mov cl, byte ptr [r13 + 4]
mov cx, word ptr [r13 + 4]
mov ecx, dword ptr [r13 + 4]
mov rcx, qword ptr [r13 + 4]

mov byte ptr [rcx - 4], r13b
mov byte ptr [rcx + 4], r13b
mov word ptr [rcx + 4], r13w
mov dword ptr [rcx + 4], r13d
mov qword ptr [rcx + 4], r13

movzx cx, r13b
movzx cx, byte ptr [r13 + 4]

movzx ecx, r13b
movzx ecx, r13w
movzx ecx, byte ptr [r13 + 4]
movzx ecx, word ptr [r13 + 4]

movzx rcx, r13b
movzx rcx, r13w
movzx rcx, byte ptr [r13 + 4]
movzx rcx, word ptr [r13 + 4]


; Source: r13, target: rdx
mov dl, 1
mov dx, 1
mov edx, 1
mov rdx, 1

mov dl, r13b
mov dx, r13w
mov edx, r13d
mov rdx, r13

mov dl, byte ptr [r13 - 4]
mov dl, byte ptr [r13 + 4]
mov dx, word ptr [r13 + 4]
mov edx, dword ptr [r13 + 4]
mov rdx, qword ptr [r13 + 4]

mov byte ptr [rdx - 4], r13b
mov byte ptr [rdx + 4], r13b
mov word ptr [rdx + 4], r13w
mov dword ptr [rdx + 4], r13d
mov qword ptr [rdx + 4], r13

movzx dx, r13b
movzx dx, byte ptr [r13 + 4]

movzx edx, r13b
movzx edx, r13w
movzx edx, byte ptr [r13 + 4]
movzx edx, word ptr [r13 + 4]

movzx rdx, r13b
movzx rdx, r13w
movzx rdx, byte ptr [r13 + 4]
movzx rdx, word ptr [r13 + 4]


; Source: r13, target: rbx
mov bl, 1
mov bx, 1
mov ebx, 1
mov rbx, 1

mov bl, r13b
mov bx, r13w
mov ebx, r13d
mov rbx, r13

mov bl, byte ptr [r13 - 4]
mov bl, byte ptr [r13 + 4]
mov bx, word ptr [r13 + 4]
mov ebx, dword ptr [r13 + 4]
mov rbx, qword ptr [r13 + 4]

mov byte ptr [rbx - 4], r13b
mov byte ptr [rbx + 4], r13b
mov word ptr [rbx + 4], r13w
mov dword ptr [rbx + 4], r13d
mov qword ptr [rbx + 4], r13

movzx bx, r13b
movzx bx, byte ptr [r13 + 4]

movzx ebx, r13b
movzx ebx, r13w
movzx ebx, byte ptr [r13 + 4]
movzx ebx, word ptr [r13 + 4]

movzx rbx, r13b
movzx rbx, r13w
movzx rbx, byte ptr [r13 + 4]
movzx rbx, word ptr [r13 + 4]


; Source: r13, target: rsp
mov spl, 1
mov sp, 1
mov esp, 1
mov rsp, 1

mov spl, r13b
mov sp, r13w
mov esp, r13d
mov rsp, r13

mov spl, byte ptr [r13 - 4]
mov spl, byte ptr [r13 + 4]
mov sp, word ptr [r13 + 4]
mov esp, dword ptr [r13 + 4]
mov rsp, qword ptr [r13 + 4]

mov byte ptr [rsp - 4], r13b
mov byte ptr [rsp + 4], r13b
mov word ptr [rsp + 4], r13w
mov dword ptr [rsp + 4], r13d
mov qword ptr [rsp + 4], r13

movzx sp, r13b
movzx sp, byte ptr [r13 + 4]

movzx esp, r13b
movzx esp, r13w
movzx esp, byte ptr [r13 + 4]
movzx esp, word ptr [r13 + 4]

movzx rsp, r13b
movzx rsp, r13w
movzx rsp, byte ptr [r13 + 4]
movzx rsp, word ptr [r13 + 4]


; Source: r13, target: rbp
mov bpl, 1
mov bp, 1
mov ebp, 1
mov rbp, 1

mov bpl, r13b
mov bp, r13w
mov ebp, r13d
mov rbp, r13

mov bpl, byte ptr [r13 - 4]
mov bpl, byte ptr [r13 + 4]
mov bp, word ptr [r13 + 4]
mov ebp, dword ptr [r13 + 4]
mov rbp, qword ptr [r13 + 4]

mov byte ptr [rbp - 4], r13b
mov byte ptr [rbp + 4], r13b
mov word ptr [rbp + 4], r13w
mov dword ptr [rbp + 4], r13d
mov qword ptr [rbp + 4], r13

movzx bp, r13b
movzx bp, byte ptr [r13 + 4]

movzx ebp, r13b
movzx ebp, r13w
movzx ebp, byte ptr [r13 + 4]
movzx ebp, word ptr [r13 + 4]

movzx rbp, r13b
movzx rbp, r13w
movzx rbp, byte ptr [r13 + 4]
movzx rbp, word ptr [r13 + 4]


; Source: r13, target: rsi
mov dil, 1
mov si, 1
mov esi, 1
mov rsi, 1

mov dil, r13b
mov si, r13w
mov esi, r13d
mov rsi, r13

mov dil, byte ptr [r13 - 4]
mov dil, byte ptr [r13 + 4]
mov si, word ptr [r13 + 4]
mov esi, dword ptr [r13 + 4]
mov rsi, qword ptr [r13 + 4]

mov byte ptr [rsi - 4], r13b
mov byte ptr [rsi + 4], r13b
mov word ptr [rsi + 4], r13w
mov dword ptr [rsi + 4], r13d
mov qword ptr [rsi + 4], r13

movzx si, r13b
movzx si, byte ptr [r13 + 4]

movzx esi, r13b
movzx esi, r13w
movzx esi, byte ptr [r13 + 4]
movzx esi, word ptr [r13 + 4]

movzx rsi, r13b
movzx rsi, r13w
movzx rsi, byte ptr [r13 + 4]
movzx rsi, word ptr [r13 + 4]


; Source: r13, target: rdi
mov sil, 1
mov di, 1
mov edi, 1
mov rdi, 1

mov sil, r13b
mov di, r13w
mov edi, r13d
mov rdi, r13

mov sil, byte ptr [r13 - 4]
mov sil, byte ptr [r13 + 4]
mov di, word ptr [r13 + 4]
mov edi, dword ptr [r13 + 4]
mov rdi, qword ptr [r13 + 4]

mov byte ptr [rdi - 4], r13b
mov byte ptr [rdi + 4], r13b
mov word ptr [rdi + 4], r13w
mov dword ptr [rdi + 4], r13d
mov qword ptr [rdi + 4], r13

movzx di, r13b
movzx di, byte ptr [r13 + 4]

movzx edi, r13b
movzx edi, r13w
movzx edi, byte ptr [r13 + 4]
movzx edi, word ptr [r13 + 4]

movzx rdi, r13b
movzx rdi, r13w
movzx rdi, byte ptr [r13 + 4]
movzx rdi, word ptr [r13 + 4]


; Source: r13, target: r8
mov r8b, 1
mov r8w, 1
mov r8d, 1
mov r8, 1

mov r8b, r13b
mov r8w, r13w
mov r8d, r13d
mov r8, r13

mov r8b, byte ptr [r13 - 4]
mov r8b, byte ptr [r13 + 4]
mov r8w, word ptr [r13 + 4]
mov r8d, dword ptr [r13 + 4]
mov r8, qword ptr [r13 + 4]

mov byte ptr [r8 - 4], r13b
mov byte ptr [r8 + 4], r13b
mov word ptr [r8 + 4], r13w
mov dword ptr [r8 + 4], r13d
mov qword ptr [r8 + 4], r13

movzx r8w, r13b
movzx r8w, byte ptr [r13 + 4]

movzx r8d, r13b
movzx r8d, r13w
movzx r8d, byte ptr [r13 + 4]
movzx r8d, word ptr [r13 + 4]

movzx r8, r13b
movzx r8, r13w
movzx r8, byte ptr [r13 + 4]
movzx r8, word ptr [r13 + 4]


; Source: r13, target: r9
mov r9b, 1
mov r9w, 1
mov r9d, 1
mov r9, 1

mov r9b, r13b
mov r9w, r13w
mov r9d, r13d
mov r9, r13

mov r9b, byte ptr [r13 - 4]
mov r9b, byte ptr [r13 + 4]
mov r9w, word ptr [r13 + 4]
mov r9d, dword ptr [r13 + 4]
mov r9, qword ptr [r13 + 4]

mov byte ptr [r9 - 4], r13b
mov byte ptr [r9 + 4], r13b
mov word ptr [r9 + 4], r13w
mov dword ptr [r9 + 4], r13d
mov qword ptr [r9 + 4], r13

movzx r9w, r13b
movzx r9w, byte ptr [r13 + 4]

movzx r9d, r13b
movzx r9d, r13w
movzx r9d, byte ptr [r13 + 4]
movzx r9d, word ptr [r13 + 4]

movzx r9, r13b
movzx r9, r13w
movzx r9, byte ptr [r13 + 4]
movzx r9, word ptr [r13 + 4]


; Source: r13, target: r10
mov r10b, 1
mov r10w, 1
mov r10d, 1
mov r10, 1

mov r10b, r13b
mov r10w, r13w
mov r10d, r13d
mov r10, r13

mov r10b, byte ptr [r13 - 4]
mov r10b, byte ptr [r13 + 4]
mov r10w, word ptr [r13 + 4]
mov r10d, dword ptr [r13 + 4]
mov r10, qword ptr [r13 + 4]

mov byte ptr [r10 - 4], r13b
mov byte ptr [r10 + 4], r13b
mov word ptr [r10 + 4], r13w
mov dword ptr [r10 + 4], r13d
mov qword ptr [r10 + 4], r13

movzx r10w, r13b
movzx r10w, byte ptr [r13 + 4]

movzx r10d, r13b
movzx r10d, r13w
movzx r10d, byte ptr [r13 + 4]
movzx r10d, word ptr [r13 + 4]

movzx r10, r13b
movzx r10, r13w
movzx r10, byte ptr [r13 + 4]
movzx r10, word ptr [r13 + 4]


; Source: r13, target: r11
mov r11b, 1
mov r11w, 1
mov r11d, 1
mov r11, 1

mov r11b, r13b
mov r11w, r13w
mov r11d, r13d
mov r11, r13

mov r11b, byte ptr [r13 - 4]
mov r11b, byte ptr [r13 + 4]
mov r11w, word ptr [r13 + 4]
mov r11d, dword ptr [r13 + 4]
mov r11, qword ptr [r13 + 4]

mov byte ptr [r11 - 4], r13b
mov byte ptr [r11 + 4], r13b
mov word ptr [r11 + 4], r13w
mov dword ptr [r11 + 4], r13d
mov qword ptr [r11 + 4], r13

movzx r11w, r13b
movzx r11w, byte ptr [r13 + 4]

movzx r11d, r13b
movzx r11d, r13w
movzx r11d, byte ptr [r13 + 4]
movzx r11d, word ptr [r13 + 4]

movzx r11, r13b
movzx r11, r13w
movzx r11, byte ptr [r13 + 4]
movzx r11, word ptr [r13 + 4]


; Source: r13, target: r12
mov r12b, 1
mov r12w, 1
mov r12d, 1
mov r12, 1

mov r12b, r13b
mov r12w, r13w
mov r12d, r13d
mov r12, r13

mov r12b, byte ptr [r13 - 4]
mov r12b, byte ptr [r13 + 4]
mov r12w, word ptr [r13 + 4]
mov r12d, dword ptr [r13 + 4]
mov r12, qword ptr [r13 + 4]

mov byte ptr [r12 - 4], r13b
mov byte ptr [r12 + 4], r13b
mov word ptr [r12 + 4], r13w
mov dword ptr [r12 + 4], r13d
mov qword ptr [r12 + 4], r13

movzx r12w, r13b
movzx r12w, byte ptr [r13 + 4]

movzx r12d, r13b
movzx r12d, r13w
movzx r12d, byte ptr [r13 + 4]
movzx r12d, word ptr [r13 + 4]

movzx r12, r13b
movzx r12, r13w
movzx r12, byte ptr [r13 + 4]
movzx r12, word ptr [r13 + 4]


; Source: r13, target: r13
mov r13b, 1
mov r13w, 1
mov r13d, 1
mov r13, 1

mov r13b, r13b
mov r13w, r13w
mov r13d, r13d
mov r13, r13

mov r13b, byte ptr [r13 - 4]
mov r13b, byte ptr [r13 + 4]
mov r13w, word ptr [r13 + 4]
mov r13d, dword ptr [r13 + 4]
mov r13, qword ptr [r13 + 4]

mov byte ptr [r13 - 4], r13b
mov byte ptr [r13 + 4], r13b
mov word ptr [r13 + 4], r13w
mov dword ptr [r13 + 4], r13d
mov qword ptr [r13 + 4], r13

movzx r13w, r13b
movzx r13w, byte ptr [r13 + 4]

movzx r13d, r13b
movzx r13d, r13w
movzx r13d, byte ptr [r13 + 4]
movzx r13d, word ptr [r13 + 4]

movzx r13, r13b
movzx r13, r13w
movzx r13, byte ptr [r13 + 4]
movzx r13, word ptr [r13 + 4]


; Source: r13, target: r14
mov r14b, 1
mov r14w, 1
mov r14d, 1
mov r14, 1

mov r14b, r13b
mov r14w, r13w
mov r14d, r13d
mov r14, r13

mov r14b, byte ptr [r13 - 4]
mov r14b, byte ptr [r13 + 4]
mov r14w, word ptr [r13 + 4]
mov r14d, dword ptr [r13 + 4]
mov r14, qword ptr [r13 + 4]

mov byte ptr [r14 - 4], r13b
mov byte ptr [r14 + 4], r13b
mov word ptr [r14 + 4], r13w
mov dword ptr [r14 + 4], r13d
mov qword ptr [r14 + 4], r13

movzx r14w, r13b
movzx r14w, byte ptr [r13 + 4]

movzx r14d, r13b
movzx r14d, r13w
movzx r14d, byte ptr [r13 + 4]
movzx r14d, word ptr [r13 + 4]

movzx r14, r13b
movzx r14, r13w
movzx r14, byte ptr [r13 + 4]
movzx r14, word ptr [r13 + 4]


; Source: r13, target: r15
mov r15b, 1
mov r15w, 1
mov r15d, 1
mov r15, 1

mov r15b, r13b
mov r15w, r13w
mov r15d, r13d
mov r15, r13

mov r15b, byte ptr [r13 - 4]
mov r15b, byte ptr [r13 + 4]
mov r15w, word ptr [r13 + 4]
mov r15d, dword ptr [r13 + 4]
mov r15, qword ptr [r13 + 4]

mov byte ptr [r15 - 4], r13b
mov byte ptr [r15 + 4], r13b
mov word ptr [r15 + 4], r13w
mov dword ptr [r15 + 4], r13d
mov qword ptr [r15 + 4], r13

movzx r15w, r13b
movzx r15w, byte ptr [r13 + 4]

movzx r15d, r13b
movzx r15d, r13w
movzx r15d, byte ptr [r13 + 4]
movzx r15d, word ptr [r13 + 4]

movzx r15, r13b
movzx r15, r13w
movzx r15, byte ptr [r13 + 4]
movzx r15, word ptr [r13 + 4]


; Source: r14, target: rax
mov al, 1
mov ax, 1
mov eax, 1
mov rax, 1

mov al, r14b
mov ax, r14w
mov eax, r14d
mov rax, r14

mov al, byte ptr [r14 - 4]
mov al, byte ptr [r14 + 4]
mov ax, word ptr [r14 + 4]
mov eax, dword ptr [r14 + 4]
mov rax, qword ptr [r14 + 4]

mov byte ptr [rax - 4], r14b
mov byte ptr [rax + 4], r14b
mov word ptr [rax + 4], r14w
mov dword ptr [rax + 4], r14d
mov qword ptr [rax + 4], r14

movzx ax, r14b
movzx ax, byte ptr [r14 + 4]

movzx eax, r14b
movzx eax, r14w
movzx eax, byte ptr [r14 + 4]
movzx eax, word ptr [r14 + 4]

movzx rax, r14b
movzx rax, r14w
movzx rax, byte ptr [r14 + 4]
movzx rax, word ptr [r14 + 4]


; Source: r14, target: rcx
mov cl, 1
mov cx, 1
mov ecx, 1
mov rcx, 1

mov cl, r14b
mov cx, r14w
mov ecx, r14d
mov rcx, r14

mov cl, byte ptr [r14 - 4]
mov cl, byte ptr [r14 + 4]
mov cx, word ptr [r14 + 4]
mov ecx, dword ptr [r14 + 4]
mov rcx, qword ptr [r14 + 4]

mov byte ptr [rcx - 4], r14b
mov byte ptr [rcx + 4], r14b
mov word ptr [rcx + 4], r14w
mov dword ptr [rcx + 4], r14d
mov qword ptr [rcx + 4], r14

movzx cx, r14b
movzx cx, byte ptr [r14 + 4]

movzx ecx, r14b
movzx ecx, r14w
movzx ecx, byte ptr [r14 + 4]
movzx ecx, word ptr [r14 + 4]

movzx rcx, r14b
movzx rcx, r14w
movzx rcx, byte ptr [r14 + 4]
movzx rcx, word ptr [r14 + 4]


; Source: r14, target: rdx
mov dl, 1
mov dx, 1
mov edx, 1
mov rdx, 1

mov dl, r14b
mov dx, r14w
mov edx, r14d
mov rdx, r14

mov dl, byte ptr [r14 - 4]
mov dl, byte ptr [r14 + 4]
mov dx, word ptr [r14 + 4]
mov edx, dword ptr [r14 + 4]
mov rdx, qword ptr [r14 + 4]

mov byte ptr [rdx - 4], r14b
mov byte ptr [rdx + 4], r14b
mov word ptr [rdx + 4], r14w
mov dword ptr [rdx + 4], r14d
mov qword ptr [rdx + 4], r14

movzx dx, r14b
movzx dx, byte ptr [r14 + 4]

movzx edx, r14b
movzx edx, r14w
movzx edx, byte ptr [r14 + 4]
movzx edx, word ptr [r14 + 4]

movzx rdx, r14b
movzx rdx, r14w
movzx rdx, byte ptr [r14 + 4]
movzx rdx, word ptr [r14 + 4]


; Source: r14, target: rbx
mov bl, 1
mov bx, 1
mov ebx, 1
mov rbx, 1

mov bl, r14b
mov bx, r14w
mov ebx, r14d
mov rbx, r14

mov bl, byte ptr [r14 - 4]
mov bl, byte ptr [r14 + 4]
mov bx, word ptr [r14 + 4]
mov ebx, dword ptr [r14 + 4]
mov rbx, qword ptr [r14 + 4]

mov byte ptr [rbx - 4], r14b
mov byte ptr [rbx + 4], r14b
mov word ptr [rbx + 4], r14w
mov dword ptr [rbx + 4], r14d
mov qword ptr [rbx + 4], r14

movzx bx, r14b
movzx bx, byte ptr [r14 + 4]

movzx ebx, r14b
movzx ebx, r14w
movzx ebx, byte ptr [r14 + 4]
movzx ebx, word ptr [r14 + 4]

movzx rbx, r14b
movzx rbx, r14w
movzx rbx, byte ptr [r14 + 4]
movzx rbx, word ptr [r14 + 4]


; Source: r14, target: rsp
mov spl, 1
mov sp, 1
mov esp, 1
mov rsp, 1

mov spl, r14b
mov sp, r14w
mov esp, r14d
mov rsp, r14

mov spl, byte ptr [r14 - 4]
mov spl, byte ptr [r14 + 4]
mov sp, word ptr [r14 + 4]
mov esp, dword ptr [r14 + 4]
mov rsp, qword ptr [r14 + 4]

mov byte ptr [rsp - 4], r14b
mov byte ptr [rsp + 4], r14b
mov word ptr [rsp + 4], r14w
mov dword ptr [rsp + 4], r14d
mov qword ptr [rsp + 4], r14

movzx sp, r14b
movzx sp, byte ptr [r14 + 4]

movzx esp, r14b
movzx esp, r14w
movzx esp, byte ptr [r14 + 4]
movzx esp, word ptr [r14 + 4]

movzx rsp, r14b
movzx rsp, r14w
movzx rsp, byte ptr [r14 + 4]
movzx rsp, word ptr [r14 + 4]


; Source: r14, target: rbp
mov bpl, 1
mov bp, 1
mov ebp, 1
mov rbp, 1

mov bpl, r14b
mov bp, r14w
mov ebp, r14d
mov rbp, r14

mov bpl, byte ptr [r14 - 4]
mov bpl, byte ptr [r14 + 4]
mov bp, word ptr [r14 + 4]
mov ebp, dword ptr [r14 + 4]
mov rbp, qword ptr [r14 + 4]

mov byte ptr [rbp - 4], r14b
mov byte ptr [rbp + 4], r14b
mov word ptr [rbp + 4], r14w
mov dword ptr [rbp + 4], r14d
mov qword ptr [rbp + 4], r14

movzx bp, r14b
movzx bp, byte ptr [r14 + 4]

movzx ebp, r14b
movzx ebp, r14w
movzx ebp, byte ptr [r14 + 4]
movzx ebp, word ptr [r14 + 4]

movzx rbp, r14b
movzx rbp, r14w
movzx rbp, byte ptr [r14 + 4]
movzx rbp, word ptr [r14 + 4]


; Source: r14, target: rsi
mov dil, 1
mov si, 1
mov esi, 1
mov rsi, 1

mov dil, r14b
mov si, r14w
mov esi, r14d
mov rsi, r14

mov dil, byte ptr [r14 - 4]
mov dil, byte ptr [r14 + 4]
mov si, word ptr [r14 + 4]
mov esi, dword ptr [r14 + 4]
mov rsi, qword ptr [r14 + 4]

mov byte ptr [rsi - 4], r14b
mov byte ptr [rsi + 4], r14b
mov word ptr [rsi + 4], r14w
mov dword ptr [rsi + 4], r14d
mov qword ptr [rsi + 4], r14

movzx si, r14b
movzx si, byte ptr [r14 + 4]

movzx esi, r14b
movzx esi, r14w
movzx esi, byte ptr [r14 + 4]
movzx esi, word ptr [r14 + 4]

movzx rsi, r14b
movzx rsi, r14w
movzx rsi, byte ptr [r14 + 4]
movzx rsi, word ptr [r14 + 4]


; Source: r14, target: rdi
mov sil, 1
mov di, 1
mov edi, 1
mov rdi, 1

mov sil, r14b
mov di, r14w
mov edi, r14d
mov rdi, r14

mov sil, byte ptr [r14 - 4]
mov sil, byte ptr [r14 + 4]
mov di, word ptr [r14 + 4]
mov edi, dword ptr [r14 + 4]
mov rdi, qword ptr [r14 + 4]

mov byte ptr [rdi - 4], r14b
mov byte ptr [rdi + 4], r14b
mov word ptr [rdi + 4], r14w
mov dword ptr [rdi + 4], r14d
mov qword ptr [rdi + 4], r14

movzx di, r14b
movzx di, byte ptr [r14 + 4]

movzx edi, r14b
movzx edi, r14w
movzx edi, byte ptr [r14 + 4]
movzx edi, word ptr [r14 + 4]

movzx rdi, r14b
movzx rdi, r14w
movzx rdi, byte ptr [r14 + 4]
movzx rdi, word ptr [r14 + 4]


; Source: r14, target: r8
mov r8b, 1
mov r8w, 1
mov r8d, 1
mov r8, 1

mov r8b, r14b
mov r8w, r14w
mov r8d, r14d
mov r8, r14

mov r8b, byte ptr [r14 - 4]
mov r8b, byte ptr [r14 + 4]
mov r8w, word ptr [r14 + 4]
mov r8d, dword ptr [r14 + 4]
mov r8, qword ptr [r14 + 4]

mov byte ptr [r8 - 4], r14b
mov byte ptr [r8 + 4], r14b
mov word ptr [r8 + 4], r14w
mov dword ptr [r8 + 4], r14d
mov qword ptr [r8 + 4], r14

movzx r8w, r14b
movzx r8w, byte ptr [r14 + 4]

movzx r8d, r14b
movzx r8d, r14w
movzx r8d, byte ptr [r14 + 4]
movzx r8d, word ptr [r14 + 4]

movzx r8, r14b
movzx r8, r14w
movzx r8, byte ptr [r14 + 4]
movzx r8, word ptr [r14 + 4]


; Source: r14, target: r9
mov r9b, 1
mov r9w, 1
mov r9d, 1
mov r9, 1

mov r9b, r14b
mov r9w, r14w
mov r9d, r14d
mov r9, r14

mov r9b, byte ptr [r14 - 4]
mov r9b, byte ptr [r14 + 4]
mov r9w, word ptr [r14 + 4]
mov r9d, dword ptr [r14 + 4]
mov r9, qword ptr [r14 + 4]

mov byte ptr [r9 - 4], r14b
mov byte ptr [r9 + 4], r14b
mov word ptr [r9 + 4], r14w
mov dword ptr [r9 + 4], r14d
mov qword ptr [r9 + 4], r14

movzx r9w, r14b
movzx r9w, byte ptr [r14 + 4]

movzx r9d, r14b
movzx r9d, r14w
movzx r9d, byte ptr [r14 + 4]
movzx r9d, word ptr [r14 + 4]

movzx r9, r14b
movzx r9, r14w
movzx r9, byte ptr [r14 + 4]
movzx r9, word ptr [r14 + 4]


; Source: r14, target: r10
mov r10b, 1
mov r10w, 1
mov r10d, 1
mov r10, 1

mov r10b, r14b
mov r10w, r14w
mov r10d, r14d
mov r10, r14

mov r10b, byte ptr [r14 - 4]
mov r10b, byte ptr [r14 + 4]
mov r10w, word ptr [r14 + 4]
mov r10d, dword ptr [r14 + 4]
mov r10, qword ptr [r14 + 4]

mov byte ptr [r10 - 4], r14b
mov byte ptr [r10 + 4], r14b
mov word ptr [r10 + 4], r14w
mov dword ptr [r10 + 4], r14d
mov qword ptr [r10 + 4], r14

movzx r10w, r14b
movzx r10w, byte ptr [r14 + 4]

movzx r10d, r14b
movzx r10d, r14w
movzx r10d, byte ptr [r14 + 4]
movzx r10d, word ptr [r14 + 4]

movzx r10, r14b
movzx r10, r14w
movzx r10, byte ptr [r14 + 4]
movzx r10, word ptr [r14 + 4]


; Source: r14, target: r11
mov r11b, 1
mov r11w, 1
mov r11d, 1
mov r11, 1

mov r11b, r14b
mov r11w, r14w
mov r11d, r14d
mov r11, r14

mov r11b, byte ptr [r14 - 4]
mov r11b, byte ptr [r14 + 4]
mov r11w, word ptr [r14 + 4]
mov r11d, dword ptr [r14 + 4]
mov r11, qword ptr [r14 + 4]

mov byte ptr [r11 - 4], r14b
mov byte ptr [r11 + 4], r14b
mov word ptr [r11 + 4], r14w
mov dword ptr [r11 + 4], r14d
mov qword ptr [r11 + 4], r14

movzx r11w, r14b
movzx r11w, byte ptr [r14 + 4]

movzx r11d, r14b
movzx r11d, r14w
movzx r11d, byte ptr [r14 + 4]
movzx r11d, word ptr [r14 + 4]

movzx r11, r14b
movzx r11, r14w
movzx r11, byte ptr [r14 + 4]
movzx r11, word ptr [r14 + 4]


; Source: r14, target: r12
mov r12b, 1
mov r12w, 1
mov r12d, 1
mov r12, 1

mov r12b, r14b
mov r12w, r14w
mov r12d, r14d
mov r12, r14

mov r12b, byte ptr [r14 - 4]
mov r12b, byte ptr [r14 + 4]
mov r12w, word ptr [r14 + 4]
mov r12d, dword ptr [r14 + 4]
mov r12, qword ptr [r14 + 4]

mov byte ptr [r12 - 4], r14b
mov byte ptr [r12 + 4], r14b
mov word ptr [r12 + 4], r14w
mov dword ptr [r12 + 4], r14d
mov qword ptr [r12 + 4], r14

movzx r12w, r14b
movzx r12w, byte ptr [r14 + 4]

movzx r12d, r14b
movzx r12d, r14w
movzx r12d, byte ptr [r14 + 4]
movzx r12d, word ptr [r14 + 4]

movzx r12, r14b
movzx r12, r14w
movzx r12, byte ptr [r14 + 4]
movzx r12, word ptr [r14 + 4]


; Source: r14, target: r13
mov r13b, 1
mov r13w, 1
mov r13d, 1
mov r13, 1

mov r13b, r14b
mov r13w, r14w
mov r13d, r14d
mov r13, r14

mov r13b, byte ptr [r14 - 4]
mov r13b, byte ptr [r14 + 4]
mov r13w, word ptr [r14 + 4]
mov r13d, dword ptr [r14 + 4]
mov r13, qword ptr [r14 + 4]

mov byte ptr [r13 - 4], r14b
mov byte ptr [r13 + 4], r14b
mov word ptr [r13 + 4], r14w
mov dword ptr [r13 + 4], r14d
mov qword ptr [r13 + 4], r14

movzx r13w, r14b
movzx r13w, byte ptr [r14 + 4]

movzx r13d, r14b
movzx r13d, r14w
movzx r13d, byte ptr [r14 + 4]
movzx r13d, word ptr [r14 + 4]

movzx r13, r14b
movzx r13, r14w
movzx r13, byte ptr [r14 + 4]
movzx r13, word ptr [r14 + 4]


; Source: r14, target: r14
mov r14b, 1
mov r14w, 1
mov r14d, 1
mov r14, 1

mov r14b, r14b
mov r14w, r14w
mov r14d, r14d
mov r14, r14

mov r14b, byte ptr [r14 - 4]
mov r14b, byte ptr [r14 + 4]
mov r14w, word ptr [r14 + 4]
mov r14d, dword ptr [r14 + 4]
mov r14, qword ptr [r14 + 4]

mov byte ptr [r14 - 4], r14b
mov byte ptr [r14 + 4], r14b
mov word ptr [r14 + 4], r14w
mov dword ptr [r14 + 4], r14d
mov qword ptr [r14 + 4], r14

movzx r14w, r14b
movzx r14w, byte ptr [r14 + 4]

movzx r14d, r14b
movzx r14d, r14w
movzx r14d, byte ptr [r14 + 4]
movzx r14d, word ptr [r14 + 4]

movzx r14, r14b
movzx r14, r14w
movzx r14, byte ptr [r14 + 4]
movzx r14, word ptr [r14 + 4]


; Source: r14, target: r15
mov r15b, 1
mov r15w, 1
mov r15d, 1
mov r15, 1

mov r15b, r14b
mov r15w, r14w
mov r15d, r14d
mov r15, r14

mov r15b, byte ptr [r14 - 4]
mov r15b, byte ptr [r14 + 4]
mov r15w, word ptr [r14 + 4]
mov r15d, dword ptr [r14 + 4]
mov r15, qword ptr [r14 + 4]

mov byte ptr [r15 - 4], r14b
mov byte ptr [r15 + 4], r14b
mov word ptr [r15 + 4], r14w
mov dword ptr [r15 + 4], r14d
mov qword ptr [r15 + 4], r14

movzx r15w, r14b
movzx r15w, byte ptr [r14 + 4]

movzx r15d, r14b
movzx r15d, r14w
movzx r15d, byte ptr [r14 + 4]
movzx r15d, word ptr [r14 + 4]

movzx r15, r14b
movzx r15, r14w
movzx r15, byte ptr [r14 + 4]
movzx r15, word ptr [r14 + 4]


; Source: r15, target: rax
mov al, 1
mov ax, 1
mov eax, 1
mov rax, 1

mov al, r15b
mov ax, r15w
mov eax, r15d
mov rax, r15

mov al, byte ptr [r15 - 4]
mov al, byte ptr [r15 + 4]
mov ax, word ptr [r15 + 4]
mov eax, dword ptr [r15 + 4]
mov rax, qword ptr [r15 + 4]

mov byte ptr [rax - 4], r15b
mov byte ptr [rax + 4], r15b
mov word ptr [rax + 4], r15w
mov dword ptr [rax + 4], r15d
mov qword ptr [rax + 4], r15

movzx ax, r15b
movzx ax, byte ptr [r15 + 4]

movzx eax, r15b
movzx eax, r15w
movzx eax, byte ptr [r15 + 4]
movzx eax, word ptr [r15 + 4]

movzx rax, r15b
movzx rax, r15w
movzx rax, byte ptr [r15 + 4]
movzx rax, word ptr [r15 + 4]


; Source: r15, target: rcx
mov cl, 1
mov cx, 1
mov ecx, 1
mov rcx, 1

mov cl, r15b
mov cx, r15w
mov ecx, r15d
mov rcx, r15

mov cl, byte ptr [r15 - 4]
mov cl, byte ptr [r15 + 4]
mov cx, word ptr [r15 + 4]
mov ecx, dword ptr [r15 + 4]
mov rcx, qword ptr [r15 + 4]

mov byte ptr [rcx - 4], r15b
mov byte ptr [rcx + 4], r15b
mov word ptr [rcx + 4], r15w
mov dword ptr [rcx + 4], r15d
mov qword ptr [rcx + 4], r15

movzx cx, r15b
movzx cx, byte ptr [r15 + 4]

movzx ecx, r15b
movzx ecx, r15w
movzx ecx, byte ptr [r15 + 4]
movzx ecx, word ptr [r15 + 4]

movzx rcx, r15b
movzx rcx, r15w
movzx rcx, byte ptr [r15 + 4]
movzx rcx, word ptr [r15 + 4]


; Source: r15, target: rdx
mov dl, 1
mov dx, 1
mov edx, 1
mov rdx, 1

mov dl, r15b
mov dx, r15w
mov edx, r15d
mov rdx, r15

mov dl, byte ptr [r15 - 4]
mov dl, byte ptr [r15 + 4]
mov dx, word ptr [r15 + 4]
mov edx, dword ptr [r15 + 4]
mov rdx, qword ptr [r15 + 4]

mov byte ptr [rdx - 4], r15b
mov byte ptr [rdx + 4], r15b
mov word ptr [rdx + 4], r15w
mov dword ptr [rdx + 4], r15d
mov qword ptr [rdx + 4], r15

movzx dx, r15b
movzx dx, byte ptr [r15 + 4]

movzx edx, r15b
movzx edx, r15w
movzx edx, byte ptr [r15 + 4]
movzx edx, word ptr [r15 + 4]

movzx rdx, r15b
movzx rdx, r15w
movzx rdx, byte ptr [r15 + 4]
movzx rdx, word ptr [r15 + 4]


; Source: r15, target: rbx
mov bl, 1
mov bx, 1
mov ebx, 1
mov rbx, 1

mov bl, r15b
mov bx, r15w
mov ebx, r15d
mov rbx, r15

mov bl, byte ptr [r15 - 4]
mov bl, byte ptr [r15 + 4]
mov bx, word ptr [r15 + 4]
mov ebx, dword ptr [r15 + 4]
mov rbx, qword ptr [r15 + 4]

mov byte ptr [rbx - 4], r15b
mov byte ptr [rbx + 4], r15b
mov word ptr [rbx + 4], r15w
mov dword ptr [rbx + 4], r15d
mov qword ptr [rbx + 4], r15

movzx bx, r15b
movzx bx, byte ptr [r15 + 4]

movzx ebx, r15b
movzx ebx, r15w
movzx ebx, byte ptr [r15 + 4]
movzx ebx, word ptr [r15 + 4]

movzx rbx, r15b
movzx rbx, r15w
movzx rbx, byte ptr [r15 + 4]
movzx rbx, word ptr [r15 + 4]


; Source: r15, target: rsp
mov spl, 1
mov sp, 1
mov esp, 1
mov rsp, 1

mov spl, r15b
mov sp, r15w
mov esp, r15d
mov rsp, r15

mov spl, byte ptr [r15 - 4]
mov spl, byte ptr [r15 + 4]
mov sp, word ptr [r15 + 4]
mov esp, dword ptr [r15 + 4]
mov rsp, qword ptr [r15 + 4]

mov byte ptr [rsp - 4], r15b
mov byte ptr [rsp + 4], r15b
mov word ptr [rsp + 4], r15w
mov dword ptr [rsp + 4], r15d
mov qword ptr [rsp + 4], r15

movzx sp, r15b
movzx sp, byte ptr [r15 + 4]

movzx esp, r15b
movzx esp, r15w
movzx esp, byte ptr [r15 + 4]
movzx esp, word ptr [r15 + 4]

movzx rsp, r15b
movzx rsp, r15w
movzx rsp, byte ptr [r15 + 4]
movzx rsp, word ptr [r15 + 4]


; Source: r15, target: rbp
mov bpl, 1
mov bp, 1
mov ebp, 1
mov rbp, 1

mov bpl, r15b
mov bp, r15w
mov ebp, r15d
mov rbp, r15

mov bpl, byte ptr [r15 - 4]
mov bpl, byte ptr [r15 + 4]
mov bp, word ptr [r15 + 4]
mov ebp, dword ptr [r15 + 4]
mov rbp, qword ptr [r15 + 4]

mov byte ptr [rbp - 4], r15b
mov byte ptr [rbp + 4], r15b
mov word ptr [rbp + 4], r15w
mov dword ptr [rbp + 4], r15d
mov qword ptr [rbp + 4], r15

movzx bp, r15b
movzx bp, byte ptr [r15 + 4]

movzx ebp, r15b
movzx ebp, r15w
movzx ebp, byte ptr [r15 + 4]
movzx ebp, word ptr [r15 + 4]

movzx rbp, r15b
movzx rbp, r15w
movzx rbp, byte ptr [r15 + 4]
movzx rbp, word ptr [r15 + 4]


; Source: r15, target: rsi
mov dil, 1
mov si, 1
mov esi, 1
mov rsi, 1

mov dil, r15b
mov si, r15w
mov esi, r15d
mov rsi, r15

mov dil, byte ptr [r15 - 4]
mov dil, byte ptr [r15 + 4]
mov si, word ptr [r15 + 4]
mov esi, dword ptr [r15 + 4]
mov rsi, qword ptr [r15 + 4]

mov byte ptr [rsi - 4], r15b
mov byte ptr [rsi + 4], r15b
mov word ptr [rsi + 4], r15w
mov dword ptr [rsi + 4], r15d
mov qword ptr [rsi + 4], r15

movzx si, r15b
movzx si, byte ptr [r15 + 4]

movzx esi, r15b
movzx esi, r15w
movzx esi, byte ptr [r15 + 4]
movzx esi, word ptr [r15 + 4]

movzx rsi, r15b
movzx rsi, r15w
movzx rsi, byte ptr [r15 + 4]
movzx rsi, word ptr [r15 + 4]


; Source: r15, target: rdi
mov sil, 1
mov di, 1
mov edi, 1
mov rdi, 1

mov sil, r15b
mov di, r15w
mov edi, r15d
mov rdi, r15

mov sil, byte ptr [r15 - 4]
mov sil, byte ptr [r15 + 4]
mov di, word ptr [r15 + 4]
mov edi, dword ptr [r15 + 4]
mov rdi, qword ptr [r15 + 4]

mov byte ptr [rdi - 4], r15b
mov byte ptr [rdi + 4], r15b
mov word ptr [rdi + 4], r15w
mov dword ptr [rdi + 4], r15d
mov qword ptr [rdi + 4], r15

movzx di, r15b
movzx di, byte ptr [r15 + 4]

movzx edi, r15b
movzx edi, r15w
movzx edi, byte ptr [r15 + 4]
movzx edi, word ptr [r15 + 4]

movzx rdi, r15b
movzx rdi, r15w
movzx rdi, byte ptr [r15 + 4]
movzx rdi, word ptr [r15 + 4]


; Source: r15, target: r8
mov r8b, 1
mov r8w, 1
mov r8d, 1
mov r8, 1

mov r8b, r15b
mov r8w, r15w
mov r8d, r15d
mov r8, r15

mov r8b, byte ptr [r15 - 4]
mov r8b, byte ptr [r15 + 4]
mov r8w, word ptr [r15 + 4]
mov r8d, dword ptr [r15 + 4]
mov r8, qword ptr [r15 + 4]

mov byte ptr [r8 - 4], r15b
mov byte ptr [r8 + 4], r15b
mov word ptr [r8 + 4], r15w
mov dword ptr [r8 + 4], r15d
mov qword ptr [r8 + 4], r15

movzx r8w, r15b
movzx r8w, byte ptr [r15 + 4]

movzx r8d, r15b
movzx r8d, r15w
movzx r8d, byte ptr [r15 + 4]
movzx r8d, word ptr [r15 + 4]

movzx r8, r15b
movzx r8, r15w
movzx r8, byte ptr [r15 + 4]
movzx r8, word ptr [r15 + 4]


; Source: r15, target: r9
mov r9b, 1
mov r9w, 1
mov r9d, 1
mov r9, 1

mov r9b, r15b
mov r9w, r15w
mov r9d, r15d
mov r9, r15

mov r9b, byte ptr [r15 - 4]
mov r9b, byte ptr [r15 + 4]
mov r9w, word ptr [r15 + 4]
mov r9d, dword ptr [r15 + 4]
mov r9, qword ptr [r15 + 4]

mov byte ptr [r9 - 4], r15b
mov byte ptr [r9 + 4], r15b
mov word ptr [r9 + 4], r15w
mov dword ptr [r9 + 4], r15d
mov qword ptr [r9 + 4], r15

movzx r9w, r15b
movzx r9w, byte ptr [r15 + 4]

movzx r9d, r15b
movzx r9d, r15w
movzx r9d, byte ptr [r15 + 4]
movzx r9d, word ptr [r15 + 4]

movzx r9, r15b
movzx r9, r15w
movzx r9, byte ptr [r15 + 4]
movzx r9, word ptr [r15 + 4]


; Source: r15, target: r10
mov r10b, 1
mov r10w, 1
mov r10d, 1
mov r10, 1

mov r10b, r15b
mov r10w, r15w
mov r10d, r15d
mov r10, r15

mov r10b, byte ptr [r15 - 4]
mov r10b, byte ptr [r15 + 4]
mov r10w, word ptr [r15 + 4]
mov r10d, dword ptr [r15 + 4]
mov r10, qword ptr [r15 + 4]

mov byte ptr [r10 - 4], r15b
mov byte ptr [r10 + 4], r15b
mov word ptr [r10 + 4], r15w
mov dword ptr [r10 + 4], r15d
mov qword ptr [r10 + 4], r15

movzx r10w, r15b
movzx r10w, byte ptr [r15 + 4]

movzx r10d, r15b
movzx r10d, r15w
movzx r10d, byte ptr [r15 + 4]
movzx r10d, word ptr [r15 + 4]

movzx r10, r15b
movzx r10, r15w
movzx r10, byte ptr [r15 + 4]
movzx r10, word ptr [r15 + 4]


; Source: r15, target: r11
mov r11b, 1
mov r11w, 1
mov r11d, 1
mov r11, 1

mov r11b, r15b
mov r11w, r15w
mov r11d, r15d
mov r11, r15

mov r11b, byte ptr [r15 - 4]
mov r11b, byte ptr [r15 + 4]
mov r11w, word ptr [r15 + 4]
mov r11d, dword ptr [r15 + 4]
mov r11, qword ptr [r15 + 4]

mov byte ptr [r11 - 4], r15b
mov byte ptr [r11 + 4], r15b
mov word ptr [r11 + 4], r15w
mov dword ptr [r11 + 4], r15d
mov qword ptr [r11 + 4], r15

movzx r11w, r15b
movzx r11w, byte ptr [r15 + 4]

movzx r11d, r15b
movzx r11d, r15w
movzx r11d, byte ptr [r15 + 4]
movzx r11d, word ptr [r15 + 4]

movzx r11, r15b
movzx r11, r15w
movzx r11, byte ptr [r15 + 4]
movzx r11, word ptr [r15 + 4]


; Source: r15, target: r12
mov r12b, 1
mov r12w, 1
mov r12d, 1
mov r12, 1

mov r12b, r15b
mov r12w, r15w
mov r12d, r15d
mov r12, r15

mov r12b, byte ptr [r15 - 4]
mov r12b, byte ptr [r15 + 4]
mov r12w, word ptr [r15 + 4]
mov r12d, dword ptr [r15 + 4]
mov r12, qword ptr [r15 + 4]

mov byte ptr [r12 - 4], r15b
mov byte ptr [r12 + 4], r15b
mov word ptr [r12 + 4], r15w
mov dword ptr [r12 + 4], r15d
mov qword ptr [r12 + 4], r15

movzx r12w, r15b
movzx r12w, byte ptr [r15 + 4]

movzx r12d, r15b
movzx r12d, r15w
movzx r12d, byte ptr [r15 + 4]
movzx r12d, word ptr [r15 + 4]

movzx r12, r15b
movzx r12, r15w
movzx r12, byte ptr [r15 + 4]
movzx r12, word ptr [r15 + 4]


; Source: r15, target: r13
mov r13b, 1
mov r13w, 1
mov r13d, 1
mov r13, 1

mov r13b, r15b
mov r13w, r15w
mov r13d, r15d
mov r13, r15

mov r13b, byte ptr [r15 - 4]
mov r13b, byte ptr [r15 + 4]
mov r13w, word ptr [r15 + 4]
mov r13d, dword ptr [r15 + 4]
mov r13, qword ptr [r15 + 4]

mov byte ptr [r13 - 4], r15b
mov byte ptr [r13 + 4], r15b
mov word ptr [r13 + 4], r15w
mov dword ptr [r13 + 4], r15d
mov qword ptr [r13 + 4], r15

movzx r13w, r15b
movzx r13w, byte ptr [r15 + 4]

movzx r13d, r15b
movzx r13d, r15w
movzx r13d, byte ptr [r15 + 4]
movzx r13d, word ptr [r15 + 4]

movzx r13, r15b
movzx r13, r15w
movzx r13, byte ptr [r15 + 4]
movzx r13, word ptr [r15 + 4]


; Source: r15, target: r14
mov r14b, 1
mov r14w, 1
mov r14d, 1
mov r14, 1

mov r14b, r15b
mov r14w, r15w
mov r14d, r15d
mov r14, r15

mov r14b, byte ptr [r15 - 4]
mov r14b, byte ptr [r15 + 4]
mov r14w, word ptr [r15 + 4]
mov r14d, dword ptr [r15 + 4]
mov r14, qword ptr [r15 + 4]

mov byte ptr [r14 - 4], r15b
mov byte ptr [r14 + 4], r15b
mov word ptr [r14 + 4], r15w
mov dword ptr [r14 + 4], r15d
mov qword ptr [r14 + 4], r15

movzx r14w, r15b
movzx r14w, byte ptr [r15 + 4]

movzx r14d, r15b
movzx r14d, r15w
movzx r14d, byte ptr [r15 + 4]
movzx r14d, word ptr [r15 + 4]

movzx r14, r15b
movzx r14, r15w
movzx r14, byte ptr [r15 + 4]
movzx r14, word ptr [r15 + 4]


; Source: r15, target: r15
mov r15b, 1
mov r15w, 1
mov r15d, 1
mov r15, 1

mov r15b, r15b
mov r15w, r15w
mov r15d, r15d
mov r15, r15

mov r15b, byte ptr [r15 - 4]
mov r15b, byte ptr [r15 + 4]
mov r15w, word ptr [r15 + 4]
mov r15d, dword ptr [r15 + 4]
mov r15, qword ptr [r15 + 4]

mov byte ptr [r15 - 4], r15b
mov byte ptr [r15 + 4], r15b
mov word ptr [r15 + 4], r15w
mov dword ptr [r15 + 4], r15d
mov qword ptr [r15 + 4], r15

movzx r15w, r15b
movzx r15w, byte ptr [r15 + 4]

movzx r15d, r15b
movzx r15d, r15w
movzx r15d, byte ptr [r15 + 4]
movzx r15d, word ptr [r15 + 4]

movzx r15, r15b
movzx r15, r15w
movzx r15, byte ptr [r15 + 4]
movzx r15, word ptr [r15 + 4]


; Float <-> float

; Source: xmm0, target: xmm0
movss xmm0, xmm0
movsd xmm0, xmm0

cvtss2sd xmm0, xmm0
cvtsd2ss xmm0, xmm0


; Source: xmm0, target: xmm1
movss xmm1, xmm0
movsd xmm1, xmm0

cvtss2sd xmm1, xmm0
cvtsd2ss xmm1, xmm0


; Source: xmm0, target: xmm2
movss xmm2, xmm0
movsd xmm2, xmm0

cvtss2sd xmm2, xmm0
cvtsd2ss xmm2, xmm0


; Source: xmm0, target: xmm3
movss xmm3, xmm0
movsd xmm3, xmm0

cvtss2sd xmm3, xmm0
cvtsd2ss xmm3, xmm0


; Source: xmm0, target: xmm4
movss xmm4, xmm0
movsd xmm4, xmm0

cvtss2sd xmm4, xmm0
cvtsd2ss xmm4, xmm0


; Source: xmm0, target: xmm5
movss xmm5, xmm0
movsd xmm5, xmm0

cvtss2sd xmm5, xmm0
cvtsd2ss xmm5, xmm0


; Source: xmm0, target: xmm6
movss xmm6, xmm0
movsd xmm6, xmm0

cvtss2sd xmm6, xmm0
cvtsd2ss xmm6, xmm0


; Source: xmm0, target: xmm7
movss xmm7, xmm0
movsd xmm7, xmm0

cvtss2sd xmm7, xmm0
cvtsd2ss xmm7, xmm0


; Source: xmm0, target: xmm8
movss xmm8, xmm0
movsd xmm8, xmm0

cvtss2sd xmm8, xmm0
cvtsd2ss xmm8, xmm0


; Source: xmm0, target: xmm9
movss xmm9, xmm0
movsd xmm9, xmm0

cvtss2sd xmm9, xmm0
cvtsd2ss xmm9, xmm0


; Source: xmm0, target: xmm10
movss xmm10, xmm0
movsd xmm10, xmm0

cvtss2sd xmm10, xmm0
cvtsd2ss xmm10, xmm0


; Source: xmm0, target: xmm11
movss xmm11, xmm0
movsd xmm11, xmm0

cvtss2sd xmm11, xmm0
cvtsd2ss xmm11, xmm0


; Source: xmm0, target: xmm12
movss xmm12, xmm0
movsd xmm12, xmm0

cvtss2sd xmm12, xmm0
cvtsd2ss xmm12, xmm0


; Source: xmm0, target: xmm13
movss xmm13, xmm0
movsd xmm13, xmm0

cvtss2sd xmm13, xmm0
cvtsd2ss xmm13, xmm0


; Source: xmm0, target: xmm14
movss xmm14, xmm0
movsd xmm14, xmm0

cvtss2sd xmm14, xmm0
cvtsd2ss xmm14, xmm0


; Source: xmm0, target: xmm15
movss xmm15, xmm0
movsd xmm15, xmm0

cvtss2sd xmm15, xmm0
cvtsd2ss xmm15, xmm0


; Source: xmm1, target: xmm0
movss xmm0, xmm1
movsd xmm0, xmm1

cvtss2sd xmm0, xmm1
cvtsd2ss xmm0, xmm1


; Source: xmm1, target: xmm1
movss xmm1, xmm1
movsd xmm1, xmm1

cvtss2sd xmm1, xmm1
cvtsd2ss xmm1, xmm1


; Source: xmm1, target: xmm2
movss xmm2, xmm1
movsd xmm2, xmm1

cvtss2sd xmm2, xmm1
cvtsd2ss xmm2, xmm1


; Source: xmm1, target: xmm3
movss xmm3, xmm1
movsd xmm3, xmm1

cvtss2sd xmm3, xmm1
cvtsd2ss xmm3, xmm1


; Source: xmm1, target: xmm4
movss xmm4, xmm1
movsd xmm4, xmm1

cvtss2sd xmm4, xmm1
cvtsd2ss xmm4, xmm1


; Source: xmm1, target: xmm5
movss xmm5, xmm1
movsd xmm5, xmm1

cvtss2sd xmm5, xmm1
cvtsd2ss xmm5, xmm1


; Source: xmm1, target: xmm6
movss xmm6, xmm1
movsd xmm6, xmm1

cvtss2sd xmm6, xmm1
cvtsd2ss xmm6, xmm1


; Source: xmm1, target: xmm7
movss xmm7, xmm1
movsd xmm7, xmm1

cvtss2sd xmm7, xmm1
cvtsd2ss xmm7, xmm1


; Source: xmm1, target: xmm8
movss xmm8, xmm1
movsd xmm8, xmm1

cvtss2sd xmm8, xmm1
cvtsd2ss xmm8, xmm1


; Source: xmm1, target: xmm9
movss xmm9, xmm1
movsd xmm9, xmm1

cvtss2sd xmm9, xmm1
cvtsd2ss xmm9, xmm1


; Source: xmm1, target: xmm10
movss xmm10, xmm1
movsd xmm10, xmm1

cvtss2sd xmm10, xmm1
cvtsd2ss xmm10, xmm1


; Source: xmm1, target: xmm11
movss xmm11, xmm1
movsd xmm11, xmm1

cvtss2sd xmm11, xmm1
cvtsd2ss xmm11, xmm1


; Source: xmm1, target: xmm12
movss xmm12, xmm1
movsd xmm12, xmm1

cvtss2sd xmm12, xmm1
cvtsd2ss xmm12, xmm1


; Source: xmm1, target: xmm13
movss xmm13, xmm1
movsd xmm13, xmm1

cvtss2sd xmm13, xmm1
cvtsd2ss xmm13, xmm1


; Source: xmm1, target: xmm14
movss xmm14, xmm1
movsd xmm14, xmm1

cvtss2sd xmm14, xmm1
cvtsd2ss xmm14, xmm1


; Source: xmm1, target: xmm15
movss xmm15, xmm1
movsd xmm15, xmm1

cvtss2sd xmm15, xmm1
cvtsd2ss xmm15, xmm1


; Source: xmm2, target: xmm0
movss xmm0, xmm2
movsd xmm0, xmm2

cvtss2sd xmm0, xmm2
cvtsd2ss xmm0, xmm2


; Source: xmm2, target: xmm1
movss xmm1, xmm2
movsd xmm1, xmm2

cvtss2sd xmm1, xmm2
cvtsd2ss xmm1, xmm2


; Source: xmm2, target: xmm2
movss xmm2, xmm2
movsd xmm2, xmm2

cvtss2sd xmm2, xmm2
cvtsd2ss xmm2, xmm2


; Source: xmm2, target: xmm3
movss xmm3, xmm2
movsd xmm3, xmm2

cvtss2sd xmm3, xmm2
cvtsd2ss xmm3, xmm2


; Source: xmm2, target: xmm4
movss xmm4, xmm2
movsd xmm4, xmm2

cvtss2sd xmm4, xmm2
cvtsd2ss xmm4, xmm2


; Source: xmm2, target: xmm5
movss xmm5, xmm2
movsd xmm5, xmm2

cvtss2sd xmm5, xmm2
cvtsd2ss xmm5, xmm2


; Source: xmm2, target: xmm6
movss xmm6, xmm2
movsd xmm6, xmm2

cvtss2sd xmm6, xmm2
cvtsd2ss xmm6, xmm2


; Source: xmm2, target: xmm7
movss xmm7, xmm2
movsd xmm7, xmm2

cvtss2sd xmm7, xmm2
cvtsd2ss xmm7, xmm2


; Source: xmm2, target: xmm8
movss xmm8, xmm2
movsd xmm8, xmm2

cvtss2sd xmm8, xmm2
cvtsd2ss xmm8, xmm2


; Source: xmm2, target: xmm9
movss xmm9, xmm2
movsd xmm9, xmm2

cvtss2sd xmm9, xmm2
cvtsd2ss xmm9, xmm2


; Source: xmm2, target: xmm10
movss xmm10, xmm2
movsd xmm10, xmm2

cvtss2sd xmm10, xmm2
cvtsd2ss xmm10, xmm2


; Source: xmm2, target: xmm11
movss xmm11, xmm2
movsd xmm11, xmm2

cvtss2sd xmm11, xmm2
cvtsd2ss xmm11, xmm2


; Source: xmm2, target: xmm12
movss xmm12, xmm2
movsd xmm12, xmm2

cvtss2sd xmm12, xmm2
cvtsd2ss xmm12, xmm2


; Source: xmm2, target: xmm13
movss xmm13, xmm2
movsd xmm13, xmm2

cvtss2sd xmm13, xmm2
cvtsd2ss xmm13, xmm2


; Source: xmm2, target: xmm14
movss xmm14, xmm2
movsd xmm14, xmm2

cvtss2sd xmm14, xmm2
cvtsd2ss xmm14, xmm2


; Source: xmm2, target: xmm15
movss xmm15, xmm2
movsd xmm15, xmm2

cvtss2sd xmm15, xmm2
cvtsd2ss xmm15, xmm2


; Source: xmm3, target: xmm0
movss xmm0, xmm3
movsd xmm0, xmm3

cvtss2sd xmm0, xmm3
cvtsd2ss xmm0, xmm3


; Source: xmm3, target: xmm1
movss xmm1, xmm3
movsd xmm1, xmm3

cvtss2sd xmm1, xmm3
cvtsd2ss xmm1, xmm3


; Source: xmm3, target: xmm2
movss xmm2, xmm3
movsd xmm2, xmm3

cvtss2sd xmm2, xmm3
cvtsd2ss xmm2, xmm3


; Source: xmm3, target: xmm3
movss xmm3, xmm3
movsd xmm3, xmm3

cvtss2sd xmm3, xmm3
cvtsd2ss xmm3, xmm3


; Source: xmm3, target: xmm4
movss xmm4, xmm3
movsd xmm4, xmm3

cvtss2sd xmm4, xmm3
cvtsd2ss xmm4, xmm3


; Source: xmm3, target: xmm5
movss xmm5, xmm3
movsd xmm5, xmm3

cvtss2sd xmm5, xmm3
cvtsd2ss xmm5, xmm3


; Source: xmm3, target: xmm6
movss xmm6, xmm3
movsd xmm6, xmm3

cvtss2sd xmm6, xmm3
cvtsd2ss xmm6, xmm3


; Source: xmm3, target: xmm7
movss xmm7, xmm3
movsd xmm7, xmm3

cvtss2sd xmm7, xmm3
cvtsd2ss xmm7, xmm3


; Source: xmm3, target: xmm8
movss xmm8, xmm3
movsd xmm8, xmm3

cvtss2sd xmm8, xmm3
cvtsd2ss xmm8, xmm3


; Source: xmm3, target: xmm9
movss xmm9, xmm3
movsd xmm9, xmm3

cvtss2sd xmm9, xmm3
cvtsd2ss xmm9, xmm3


; Source: xmm3, target: xmm10
movss xmm10, xmm3
movsd xmm10, xmm3

cvtss2sd xmm10, xmm3
cvtsd2ss xmm10, xmm3


; Source: xmm3, target: xmm11
movss xmm11, xmm3
movsd xmm11, xmm3

cvtss2sd xmm11, xmm3
cvtsd2ss xmm11, xmm3


; Source: xmm3, target: xmm12
movss xmm12, xmm3
movsd xmm12, xmm3

cvtss2sd xmm12, xmm3
cvtsd2ss xmm12, xmm3


; Source: xmm3, target: xmm13
movss xmm13, xmm3
movsd xmm13, xmm3

cvtss2sd xmm13, xmm3
cvtsd2ss xmm13, xmm3


; Source: xmm3, target: xmm14
movss xmm14, xmm3
movsd xmm14, xmm3

cvtss2sd xmm14, xmm3
cvtsd2ss xmm14, xmm3


; Source: xmm3, target: xmm15
movss xmm15, xmm3
movsd xmm15, xmm3

cvtss2sd xmm15, xmm3
cvtsd2ss xmm15, xmm3


; Source: xmm4, target: xmm0
movss xmm0, xmm4
movsd xmm0, xmm4

cvtss2sd xmm0, xmm4
cvtsd2ss xmm0, xmm4


; Source: xmm4, target: xmm1
movss xmm1, xmm4
movsd xmm1, xmm4

cvtss2sd xmm1, xmm4
cvtsd2ss xmm1, xmm4


; Source: xmm4, target: xmm2
movss xmm2, xmm4
movsd xmm2, xmm4

cvtss2sd xmm2, xmm4
cvtsd2ss xmm2, xmm4


; Source: xmm4, target: xmm3
movss xmm3, xmm4
movsd xmm3, xmm4

cvtss2sd xmm3, xmm4
cvtsd2ss xmm3, xmm4


; Source: xmm4, target: xmm4
movss xmm4, xmm4
movsd xmm4, xmm4

cvtss2sd xmm4, xmm4
cvtsd2ss xmm4, xmm4


; Source: xmm4, target: xmm5
movss xmm5, xmm4
movsd xmm5, xmm4

cvtss2sd xmm5, xmm4
cvtsd2ss xmm5, xmm4


; Source: xmm4, target: xmm6
movss xmm6, xmm4
movsd xmm6, xmm4

cvtss2sd xmm6, xmm4
cvtsd2ss xmm6, xmm4


; Source: xmm4, target: xmm7
movss xmm7, xmm4
movsd xmm7, xmm4

cvtss2sd xmm7, xmm4
cvtsd2ss xmm7, xmm4


; Source: xmm4, target: xmm8
movss xmm8, xmm4
movsd xmm8, xmm4

cvtss2sd xmm8, xmm4
cvtsd2ss xmm8, xmm4


; Source: xmm4, target: xmm9
movss xmm9, xmm4
movsd xmm9, xmm4

cvtss2sd xmm9, xmm4
cvtsd2ss xmm9, xmm4


; Source: xmm4, target: xmm10
movss xmm10, xmm4
movsd xmm10, xmm4

cvtss2sd xmm10, xmm4
cvtsd2ss xmm10, xmm4


; Source: xmm4, target: xmm11
movss xmm11, xmm4
movsd xmm11, xmm4

cvtss2sd xmm11, xmm4
cvtsd2ss xmm11, xmm4


; Source: xmm4, target: xmm12
movss xmm12, xmm4
movsd xmm12, xmm4

cvtss2sd xmm12, xmm4
cvtsd2ss xmm12, xmm4


; Source: xmm4, target: xmm13
movss xmm13, xmm4
movsd xmm13, xmm4

cvtss2sd xmm13, xmm4
cvtsd2ss xmm13, xmm4


; Source: xmm4, target: xmm14
movss xmm14, xmm4
movsd xmm14, xmm4

cvtss2sd xmm14, xmm4
cvtsd2ss xmm14, xmm4


; Source: xmm4, target: xmm15
movss xmm15, xmm4
movsd xmm15, xmm4

cvtss2sd xmm15, xmm4
cvtsd2ss xmm15, xmm4


; Source: xmm5, target: xmm0
movss xmm0, xmm5
movsd xmm0, xmm5

cvtss2sd xmm0, xmm5
cvtsd2ss xmm0, xmm5


; Source: xmm5, target: xmm1
movss xmm1, xmm5
movsd xmm1, xmm5

cvtss2sd xmm1, xmm5
cvtsd2ss xmm1, xmm5


; Source: xmm5, target: xmm2
movss xmm2, xmm5
movsd xmm2, xmm5

cvtss2sd xmm2, xmm5
cvtsd2ss xmm2, xmm5


; Source: xmm5, target: xmm3
movss xmm3, xmm5
movsd xmm3, xmm5

cvtss2sd xmm3, xmm5
cvtsd2ss xmm3, xmm5


; Source: xmm5, target: xmm4
movss xmm4, xmm5
movsd xmm4, xmm5

cvtss2sd xmm4, xmm5
cvtsd2ss xmm4, xmm5


; Source: xmm5, target: xmm5
movss xmm5, xmm5
movsd xmm5, xmm5

cvtss2sd xmm5, xmm5
cvtsd2ss xmm5, xmm5


; Source: xmm5, target: xmm6
movss xmm6, xmm5
movsd xmm6, xmm5

cvtss2sd xmm6, xmm5
cvtsd2ss xmm6, xmm5


; Source: xmm5, target: xmm7
movss xmm7, xmm5
movsd xmm7, xmm5

cvtss2sd xmm7, xmm5
cvtsd2ss xmm7, xmm5


; Source: xmm5, target: xmm8
movss xmm8, xmm5
movsd xmm8, xmm5

cvtss2sd xmm8, xmm5
cvtsd2ss xmm8, xmm5


; Source: xmm5, target: xmm9
movss xmm9, xmm5
movsd xmm9, xmm5

cvtss2sd xmm9, xmm5
cvtsd2ss xmm9, xmm5


; Source: xmm5, target: xmm10
movss xmm10, xmm5
movsd xmm10, xmm5

cvtss2sd xmm10, xmm5
cvtsd2ss xmm10, xmm5


; Source: xmm5, target: xmm11
movss xmm11, xmm5
movsd xmm11, xmm5

cvtss2sd xmm11, xmm5
cvtsd2ss xmm11, xmm5


; Source: xmm5, target: xmm12
movss xmm12, xmm5
movsd xmm12, xmm5

cvtss2sd xmm12, xmm5
cvtsd2ss xmm12, xmm5


; Source: xmm5, target: xmm13
movss xmm13, xmm5
movsd xmm13, xmm5

cvtss2sd xmm13, xmm5
cvtsd2ss xmm13, xmm5


; Source: xmm5, target: xmm14
movss xmm14, xmm5
movsd xmm14, xmm5

cvtss2sd xmm14, xmm5
cvtsd2ss xmm14, xmm5


; Source: xmm5, target: xmm15
movss xmm15, xmm5
movsd xmm15, xmm5

cvtss2sd xmm15, xmm5
cvtsd2ss xmm15, xmm5


; Source: xmm6, target: xmm0
movss xmm0, xmm6
movsd xmm0, xmm6

cvtss2sd xmm0, xmm6
cvtsd2ss xmm0, xmm6


; Source: xmm6, target: xmm1
movss xmm1, xmm6
movsd xmm1, xmm6

cvtss2sd xmm1, xmm6
cvtsd2ss xmm1, xmm6


; Source: xmm6, target: xmm2
movss xmm2, xmm6
movsd xmm2, xmm6

cvtss2sd xmm2, xmm6
cvtsd2ss xmm2, xmm6


; Source: xmm6, target: xmm3
movss xmm3, xmm6
movsd xmm3, xmm6

cvtss2sd xmm3, xmm6
cvtsd2ss xmm3, xmm6


; Source: xmm6, target: xmm4
movss xmm4, xmm6
movsd xmm4, xmm6

cvtss2sd xmm4, xmm6
cvtsd2ss xmm4, xmm6


; Source: xmm6, target: xmm5
movss xmm5, xmm6
movsd xmm5, xmm6

cvtss2sd xmm5, xmm6
cvtsd2ss xmm5, xmm6


; Source: xmm6, target: xmm6
movss xmm6, xmm6
movsd xmm6, xmm6

cvtss2sd xmm6, xmm6
cvtsd2ss xmm6, xmm6


; Source: xmm6, target: xmm7
movss xmm7, xmm6
movsd xmm7, xmm6

cvtss2sd xmm7, xmm6
cvtsd2ss xmm7, xmm6


; Source: xmm6, target: xmm8
movss xmm8, xmm6
movsd xmm8, xmm6

cvtss2sd xmm8, xmm6
cvtsd2ss xmm8, xmm6


; Source: xmm6, target: xmm9
movss xmm9, xmm6
movsd xmm9, xmm6

cvtss2sd xmm9, xmm6
cvtsd2ss xmm9, xmm6


; Source: xmm6, target: xmm10
movss xmm10, xmm6
movsd xmm10, xmm6

cvtss2sd xmm10, xmm6
cvtsd2ss xmm10, xmm6


; Source: xmm6, target: xmm11
movss xmm11, xmm6
movsd xmm11, xmm6

cvtss2sd xmm11, xmm6
cvtsd2ss xmm11, xmm6


; Source: xmm6, target: xmm12
movss xmm12, xmm6
movsd xmm12, xmm6

cvtss2sd xmm12, xmm6
cvtsd2ss xmm12, xmm6


; Source: xmm6, target: xmm13
movss xmm13, xmm6
movsd xmm13, xmm6

cvtss2sd xmm13, xmm6
cvtsd2ss xmm13, xmm6


; Source: xmm6, target: xmm14
movss xmm14, xmm6
movsd xmm14, xmm6

cvtss2sd xmm14, xmm6
cvtsd2ss xmm14, xmm6


; Source: xmm6, target: xmm15
movss xmm15, xmm6
movsd xmm15, xmm6

cvtss2sd xmm15, xmm6
cvtsd2ss xmm15, xmm6


; Source: xmm7, target: xmm0
movss xmm0, xmm7
movsd xmm0, xmm7

cvtss2sd xmm0, xmm7
cvtsd2ss xmm0, xmm7


; Source: xmm7, target: xmm1
movss xmm1, xmm7
movsd xmm1, xmm7

cvtss2sd xmm1, xmm7
cvtsd2ss xmm1, xmm7


; Source: xmm7, target: xmm2
movss xmm2, xmm7
movsd xmm2, xmm7

cvtss2sd xmm2, xmm7
cvtsd2ss xmm2, xmm7


; Source: xmm7, target: xmm3
movss xmm3, xmm7
movsd xmm3, xmm7

cvtss2sd xmm3, xmm7
cvtsd2ss xmm3, xmm7


; Source: xmm7, target: xmm4
movss xmm4, xmm7
movsd xmm4, xmm7

cvtss2sd xmm4, xmm7
cvtsd2ss xmm4, xmm7


; Source: xmm7, target: xmm5
movss xmm5, xmm7
movsd xmm5, xmm7

cvtss2sd xmm5, xmm7
cvtsd2ss xmm5, xmm7


; Source: xmm7, target: xmm6
movss xmm6, xmm7
movsd xmm6, xmm7

cvtss2sd xmm6, xmm7
cvtsd2ss xmm6, xmm7


; Source: xmm7, target: xmm7
movss xmm7, xmm7
movsd xmm7, xmm7

cvtss2sd xmm7, xmm7
cvtsd2ss xmm7, xmm7


; Source: xmm7, target: xmm8
movss xmm8, xmm7
movsd xmm8, xmm7

cvtss2sd xmm8, xmm7
cvtsd2ss xmm8, xmm7


; Source: xmm7, target: xmm9
movss xmm9, xmm7
movsd xmm9, xmm7

cvtss2sd xmm9, xmm7
cvtsd2ss xmm9, xmm7


; Source: xmm7, target: xmm10
movss xmm10, xmm7
movsd xmm10, xmm7

cvtss2sd xmm10, xmm7
cvtsd2ss xmm10, xmm7


; Source: xmm7, target: xmm11
movss xmm11, xmm7
movsd xmm11, xmm7

cvtss2sd xmm11, xmm7
cvtsd2ss xmm11, xmm7


; Source: xmm7, target: xmm12
movss xmm12, xmm7
movsd xmm12, xmm7

cvtss2sd xmm12, xmm7
cvtsd2ss xmm12, xmm7


; Source: xmm7, target: xmm13
movss xmm13, xmm7
movsd xmm13, xmm7

cvtss2sd xmm13, xmm7
cvtsd2ss xmm13, xmm7


; Source: xmm7, target: xmm14
movss xmm14, xmm7
movsd xmm14, xmm7

cvtss2sd xmm14, xmm7
cvtsd2ss xmm14, xmm7


; Source: xmm7, target: xmm15
movss xmm15, xmm7
movsd xmm15, xmm7

cvtss2sd xmm15, xmm7
cvtsd2ss xmm15, xmm7


; Source: xmm8, target: xmm0
movss xmm0, xmm8
movsd xmm0, xmm8

cvtss2sd xmm0, xmm8
cvtsd2ss xmm0, xmm8


; Source: xmm8, target: xmm1
movss xmm1, xmm8
movsd xmm1, xmm8

cvtss2sd xmm1, xmm8
cvtsd2ss xmm1, xmm8


; Source: xmm8, target: xmm2
movss xmm2, xmm8
movsd xmm2, xmm8

cvtss2sd xmm2, xmm8
cvtsd2ss xmm2, xmm8


; Source: xmm8, target: xmm3
movss xmm3, xmm8
movsd xmm3, xmm8

cvtss2sd xmm3, xmm8
cvtsd2ss xmm3, xmm8


; Source: xmm8, target: xmm4
movss xmm4, xmm8
movsd xmm4, xmm8

cvtss2sd xmm4, xmm8
cvtsd2ss xmm4, xmm8


; Source: xmm8, target: xmm5
movss xmm5, xmm8
movsd xmm5, xmm8

cvtss2sd xmm5, xmm8
cvtsd2ss xmm5, xmm8


; Source: xmm8, target: xmm6
movss xmm6, xmm8
movsd xmm6, xmm8

cvtss2sd xmm6, xmm8
cvtsd2ss xmm6, xmm8


; Source: xmm8, target: xmm7
movss xmm7, xmm8
movsd xmm7, xmm8

cvtss2sd xmm7, xmm8
cvtsd2ss xmm7, xmm8


; Source: xmm8, target: xmm8
movss xmm8, xmm8
movsd xmm8, xmm8

cvtss2sd xmm8, xmm8
cvtsd2ss xmm8, xmm8


; Source: xmm8, target: xmm9
movss xmm9, xmm8
movsd xmm9, xmm8

cvtss2sd xmm9, xmm8
cvtsd2ss xmm9, xmm8


; Source: xmm8, target: xmm10
movss xmm10, xmm8
movsd xmm10, xmm8

cvtss2sd xmm10, xmm8
cvtsd2ss xmm10, xmm8


; Source: xmm8, target: xmm11
movss xmm11, xmm8
movsd xmm11, xmm8

cvtss2sd xmm11, xmm8
cvtsd2ss xmm11, xmm8


; Source: xmm8, target: xmm12
movss xmm12, xmm8
movsd xmm12, xmm8

cvtss2sd xmm12, xmm8
cvtsd2ss xmm12, xmm8


; Source: xmm8, target: xmm13
movss xmm13, xmm8
movsd xmm13, xmm8

cvtss2sd xmm13, xmm8
cvtsd2ss xmm13, xmm8


; Source: xmm8, target: xmm14
movss xmm14, xmm8
movsd xmm14, xmm8

cvtss2sd xmm14, xmm8
cvtsd2ss xmm14, xmm8


; Source: xmm8, target: xmm15
movss xmm15, xmm8
movsd xmm15, xmm8

cvtss2sd xmm15, xmm8
cvtsd2ss xmm15, xmm8


; Source: xmm9, target: xmm0
movss xmm0, xmm9
movsd xmm0, xmm9

cvtss2sd xmm0, xmm9
cvtsd2ss xmm0, xmm9


; Source: xmm9, target: xmm1
movss xmm1, xmm9
movsd xmm1, xmm9

cvtss2sd xmm1, xmm9
cvtsd2ss xmm1, xmm9


; Source: xmm9, target: xmm2
movss xmm2, xmm9
movsd xmm2, xmm9

cvtss2sd xmm2, xmm9
cvtsd2ss xmm2, xmm9


; Source: xmm9, target: xmm3
movss xmm3, xmm9
movsd xmm3, xmm9

cvtss2sd xmm3, xmm9
cvtsd2ss xmm3, xmm9


; Source: xmm9, target: xmm4
movss xmm4, xmm9
movsd xmm4, xmm9

cvtss2sd xmm4, xmm9
cvtsd2ss xmm4, xmm9


; Source: xmm9, target: xmm5
movss xmm5, xmm9
movsd xmm5, xmm9

cvtss2sd xmm5, xmm9
cvtsd2ss xmm5, xmm9


; Source: xmm9, target: xmm6
movss xmm6, xmm9
movsd xmm6, xmm9

cvtss2sd xmm6, xmm9
cvtsd2ss xmm6, xmm9


; Source: xmm9, target: xmm7
movss xmm7, xmm9
movsd xmm7, xmm9

cvtss2sd xmm7, xmm9
cvtsd2ss xmm7, xmm9


; Source: xmm9, target: xmm8
movss xmm8, xmm9
movsd xmm8, xmm9

cvtss2sd xmm8, xmm9
cvtsd2ss xmm8, xmm9


; Source: xmm9, target: xmm9
movss xmm9, xmm9
movsd xmm9, xmm9

cvtss2sd xmm9, xmm9
cvtsd2ss xmm9, xmm9


; Source: xmm9, target: xmm10
movss xmm10, xmm9
movsd xmm10, xmm9

cvtss2sd xmm10, xmm9
cvtsd2ss xmm10, xmm9


; Source: xmm9, target: xmm11
movss xmm11, xmm9
movsd xmm11, xmm9

cvtss2sd xmm11, xmm9
cvtsd2ss xmm11, xmm9


; Source: xmm9, target: xmm12
movss xmm12, xmm9
movsd xmm12, xmm9

cvtss2sd xmm12, xmm9
cvtsd2ss xmm12, xmm9


; Source: xmm9, target: xmm13
movss xmm13, xmm9
movsd xmm13, xmm9

cvtss2sd xmm13, xmm9
cvtsd2ss xmm13, xmm9


; Source: xmm9, target: xmm14
movss xmm14, xmm9
movsd xmm14, xmm9

cvtss2sd xmm14, xmm9
cvtsd2ss xmm14, xmm9


; Source: xmm9, target: xmm15
movss xmm15, xmm9
movsd xmm15, xmm9

cvtss2sd xmm15, xmm9
cvtsd2ss xmm15, xmm9


; Source: xmm10, target: xmm0
movss xmm0, xmm10
movsd xmm0, xmm10

cvtss2sd xmm0, xmm10
cvtsd2ss xmm0, xmm10


; Source: xmm10, target: xmm1
movss xmm1, xmm10
movsd xmm1, xmm10

cvtss2sd xmm1, xmm10
cvtsd2ss xmm1, xmm10


; Source: xmm10, target: xmm2
movss xmm2, xmm10
movsd xmm2, xmm10

cvtss2sd xmm2, xmm10
cvtsd2ss xmm2, xmm10


; Source: xmm10, target: xmm3
movss xmm3, xmm10
movsd xmm3, xmm10

cvtss2sd xmm3, xmm10
cvtsd2ss xmm3, xmm10


; Source: xmm10, target: xmm4
movss xmm4, xmm10
movsd xmm4, xmm10

cvtss2sd xmm4, xmm10
cvtsd2ss xmm4, xmm10


; Source: xmm10, target: xmm5
movss xmm5, xmm10
movsd xmm5, xmm10

cvtss2sd xmm5, xmm10
cvtsd2ss xmm5, xmm10


; Source: xmm10, target: xmm6
movss xmm6, xmm10
movsd xmm6, xmm10

cvtss2sd xmm6, xmm10
cvtsd2ss xmm6, xmm10


; Source: xmm10, target: xmm7
movss xmm7, xmm10
movsd xmm7, xmm10

cvtss2sd xmm7, xmm10
cvtsd2ss xmm7, xmm10


; Source: xmm10, target: xmm8
movss xmm8, xmm10
movsd xmm8, xmm10

cvtss2sd xmm8, xmm10
cvtsd2ss xmm8, xmm10


; Source: xmm10, target: xmm9
movss xmm9, xmm10
movsd xmm9, xmm10

cvtss2sd xmm9, xmm10
cvtsd2ss xmm9, xmm10


; Source: xmm10, target: xmm10
movss xmm10, xmm10
movsd xmm10, xmm10

cvtss2sd xmm10, xmm10
cvtsd2ss xmm10, xmm10


; Source: xmm10, target: xmm11
movss xmm11, xmm10
movsd xmm11, xmm10

cvtss2sd xmm11, xmm10
cvtsd2ss xmm11, xmm10


; Source: xmm10, target: xmm12
movss xmm12, xmm10
movsd xmm12, xmm10

cvtss2sd xmm12, xmm10
cvtsd2ss xmm12, xmm10


; Source: xmm10, target: xmm13
movss xmm13, xmm10
movsd xmm13, xmm10

cvtss2sd xmm13, xmm10
cvtsd2ss xmm13, xmm10


; Source: xmm10, target: xmm14
movss xmm14, xmm10
movsd xmm14, xmm10

cvtss2sd xmm14, xmm10
cvtsd2ss xmm14, xmm10


; Source: xmm10, target: xmm15
movss xmm15, xmm10
movsd xmm15, xmm10

cvtss2sd xmm15, xmm10
cvtsd2ss xmm15, xmm10


; Source: xmm11, target: xmm0
movss xmm0, xmm11
movsd xmm0, xmm11

cvtss2sd xmm0, xmm11
cvtsd2ss xmm0, xmm11


; Source: xmm11, target: xmm1
movss xmm1, xmm11
movsd xmm1, xmm11

cvtss2sd xmm1, xmm11
cvtsd2ss xmm1, xmm11


; Source: xmm11, target: xmm2
movss xmm2, xmm11
movsd xmm2, xmm11

cvtss2sd xmm2, xmm11
cvtsd2ss xmm2, xmm11


; Source: xmm11, target: xmm3
movss xmm3, xmm11
movsd xmm3, xmm11

cvtss2sd xmm3, xmm11
cvtsd2ss xmm3, xmm11


; Source: xmm11, target: xmm4
movss xmm4, xmm11
movsd xmm4, xmm11

cvtss2sd xmm4, xmm11
cvtsd2ss xmm4, xmm11


; Source: xmm11, target: xmm5
movss xmm5, xmm11
movsd xmm5, xmm11

cvtss2sd xmm5, xmm11
cvtsd2ss xmm5, xmm11


; Source: xmm11, target: xmm6
movss xmm6, xmm11
movsd xmm6, xmm11

cvtss2sd xmm6, xmm11
cvtsd2ss xmm6, xmm11


; Source: xmm11, target: xmm7
movss xmm7, xmm11
movsd xmm7, xmm11

cvtss2sd xmm7, xmm11
cvtsd2ss xmm7, xmm11


; Source: xmm11, target: xmm8
movss xmm8, xmm11
movsd xmm8, xmm11

cvtss2sd xmm8, xmm11
cvtsd2ss xmm8, xmm11


; Source: xmm11, target: xmm9
movss xmm9, xmm11
movsd xmm9, xmm11

cvtss2sd xmm9, xmm11
cvtsd2ss xmm9, xmm11


; Source: xmm11, target: xmm10
movss xmm10, xmm11
movsd xmm10, xmm11

cvtss2sd xmm10, xmm11
cvtsd2ss xmm10, xmm11


; Source: xmm11, target: xmm11
movss xmm11, xmm11
movsd xmm11, xmm11

cvtss2sd xmm11, xmm11
cvtsd2ss xmm11, xmm11


; Source: xmm11, target: xmm12
movss xmm12, xmm11
movsd xmm12, xmm11

cvtss2sd xmm12, xmm11
cvtsd2ss xmm12, xmm11


; Source: xmm11, target: xmm13
movss xmm13, xmm11
movsd xmm13, xmm11

cvtss2sd xmm13, xmm11
cvtsd2ss xmm13, xmm11


; Source: xmm11, target: xmm14
movss xmm14, xmm11
movsd xmm14, xmm11

cvtss2sd xmm14, xmm11
cvtsd2ss xmm14, xmm11


; Source: xmm11, target: xmm15
movss xmm15, xmm11
movsd xmm15, xmm11

cvtss2sd xmm15, xmm11
cvtsd2ss xmm15, xmm11


; Source: xmm12, target: xmm0
movss xmm0, xmm12
movsd xmm0, xmm12

cvtss2sd xmm0, xmm12
cvtsd2ss xmm0, xmm12


; Source: xmm12, target: xmm1
movss xmm1, xmm12
movsd xmm1, xmm12

cvtss2sd xmm1, xmm12
cvtsd2ss xmm1, xmm12


; Source: xmm12, target: xmm2
movss xmm2, xmm12
movsd xmm2, xmm12

cvtss2sd xmm2, xmm12
cvtsd2ss xmm2, xmm12


; Source: xmm12, target: xmm3
movss xmm3, xmm12
movsd xmm3, xmm12

cvtss2sd xmm3, xmm12
cvtsd2ss xmm3, xmm12


; Source: xmm12, target: xmm4
movss xmm4, xmm12
movsd xmm4, xmm12

cvtss2sd xmm4, xmm12
cvtsd2ss xmm4, xmm12


; Source: xmm12, target: xmm5
movss xmm5, xmm12
movsd xmm5, xmm12

cvtss2sd xmm5, xmm12
cvtsd2ss xmm5, xmm12


; Source: xmm12, target: xmm6
movss xmm6, xmm12
movsd xmm6, xmm12

cvtss2sd xmm6, xmm12
cvtsd2ss xmm6, xmm12


; Source: xmm12, target: xmm7
movss xmm7, xmm12
movsd xmm7, xmm12

cvtss2sd xmm7, xmm12
cvtsd2ss xmm7, xmm12


; Source: xmm12, target: xmm8
movss xmm8, xmm12
movsd xmm8, xmm12

cvtss2sd xmm8, xmm12
cvtsd2ss xmm8, xmm12


; Source: xmm12, target: xmm9
movss xmm9, xmm12
movsd xmm9, xmm12

cvtss2sd xmm9, xmm12
cvtsd2ss xmm9, xmm12


; Source: xmm12, target: xmm10
movss xmm10, xmm12
movsd xmm10, xmm12

cvtss2sd xmm10, xmm12
cvtsd2ss xmm10, xmm12


; Source: xmm12, target: xmm11
movss xmm11, xmm12
movsd xmm11, xmm12

cvtss2sd xmm11, xmm12
cvtsd2ss xmm11, xmm12


; Source: xmm12, target: xmm12
movss xmm12, xmm12
movsd xmm12, xmm12

cvtss2sd xmm12, xmm12
cvtsd2ss xmm12, xmm12


; Source: xmm12, target: xmm13
movss xmm13, xmm12
movsd xmm13, xmm12

cvtss2sd xmm13, xmm12
cvtsd2ss xmm13, xmm12


; Source: xmm12, target: xmm14
movss xmm14, xmm12
movsd xmm14, xmm12

cvtss2sd xmm14, xmm12
cvtsd2ss xmm14, xmm12


; Source: xmm12, target: xmm15
movss xmm15, xmm12
movsd xmm15, xmm12

cvtss2sd xmm15, xmm12
cvtsd2ss xmm15, xmm12


; Source: xmm13, target: xmm0
movss xmm0, xmm13
movsd xmm0, xmm13

cvtss2sd xmm0, xmm13
cvtsd2ss xmm0, xmm13


; Source: xmm13, target: xmm1
movss xmm1, xmm13
movsd xmm1, xmm13

cvtss2sd xmm1, xmm13
cvtsd2ss xmm1, xmm13


; Source: xmm13, target: xmm2
movss xmm2, xmm13
movsd xmm2, xmm13

cvtss2sd xmm2, xmm13
cvtsd2ss xmm2, xmm13


; Source: xmm13, target: xmm3
movss xmm3, xmm13
movsd xmm3, xmm13

cvtss2sd xmm3, xmm13
cvtsd2ss xmm3, xmm13


; Source: xmm13, target: xmm4
movss xmm4, xmm13
movsd xmm4, xmm13

cvtss2sd xmm4, xmm13
cvtsd2ss xmm4, xmm13


; Source: xmm13, target: xmm5
movss xmm5, xmm13
movsd xmm5, xmm13

cvtss2sd xmm5, xmm13
cvtsd2ss xmm5, xmm13


; Source: xmm13, target: xmm6
movss xmm6, xmm13
movsd xmm6, xmm13

cvtss2sd xmm6, xmm13
cvtsd2ss xmm6, xmm13


; Source: xmm13, target: xmm7
movss xmm7, xmm13
movsd xmm7, xmm13

cvtss2sd xmm7, xmm13
cvtsd2ss xmm7, xmm13


; Source: xmm13, target: xmm8
movss xmm8, xmm13
movsd xmm8, xmm13

cvtss2sd xmm8, xmm13
cvtsd2ss xmm8, xmm13


; Source: xmm13, target: xmm9
movss xmm9, xmm13
movsd xmm9, xmm13

cvtss2sd xmm9, xmm13
cvtsd2ss xmm9, xmm13


; Source: xmm13, target: xmm10
movss xmm10, xmm13
movsd xmm10, xmm13

cvtss2sd xmm10, xmm13
cvtsd2ss xmm10, xmm13


; Source: xmm13, target: xmm11
movss xmm11, xmm13
movsd xmm11, xmm13

cvtss2sd xmm11, xmm13
cvtsd2ss xmm11, xmm13


; Source: xmm13, target: xmm12
movss xmm12, xmm13
movsd xmm12, xmm13

cvtss2sd xmm12, xmm13
cvtsd2ss xmm12, xmm13


; Source: xmm13, target: xmm13
movss xmm13, xmm13
movsd xmm13, xmm13

cvtss2sd xmm13, xmm13
cvtsd2ss xmm13, xmm13


; Source: xmm13, target: xmm14
movss xmm14, xmm13
movsd xmm14, xmm13

cvtss2sd xmm14, xmm13
cvtsd2ss xmm14, xmm13


; Source: xmm13, target: xmm15
movss xmm15, xmm13
movsd xmm15, xmm13

cvtss2sd xmm15, xmm13
cvtsd2ss xmm15, xmm13


; Source: xmm14, target: xmm0
movss xmm0, xmm14
movsd xmm0, xmm14

cvtss2sd xmm0, xmm14
cvtsd2ss xmm0, xmm14


; Source: xmm14, target: xmm1
movss xmm1, xmm14
movsd xmm1, xmm14

cvtss2sd xmm1, xmm14
cvtsd2ss xmm1, xmm14


; Source: xmm14, target: xmm2
movss xmm2, xmm14
movsd xmm2, xmm14

cvtss2sd xmm2, xmm14
cvtsd2ss xmm2, xmm14


; Source: xmm14, target: xmm3
movss xmm3, xmm14
movsd xmm3, xmm14

cvtss2sd xmm3, xmm14
cvtsd2ss xmm3, xmm14


; Source: xmm14, target: xmm4
movss xmm4, xmm14
movsd xmm4, xmm14

cvtss2sd xmm4, xmm14
cvtsd2ss xmm4, xmm14


; Source: xmm14, target: xmm5
movss xmm5, xmm14
movsd xmm5, xmm14

cvtss2sd xmm5, xmm14
cvtsd2ss xmm5, xmm14


; Source: xmm14, target: xmm6
movss xmm6, xmm14
movsd xmm6, xmm14

cvtss2sd xmm6, xmm14
cvtsd2ss xmm6, xmm14


; Source: xmm14, target: xmm7
movss xmm7, xmm14
movsd xmm7, xmm14

cvtss2sd xmm7, xmm14
cvtsd2ss xmm7, xmm14


; Source: xmm14, target: xmm8
movss xmm8, xmm14
movsd xmm8, xmm14

cvtss2sd xmm8, xmm14
cvtsd2ss xmm8, xmm14


; Source: xmm14, target: xmm9
movss xmm9, xmm14
movsd xmm9, xmm14

cvtss2sd xmm9, xmm14
cvtsd2ss xmm9, xmm14


; Source: xmm14, target: xmm10
movss xmm10, xmm14
movsd xmm10, xmm14

cvtss2sd xmm10, xmm14
cvtsd2ss xmm10, xmm14


; Source: xmm14, target: xmm11
movss xmm11, xmm14
movsd xmm11, xmm14

cvtss2sd xmm11, xmm14
cvtsd2ss xmm11, xmm14


; Source: xmm14, target: xmm12
movss xmm12, xmm14
movsd xmm12, xmm14

cvtss2sd xmm12, xmm14
cvtsd2ss xmm12, xmm14


; Source: xmm14, target: xmm13
movss xmm13, xmm14
movsd xmm13, xmm14

cvtss2sd xmm13, xmm14
cvtsd2ss xmm13, xmm14


; Source: xmm14, target: xmm14
movss xmm14, xmm14
movsd xmm14, xmm14

cvtss2sd xmm14, xmm14
cvtsd2ss xmm14, xmm14


; Source: xmm14, target: xmm15
movss xmm15, xmm14
movsd xmm15, xmm14

cvtss2sd xmm15, xmm14
cvtsd2ss xmm15, xmm14


; Source: xmm15, target: xmm0
movss xmm0, xmm15
movsd xmm0, xmm15

cvtss2sd xmm0, xmm15
cvtsd2ss xmm0, xmm15


; Source: xmm15, target: xmm1
movss xmm1, xmm15
movsd xmm1, xmm15

cvtss2sd xmm1, xmm15
cvtsd2ss xmm1, xmm15


; Source: xmm15, target: xmm2
movss xmm2, xmm15
movsd xmm2, xmm15

cvtss2sd xmm2, xmm15
cvtsd2ss xmm2, xmm15


; Source: xmm15, target: xmm3
movss xmm3, xmm15
movsd xmm3, xmm15

cvtss2sd xmm3, xmm15
cvtsd2ss xmm3, xmm15


; Source: xmm15, target: xmm4
movss xmm4, xmm15
movsd xmm4, xmm15

cvtss2sd xmm4, xmm15
cvtsd2ss xmm4, xmm15


; Source: xmm15, target: xmm5
movss xmm5, xmm15
movsd xmm5, xmm15

cvtss2sd xmm5, xmm15
cvtsd2ss xmm5, xmm15


; Source: xmm15, target: xmm6
movss xmm6, xmm15
movsd xmm6, xmm15

cvtss2sd xmm6, xmm15
cvtsd2ss xmm6, xmm15


; Source: xmm15, target: xmm7
movss xmm7, xmm15
movsd xmm7, xmm15

cvtss2sd xmm7, xmm15
cvtsd2ss xmm7, xmm15


; Source: xmm15, target: xmm8
movss xmm8, xmm15
movsd xmm8, xmm15

cvtss2sd xmm8, xmm15
cvtsd2ss xmm8, xmm15


; Source: xmm15, target: xmm9
movss xmm9, xmm15
movsd xmm9, xmm15

cvtss2sd xmm9, xmm15
cvtsd2ss xmm9, xmm15


; Source: xmm15, target: xmm10
movss xmm10, xmm15
movsd xmm10, xmm15

cvtss2sd xmm10, xmm15
cvtsd2ss xmm10, xmm15


; Source: xmm15, target: xmm11
movss xmm11, xmm15
movsd xmm11, xmm15

cvtss2sd xmm11, xmm15
cvtsd2ss xmm11, xmm15


; Source: xmm15, target: xmm12
movss xmm12, xmm15
movsd xmm12, xmm15

cvtss2sd xmm12, xmm15
cvtsd2ss xmm12, xmm15


; Source: xmm15, target: xmm13
movss xmm13, xmm15
movsd xmm13, xmm15

cvtss2sd xmm13, xmm15
cvtsd2ss xmm13, xmm15


; Source: xmm15, target: xmm14
movss xmm14, xmm15
movsd xmm14, xmm15

cvtss2sd xmm14, xmm15
cvtsd2ss xmm14, xmm15


; Source: xmm15, target: xmm15
movss xmm15, xmm15
movsd xmm15, xmm15

cvtss2sd xmm15, xmm15
cvtsd2ss xmm15, xmm15


; Float <-> Int

; Between: xmm0 and rax
movss dword ptr [rax - 4], xmm0
movsd qword ptr [rax + 4], xmm0

movss xmm0, dword ptr [rax - 4]
movsd xmm0, qword ptr [rax + 4]
cvtsi2ss xmm0, qword ptr [rax - 4]
cvtsi2sd xmm0, dword ptr [rax + 4]

cvttss2si eax, xmm0
cvttsd2si eax, xmm0

cvtsi2ss xmm0, eax
cvtsi2sd xmm0, eax

cvttss2si rax, xmm0
cvttsd2si rax, xmm0

cvtsi2ss xmm0, rax
cvtsi2sd xmm0, rax


; Between: xmm0 and rcx
movss dword ptr [rcx - 4], xmm0
movsd qword ptr [rcx + 4], xmm0

movss xmm0, dword ptr [rcx - 4]
movsd xmm0, qword ptr [rcx + 4]
cvtsi2ss xmm0, qword ptr [rcx - 4]
cvtsi2sd xmm0, dword ptr [rcx + 4]

cvttss2si ecx, xmm0
cvttsd2si ecx, xmm0

cvtsi2ss xmm0, ecx
cvtsi2sd xmm0, ecx

cvttss2si rcx, xmm0
cvttsd2si rcx, xmm0

cvtsi2ss xmm0, rcx
cvtsi2sd xmm0, rcx


; Between: xmm0 and rdx
movss dword ptr [rdx - 4], xmm0
movsd qword ptr [rdx + 4], xmm0

movss xmm0, dword ptr [rdx - 4]
movsd xmm0, qword ptr [rdx + 4]
cvtsi2ss xmm0, qword ptr [rdx - 4]
cvtsi2sd xmm0, dword ptr [rdx + 4]

cvttss2si edx, xmm0
cvttsd2si edx, xmm0

cvtsi2ss xmm0, edx
cvtsi2sd xmm0, edx

cvttss2si rdx, xmm0
cvttsd2si rdx, xmm0

cvtsi2ss xmm0, rdx
cvtsi2sd xmm0, rdx


; Between: xmm0 and rbx
movss dword ptr [rbx - 4], xmm0
movsd qword ptr [rbx + 4], xmm0

movss xmm0, dword ptr [rbx - 4]
movsd xmm0, qword ptr [rbx + 4]
cvtsi2ss xmm0, qword ptr [rbx - 4]
cvtsi2sd xmm0, dword ptr [rbx + 4]

cvttss2si ebx, xmm0
cvttsd2si ebx, xmm0

cvtsi2ss xmm0, ebx
cvtsi2sd xmm0, ebx

cvttss2si rbx, xmm0
cvttsd2si rbx, xmm0

cvtsi2ss xmm0, rbx
cvtsi2sd xmm0, rbx


; Between: xmm0 and rsp
movss dword ptr [rsp - 4], xmm0
movsd qword ptr [rsp + 4], xmm0

movss xmm0, dword ptr [rsp - 4]
movsd xmm0, qword ptr [rsp + 4]
cvtsi2ss xmm0, qword ptr [rsp - 4]
cvtsi2sd xmm0, dword ptr [rsp + 4]

cvttss2si esp, xmm0
cvttsd2si esp, xmm0

cvtsi2ss xmm0, esp
cvtsi2sd xmm0, esp

cvttss2si rsp, xmm0
cvttsd2si rsp, xmm0

cvtsi2ss xmm0, rsp
cvtsi2sd xmm0, rsp


; Between: xmm0 and rbp
movss dword ptr [rbp - 4], xmm0
movsd qword ptr [rbp + 4], xmm0

movss xmm0, dword ptr [rbp - 4]
movsd xmm0, qword ptr [rbp + 4]
cvtsi2ss xmm0, qword ptr [rbp - 4]
cvtsi2sd xmm0, dword ptr [rbp + 4]

cvttss2si ebp, xmm0
cvttsd2si ebp, xmm0

cvtsi2ss xmm0, ebp
cvtsi2sd xmm0, ebp

cvttss2si rbp, xmm0
cvttsd2si rbp, xmm0

cvtsi2ss xmm0, rbp
cvtsi2sd xmm0, rbp


; Between: xmm0 and rsi
movss dword ptr [rsi - 4], xmm0
movsd qword ptr [rsi + 4], xmm0

movss xmm0, dword ptr [rsi - 4]
movsd xmm0, qword ptr [rsi + 4]
cvtsi2ss xmm0, qword ptr [rsi - 4]
cvtsi2sd xmm0, dword ptr [rsi + 4]

cvttss2si esi, xmm0
cvttsd2si esi, xmm0

cvtsi2ss xmm0, esi
cvtsi2sd xmm0, esi

cvttss2si rsi, xmm0
cvttsd2si rsi, xmm0

cvtsi2ss xmm0, rsi
cvtsi2sd xmm0, rsi


; Between: xmm0 and rdi
movss dword ptr [rdi - 4], xmm0
movsd qword ptr [rdi + 4], xmm0

movss xmm0, dword ptr [rdi - 4]
movsd xmm0, qword ptr [rdi + 4]
cvtsi2ss xmm0, qword ptr [rdi - 4]
cvtsi2sd xmm0, dword ptr [rdi + 4]

cvttss2si edi, xmm0
cvttsd2si edi, xmm0

cvtsi2ss xmm0, edi
cvtsi2sd xmm0, edi

cvttss2si rdi, xmm0
cvttsd2si rdi, xmm0

cvtsi2ss xmm0, rdi
cvtsi2sd xmm0, rdi


; Between: xmm0 and r8
movss dword ptr [r8 - 4], xmm0
movsd qword ptr [r8 + 4], xmm0

movss xmm0, dword ptr [r8 - 4]
movsd xmm0, qword ptr [r8 + 4]
cvtsi2ss xmm0, qword ptr [r8 - 4]
cvtsi2sd xmm0, dword ptr [r8 + 4]

cvttss2si r8d, xmm0
cvttsd2si r8d, xmm0

cvtsi2ss xmm0, r8d
cvtsi2sd xmm0, r8d

cvttss2si r8, xmm0
cvttsd2si r8, xmm0

cvtsi2ss xmm0, r8
cvtsi2sd xmm0, r8


; Between: xmm0 and r9
movss dword ptr [r9 - 4], xmm0
movsd qword ptr [r9 + 4], xmm0

movss xmm0, dword ptr [r9 - 4]
movsd xmm0, qword ptr [r9 + 4]
cvtsi2ss xmm0, qword ptr [r9 - 4]
cvtsi2sd xmm0, dword ptr [r9 + 4]

cvttss2si r9d, xmm0
cvttsd2si r9d, xmm0

cvtsi2ss xmm0, r9d
cvtsi2sd xmm0, r9d

cvttss2si r9, xmm0
cvttsd2si r9, xmm0

cvtsi2ss xmm0, r9
cvtsi2sd xmm0, r9


; Between: xmm0 and r10
movss dword ptr [r10 - 4], xmm0
movsd qword ptr [r10 + 4], xmm0

movss xmm0, dword ptr [r10 - 4]
movsd xmm0, qword ptr [r10 + 4]
cvtsi2ss xmm0, qword ptr [r10 - 4]
cvtsi2sd xmm0, dword ptr [r10 + 4]

cvttss2si r10d, xmm0
cvttsd2si r10d, xmm0

cvtsi2ss xmm0, r10d
cvtsi2sd xmm0, r10d

cvttss2si r10, xmm0
cvttsd2si r10, xmm0

cvtsi2ss xmm0, r10
cvtsi2sd xmm0, r10


; Between: xmm0 and r11
movss dword ptr [r11 - 4], xmm0
movsd qword ptr [r11 + 4], xmm0

movss xmm0, dword ptr [r11 - 4]
movsd xmm0, qword ptr [r11 + 4]
cvtsi2ss xmm0, qword ptr [r11 - 4]
cvtsi2sd xmm0, dword ptr [r11 + 4]

cvttss2si r11d, xmm0
cvttsd2si r11d, xmm0

cvtsi2ss xmm0, r11d
cvtsi2sd xmm0, r11d

cvttss2si r11, xmm0
cvttsd2si r11, xmm0

cvtsi2ss xmm0, r11
cvtsi2sd xmm0, r11


; Between: xmm0 and r12
movss dword ptr [r12 - 4], xmm0
movsd qword ptr [r12 + 4], xmm0

movss xmm0, dword ptr [r12 - 4]
movsd xmm0, qword ptr [r12 + 4]
cvtsi2ss xmm0, qword ptr [r12 - 4]
cvtsi2sd xmm0, dword ptr [r12 + 4]

cvttss2si r12d, xmm0
cvttsd2si r12d, xmm0

cvtsi2ss xmm0, r12d
cvtsi2sd xmm0, r12d

cvttss2si r12, xmm0
cvttsd2si r12, xmm0

cvtsi2ss xmm0, r12
cvtsi2sd xmm0, r12


; Between: xmm0 and r13
movss dword ptr [r13 - 4], xmm0
movsd qword ptr [r13 + 4], xmm0

movss xmm0, dword ptr [r13 - 4]
movsd xmm0, qword ptr [r13 + 4]
cvtsi2ss xmm0, qword ptr [r13 - 4]
cvtsi2sd xmm0, dword ptr [r13 + 4]

cvttss2si r13d, xmm0
cvttsd2si r13d, xmm0

cvtsi2ss xmm0, r13d
cvtsi2sd xmm0, r13d

cvttss2si r13, xmm0
cvttsd2si r13, xmm0

cvtsi2ss xmm0, r13
cvtsi2sd xmm0, r13


; Between: xmm0 and r14
movss dword ptr [r14 - 4], xmm0
movsd qword ptr [r14 + 4], xmm0

movss xmm0, dword ptr [r14 - 4]
movsd xmm0, qword ptr [r14 + 4]
cvtsi2ss xmm0, qword ptr [r14 - 4]
cvtsi2sd xmm0, dword ptr [r14 + 4]

cvttss2si r14d, xmm0
cvttsd2si r14d, xmm0

cvtsi2ss xmm0, r14d
cvtsi2sd xmm0, r14d

cvttss2si r14, xmm0
cvttsd2si r14, xmm0

cvtsi2ss xmm0, r14
cvtsi2sd xmm0, r14


; Between: xmm0 and r15
movss dword ptr [r15 - 4], xmm0
movsd qword ptr [r15 + 4], xmm0

movss xmm0, dword ptr [r15 - 4]
movsd xmm0, qword ptr [r15 + 4]
cvtsi2ss xmm0, qword ptr [r15 - 4]
cvtsi2sd xmm0, dword ptr [r15 + 4]

cvttss2si r15d, xmm0
cvttsd2si r15d, xmm0

cvtsi2ss xmm0, r15d
cvtsi2sd xmm0, r15d

cvttss2si r15, xmm0
cvttsd2si r15, xmm0

cvtsi2ss xmm0, r15
cvtsi2sd xmm0, r15


; Between: xmm1 and rax
movss dword ptr [rax - 4], xmm1
movsd qword ptr [rax + 4], xmm1

movss xmm1, dword ptr [rax - 4]
movsd xmm1, qword ptr [rax + 4]
cvtsi2ss xmm1, qword ptr [rax - 4]
cvtsi2sd xmm1, dword ptr [rax + 4]

cvttss2si eax, xmm1
cvttsd2si eax, xmm1

cvtsi2ss xmm1, eax
cvtsi2sd xmm1, eax

cvttss2si rax, xmm1
cvttsd2si rax, xmm1

cvtsi2ss xmm1, rax
cvtsi2sd xmm1, rax


; Between: xmm1 and rcx
movss dword ptr [rcx - 4], xmm1
movsd qword ptr [rcx + 4], xmm1

movss xmm1, dword ptr [rcx - 4]
movsd xmm1, qword ptr [rcx + 4]
cvtsi2ss xmm1, qword ptr [rcx - 4]
cvtsi2sd xmm1, dword ptr [rcx + 4]

cvttss2si ecx, xmm1
cvttsd2si ecx, xmm1

cvtsi2ss xmm1, ecx
cvtsi2sd xmm1, ecx

cvttss2si rcx, xmm1
cvttsd2si rcx, xmm1

cvtsi2ss xmm1, rcx
cvtsi2sd xmm1, rcx


; Between: xmm1 and rdx
movss dword ptr [rdx - 4], xmm1
movsd qword ptr [rdx + 4], xmm1

movss xmm1, dword ptr [rdx - 4]
movsd xmm1, qword ptr [rdx + 4]
cvtsi2ss xmm1, qword ptr [rdx - 4]
cvtsi2sd xmm1, dword ptr [rdx + 4]

cvttss2si edx, xmm1
cvttsd2si edx, xmm1

cvtsi2ss xmm1, edx
cvtsi2sd xmm1, edx

cvttss2si rdx, xmm1
cvttsd2si rdx, xmm1

cvtsi2ss xmm1, rdx
cvtsi2sd xmm1, rdx


; Between: xmm1 and rbx
movss dword ptr [rbx - 4], xmm1
movsd qword ptr [rbx + 4], xmm1

movss xmm1, dword ptr [rbx - 4]
movsd xmm1, qword ptr [rbx + 4]
cvtsi2ss xmm1, qword ptr [rbx - 4]
cvtsi2sd xmm1, dword ptr [rbx + 4]

cvttss2si ebx, xmm1
cvttsd2si ebx, xmm1

cvtsi2ss xmm1, ebx
cvtsi2sd xmm1, ebx

cvttss2si rbx, xmm1
cvttsd2si rbx, xmm1

cvtsi2ss xmm1, rbx
cvtsi2sd xmm1, rbx


; Between: xmm1 and rsp
movss dword ptr [rsp - 4], xmm1
movsd qword ptr [rsp + 4], xmm1

movss xmm1, dword ptr [rsp - 4]
movsd xmm1, qword ptr [rsp + 4]
cvtsi2ss xmm1, qword ptr [rsp - 4]
cvtsi2sd xmm1, dword ptr [rsp + 4]

cvttss2si esp, xmm1
cvttsd2si esp, xmm1

cvtsi2ss xmm1, esp
cvtsi2sd xmm1, esp

cvttss2si rsp, xmm1
cvttsd2si rsp, xmm1

cvtsi2ss xmm1, rsp
cvtsi2sd xmm1, rsp


; Between: xmm1 and rbp
movss dword ptr [rbp - 4], xmm1
movsd qword ptr [rbp + 4], xmm1

movss xmm1, dword ptr [rbp - 4]
movsd xmm1, qword ptr [rbp + 4]
cvtsi2ss xmm1, qword ptr [rbp - 4]
cvtsi2sd xmm1, dword ptr [rbp + 4]

cvttss2si ebp, xmm1
cvttsd2si ebp, xmm1

cvtsi2ss xmm1, ebp
cvtsi2sd xmm1, ebp

cvttss2si rbp, xmm1
cvttsd2si rbp, xmm1

cvtsi2ss xmm1, rbp
cvtsi2sd xmm1, rbp


; Between: xmm1 and rsi
movss dword ptr [rsi - 4], xmm1
movsd qword ptr [rsi + 4], xmm1

movss xmm1, dword ptr [rsi - 4]
movsd xmm1, qword ptr [rsi + 4]
cvtsi2ss xmm1, qword ptr [rsi - 4]
cvtsi2sd xmm1, dword ptr [rsi + 4]

cvttss2si esi, xmm1
cvttsd2si esi, xmm1

cvtsi2ss xmm1, esi
cvtsi2sd xmm1, esi

cvttss2si rsi, xmm1
cvttsd2si rsi, xmm1

cvtsi2ss xmm1, rsi
cvtsi2sd xmm1, rsi


; Between: xmm1 and rdi
movss dword ptr [rdi - 4], xmm1
movsd qword ptr [rdi + 4], xmm1

movss xmm1, dword ptr [rdi - 4]
movsd xmm1, qword ptr [rdi + 4]
cvtsi2ss xmm1, qword ptr [rdi - 4]
cvtsi2sd xmm1, dword ptr [rdi + 4]

cvttss2si edi, xmm1
cvttsd2si edi, xmm1

cvtsi2ss xmm1, edi
cvtsi2sd xmm1, edi

cvttss2si rdi, xmm1
cvttsd2si rdi, xmm1

cvtsi2ss xmm1, rdi
cvtsi2sd xmm1, rdi


; Between: xmm1 and r8
movss dword ptr [r8 - 4], xmm1
movsd qword ptr [r8 + 4], xmm1

movss xmm1, dword ptr [r8 - 4]
movsd xmm1, qword ptr [r8 + 4]
cvtsi2ss xmm1, qword ptr [r8 - 4]
cvtsi2sd xmm1, dword ptr [r8 + 4]

cvttss2si r8d, xmm1
cvttsd2si r8d, xmm1

cvtsi2ss xmm1, r8d
cvtsi2sd xmm1, r8d

cvttss2si r8, xmm1
cvttsd2si r8, xmm1

cvtsi2ss xmm1, r8
cvtsi2sd xmm1, r8


; Between: xmm1 and r9
movss dword ptr [r9 - 4], xmm1
movsd qword ptr [r9 + 4], xmm1

movss xmm1, dword ptr [r9 - 4]
movsd xmm1, qword ptr [r9 + 4]
cvtsi2ss xmm1, qword ptr [r9 - 4]
cvtsi2sd xmm1, dword ptr [r9 + 4]

cvttss2si r9d, xmm1
cvttsd2si r9d, xmm1

cvtsi2ss xmm1, r9d
cvtsi2sd xmm1, r9d

cvttss2si r9, xmm1
cvttsd2si r9, xmm1

cvtsi2ss xmm1, r9
cvtsi2sd xmm1, r9


; Between: xmm1 and r10
movss dword ptr [r10 - 4], xmm1
movsd qword ptr [r10 + 4], xmm1

movss xmm1, dword ptr [r10 - 4]
movsd xmm1, qword ptr [r10 + 4]
cvtsi2ss xmm1, qword ptr [r10 - 4]
cvtsi2sd xmm1, dword ptr [r10 + 4]

cvttss2si r10d, xmm1
cvttsd2si r10d, xmm1

cvtsi2ss xmm1, r10d
cvtsi2sd xmm1, r10d

cvttss2si r10, xmm1
cvttsd2si r10, xmm1

cvtsi2ss xmm1, r10
cvtsi2sd xmm1, r10


; Between: xmm1 and r11
movss dword ptr [r11 - 4], xmm1
movsd qword ptr [r11 + 4], xmm1

movss xmm1, dword ptr [r11 - 4]
movsd xmm1, qword ptr [r11 + 4]
cvtsi2ss xmm1, qword ptr [r11 - 4]
cvtsi2sd xmm1, dword ptr [r11 + 4]

cvttss2si r11d, xmm1
cvttsd2si r11d, xmm1

cvtsi2ss xmm1, r11d
cvtsi2sd xmm1, r11d

cvttss2si r11, xmm1
cvttsd2si r11, xmm1

cvtsi2ss xmm1, r11
cvtsi2sd xmm1, r11


; Between: xmm1 and r12
movss dword ptr [r12 - 4], xmm1
movsd qword ptr [r12 + 4], xmm1

movss xmm1, dword ptr [r12 - 4]
movsd xmm1, qword ptr [r12 + 4]
cvtsi2ss xmm1, qword ptr [r12 - 4]
cvtsi2sd xmm1, dword ptr [r12 + 4]

cvttss2si r12d, xmm1
cvttsd2si r12d, xmm1

cvtsi2ss xmm1, r12d
cvtsi2sd xmm1, r12d

cvttss2si r12, xmm1
cvttsd2si r12, xmm1

cvtsi2ss xmm1, r12
cvtsi2sd xmm1, r12


; Between: xmm1 and r13
movss dword ptr [r13 - 4], xmm1
movsd qword ptr [r13 + 4], xmm1

movss xmm1, dword ptr [r13 - 4]
movsd xmm1, qword ptr [r13 + 4]
cvtsi2ss xmm1, qword ptr [r13 - 4]
cvtsi2sd xmm1, dword ptr [r13 + 4]

cvttss2si r13d, xmm1
cvttsd2si r13d, xmm1

cvtsi2ss xmm1, r13d
cvtsi2sd xmm1, r13d

cvttss2si r13, xmm1
cvttsd2si r13, xmm1

cvtsi2ss xmm1, r13
cvtsi2sd xmm1, r13


; Between: xmm1 and r14
movss dword ptr [r14 - 4], xmm1
movsd qword ptr [r14 + 4], xmm1

movss xmm1, dword ptr [r14 - 4]
movsd xmm1, qword ptr [r14 + 4]
cvtsi2ss xmm1, qword ptr [r14 - 4]
cvtsi2sd xmm1, dword ptr [r14 + 4]

cvttss2si r14d, xmm1
cvttsd2si r14d, xmm1

cvtsi2ss xmm1, r14d
cvtsi2sd xmm1, r14d

cvttss2si r14, xmm1
cvttsd2si r14, xmm1

cvtsi2ss xmm1, r14
cvtsi2sd xmm1, r14


; Between: xmm1 and r15
movss dword ptr [r15 - 4], xmm1
movsd qword ptr [r15 + 4], xmm1

movss xmm1, dword ptr [r15 - 4]
movsd xmm1, qword ptr [r15 + 4]
cvtsi2ss xmm1, qword ptr [r15 - 4]
cvtsi2sd xmm1, dword ptr [r15 + 4]

cvttss2si r15d, xmm1
cvttsd2si r15d, xmm1

cvtsi2ss xmm1, r15d
cvtsi2sd xmm1, r15d

cvttss2si r15, xmm1
cvttsd2si r15, xmm1

cvtsi2ss xmm1, r15
cvtsi2sd xmm1, r15


; Between: xmm2 and rax
movss dword ptr [rax - 4], xmm2
movsd qword ptr [rax + 4], xmm2

movss xmm2, dword ptr [rax - 4]
movsd xmm2, qword ptr [rax + 4]
cvtsi2ss xmm2, qword ptr [rax - 4]
cvtsi2sd xmm2, dword ptr [rax + 4]

cvttss2si eax, xmm2
cvttsd2si eax, xmm2

cvtsi2ss xmm2, eax
cvtsi2sd xmm2, eax

cvttss2si rax, xmm2
cvttsd2si rax, xmm2

cvtsi2ss xmm2, rax
cvtsi2sd xmm2, rax


; Between: xmm2 and rcx
movss dword ptr [rcx - 4], xmm2
movsd qword ptr [rcx + 4], xmm2

movss xmm2, dword ptr [rcx - 4]
movsd xmm2, qword ptr [rcx + 4]
cvtsi2ss xmm2, qword ptr [rcx - 4]
cvtsi2sd xmm2, dword ptr [rcx + 4]

cvttss2si ecx, xmm2
cvttsd2si ecx, xmm2

cvtsi2ss xmm2, ecx
cvtsi2sd xmm2, ecx

cvttss2si rcx, xmm2
cvttsd2si rcx, xmm2

cvtsi2ss xmm2, rcx
cvtsi2sd xmm2, rcx


; Between: xmm2 and rdx
movss dword ptr [rdx - 4], xmm2
movsd qword ptr [rdx + 4], xmm2

movss xmm2, dword ptr [rdx - 4]
movsd xmm2, qword ptr [rdx + 4]
cvtsi2ss xmm2, qword ptr [rdx - 4]
cvtsi2sd xmm2, dword ptr [rdx + 4]

cvttss2si edx, xmm2
cvttsd2si edx, xmm2

cvtsi2ss xmm2, edx
cvtsi2sd xmm2, edx

cvttss2si rdx, xmm2
cvttsd2si rdx, xmm2

cvtsi2ss xmm2, rdx
cvtsi2sd xmm2, rdx


; Between: xmm2 and rbx
movss dword ptr [rbx - 4], xmm2
movsd qword ptr [rbx + 4], xmm2

movss xmm2, dword ptr [rbx - 4]
movsd xmm2, qword ptr [rbx + 4]
cvtsi2ss xmm2, qword ptr [rbx - 4]
cvtsi2sd xmm2, dword ptr [rbx + 4]

cvttss2si ebx, xmm2
cvttsd2si ebx, xmm2

cvtsi2ss xmm2, ebx
cvtsi2sd xmm2, ebx

cvttss2si rbx, xmm2
cvttsd2si rbx, xmm2

cvtsi2ss xmm2, rbx
cvtsi2sd xmm2, rbx


; Between: xmm2 and rsp
movss dword ptr [rsp - 4], xmm2
movsd qword ptr [rsp + 4], xmm2

movss xmm2, dword ptr [rsp - 4]
movsd xmm2, qword ptr [rsp + 4]
cvtsi2ss xmm2, qword ptr [rsp - 4]
cvtsi2sd xmm2, dword ptr [rsp + 4]

cvttss2si esp, xmm2
cvttsd2si esp, xmm2

cvtsi2ss xmm2, esp
cvtsi2sd xmm2, esp

cvttss2si rsp, xmm2
cvttsd2si rsp, xmm2

cvtsi2ss xmm2, rsp
cvtsi2sd xmm2, rsp


; Between: xmm2 and rbp
movss dword ptr [rbp - 4], xmm2
movsd qword ptr [rbp + 4], xmm2

movss xmm2, dword ptr [rbp - 4]
movsd xmm2, qword ptr [rbp + 4]
cvtsi2ss xmm2, qword ptr [rbp - 4]
cvtsi2sd xmm2, dword ptr [rbp + 4]

cvttss2si ebp, xmm2
cvttsd2si ebp, xmm2

cvtsi2ss xmm2, ebp
cvtsi2sd xmm2, ebp

cvttss2si rbp, xmm2
cvttsd2si rbp, xmm2

cvtsi2ss xmm2, rbp
cvtsi2sd xmm2, rbp


; Between: xmm2 and rsi
movss dword ptr [rsi - 4], xmm2
movsd qword ptr [rsi + 4], xmm2

movss xmm2, dword ptr [rsi - 4]
movsd xmm2, qword ptr [rsi + 4]
cvtsi2ss xmm2, qword ptr [rsi - 4]
cvtsi2sd xmm2, dword ptr [rsi + 4]

cvttss2si esi, xmm2
cvttsd2si esi, xmm2

cvtsi2ss xmm2, esi
cvtsi2sd xmm2, esi

cvttss2si rsi, xmm2
cvttsd2si rsi, xmm2

cvtsi2ss xmm2, rsi
cvtsi2sd xmm2, rsi


; Between: xmm2 and rdi
movss dword ptr [rdi - 4], xmm2
movsd qword ptr [rdi + 4], xmm2

movss xmm2, dword ptr [rdi - 4]
movsd xmm2, qword ptr [rdi + 4]
cvtsi2ss xmm2, qword ptr [rdi - 4]
cvtsi2sd xmm2, dword ptr [rdi + 4]

cvttss2si edi, xmm2
cvttsd2si edi, xmm2

cvtsi2ss xmm2, edi
cvtsi2sd xmm2, edi

cvttss2si rdi, xmm2
cvttsd2si rdi, xmm2

cvtsi2ss xmm2, rdi
cvtsi2sd xmm2, rdi


; Between: xmm2 and r8
movss dword ptr [r8 - 4], xmm2
movsd qword ptr [r8 + 4], xmm2

movss xmm2, dword ptr [r8 - 4]
movsd xmm2, qword ptr [r8 + 4]
cvtsi2ss xmm2, qword ptr [r8 - 4]
cvtsi2sd xmm2, dword ptr [r8 + 4]

cvttss2si r8d, xmm2
cvttsd2si r8d, xmm2

cvtsi2ss xmm2, r8d
cvtsi2sd xmm2, r8d

cvttss2si r8, xmm2
cvttsd2si r8, xmm2

cvtsi2ss xmm2, r8
cvtsi2sd xmm2, r8


; Between: xmm2 and r9
movss dword ptr [r9 - 4], xmm2
movsd qword ptr [r9 + 4], xmm2

movss xmm2, dword ptr [r9 - 4]
movsd xmm2, qword ptr [r9 + 4]
cvtsi2ss xmm2, qword ptr [r9 - 4]
cvtsi2sd xmm2, dword ptr [r9 + 4]

cvttss2si r9d, xmm2
cvttsd2si r9d, xmm2

cvtsi2ss xmm2, r9d
cvtsi2sd xmm2, r9d

cvttss2si r9, xmm2
cvttsd2si r9, xmm2

cvtsi2ss xmm2, r9
cvtsi2sd xmm2, r9


; Between: xmm2 and r10
movss dword ptr [r10 - 4], xmm2
movsd qword ptr [r10 + 4], xmm2

movss xmm2, dword ptr [r10 - 4]
movsd xmm2, qword ptr [r10 + 4]
cvtsi2ss xmm2, qword ptr [r10 - 4]
cvtsi2sd xmm2, dword ptr [r10 + 4]

cvttss2si r10d, xmm2
cvttsd2si r10d, xmm2

cvtsi2ss xmm2, r10d
cvtsi2sd xmm2, r10d

cvttss2si r10, xmm2
cvttsd2si r10, xmm2

cvtsi2ss xmm2, r10
cvtsi2sd xmm2, r10


; Between: xmm2 and r11
movss dword ptr [r11 - 4], xmm2
movsd qword ptr [r11 + 4], xmm2

movss xmm2, dword ptr [r11 - 4]
movsd xmm2, qword ptr [r11 + 4]
cvtsi2ss xmm2, qword ptr [r11 - 4]
cvtsi2sd xmm2, dword ptr [r11 + 4]

cvttss2si r11d, xmm2
cvttsd2si r11d, xmm2

cvtsi2ss xmm2, r11d
cvtsi2sd xmm2, r11d

cvttss2si r11, xmm2
cvttsd2si r11, xmm2

cvtsi2ss xmm2, r11
cvtsi2sd xmm2, r11


; Between: xmm2 and r12
movss dword ptr [r12 - 4], xmm2
movsd qword ptr [r12 + 4], xmm2

movss xmm2, dword ptr [r12 - 4]
movsd xmm2, qword ptr [r12 + 4]
cvtsi2ss xmm2, qword ptr [r12 - 4]
cvtsi2sd xmm2, dword ptr [r12 + 4]

cvttss2si r12d, xmm2
cvttsd2si r12d, xmm2

cvtsi2ss xmm2, r12d
cvtsi2sd xmm2, r12d

cvttss2si r12, xmm2
cvttsd2si r12, xmm2

cvtsi2ss xmm2, r12
cvtsi2sd xmm2, r12


; Between: xmm2 and r13
movss dword ptr [r13 - 4], xmm2
movsd qword ptr [r13 + 4], xmm2

movss xmm2, dword ptr [r13 - 4]
movsd xmm2, qword ptr [r13 + 4]
cvtsi2ss xmm2, qword ptr [r13 - 4]
cvtsi2sd xmm2, dword ptr [r13 + 4]

cvttss2si r13d, xmm2
cvttsd2si r13d, xmm2

cvtsi2ss xmm2, r13d
cvtsi2sd xmm2, r13d

cvttss2si r13, xmm2
cvttsd2si r13, xmm2

cvtsi2ss xmm2, r13
cvtsi2sd xmm2, r13


; Between: xmm2 and r14
movss dword ptr [r14 - 4], xmm2
movsd qword ptr [r14 + 4], xmm2

movss xmm2, dword ptr [r14 - 4]
movsd xmm2, qword ptr [r14 + 4]
cvtsi2ss xmm2, qword ptr [r14 - 4]
cvtsi2sd xmm2, dword ptr [r14 + 4]

cvttss2si r14d, xmm2
cvttsd2si r14d, xmm2

cvtsi2ss xmm2, r14d
cvtsi2sd xmm2, r14d

cvttss2si r14, xmm2
cvttsd2si r14, xmm2

cvtsi2ss xmm2, r14
cvtsi2sd xmm2, r14


; Between: xmm2 and r15
movss dword ptr [r15 - 4], xmm2
movsd qword ptr [r15 + 4], xmm2

movss xmm2, dword ptr [r15 - 4]
movsd xmm2, qword ptr [r15 + 4]
cvtsi2ss xmm2, qword ptr [r15 - 4]
cvtsi2sd xmm2, dword ptr [r15 + 4]

cvttss2si r15d, xmm2
cvttsd2si r15d, xmm2

cvtsi2ss xmm2, r15d
cvtsi2sd xmm2, r15d

cvttss2si r15, xmm2
cvttsd2si r15, xmm2

cvtsi2ss xmm2, r15
cvtsi2sd xmm2, r15


; Between: xmm3 and rax
movss dword ptr [rax - 4], xmm3
movsd qword ptr [rax + 4], xmm3

movss xmm3, dword ptr [rax - 4]
movsd xmm3, qword ptr [rax + 4]
cvtsi2ss xmm3, qword ptr [rax - 4]
cvtsi2sd xmm3, dword ptr [rax + 4]

cvttss2si eax, xmm3
cvttsd2si eax, xmm3

cvtsi2ss xmm3, eax
cvtsi2sd xmm3, eax

cvttss2si rax, xmm3
cvttsd2si rax, xmm3

cvtsi2ss xmm3, rax
cvtsi2sd xmm3, rax


; Between: xmm3 and rcx
movss dword ptr [rcx - 4], xmm3
movsd qword ptr [rcx + 4], xmm3

movss xmm3, dword ptr [rcx - 4]
movsd xmm3, qword ptr [rcx + 4]
cvtsi2ss xmm3, qword ptr [rcx - 4]
cvtsi2sd xmm3, dword ptr [rcx + 4]

cvttss2si ecx, xmm3
cvttsd2si ecx, xmm3

cvtsi2ss xmm3, ecx
cvtsi2sd xmm3, ecx

cvttss2si rcx, xmm3
cvttsd2si rcx, xmm3

cvtsi2ss xmm3, rcx
cvtsi2sd xmm3, rcx


; Between: xmm3 and rdx
movss dword ptr [rdx - 4], xmm3
movsd qword ptr [rdx + 4], xmm3

movss xmm3, dword ptr [rdx - 4]
movsd xmm3, qword ptr [rdx + 4]
cvtsi2ss xmm3, qword ptr [rdx - 4]
cvtsi2sd xmm3, dword ptr [rdx + 4]

cvttss2si edx, xmm3
cvttsd2si edx, xmm3

cvtsi2ss xmm3, edx
cvtsi2sd xmm3, edx

cvttss2si rdx, xmm3
cvttsd2si rdx, xmm3

cvtsi2ss xmm3, rdx
cvtsi2sd xmm3, rdx


; Between: xmm3 and rbx
movss dword ptr [rbx - 4], xmm3
movsd qword ptr [rbx + 4], xmm3

movss xmm3, dword ptr [rbx - 4]
movsd xmm3, qword ptr [rbx + 4]
cvtsi2ss xmm3, qword ptr [rbx - 4]
cvtsi2sd xmm3, dword ptr [rbx + 4]

cvttss2si ebx, xmm3
cvttsd2si ebx, xmm3

cvtsi2ss xmm3, ebx
cvtsi2sd xmm3, ebx

cvttss2si rbx, xmm3
cvttsd2si rbx, xmm3

cvtsi2ss xmm3, rbx
cvtsi2sd xmm3, rbx


; Between: xmm3 and rsp
movss dword ptr [rsp - 4], xmm3
movsd qword ptr [rsp + 4], xmm3

movss xmm3, dword ptr [rsp - 4]
movsd xmm3, qword ptr [rsp + 4]
cvtsi2ss xmm3, qword ptr [rsp - 4]
cvtsi2sd xmm3, dword ptr [rsp + 4]

cvttss2si esp, xmm3
cvttsd2si esp, xmm3

cvtsi2ss xmm3, esp
cvtsi2sd xmm3, esp

cvttss2si rsp, xmm3
cvttsd2si rsp, xmm3

cvtsi2ss xmm3, rsp
cvtsi2sd xmm3, rsp


; Between: xmm3 and rbp
movss dword ptr [rbp - 4], xmm3
movsd qword ptr [rbp + 4], xmm3

movss xmm3, dword ptr [rbp - 4]
movsd xmm3, qword ptr [rbp + 4]
cvtsi2ss xmm3, qword ptr [rbp - 4]
cvtsi2sd xmm3, dword ptr [rbp + 4]

cvttss2si ebp, xmm3
cvttsd2si ebp, xmm3

cvtsi2ss xmm3, ebp
cvtsi2sd xmm3, ebp

cvttss2si rbp, xmm3
cvttsd2si rbp, xmm3

cvtsi2ss xmm3, rbp
cvtsi2sd xmm3, rbp


; Between: xmm3 and rsi
movss dword ptr [rsi - 4], xmm3
movsd qword ptr [rsi + 4], xmm3

movss xmm3, dword ptr [rsi - 4]
movsd xmm3, qword ptr [rsi + 4]
cvtsi2ss xmm3, qword ptr [rsi - 4]
cvtsi2sd xmm3, dword ptr [rsi + 4]

cvttss2si esi, xmm3
cvttsd2si esi, xmm3

cvtsi2ss xmm3, esi
cvtsi2sd xmm3, esi

cvttss2si rsi, xmm3
cvttsd2si rsi, xmm3

cvtsi2ss xmm3, rsi
cvtsi2sd xmm3, rsi


; Between: xmm3 and rdi
movss dword ptr [rdi - 4], xmm3
movsd qword ptr [rdi + 4], xmm3

movss xmm3, dword ptr [rdi - 4]
movsd xmm3, qword ptr [rdi + 4]
cvtsi2ss xmm3, qword ptr [rdi - 4]
cvtsi2sd xmm3, dword ptr [rdi + 4]

cvttss2si edi, xmm3
cvttsd2si edi, xmm3

cvtsi2ss xmm3, edi
cvtsi2sd xmm3, edi

cvttss2si rdi, xmm3
cvttsd2si rdi, xmm3

cvtsi2ss xmm3, rdi
cvtsi2sd xmm3, rdi


; Between: xmm3 and r8
movss dword ptr [r8 - 4], xmm3
movsd qword ptr [r8 + 4], xmm3

movss xmm3, dword ptr [r8 - 4]
movsd xmm3, qword ptr [r8 + 4]
cvtsi2ss xmm3, qword ptr [r8 - 4]
cvtsi2sd xmm3, dword ptr [r8 + 4]

cvttss2si r8d, xmm3
cvttsd2si r8d, xmm3

cvtsi2ss xmm3, r8d
cvtsi2sd xmm3, r8d

cvttss2si r8, xmm3
cvttsd2si r8, xmm3

cvtsi2ss xmm3, r8
cvtsi2sd xmm3, r8


; Between: xmm3 and r9
movss dword ptr [r9 - 4], xmm3
movsd qword ptr [r9 + 4], xmm3

movss xmm3, dword ptr [r9 - 4]
movsd xmm3, qword ptr [r9 + 4]
cvtsi2ss xmm3, qword ptr [r9 - 4]
cvtsi2sd xmm3, dword ptr [r9 + 4]

cvttss2si r9d, xmm3
cvttsd2si r9d, xmm3

cvtsi2ss xmm3, r9d
cvtsi2sd xmm3, r9d

cvttss2si r9, xmm3
cvttsd2si r9, xmm3

cvtsi2ss xmm3, r9
cvtsi2sd xmm3, r9


; Between: xmm3 and r10
movss dword ptr [r10 - 4], xmm3
movsd qword ptr [r10 + 4], xmm3

movss xmm3, dword ptr [r10 - 4]
movsd xmm3, qword ptr [r10 + 4]
cvtsi2ss xmm3, qword ptr [r10 - 4]
cvtsi2sd xmm3, dword ptr [r10 + 4]

cvttss2si r10d, xmm3
cvttsd2si r10d, xmm3

cvtsi2ss xmm3, r10d
cvtsi2sd xmm3, r10d

cvttss2si r10, xmm3
cvttsd2si r10, xmm3

cvtsi2ss xmm3, r10
cvtsi2sd xmm3, r10


; Between: xmm3 and r11
movss dword ptr [r11 - 4], xmm3
movsd qword ptr [r11 + 4], xmm3

movss xmm3, dword ptr [r11 - 4]
movsd xmm3, qword ptr [r11 + 4]
cvtsi2ss xmm3, qword ptr [r11 - 4]
cvtsi2sd xmm3, dword ptr [r11 + 4]

cvttss2si r11d, xmm3
cvttsd2si r11d, xmm3

cvtsi2ss xmm3, r11d
cvtsi2sd xmm3, r11d

cvttss2si r11, xmm3
cvttsd2si r11, xmm3

cvtsi2ss xmm3, r11
cvtsi2sd xmm3, r11


; Between: xmm3 and r12
movss dword ptr [r12 - 4], xmm3
movsd qword ptr [r12 + 4], xmm3

movss xmm3, dword ptr [r12 - 4]
movsd xmm3, qword ptr [r12 + 4]
cvtsi2ss xmm3, qword ptr [r12 - 4]
cvtsi2sd xmm3, dword ptr [r12 + 4]

cvttss2si r12d, xmm3
cvttsd2si r12d, xmm3

cvtsi2ss xmm3, r12d
cvtsi2sd xmm3, r12d

cvttss2si r12, xmm3
cvttsd2si r12, xmm3

cvtsi2ss xmm3, r12
cvtsi2sd xmm3, r12


; Between: xmm3 and r13
movss dword ptr [r13 - 4], xmm3
movsd qword ptr [r13 + 4], xmm3

movss xmm3, dword ptr [r13 - 4]
movsd xmm3, qword ptr [r13 + 4]
cvtsi2ss xmm3, qword ptr [r13 - 4]
cvtsi2sd xmm3, dword ptr [r13 + 4]

cvttss2si r13d, xmm3
cvttsd2si r13d, xmm3

cvtsi2ss xmm3, r13d
cvtsi2sd xmm3, r13d

cvttss2si r13, xmm3
cvttsd2si r13, xmm3

cvtsi2ss xmm3, r13
cvtsi2sd xmm3, r13


; Between: xmm3 and r14
movss dword ptr [r14 - 4], xmm3
movsd qword ptr [r14 + 4], xmm3

movss xmm3, dword ptr [r14 - 4]
movsd xmm3, qword ptr [r14 + 4]
cvtsi2ss xmm3, qword ptr [r14 - 4]
cvtsi2sd xmm3, dword ptr [r14 + 4]

cvttss2si r14d, xmm3
cvttsd2si r14d, xmm3

cvtsi2ss xmm3, r14d
cvtsi2sd xmm3, r14d

cvttss2si r14, xmm3
cvttsd2si r14, xmm3

cvtsi2ss xmm3, r14
cvtsi2sd xmm3, r14


; Between: xmm3 and r15
movss dword ptr [r15 - 4], xmm3
movsd qword ptr [r15 + 4], xmm3

movss xmm3, dword ptr [r15 - 4]
movsd xmm3, qword ptr [r15 + 4]
cvtsi2ss xmm3, qword ptr [r15 - 4]
cvtsi2sd xmm3, dword ptr [r15 + 4]

cvttss2si r15d, xmm3
cvttsd2si r15d, xmm3

cvtsi2ss xmm3, r15d
cvtsi2sd xmm3, r15d

cvttss2si r15, xmm3
cvttsd2si r15, xmm3

cvtsi2ss xmm3, r15
cvtsi2sd xmm3, r15


; Between: xmm4 and rax
movss dword ptr [rax - 4], xmm4
movsd qword ptr [rax + 4], xmm4

movss xmm4, dword ptr [rax - 4]
movsd xmm4, qword ptr [rax + 4]
cvtsi2ss xmm4, qword ptr [rax - 4]
cvtsi2sd xmm4, dword ptr [rax + 4]

cvttss2si eax, xmm4
cvttsd2si eax, xmm4

cvtsi2ss xmm4, eax
cvtsi2sd xmm4, eax

cvttss2si rax, xmm4
cvttsd2si rax, xmm4

cvtsi2ss xmm4, rax
cvtsi2sd xmm4, rax


; Between: xmm4 and rcx
movss dword ptr [rcx - 4], xmm4
movsd qword ptr [rcx + 4], xmm4

movss xmm4, dword ptr [rcx - 4]
movsd xmm4, qword ptr [rcx + 4]
cvtsi2ss xmm4, qword ptr [rcx - 4]
cvtsi2sd xmm4, dword ptr [rcx + 4]

cvttss2si ecx, xmm4
cvttsd2si ecx, xmm4

cvtsi2ss xmm4, ecx
cvtsi2sd xmm4, ecx

cvttss2si rcx, xmm4
cvttsd2si rcx, xmm4

cvtsi2ss xmm4, rcx
cvtsi2sd xmm4, rcx


; Between: xmm4 and rdx
movss dword ptr [rdx - 4], xmm4
movsd qword ptr [rdx + 4], xmm4

movss xmm4, dword ptr [rdx - 4]
movsd xmm4, qword ptr [rdx + 4]
cvtsi2ss xmm4, qword ptr [rdx - 4]
cvtsi2sd xmm4, dword ptr [rdx + 4]

cvttss2si edx, xmm4
cvttsd2si edx, xmm4

cvtsi2ss xmm4, edx
cvtsi2sd xmm4, edx

cvttss2si rdx, xmm4
cvttsd2si rdx, xmm4

cvtsi2ss xmm4, rdx
cvtsi2sd xmm4, rdx


; Between: xmm4 and rbx
movss dword ptr [rbx - 4], xmm4
movsd qword ptr [rbx + 4], xmm4

movss xmm4, dword ptr [rbx - 4]
movsd xmm4, qword ptr [rbx + 4]
cvtsi2ss xmm4, qword ptr [rbx - 4]
cvtsi2sd xmm4, dword ptr [rbx + 4]

cvttss2si ebx, xmm4
cvttsd2si ebx, xmm4

cvtsi2ss xmm4, ebx
cvtsi2sd xmm4, ebx

cvttss2si rbx, xmm4
cvttsd2si rbx, xmm4

cvtsi2ss xmm4, rbx
cvtsi2sd xmm4, rbx


; Between: xmm4 and rsp
movss dword ptr [rsp - 4], xmm4
movsd qword ptr [rsp + 4], xmm4

movss xmm4, dword ptr [rsp - 4]
movsd xmm4, qword ptr [rsp + 4]
cvtsi2ss xmm4, qword ptr [rsp - 4]
cvtsi2sd xmm4, dword ptr [rsp + 4]

cvttss2si esp, xmm4
cvttsd2si esp, xmm4

cvtsi2ss xmm4, esp
cvtsi2sd xmm4, esp

cvttss2si rsp, xmm4
cvttsd2si rsp, xmm4

cvtsi2ss xmm4, rsp
cvtsi2sd xmm4, rsp


; Between: xmm4 and rbp
movss dword ptr [rbp - 4], xmm4
movsd qword ptr [rbp + 4], xmm4

movss xmm4, dword ptr [rbp - 4]
movsd xmm4, qword ptr [rbp + 4]
cvtsi2ss xmm4, qword ptr [rbp - 4]
cvtsi2sd xmm4, dword ptr [rbp + 4]

cvttss2si ebp, xmm4
cvttsd2si ebp, xmm4

cvtsi2ss xmm4, ebp
cvtsi2sd xmm4, ebp

cvttss2si rbp, xmm4
cvttsd2si rbp, xmm4

cvtsi2ss xmm4, rbp
cvtsi2sd xmm4, rbp


; Between: xmm4 and rsi
movss dword ptr [rsi - 4], xmm4
movsd qword ptr [rsi + 4], xmm4

movss xmm4, dword ptr [rsi - 4]
movsd xmm4, qword ptr [rsi + 4]
cvtsi2ss xmm4, qword ptr [rsi - 4]
cvtsi2sd xmm4, dword ptr [rsi + 4]

cvttss2si esi, xmm4
cvttsd2si esi, xmm4

cvtsi2ss xmm4, esi
cvtsi2sd xmm4, esi

cvttss2si rsi, xmm4
cvttsd2si rsi, xmm4

cvtsi2ss xmm4, rsi
cvtsi2sd xmm4, rsi


; Between: xmm4 and rdi
movss dword ptr [rdi - 4], xmm4
movsd qword ptr [rdi + 4], xmm4

movss xmm4, dword ptr [rdi - 4]
movsd xmm4, qword ptr [rdi + 4]
cvtsi2ss xmm4, qword ptr [rdi - 4]
cvtsi2sd xmm4, dword ptr [rdi + 4]

cvttss2si edi, xmm4
cvttsd2si edi, xmm4

cvtsi2ss xmm4, edi
cvtsi2sd xmm4, edi

cvttss2si rdi, xmm4
cvttsd2si rdi, xmm4

cvtsi2ss xmm4, rdi
cvtsi2sd xmm4, rdi


; Between: xmm4 and r8
movss dword ptr [r8 - 4], xmm4
movsd qword ptr [r8 + 4], xmm4

movss xmm4, dword ptr [r8 - 4]
movsd xmm4, qword ptr [r8 + 4]
cvtsi2ss xmm4, qword ptr [r8 - 4]
cvtsi2sd xmm4, dword ptr [r8 + 4]

cvttss2si r8d, xmm4
cvttsd2si r8d, xmm4

cvtsi2ss xmm4, r8d
cvtsi2sd xmm4, r8d

cvttss2si r8, xmm4
cvttsd2si r8, xmm4

cvtsi2ss xmm4, r8
cvtsi2sd xmm4, r8


; Between: xmm4 and r9
movss dword ptr [r9 - 4], xmm4
movsd qword ptr [r9 + 4], xmm4

movss xmm4, dword ptr [r9 - 4]
movsd xmm4, qword ptr [r9 + 4]
cvtsi2ss xmm4, qword ptr [r9 - 4]
cvtsi2sd xmm4, dword ptr [r9 + 4]

cvttss2si r9d, xmm4
cvttsd2si r9d, xmm4

cvtsi2ss xmm4, r9d
cvtsi2sd xmm4, r9d

cvttss2si r9, xmm4
cvttsd2si r9, xmm4

cvtsi2ss xmm4, r9
cvtsi2sd xmm4, r9


; Between: xmm4 and r10
movss dword ptr [r10 - 4], xmm4
movsd qword ptr [r10 + 4], xmm4

movss xmm4, dword ptr [r10 - 4]
movsd xmm4, qword ptr [r10 + 4]
cvtsi2ss xmm4, qword ptr [r10 - 4]
cvtsi2sd xmm4, dword ptr [r10 + 4]

cvttss2si r10d, xmm4
cvttsd2si r10d, xmm4

cvtsi2ss xmm4, r10d
cvtsi2sd xmm4, r10d

cvttss2si r10, xmm4
cvttsd2si r10, xmm4

cvtsi2ss xmm4, r10
cvtsi2sd xmm4, r10


; Between: xmm4 and r11
movss dword ptr [r11 - 4], xmm4
movsd qword ptr [r11 + 4], xmm4

movss xmm4, dword ptr [r11 - 4]
movsd xmm4, qword ptr [r11 + 4]
cvtsi2ss xmm4, qword ptr [r11 - 4]
cvtsi2sd xmm4, dword ptr [r11 + 4]

cvttss2si r11d, xmm4
cvttsd2si r11d, xmm4

cvtsi2ss xmm4, r11d
cvtsi2sd xmm4, r11d

cvttss2si r11, xmm4
cvttsd2si r11, xmm4

cvtsi2ss xmm4, r11
cvtsi2sd xmm4, r11


; Between: xmm4 and r12
movss dword ptr [r12 - 4], xmm4
movsd qword ptr [r12 + 4], xmm4

movss xmm4, dword ptr [r12 - 4]
movsd xmm4, qword ptr [r12 + 4]
cvtsi2ss xmm4, qword ptr [r12 - 4]
cvtsi2sd xmm4, dword ptr [r12 + 4]

cvttss2si r12d, xmm4
cvttsd2si r12d, xmm4

cvtsi2ss xmm4, r12d
cvtsi2sd xmm4, r12d

cvttss2si r12, xmm4
cvttsd2si r12, xmm4

cvtsi2ss xmm4, r12
cvtsi2sd xmm4, r12


; Between: xmm4 and r13
movss dword ptr [r13 - 4], xmm4
movsd qword ptr [r13 + 4], xmm4

movss xmm4, dword ptr [r13 - 4]
movsd xmm4, qword ptr [r13 + 4]
cvtsi2ss xmm4, qword ptr [r13 - 4]
cvtsi2sd xmm4, dword ptr [r13 + 4]

cvttss2si r13d, xmm4
cvttsd2si r13d, xmm4

cvtsi2ss xmm4, r13d
cvtsi2sd xmm4, r13d

cvttss2si r13, xmm4
cvttsd2si r13, xmm4

cvtsi2ss xmm4, r13
cvtsi2sd xmm4, r13


; Between: xmm4 and r14
movss dword ptr [r14 - 4], xmm4
movsd qword ptr [r14 + 4], xmm4

movss xmm4, dword ptr [r14 - 4]
movsd xmm4, qword ptr [r14 + 4]
cvtsi2ss xmm4, qword ptr [r14 - 4]
cvtsi2sd xmm4, dword ptr [r14 + 4]

cvttss2si r14d, xmm4
cvttsd2si r14d, xmm4

cvtsi2ss xmm4, r14d
cvtsi2sd xmm4, r14d

cvttss2si r14, xmm4
cvttsd2si r14, xmm4

cvtsi2ss xmm4, r14
cvtsi2sd xmm4, r14


; Between: xmm4 and r15
movss dword ptr [r15 - 4], xmm4
movsd qword ptr [r15 + 4], xmm4

movss xmm4, dword ptr [r15 - 4]
movsd xmm4, qword ptr [r15 + 4]
cvtsi2ss xmm4, qword ptr [r15 - 4]
cvtsi2sd xmm4, dword ptr [r15 + 4]

cvttss2si r15d, xmm4
cvttsd2si r15d, xmm4

cvtsi2ss xmm4, r15d
cvtsi2sd xmm4, r15d

cvttss2si r15, xmm4
cvttsd2si r15, xmm4

cvtsi2ss xmm4, r15
cvtsi2sd xmm4, r15


; Between: xmm5 and rax
movss dword ptr [rax - 4], xmm5
movsd qword ptr [rax + 4], xmm5

movss xmm5, dword ptr [rax - 4]
movsd xmm5, qword ptr [rax + 4]
cvtsi2ss xmm5, qword ptr [rax - 4]
cvtsi2sd xmm5, dword ptr [rax + 4]

cvttss2si eax, xmm5
cvttsd2si eax, xmm5

cvtsi2ss xmm5, eax
cvtsi2sd xmm5, eax

cvttss2si rax, xmm5
cvttsd2si rax, xmm5

cvtsi2ss xmm5, rax
cvtsi2sd xmm5, rax


; Between: xmm5 and rcx
movss dword ptr [rcx - 4], xmm5
movsd qword ptr [rcx + 4], xmm5

movss xmm5, dword ptr [rcx - 4]
movsd xmm5, qword ptr [rcx + 4]
cvtsi2ss xmm5, qword ptr [rcx - 4]
cvtsi2sd xmm5, dword ptr [rcx + 4]

cvttss2si ecx, xmm5
cvttsd2si ecx, xmm5

cvtsi2ss xmm5, ecx
cvtsi2sd xmm5, ecx

cvttss2si rcx, xmm5
cvttsd2si rcx, xmm5

cvtsi2ss xmm5, rcx
cvtsi2sd xmm5, rcx


; Between: xmm5 and rdx
movss dword ptr [rdx - 4], xmm5
movsd qword ptr [rdx + 4], xmm5

movss xmm5, dword ptr [rdx - 4]
movsd xmm5, qword ptr [rdx + 4]
cvtsi2ss xmm5, qword ptr [rdx - 4]
cvtsi2sd xmm5, dword ptr [rdx + 4]

cvttss2si edx, xmm5
cvttsd2si edx, xmm5

cvtsi2ss xmm5, edx
cvtsi2sd xmm5, edx

cvttss2si rdx, xmm5
cvttsd2si rdx, xmm5

cvtsi2ss xmm5, rdx
cvtsi2sd xmm5, rdx


; Between: xmm5 and rbx
movss dword ptr [rbx - 4], xmm5
movsd qword ptr [rbx + 4], xmm5

movss xmm5, dword ptr [rbx - 4]
movsd xmm5, qword ptr [rbx + 4]
cvtsi2ss xmm5, qword ptr [rbx - 4]
cvtsi2sd xmm5, dword ptr [rbx + 4]

cvttss2si ebx, xmm5
cvttsd2si ebx, xmm5

cvtsi2ss xmm5, ebx
cvtsi2sd xmm5, ebx

cvttss2si rbx, xmm5
cvttsd2si rbx, xmm5

cvtsi2ss xmm5, rbx
cvtsi2sd xmm5, rbx


; Between: xmm5 and rsp
movss dword ptr [rsp - 4], xmm5
movsd qword ptr [rsp + 4], xmm5

movss xmm5, dword ptr [rsp - 4]
movsd xmm5, qword ptr [rsp + 4]
cvtsi2ss xmm5, qword ptr [rsp - 4]
cvtsi2sd xmm5, dword ptr [rsp + 4]

cvttss2si esp, xmm5
cvttsd2si esp, xmm5

cvtsi2ss xmm5, esp
cvtsi2sd xmm5, esp

cvttss2si rsp, xmm5
cvttsd2si rsp, xmm5

cvtsi2ss xmm5, rsp
cvtsi2sd xmm5, rsp


; Between: xmm5 and rbp
movss dword ptr [rbp - 4], xmm5
movsd qword ptr [rbp + 4], xmm5

movss xmm5, dword ptr [rbp - 4]
movsd xmm5, qword ptr [rbp + 4]
cvtsi2ss xmm5, qword ptr [rbp - 4]
cvtsi2sd xmm5, dword ptr [rbp + 4]

cvttss2si ebp, xmm5
cvttsd2si ebp, xmm5

cvtsi2ss xmm5, ebp
cvtsi2sd xmm5, ebp

cvttss2si rbp, xmm5
cvttsd2si rbp, xmm5

cvtsi2ss xmm5, rbp
cvtsi2sd xmm5, rbp


; Between: xmm5 and rsi
movss dword ptr [rsi - 4], xmm5
movsd qword ptr [rsi + 4], xmm5

movss xmm5, dword ptr [rsi - 4]
movsd xmm5, qword ptr [rsi + 4]
cvtsi2ss xmm5, qword ptr [rsi - 4]
cvtsi2sd xmm5, dword ptr [rsi + 4]

cvttss2si esi, xmm5
cvttsd2si esi, xmm5

cvtsi2ss xmm5, esi
cvtsi2sd xmm5, esi

cvttss2si rsi, xmm5
cvttsd2si rsi, xmm5

cvtsi2ss xmm5, rsi
cvtsi2sd xmm5, rsi


; Between: xmm5 and rdi
movss dword ptr [rdi - 4], xmm5
movsd qword ptr [rdi + 4], xmm5

movss xmm5, dword ptr [rdi - 4]
movsd xmm5, qword ptr [rdi + 4]
cvtsi2ss xmm5, qword ptr [rdi - 4]
cvtsi2sd xmm5, dword ptr [rdi + 4]

cvttss2si edi, xmm5
cvttsd2si edi, xmm5

cvtsi2ss xmm5, edi
cvtsi2sd xmm5, edi

cvttss2si rdi, xmm5
cvttsd2si rdi, xmm5

cvtsi2ss xmm5, rdi
cvtsi2sd xmm5, rdi


; Between: xmm5 and r8
movss dword ptr [r8 - 4], xmm5
movsd qword ptr [r8 + 4], xmm5

movss xmm5, dword ptr [r8 - 4]
movsd xmm5, qword ptr [r8 + 4]
cvtsi2ss xmm5, qword ptr [r8 - 4]
cvtsi2sd xmm5, dword ptr [r8 + 4]

cvttss2si r8d, xmm5
cvttsd2si r8d, xmm5

cvtsi2ss xmm5, r8d
cvtsi2sd xmm5, r8d

cvttss2si r8, xmm5
cvttsd2si r8, xmm5

cvtsi2ss xmm5, r8
cvtsi2sd xmm5, r8


; Between: xmm5 and r9
movss dword ptr [r9 - 4], xmm5
movsd qword ptr [r9 + 4], xmm5

movss xmm5, dword ptr [r9 - 4]
movsd xmm5, qword ptr [r9 + 4]
cvtsi2ss xmm5, qword ptr [r9 - 4]
cvtsi2sd xmm5, dword ptr [r9 + 4]

cvttss2si r9d, xmm5
cvttsd2si r9d, xmm5

cvtsi2ss xmm5, r9d
cvtsi2sd xmm5, r9d

cvttss2si r9, xmm5
cvttsd2si r9, xmm5

cvtsi2ss xmm5, r9
cvtsi2sd xmm5, r9


; Between: xmm5 and r10
movss dword ptr [r10 - 4], xmm5
movsd qword ptr [r10 + 4], xmm5

movss xmm5, dword ptr [r10 - 4]
movsd xmm5, qword ptr [r10 + 4]
cvtsi2ss xmm5, qword ptr [r10 - 4]
cvtsi2sd xmm5, dword ptr [r10 + 4]

cvttss2si r10d, xmm5
cvttsd2si r10d, xmm5

cvtsi2ss xmm5, r10d
cvtsi2sd xmm5, r10d

cvttss2si r10, xmm5
cvttsd2si r10, xmm5

cvtsi2ss xmm5, r10
cvtsi2sd xmm5, r10


; Between: xmm5 and r11
movss dword ptr [r11 - 4], xmm5
movsd qword ptr [r11 + 4], xmm5

movss xmm5, dword ptr [r11 - 4]
movsd xmm5, qword ptr [r11 + 4]
cvtsi2ss xmm5, qword ptr [r11 - 4]
cvtsi2sd xmm5, dword ptr [r11 + 4]

cvttss2si r11d, xmm5
cvttsd2si r11d, xmm5

cvtsi2ss xmm5, r11d
cvtsi2sd xmm5, r11d

cvttss2si r11, xmm5
cvttsd2si r11, xmm5

cvtsi2ss xmm5, r11
cvtsi2sd xmm5, r11


; Between: xmm5 and r12
movss dword ptr [r12 - 4], xmm5
movsd qword ptr [r12 + 4], xmm5

movss xmm5, dword ptr [r12 - 4]
movsd xmm5, qword ptr [r12 + 4]
cvtsi2ss xmm5, qword ptr [r12 - 4]
cvtsi2sd xmm5, dword ptr [r12 + 4]

cvttss2si r12d, xmm5
cvttsd2si r12d, xmm5

cvtsi2ss xmm5, r12d
cvtsi2sd xmm5, r12d

cvttss2si r12, xmm5
cvttsd2si r12, xmm5

cvtsi2ss xmm5, r12
cvtsi2sd xmm5, r12


; Between: xmm5 and r13
movss dword ptr [r13 - 4], xmm5
movsd qword ptr [r13 + 4], xmm5

movss xmm5, dword ptr [r13 - 4]
movsd xmm5, qword ptr [r13 + 4]
cvtsi2ss xmm5, qword ptr [r13 - 4]
cvtsi2sd xmm5, dword ptr [r13 + 4]

cvttss2si r13d, xmm5
cvttsd2si r13d, xmm5

cvtsi2ss xmm5, r13d
cvtsi2sd xmm5, r13d

cvttss2si r13, xmm5
cvttsd2si r13, xmm5

cvtsi2ss xmm5, r13
cvtsi2sd xmm5, r13


; Between: xmm5 and r14
movss dword ptr [r14 - 4], xmm5
movsd qword ptr [r14 + 4], xmm5

movss xmm5, dword ptr [r14 - 4]
movsd xmm5, qword ptr [r14 + 4]
cvtsi2ss xmm5, qword ptr [r14 - 4]
cvtsi2sd xmm5, dword ptr [r14 + 4]

cvttss2si r14d, xmm5
cvttsd2si r14d, xmm5

cvtsi2ss xmm5, r14d
cvtsi2sd xmm5, r14d

cvttss2si r14, xmm5
cvttsd2si r14, xmm5

cvtsi2ss xmm5, r14
cvtsi2sd xmm5, r14


; Between: xmm5 and r15
movss dword ptr [r15 - 4], xmm5
movsd qword ptr [r15 + 4], xmm5

movss xmm5, dword ptr [r15 - 4]
movsd xmm5, qword ptr [r15 + 4]
cvtsi2ss xmm5, qword ptr [r15 - 4]
cvtsi2sd xmm5, dword ptr [r15 + 4]

cvttss2si r15d, xmm5
cvttsd2si r15d, xmm5

cvtsi2ss xmm5, r15d
cvtsi2sd xmm5, r15d

cvttss2si r15, xmm5
cvttsd2si r15, xmm5

cvtsi2ss xmm5, r15
cvtsi2sd xmm5, r15


; Between: xmm6 and rax
movss dword ptr [rax - 4], xmm6
movsd qword ptr [rax + 4], xmm6

movss xmm6, dword ptr [rax - 4]
movsd xmm6, qword ptr [rax + 4]
cvtsi2ss xmm6, qword ptr [rax - 4]
cvtsi2sd xmm6, dword ptr [rax + 4]

cvttss2si eax, xmm6
cvttsd2si eax, xmm6

cvtsi2ss xmm6, eax
cvtsi2sd xmm6, eax

cvttss2si rax, xmm6
cvttsd2si rax, xmm6

cvtsi2ss xmm6, rax
cvtsi2sd xmm6, rax


; Between: xmm6 and rcx
movss dword ptr [rcx - 4], xmm6
movsd qword ptr [rcx + 4], xmm6

movss xmm6, dword ptr [rcx - 4]
movsd xmm6, qword ptr [rcx + 4]
cvtsi2ss xmm6, qword ptr [rcx - 4]
cvtsi2sd xmm6, dword ptr [rcx + 4]

cvttss2si ecx, xmm6
cvttsd2si ecx, xmm6

cvtsi2ss xmm6, ecx
cvtsi2sd xmm6, ecx

cvttss2si rcx, xmm6
cvttsd2si rcx, xmm6

cvtsi2ss xmm6, rcx
cvtsi2sd xmm6, rcx


; Between: xmm6 and rdx
movss dword ptr [rdx - 4], xmm6
movsd qword ptr [rdx + 4], xmm6

movss xmm6, dword ptr [rdx - 4]
movsd xmm6, qword ptr [rdx + 4]
cvtsi2ss xmm6, qword ptr [rdx - 4]
cvtsi2sd xmm6, dword ptr [rdx + 4]

cvttss2si edx, xmm6
cvttsd2si edx, xmm6

cvtsi2ss xmm6, edx
cvtsi2sd xmm6, edx

cvttss2si rdx, xmm6
cvttsd2si rdx, xmm6

cvtsi2ss xmm6, rdx
cvtsi2sd xmm6, rdx


; Between: xmm6 and rbx
movss dword ptr [rbx - 4], xmm6
movsd qword ptr [rbx + 4], xmm6

movss xmm6, dword ptr [rbx - 4]
movsd xmm6, qword ptr [rbx + 4]
cvtsi2ss xmm6, qword ptr [rbx - 4]
cvtsi2sd xmm6, dword ptr [rbx + 4]

cvttss2si ebx, xmm6
cvttsd2si ebx, xmm6

cvtsi2ss xmm6, ebx
cvtsi2sd xmm6, ebx

cvttss2si rbx, xmm6
cvttsd2si rbx, xmm6

cvtsi2ss xmm6, rbx
cvtsi2sd xmm6, rbx


; Between: xmm6 and rsp
movss dword ptr [rsp - 4], xmm6
movsd qword ptr [rsp + 4], xmm6

movss xmm6, dword ptr [rsp - 4]
movsd xmm6, qword ptr [rsp + 4]
cvtsi2ss xmm6, qword ptr [rsp - 4]
cvtsi2sd xmm6, dword ptr [rsp + 4]

cvttss2si esp, xmm6
cvttsd2si esp, xmm6

cvtsi2ss xmm6, esp
cvtsi2sd xmm6, esp

cvttss2si rsp, xmm6
cvttsd2si rsp, xmm6

cvtsi2ss xmm6, rsp
cvtsi2sd xmm6, rsp


; Between: xmm6 and rbp
movss dword ptr [rbp - 4], xmm6
movsd qword ptr [rbp + 4], xmm6

movss xmm6, dword ptr [rbp - 4]
movsd xmm6, qword ptr [rbp + 4]
cvtsi2ss xmm6, qword ptr [rbp - 4]
cvtsi2sd xmm6, dword ptr [rbp + 4]

cvttss2si ebp, xmm6
cvttsd2si ebp, xmm6

cvtsi2ss xmm6, ebp
cvtsi2sd xmm6, ebp

cvttss2si rbp, xmm6
cvttsd2si rbp, xmm6

cvtsi2ss xmm6, rbp
cvtsi2sd xmm6, rbp


; Between: xmm6 and rsi
movss dword ptr [rsi - 4], xmm6
movsd qword ptr [rsi + 4], xmm6

movss xmm6, dword ptr [rsi - 4]
movsd xmm6, qword ptr [rsi + 4]
cvtsi2ss xmm6, qword ptr [rsi - 4]
cvtsi2sd xmm6, dword ptr [rsi + 4]

cvttss2si esi, xmm6
cvttsd2si esi, xmm6

cvtsi2ss xmm6, esi
cvtsi2sd xmm6, esi

cvttss2si rsi, xmm6
cvttsd2si rsi, xmm6

cvtsi2ss xmm6, rsi
cvtsi2sd xmm6, rsi


; Between: xmm6 and rdi
movss dword ptr [rdi - 4], xmm6
movsd qword ptr [rdi + 4], xmm6

movss xmm6, dword ptr [rdi - 4]
movsd xmm6, qword ptr [rdi + 4]
cvtsi2ss xmm6, qword ptr [rdi - 4]
cvtsi2sd xmm6, dword ptr [rdi + 4]

cvttss2si edi, xmm6
cvttsd2si edi, xmm6

cvtsi2ss xmm6, edi
cvtsi2sd xmm6, edi

cvttss2si rdi, xmm6
cvttsd2si rdi, xmm6

cvtsi2ss xmm6, rdi
cvtsi2sd xmm6, rdi


; Between: xmm6 and r8
movss dword ptr [r8 - 4], xmm6
movsd qword ptr [r8 + 4], xmm6

movss xmm6, dword ptr [r8 - 4]
movsd xmm6, qword ptr [r8 + 4]
cvtsi2ss xmm6, qword ptr [r8 - 4]
cvtsi2sd xmm6, dword ptr [r8 + 4]

cvttss2si r8d, xmm6
cvttsd2si r8d, xmm6

cvtsi2ss xmm6, r8d
cvtsi2sd xmm6, r8d

cvttss2si r8, xmm6
cvttsd2si r8, xmm6

cvtsi2ss xmm6, r8
cvtsi2sd xmm6, r8


; Between: xmm6 and r9
movss dword ptr [r9 - 4], xmm6
movsd qword ptr [r9 + 4], xmm6

movss xmm6, dword ptr [r9 - 4]
movsd xmm6, qword ptr [r9 + 4]
cvtsi2ss xmm6, qword ptr [r9 - 4]
cvtsi2sd xmm6, dword ptr [r9 + 4]

cvttss2si r9d, xmm6
cvttsd2si r9d, xmm6

cvtsi2ss xmm6, r9d
cvtsi2sd xmm6, r9d

cvttss2si r9, xmm6
cvttsd2si r9, xmm6

cvtsi2ss xmm6, r9
cvtsi2sd xmm6, r9


; Between: xmm6 and r10
movss dword ptr [r10 - 4], xmm6
movsd qword ptr [r10 + 4], xmm6

movss xmm6, dword ptr [r10 - 4]
movsd xmm6, qword ptr [r10 + 4]
cvtsi2ss xmm6, qword ptr [r10 - 4]
cvtsi2sd xmm6, dword ptr [r10 + 4]

cvttss2si r10d, xmm6
cvttsd2si r10d, xmm6

cvtsi2ss xmm6, r10d
cvtsi2sd xmm6, r10d

cvttss2si r10, xmm6
cvttsd2si r10, xmm6

cvtsi2ss xmm6, r10
cvtsi2sd xmm6, r10


; Between: xmm6 and r11
movss dword ptr [r11 - 4], xmm6
movsd qword ptr [r11 + 4], xmm6

movss xmm6, dword ptr [r11 - 4]
movsd xmm6, qword ptr [r11 + 4]
cvtsi2ss xmm6, qword ptr [r11 - 4]
cvtsi2sd xmm6, dword ptr [r11 + 4]

cvttss2si r11d, xmm6
cvttsd2si r11d, xmm6

cvtsi2ss xmm6, r11d
cvtsi2sd xmm6, r11d

cvttss2si r11, xmm6
cvttsd2si r11, xmm6

cvtsi2ss xmm6, r11
cvtsi2sd xmm6, r11


; Between: xmm6 and r12
movss dword ptr [r12 - 4], xmm6
movsd qword ptr [r12 + 4], xmm6

movss xmm6, dword ptr [r12 - 4]
movsd xmm6, qword ptr [r12 + 4]
cvtsi2ss xmm6, qword ptr [r12 - 4]
cvtsi2sd xmm6, dword ptr [r12 + 4]

cvttss2si r12d, xmm6
cvttsd2si r12d, xmm6

cvtsi2ss xmm6, r12d
cvtsi2sd xmm6, r12d

cvttss2si r12, xmm6
cvttsd2si r12, xmm6

cvtsi2ss xmm6, r12
cvtsi2sd xmm6, r12


; Between: xmm6 and r13
movss dword ptr [r13 - 4], xmm6
movsd qword ptr [r13 + 4], xmm6

movss xmm6, dword ptr [r13 - 4]
movsd xmm6, qword ptr [r13 + 4]
cvtsi2ss xmm6, qword ptr [r13 - 4]
cvtsi2sd xmm6, dword ptr [r13 + 4]

cvttss2si r13d, xmm6
cvttsd2si r13d, xmm6

cvtsi2ss xmm6, r13d
cvtsi2sd xmm6, r13d

cvttss2si r13, xmm6
cvttsd2si r13, xmm6

cvtsi2ss xmm6, r13
cvtsi2sd xmm6, r13


; Between: xmm6 and r14
movss dword ptr [r14 - 4], xmm6
movsd qword ptr [r14 + 4], xmm6

movss xmm6, dword ptr [r14 - 4]
movsd xmm6, qword ptr [r14 + 4]
cvtsi2ss xmm6, qword ptr [r14 - 4]
cvtsi2sd xmm6, dword ptr [r14 + 4]

cvttss2si r14d, xmm6
cvttsd2si r14d, xmm6

cvtsi2ss xmm6, r14d
cvtsi2sd xmm6, r14d

cvttss2si r14, xmm6
cvttsd2si r14, xmm6

cvtsi2ss xmm6, r14
cvtsi2sd xmm6, r14


; Between: xmm6 and r15
movss dword ptr [r15 - 4], xmm6
movsd qword ptr [r15 + 4], xmm6

movss xmm6, dword ptr [r15 - 4]
movsd xmm6, qword ptr [r15 + 4]
cvtsi2ss xmm6, qword ptr [r15 - 4]
cvtsi2sd xmm6, dword ptr [r15 + 4]

cvttss2si r15d, xmm6
cvttsd2si r15d, xmm6

cvtsi2ss xmm6, r15d
cvtsi2sd xmm6, r15d

cvttss2si r15, xmm6
cvttsd2si r15, xmm6

cvtsi2ss xmm6, r15
cvtsi2sd xmm6, r15


; Between: xmm7 and rax
movss dword ptr [rax - 4], xmm7
movsd qword ptr [rax + 4], xmm7

movss xmm7, dword ptr [rax - 4]
movsd xmm7, qword ptr [rax + 4]
cvtsi2ss xmm7, qword ptr [rax - 4]
cvtsi2sd xmm7, dword ptr [rax + 4]

cvttss2si eax, xmm7
cvttsd2si eax, xmm7

cvtsi2ss xmm7, eax
cvtsi2sd xmm7, eax

cvttss2si rax, xmm7
cvttsd2si rax, xmm7

cvtsi2ss xmm7, rax
cvtsi2sd xmm7, rax


; Between: xmm7 and rcx
movss dword ptr [rcx - 4], xmm7
movsd qword ptr [rcx + 4], xmm7

movss xmm7, dword ptr [rcx - 4]
movsd xmm7, qword ptr [rcx + 4]
cvtsi2ss xmm7, qword ptr [rcx - 4]
cvtsi2sd xmm7, dword ptr [rcx + 4]

cvttss2si ecx, xmm7
cvttsd2si ecx, xmm7

cvtsi2ss xmm7, ecx
cvtsi2sd xmm7, ecx

cvttss2si rcx, xmm7
cvttsd2si rcx, xmm7

cvtsi2ss xmm7, rcx
cvtsi2sd xmm7, rcx


; Between: xmm7 and rdx
movss dword ptr [rdx - 4], xmm7
movsd qword ptr [rdx + 4], xmm7

movss xmm7, dword ptr [rdx - 4]
movsd xmm7, qword ptr [rdx + 4]
cvtsi2ss xmm7, qword ptr [rdx - 4]
cvtsi2sd xmm7, dword ptr [rdx + 4]

cvttss2si edx, xmm7
cvttsd2si edx, xmm7

cvtsi2ss xmm7, edx
cvtsi2sd xmm7, edx

cvttss2si rdx, xmm7
cvttsd2si rdx, xmm7

cvtsi2ss xmm7, rdx
cvtsi2sd xmm7, rdx


; Between: xmm7 and rbx
movss dword ptr [rbx - 4], xmm7
movsd qword ptr [rbx + 4], xmm7

movss xmm7, dword ptr [rbx - 4]
movsd xmm7, qword ptr [rbx + 4]
cvtsi2ss xmm7, qword ptr [rbx - 4]
cvtsi2sd xmm7, dword ptr [rbx + 4]

cvttss2si ebx, xmm7
cvttsd2si ebx, xmm7

cvtsi2ss xmm7, ebx
cvtsi2sd xmm7, ebx

cvttss2si rbx, xmm7
cvttsd2si rbx, xmm7

cvtsi2ss xmm7, rbx
cvtsi2sd xmm7, rbx


; Between: xmm7 and rsp
movss dword ptr [rsp - 4], xmm7
movsd qword ptr [rsp + 4], xmm7

movss xmm7, dword ptr [rsp - 4]
movsd xmm7, qword ptr [rsp + 4]
cvtsi2ss xmm7, qword ptr [rsp - 4]
cvtsi2sd xmm7, dword ptr [rsp + 4]

cvttss2si esp, xmm7
cvttsd2si esp, xmm7

cvtsi2ss xmm7, esp
cvtsi2sd xmm7, esp

cvttss2si rsp, xmm7
cvttsd2si rsp, xmm7

cvtsi2ss xmm7, rsp
cvtsi2sd xmm7, rsp


; Between: xmm7 and rbp
movss dword ptr [rbp - 4], xmm7
movsd qword ptr [rbp + 4], xmm7

movss xmm7, dword ptr [rbp - 4]
movsd xmm7, qword ptr [rbp + 4]
cvtsi2ss xmm7, qword ptr [rbp - 4]
cvtsi2sd xmm7, dword ptr [rbp + 4]

cvttss2si ebp, xmm7
cvttsd2si ebp, xmm7

cvtsi2ss xmm7, ebp
cvtsi2sd xmm7, ebp

cvttss2si rbp, xmm7
cvttsd2si rbp, xmm7

cvtsi2ss xmm7, rbp
cvtsi2sd xmm7, rbp


; Between: xmm7 and rsi
movss dword ptr [rsi - 4], xmm7
movsd qword ptr [rsi + 4], xmm7

movss xmm7, dword ptr [rsi - 4]
movsd xmm7, qword ptr [rsi + 4]
cvtsi2ss xmm7, qword ptr [rsi - 4]
cvtsi2sd xmm7, dword ptr [rsi + 4]

cvttss2si esi, xmm7
cvttsd2si esi, xmm7

cvtsi2ss xmm7, esi
cvtsi2sd xmm7, esi

cvttss2si rsi, xmm7
cvttsd2si rsi, xmm7

cvtsi2ss xmm7, rsi
cvtsi2sd xmm7, rsi


; Between: xmm7 and rdi
movss dword ptr [rdi - 4], xmm7
movsd qword ptr [rdi + 4], xmm7

movss xmm7, dword ptr [rdi - 4]
movsd xmm7, qword ptr [rdi + 4]
cvtsi2ss xmm7, qword ptr [rdi - 4]
cvtsi2sd xmm7, dword ptr [rdi + 4]

cvttss2si edi, xmm7
cvttsd2si edi, xmm7

cvtsi2ss xmm7, edi
cvtsi2sd xmm7, edi

cvttss2si rdi, xmm7
cvttsd2si rdi, xmm7

cvtsi2ss xmm7, rdi
cvtsi2sd xmm7, rdi


; Between: xmm7 and r8
movss dword ptr [r8 - 4], xmm7
movsd qword ptr [r8 + 4], xmm7

movss xmm7, dword ptr [r8 - 4]
movsd xmm7, qword ptr [r8 + 4]
cvtsi2ss xmm7, qword ptr [r8 - 4]
cvtsi2sd xmm7, dword ptr [r8 + 4]

cvttss2si r8d, xmm7
cvttsd2si r8d, xmm7

cvtsi2ss xmm7, r8d
cvtsi2sd xmm7, r8d

cvttss2si r8, xmm7
cvttsd2si r8, xmm7

cvtsi2ss xmm7, r8
cvtsi2sd xmm7, r8


; Between: xmm7 and r9
movss dword ptr [r9 - 4], xmm7
movsd qword ptr [r9 + 4], xmm7

movss xmm7, dword ptr [r9 - 4]
movsd xmm7, qword ptr [r9 + 4]
cvtsi2ss xmm7, qword ptr [r9 - 4]
cvtsi2sd xmm7, dword ptr [r9 + 4]

cvttss2si r9d, xmm7
cvttsd2si r9d, xmm7

cvtsi2ss xmm7, r9d
cvtsi2sd xmm7, r9d

cvttss2si r9, xmm7
cvttsd2si r9, xmm7

cvtsi2ss xmm7, r9
cvtsi2sd xmm7, r9


; Between: xmm7 and r10
movss dword ptr [r10 - 4], xmm7
movsd qword ptr [r10 + 4], xmm7

movss xmm7, dword ptr [r10 - 4]
movsd xmm7, qword ptr [r10 + 4]
cvtsi2ss xmm7, qword ptr [r10 - 4]
cvtsi2sd xmm7, dword ptr [r10 + 4]

cvttss2si r10d, xmm7
cvttsd2si r10d, xmm7

cvtsi2ss xmm7, r10d
cvtsi2sd xmm7, r10d

cvttss2si r10, xmm7
cvttsd2si r10, xmm7

cvtsi2ss xmm7, r10
cvtsi2sd xmm7, r10


; Between: xmm7 and r11
movss dword ptr [r11 - 4], xmm7
movsd qword ptr [r11 + 4], xmm7

movss xmm7, dword ptr [r11 - 4]
movsd xmm7, qword ptr [r11 + 4]
cvtsi2ss xmm7, qword ptr [r11 - 4]
cvtsi2sd xmm7, dword ptr [r11 + 4]

cvttss2si r11d, xmm7
cvttsd2si r11d, xmm7

cvtsi2ss xmm7, r11d
cvtsi2sd xmm7, r11d

cvttss2si r11, xmm7
cvttsd2si r11, xmm7

cvtsi2ss xmm7, r11
cvtsi2sd xmm7, r11


; Between: xmm7 and r12
movss dword ptr [r12 - 4], xmm7
movsd qword ptr [r12 + 4], xmm7

movss xmm7, dword ptr [r12 - 4]
movsd xmm7, qword ptr [r12 + 4]
cvtsi2ss xmm7, qword ptr [r12 - 4]
cvtsi2sd xmm7, dword ptr [r12 + 4]

cvttss2si r12d, xmm7
cvttsd2si r12d, xmm7

cvtsi2ss xmm7, r12d
cvtsi2sd xmm7, r12d

cvttss2si r12, xmm7
cvttsd2si r12, xmm7

cvtsi2ss xmm7, r12
cvtsi2sd xmm7, r12


; Between: xmm7 and r13
movss dword ptr [r13 - 4], xmm7
movsd qword ptr [r13 + 4], xmm7

movss xmm7, dword ptr [r13 - 4]
movsd xmm7, qword ptr [r13 + 4]
cvtsi2ss xmm7, qword ptr [r13 - 4]
cvtsi2sd xmm7, dword ptr [r13 + 4]

cvttss2si r13d, xmm7
cvttsd2si r13d, xmm7

cvtsi2ss xmm7, r13d
cvtsi2sd xmm7, r13d

cvttss2si r13, xmm7
cvttsd2si r13, xmm7

cvtsi2ss xmm7, r13
cvtsi2sd xmm7, r13


; Between: xmm7 and r14
movss dword ptr [r14 - 4], xmm7
movsd qword ptr [r14 + 4], xmm7

movss xmm7, dword ptr [r14 - 4]
movsd xmm7, qword ptr [r14 + 4]
cvtsi2ss xmm7, qword ptr [r14 - 4]
cvtsi2sd xmm7, dword ptr [r14 + 4]

cvttss2si r14d, xmm7
cvttsd2si r14d, xmm7

cvtsi2ss xmm7, r14d
cvtsi2sd xmm7, r14d

cvttss2si r14, xmm7
cvttsd2si r14, xmm7

cvtsi2ss xmm7, r14
cvtsi2sd xmm7, r14


; Between: xmm7 and r15
movss dword ptr [r15 - 4], xmm7
movsd qword ptr [r15 + 4], xmm7

movss xmm7, dword ptr [r15 - 4]
movsd xmm7, qword ptr [r15 + 4]
cvtsi2ss xmm7, qword ptr [r15 - 4]
cvtsi2sd xmm7, dword ptr [r15 + 4]

cvttss2si r15d, xmm7
cvttsd2si r15d, xmm7

cvtsi2ss xmm7, r15d
cvtsi2sd xmm7, r15d

cvttss2si r15, xmm7
cvttsd2si r15, xmm7

cvtsi2ss xmm7, r15
cvtsi2sd xmm7, r15


; Between: xmm8 and rax
movss dword ptr [rax - 4], xmm8
movsd qword ptr [rax + 4], xmm8

movss xmm8, dword ptr [rax - 4]
movsd xmm8, qword ptr [rax + 4]
cvtsi2ss xmm8, qword ptr [rax - 4]
cvtsi2sd xmm8, dword ptr [rax + 4]

cvttss2si eax, xmm8
cvttsd2si eax, xmm8

cvtsi2ss xmm8, eax
cvtsi2sd xmm8, eax

cvttss2si rax, xmm8
cvttsd2si rax, xmm8

cvtsi2ss xmm8, rax
cvtsi2sd xmm8, rax


; Between: xmm8 and rcx
movss dword ptr [rcx - 4], xmm8
movsd qword ptr [rcx + 4], xmm8

movss xmm8, dword ptr [rcx - 4]
movsd xmm8, qword ptr [rcx + 4]
cvtsi2ss xmm8, qword ptr [rcx - 4]
cvtsi2sd xmm8, dword ptr [rcx + 4]

cvttss2si ecx, xmm8
cvttsd2si ecx, xmm8

cvtsi2ss xmm8, ecx
cvtsi2sd xmm8, ecx

cvttss2si rcx, xmm8
cvttsd2si rcx, xmm8

cvtsi2ss xmm8, rcx
cvtsi2sd xmm8, rcx


; Between: xmm8 and rdx
movss dword ptr [rdx - 4], xmm8
movsd qword ptr [rdx + 4], xmm8

movss xmm8, dword ptr [rdx - 4]
movsd xmm8, qword ptr [rdx + 4]
cvtsi2ss xmm8, qword ptr [rdx - 4]
cvtsi2sd xmm8, dword ptr [rdx + 4]

cvttss2si edx, xmm8
cvttsd2si edx, xmm8

cvtsi2ss xmm8, edx
cvtsi2sd xmm8, edx

cvttss2si rdx, xmm8
cvttsd2si rdx, xmm8

cvtsi2ss xmm8, rdx
cvtsi2sd xmm8, rdx


; Between: xmm8 and rbx
movss dword ptr [rbx - 4], xmm8
movsd qword ptr [rbx + 4], xmm8

movss xmm8, dword ptr [rbx - 4]
movsd xmm8, qword ptr [rbx + 4]
cvtsi2ss xmm8, qword ptr [rbx - 4]
cvtsi2sd xmm8, dword ptr [rbx + 4]

cvttss2si ebx, xmm8
cvttsd2si ebx, xmm8

cvtsi2ss xmm8, ebx
cvtsi2sd xmm8, ebx

cvttss2si rbx, xmm8
cvttsd2si rbx, xmm8

cvtsi2ss xmm8, rbx
cvtsi2sd xmm8, rbx


; Between: xmm8 and rsp
movss dword ptr [rsp - 4], xmm8
movsd qword ptr [rsp + 4], xmm8

movss xmm8, dword ptr [rsp - 4]
movsd xmm8, qword ptr [rsp + 4]
cvtsi2ss xmm8, qword ptr [rsp - 4]
cvtsi2sd xmm8, dword ptr [rsp + 4]

cvttss2si esp, xmm8
cvttsd2si esp, xmm8

cvtsi2ss xmm8, esp
cvtsi2sd xmm8, esp

cvttss2si rsp, xmm8
cvttsd2si rsp, xmm8

cvtsi2ss xmm8, rsp
cvtsi2sd xmm8, rsp


; Between: xmm8 and rbp
movss dword ptr [rbp - 4], xmm8
movsd qword ptr [rbp + 4], xmm8

movss xmm8, dword ptr [rbp - 4]
movsd xmm8, qword ptr [rbp + 4]
cvtsi2ss xmm8, qword ptr [rbp - 4]
cvtsi2sd xmm8, dword ptr [rbp + 4]

cvttss2si ebp, xmm8
cvttsd2si ebp, xmm8

cvtsi2ss xmm8, ebp
cvtsi2sd xmm8, ebp

cvttss2si rbp, xmm8
cvttsd2si rbp, xmm8

cvtsi2ss xmm8, rbp
cvtsi2sd xmm8, rbp


; Between: xmm8 and rsi
movss dword ptr [rsi - 4], xmm8
movsd qword ptr [rsi + 4], xmm8

movss xmm8, dword ptr [rsi - 4]
movsd xmm8, qword ptr [rsi + 4]
cvtsi2ss xmm8, qword ptr [rsi - 4]
cvtsi2sd xmm8, dword ptr [rsi + 4]

cvttss2si esi, xmm8
cvttsd2si esi, xmm8

cvtsi2ss xmm8, esi
cvtsi2sd xmm8, esi

cvttss2si rsi, xmm8
cvttsd2si rsi, xmm8

cvtsi2ss xmm8, rsi
cvtsi2sd xmm8, rsi


; Between: xmm8 and rdi
movss dword ptr [rdi - 4], xmm8
movsd qword ptr [rdi + 4], xmm8

movss xmm8, dword ptr [rdi - 4]
movsd xmm8, qword ptr [rdi + 4]
cvtsi2ss xmm8, qword ptr [rdi - 4]
cvtsi2sd xmm8, dword ptr [rdi + 4]

cvttss2si edi, xmm8
cvttsd2si edi, xmm8

cvtsi2ss xmm8, edi
cvtsi2sd xmm8, edi

cvttss2si rdi, xmm8
cvttsd2si rdi, xmm8

cvtsi2ss xmm8, rdi
cvtsi2sd xmm8, rdi


; Between: xmm8 and r8
movss dword ptr [r8 - 4], xmm8
movsd qword ptr [r8 + 4], xmm8

movss xmm8, dword ptr [r8 - 4]
movsd xmm8, qword ptr [r8 + 4]
cvtsi2ss xmm8, qword ptr [r8 - 4]
cvtsi2sd xmm8, dword ptr [r8 + 4]

cvttss2si r8d, xmm8
cvttsd2si r8d, xmm8

cvtsi2ss xmm8, r8d
cvtsi2sd xmm8, r8d

cvttss2si r8, xmm8
cvttsd2si r8, xmm8

cvtsi2ss xmm8, r8
cvtsi2sd xmm8, r8


; Between: xmm8 and r9
movss dword ptr [r9 - 4], xmm8
movsd qword ptr [r9 + 4], xmm8

movss xmm8, dword ptr [r9 - 4]
movsd xmm8, qword ptr [r9 + 4]
cvtsi2ss xmm8, qword ptr [r9 - 4]
cvtsi2sd xmm8, dword ptr [r9 + 4]

cvttss2si r9d, xmm8
cvttsd2si r9d, xmm8

cvtsi2ss xmm8, r9d
cvtsi2sd xmm8, r9d

cvttss2si r9, xmm8
cvttsd2si r9, xmm8

cvtsi2ss xmm8, r9
cvtsi2sd xmm8, r9


; Between: xmm8 and r10
movss dword ptr [r10 - 4], xmm8
movsd qword ptr [r10 + 4], xmm8

movss xmm8, dword ptr [r10 - 4]
movsd xmm8, qword ptr [r10 + 4]
cvtsi2ss xmm8, qword ptr [r10 - 4]
cvtsi2sd xmm8, dword ptr [r10 + 4]

cvttss2si r10d, xmm8
cvttsd2si r10d, xmm8

cvtsi2ss xmm8, r10d
cvtsi2sd xmm8, r10d

cvttss2si r10, xmm8
cvttsd2si r10, xmm8

cvtsi2ss xmm8, r10
cvtsi2sd xmm8, r10


; Between: xmm8 and r11
movss dword ptr [r11 - 4], xmm8
movsd qword ptr [r11 + 4], xmm8

movss xmm8, dword ptr [r11 - 4]
movsd xmm8, qword ptr [r11 + 4]
cvtsi2ss xmm8, qword ptr [r11 - 4]
cvtsi2sd xmm8, dword ptr [r11 + 4]

cvttss2si r11d, xmm8
cvttsd2si r11d, xmm8

cvtsi2ss xmm8, r11d
cvtsi2sd xmm8, r11d

cvttss2si r11, xmm8
cvttsd2si r11, xmm8

cvtsi2ss xmm8, r11
cvtsi2sd xmm8, r11


; Between: xmm8 and r12
movss dword ptr [r12 - 4], xmm8
movsd qword ptr [r12 + 4], xmm8

movss xmm8, dword ptr [r12 - 4]
movsd xmm8, qword ptr [r12 + 4]
cvtsi2ss xmm8, qword ptr [r12 - 4]
cvtsi2sd xmm8, dword ptr [r12 + 4]

cvttss2si r12d, xmm8
cvttsd2si r12d, xmm8

cvtsi2ss xmm8, r12d
cvtsi2sd xmm8, r12d

cvttss2si r12, xmm8
cvttsd2si r12, xmm8

cvtsi2ss xmm8, r12
cvtsi2sd xmm8, r12


; Between: xmm8 and r13
movss dword ptr [r13 - 4], xmm8
movsd qword ptr [r13 + 4], xmm8

movss xmm8, dword ptr [r13 - 4]
movsd xmm8, qword ptr [r13 + 4]
cvtsi2ss xmm8, qword ptr [r13 - 4]
cvtsi2sd xmm8, dword ptr [r13 + 4]

cvttss2si r13d, xmm8
cvttsd2si r13d, xmm8

cvtsi2ss xmm8, r13d
cvtsi2sd xmm8, r13d

cvttss2si r13, xmm8
cvttsd2si r13, xmm8

cvtsi2ss xmm8, r13
cvtsi2sd xmm8, r13


; Between: xmm8 and r14
movss dword ptr [r14 - 4], xmm8
movsd qword ptr [r14 + 4], xmm8

movss xmm8, dword ptr [r14 - 4]
movsd xmm8, qword ptr [r14 + 4]
cvtsi2ss xmm8, qword ptr [r14 - 4]
cvtsi2sd xmm8, dword ptr [r14 + 4]

cvttss2si r14d, xmm8
cvttsd2si r14d, xmm8

cvtsi2ss xmm8, r14d
cvtsi2sd xmm8, r14d

cvttss2si r14, xmm8
cvttsd2si r14, xmm8

cvtsi2ss xmm8, r14
cvtsi2sd xmm8, r14


; Between: xmm8 and r15
movss dword ptr [r15 - 4], xmm8
movsd qword ptr [r15 + 4], xmm8

movss xmm8, dword ptr [r15 - 4]
movsd xmm8, qword ptr [r15 + 4]
cvtsi2ss xmm8, qword ptr [r15 - 4]
cvtsi2sd xmm8, dword ptr [r15 + 4]

cvttss2si r15d, xmm8
cvttsd2si r15d, xmm8

cvtsi2ss xmm8, r15d
cvtsi2sd xmm8, r15d

cvttss2si r15, xmm8
cvttsd2si r15, xmm8

cvtsi2ss xmm8, r15
cvtsi2sd xmm8, r15


; Between: xmm9 and rax
movss dword ptr [rax - 4], xmm9
movsd qword ptr [rax + 4], xmm9

movss xmm9, dword ptr [rax - 4]
movsd xmm9, qword ptr [rax + 4]
cvtsi2ss xmm9, qword ptr [rax - 4]
cvtsi2sd xmm9, dword ptr [rax + 4]

cvttss2si eax, xmm9
cvttsd2si eax, xmm9

cvtsi2ss xmm9, eax
cvtsi2sd xmm9, eax

cvttss2si rax, xmm9
cvttsd2si rax, xmm9

cvtsi2ss xmm9, rax
cvtsi2sd xmm9, rax


; Between: xmm9 and rcx
movss dword ptr [rcx - 4], xmm9
movsd qword ptr [rcx + 4], xmm9

movss xmm9, dword ptr [rcx - 4]
movsd xmm9, qword ptr [rcx + 4]
cvtsi2ss xmm9, qword ptr [rcx - 4]
cvtsi2sd xmm9, dword ptr [rcx + 4]

cvttss2si ecx, xmm9
cvttsd2si ecx, xmm9

cvtsi2ss xmm9, ecx
cvtsi2sd xmm9, ecx

cvttss2si rcx, xmm9
cvttsd2si rcx, xmm9

cvtsi2ss xmm9, rcx
cvtsi2sd xmm9, rcx


; Between: xmm9 and rdx
movss dword ptr [rdx - 4], xmm9
movsd qword ptr [rdx + 4], xmm9

movss xmm9, dword ptr [rdx - 4]
movsd xmm9, qword ptr [rdx + 4]
cvtsi2ss xmm9, qword ptr [rdx - 4]
cvtsi2sd xmm9, dword ptr [rdx + 4]

cvttss2si edx, xmm9
cvttsd2si edx, xmm9

cvtsi2ss xmm9, edx
cvtsi2sd xmm9, edx

cvttss2si rdx, xmm9
cvttsd2si rdx, xmm9

cvtsi2ss xmm9, rdx
cvtsi2sd xmm9, rdx


; Between: xmm9 and rbx
movss dword ptr [rbx - 4], xmm9
movsd qword ptr [rbx + 4], xmm9

movss xmm9, dword ptr [rbx - 4]
movsd xmm9, qword ptr [rbx + 4]
cvtsi2ss xmm9, qword ptr [rbx - 4]
cvtsi2sd xmm9, dword ptr [rbx + 4]

cvttss2si ebx, xmm9
cvttsd2si ebx, xmm9

cvtsi2ss xmm9, ebx
cvtsi2sd xmm9, ebx

cvttss2si rbx, xmm9
cvttsd2si rbx, xmm9

cvtsi2ss xmm9, rbx
cvtsi2sd xmm9, rbx


; Between: xmm9 and rsp
movss dword ptr [rsp - 4], xmm9
movsd qword ptr [rsp + 4], xmm9

movss xmm9, dword ptr [rsp - 4]
movsd xmm9, qword ptr [rsp + 4]
cvtsi2ss xmm9, qword ptr [rsp - 4]
cvtsi2sd xmm9, dword ptr [rsp + 4]

cvttss2si esp, xmm9
cvttsd2si esp, xmm9

cvtsi2ss xmm9, esp
cvtsi2sd xmm9, esp

cvttss2si rsp, xmm9
cvttsd2si rsp, xmm9

cvtsi2ss xmm9, rsp
cvtsi2sd xmm9, rsp


; Between: xmm9 and rbp
movss dword ptr [rbp - 4], xmm9
movsd qword ptr [rbp + 4], xmm9

movss xmm9, dword ptr [rbp - 4]
movsd xmm9, qword ptr [rbp + 4]
cvtsi2ss xmm9, qword ptr [rbp - 4]
cvtsi2sd xmm9, dword ptr [rbp + 4]

cvttss2si ebp, xmm9
cvttsd2si ebp, xmm9

cvtsi2ss xmm9, ebp
cvtsi2sd xmm9, ebp

cvttss2si rbp, xmm9
cvttsd2si rbp, xmm9

cvtsi2ss xmm9, rbp
cvtsi2sd xmm9, rbp


; Between: xmm9 and rsi
movss dword ptr [rsi - 4], xmm9
movsd qword ptr [rsi + 4], xmm9

movss xmm9, dword ptr [rsi - 4]
movsd xmm9, qword ptr [rsi + 4]
cvtsi2ss xmm9, qword ptr [rsi - 4]
cvtsi2sd xmm9, dword ptr [rsi + 4]

cvttss2si esi, xmm9
cvttsd2si esi, xmm9

cvtsi2ss xmm9, esi
cvtsi2sd xmm9, esi

cvttss2si rsi, xmm9
cvttsd2si rsi, xmm9

cvtsi2ss xmm9, rsi
cvtsi2sd xmm9, rsi


; Between: xmm9 and rdi
movss dword ptr [rdi - 4], xmm9
movsd qword ptr [rdi + 4], xmm9

movss xmm9, dword ptr [rdi - 4]
movsd xmm9, qword ptr [rdi + 4]
cvtsi2ss xmm9, qword ptr [rdi - 4]
cvtsi2sd xmm9, dword ptr [rdi + 4]

cvttss2si edi, xmm9
cvttsd2si edi, xmm9

cvtsi2ss xmm9, edi
cvtsi2sd xmm9, edi

cvttss2si rdi, xmm9
cvttsd2si rdi, xmm9

cvtsi2ss xmm9, rdi
cvtsi2sd xmm9, rdi


; Between: xmm9 and r8
movss dword ptr [r8 - 4], xmm9
movsd qword ptr [r8 + 4], xmm9

movss xmm9, dword ptr [r8 - 4]
movsd xmm9, qword ptr [r8 + 4]
cvtsi2ss xmm9, qword ptr [r8 - 4]
cvtsi2sd xmm9, dword ptr [r8 + 4]

cvttss2si r8d, xmm9
cvttsd2si r8d, xmm9

cvtsi2ss xmm9, r8d
cvtsi2sd xmm9, r8d

cvttss2si r8, xmm9
cvttsd2si r8, xmm9

cvtsi2ss xmm9, r8
cvtsi2sd xmm9, r8


; Between: xmm9 and r9
movss dword ptr [r9 - 4], xmm9
movsd qword ptr [r9 + 4], xmm9

movss xmm9, dword ptr [r9 - 4]
movsd xmm9, qword ptr [r9 + 4]
cvtsi2ss xmm9, qword ptr [r9 - 4]
cvtsi2sd xmm9, dword ptr [r9 + 4]

cvttss2si r9d, xmm9
cvttsd2si r9d, xmm9

cvtsi2ss xmm9, r9d
cvtsi2sd xmm9, r9d

cvttss2si r9, xmm9
cvttsd2si r9, xmm9

cvtsi2ss xmm9, r9
cvtsi2sd xmm9, r9


; Between: xmm9 and r10
movss dword ptr [r10 - 4], xmm9
movsd qword ptr [r10 + 4], xmm9

movss xmm9, dword ptr [r10 - 4]
movsd xmm9, qword ptr [r10 + 4]
cvtsi2ss xmm9, qword ptr [r10 - 4]
cvtsi2sd xmm9, dword ptr [r10 + 4]

cvttss2si r10d, xmm9
cvttsd2si r10d, xmm9

cvtsi2ss xmm9, r10d
cvtsi2sd xmm9, r10d

cvttss2si r10, xmm9
cvttsd2si r10, xmm9

cvtsi2ss xmm9, r10
cvtsi2sd xmm9, r10


; Between: xmm9 and r11
movss dword ptr [r11 - 4], xmm9
movsd qword ptr [r11 + 4], xmm9

movss xmm9, dword ptr [r11 - 4]
movsd xmm9, qword ptr [r11 + 4]
cvtsi2ss xmm9, qword ptr [r11 - 4]
cvtsi2sd xmm9, dword ptr [r11 + 4]

cvttss2si r11d, xmm9
cvttsd2si r11d, xmm9

cvtsi2ss xmm9, r11d
cvtsi2sd xmm9, r11d

cvttss2si r11, xmm9
cvttsd2si r11, xmm9

cvtsi2ss xmm9, r11
cvtsi2sd xmm9, r11


; Between: xmm9 and r12
movss dword ptr [r12 - 4], xmm9
movsd qword ptr [r12 + 4], xmm9

movss xmm9, dword ptr [r12 - 4]
movsd xmm9, qword ptr [r12 + 4]
cvtsi2ss xmm9, qword ptr [r12 - 4]
cvtsi2sd xmm9, dword ptr [r12 + 4]

cvttss2si r12d, xmm9
cvttsd2si r12d, xmm9

cvtsi2ss xmm9, r12d
cvtsi2sd xmm9, r12d

cvttss2si r12, xmm9
cvttsd2si r12, xmm9

cvtsi2ss xmm9, r12
cvtsi2sd xmm9, r12


; Between: xmm9 and r13
movss dword ptr [r13 - 4], xmm9
movsd qword ptr [r13 + 4], xmm9

movss xmm9, dword ptr [r13 - 4]
movsd xmm9, qword ptr [r13 + 4]
cvtsi2ss xmm9, qword ptr [r13 - 4]
cvtsi2sd xmm9, dword ptr [r13 + 4]

cvttss2si r13d, xmm9
cvttsd2si r13d, xmm9

cvtsi2ss xmm9, r13d
cvtsi2sd xmm9, r13d

cvttss2si r13, xmm9
cvttsd2si r13, xmm9

cvtsi2ss xmm9, r13
cvtsi2sd xmm9, r13


; Between: xmm9 and r14
movss dword ptr [r14 - 4], xmm9
movsd qword ptr [r14 + 4], xmm9

movss xmm9, dword ptr [r14 - 4]
movsd xmm9, qword ptr [r14 + 4]
cvtsi2ss xmm9, qword ptr [r14 - 4]
cvtsi2sd xmm9, dword ptr [r14 + 4]

cvttss2si r14d, xmm9
cvttsd2si r14d, xmm9

cvtsi2ss xmm9, r14d
cvtsi2sd xmm9, r14d

cvttss2si r14, xmm9
cvttsd2si r14, xmm9

cvtsi2ss xmm9, r14
cvtsi2sd xmm9, r14


; Between: xmm9 and r15
movss dword ptr [r15 - 4], xmm9
movsd qword ptr [r15 + 4], xmm9

movss xmm9, dword ptr [r15 - 4]
movsd xmm9, qword ptr [r15 + 4]
cvtsi2ss xmm9, qword ptr [r15 - 4]
cvtsi2sd xmm9, dword ptr [r15 + 4]

cvttss2si r15d, xmm9
cvttsd2si r15d, xmm9

cvtsi2ss xmm9, r15d
cvtsi2sd xmm9, r15d

cvttss2si r15, xmm9
cvttsd2si r15, xmm9

cvtsi2ss xmm9, r15
cvtsi2sd xmm9, r15


; Between: xmm10 and rax
movss dword ptr [rax - 4], xmm10
movsd qword ptr [rax + 4], xmm10

movss xmm10, dword ptr [rax - 4]
movsd xmm10, qword ptr [rax + 4]
cvtsi2ss xmm10, qword ptr [rax - 4]
cvtsi2sd xmm10, dword ptr [rax + 4]

cvttss2si eax, xmm10
cvttsd2si eax, xmm10

cvtsi2ss xmm10, eax
cvtsi2sd xmm10, eax

cvttss2si rax, xmm10
cvttsd2si rax, xmm10

cvtsi2ss xmm10, rax
cvtsi2sd xmm10, rax


; Between: xmm10 and rcx
movss dword ptr [rcx - 4], xmm10
movsd qword ptr [rcx + 4], xmm10

movss xmm10, dword ptr [rcx - 4]
movsd xmm10, qword ptr [rcx + 4]
cvtsi2ss xmm10, qword ptr [rcx - 4]
cvtsi2sd xmm10, dword ptr [rcx + 4]

cvttss2si ecx, xmm10
cvttsd2si ecx, xmm10

cvtsi2ss xmm10, ecx
cvtsi2sd xmm10, ecx

cvttss2si rcx, xmm10
cvttsd2si rcx, xmm10

cvtsi2ss xmm10, rcx
cvtsi2sd xmm10, rcx


; Between: xmm10 and rdx
movss dword ptr [rdx - 4], xmm10
movsd qword ptr [rdx + 4], xmm10

movss xmm10, dword ptr [rdx - 4]
movsd xmm10, qword ptr [rdx + 4]
cvtsi2ss xmm10, qword ptr [rdx - 4]
cvtsi2sd xmm10, dword ptr [rdx + 4]

cvttss2si edx, xmm10
cvttsd2si edx, xmm10

cvtsi2ss xmm10, edx
cvtsi2sd xmm10, edx

cvttss2si rdx, xmm10
cvttsd2si rdx, xmm10

cvtsi2ss xmm10, rdx
cvtsi2sd xmm10, rdx


; Between: xmm10 and rbx
movss dword ptr [rbx - 4], xmm10
movsd qword ptr [rbx + 4], xmm10

movss xmm10, dword ptr [rbx - 4]
movsd xmm10, qword ptr [rbx + 4]
cvtsi2ss xmm10, qword ptr [rbx - 4]
cvtsi2sd xmm10, dword ptr [rbx + 4]

cvttss2si ebx, xmm10
cvttsd2si ebx, xmm10

cvtsi2ss xmm10, ebx
cvtsi2sd xmm10, ebx

cvttss2si rbx, xmm10
cvttsd2si rbx, xmm10

cvtsi2ss xmm10, rbx
cvtsi2sd xmm10, rbx


; Between: xmm10 and rsp
movss dword ptr [rsp - 4], xmm10
movsd qword ptr [rsp + 4], xmm10

movss xmm10, dword ptr [rsp - 4]
movsd xmm10, qword ptr [rsp + 4]
cvtsi2ss xmm10, qword ptr [rsp - 4]
cvtsi2sd xmm10, dword ptr [rsp + 4]

cvttss2si esp, xmm10
cvttsd2si esp, xmm10

cvtsi2ss xmm10, esp
cvtsi2sd xmm10, esp

cvttss2si rsp, xmm10
cvttsd2si rsp, xmm10

cvtsi2ss xmm10, rsp
cvtsi2sd xmm10, rsp


; Between: xmm10 and rbp
movss dword ptr [rbp - 4], xmm10
movsd qword ptr [rbp + 4], xmm10

movss xmm10, dword ptr [rbp - 4]
movsd xmm10, qword ptr [rbp + 4]
cvtsi2ss xmm10, qword ptr [rbp - 4]
cvtsi2sd xmm10, dword ptr [rbp + 4]

cvttss2si ebp, xmm10
cvttsd2si ebp, xmm10

cvtsi2ss xmm10, ebp
cvtsi2sd xmm10, ebp

cvttss2si rbp, xmm10
cvttsd2si rbp, xmm10

cvtsi2ss xmm10, rbp
cvtsi2sd xmm10, rbp


; Between: xmm10 and rsi
movss dword ptr [rsi - 4], xmm10
movsd qword ptr [rsi + 4], xmm10

movss xmm10, dword ptr [rsi - 4]
movsd xmm10, qword ptr [rsi + 4]
cvtsi2ss xmm10, qword ptr [rsi - 4]
cvtsi2sd xmm10, dword ptr [rsi + 4]

cvttss2si esi, xmm10
cvttsd2si esi, xmm10

cvtsi2ss xmm10, esi
cvtsi2sd xmm10, esi

cvttss2si rsi, xmm10
cvttsd2si rsi, xmm10

cvtsi2ss xmm10, rsi
cvtsi2sd xmm10, rsi


; Between: xmm10 and rdi
movss dword ptr [rdi - 4], xmm10
movsd qword ptr [rdi + 4], xmm10

movss xmm10, dword ptr [rdi - 4]
movsd xmm10, qword ptr [rdi + 4]
cvtsi2ss xmm10, qword ptr [rdi - 4]
cvtsi2sd xmm10, dword ptr [rdi + 4]

cvttss2si edi, xmm10
cvttsd2si edi, xmm10

cvtsi2ss xmm10, edi
cvtsi2sd xmm10, edi

cvttss2si rdi, xmm10
cvttsd2si rdi, xmm10

cvtsi2ss xmm10, rdi
cvtsi2sd xmm10, rdi


; Between: xmm10 and r8
movss dword ptr [r8 - 4], xmm10
movsd qword ptr [r8 + 4], xmm10

movss xmm10, dword ptr [r8 - 4]
movsd xmm10, qword ptr [r8 + 4]
cvtsi2ss xmm10, qword ptr [r8 - 4]
cvtsi2sd xmm10, dword ptr [r8 + 4]

cvttss2si r8d, xmm10
cvttsd2si r8d, xmm10

cvtsi2ss xmm10, r8d
cvtsi2sd xmm10, r8d

cvttss2si r8, xmm10
cvttsd2si r8, xmm10

cvtsi2ss xmm10, r8
cvtsi2sd xmm10, r8


; Between: xmm10 and r9
movss dword ptr [r9 - 4], xmm10
movsd qword ptr [r9 + 4], xmm10

movss xmm10, dword ptr [r9 - 4]
movsd xmm10, qword ptr [r9 + 4]
cvtsi2ss xmm10, qword ptr [r9 - 4]
cvtsi2sd xmm10, dword ptr [r9 + 4]

cvttss2si r9d, xmm10
cvttsd2si r9d, xmm10

cvtsi2ss xmm10, r9d
cvtsi2sd xmm10, r9d

cvttss2si r9, xmm10
cvttsd2si r9, xmm10

cvtsi2ss xmm10, r9
cvtsi2sd xmm10, r9


; Between: xmm10 and r10
movss dword ptr [r10 - 4], xmm10
movsd qword ptr [r10 + 4], xmm10

movss xmm10, dword ptr [r10 - 4]
movsd xmm10, qword ptr [r10 + 4]
cvtsi2ss xmm10, qword ptr [r10 - 4]
cvtsi2sd xmm10, dword ptr [r10 + 4]

cvttss2si r10d, xmm10
cvttsd2si r10d, xmm10

cvtsi2ss xmm10, r10d
cvtsi2sd xmm10, r10d

cvttss2si r10, xmm10
cvttsd2si r10, xmm10

cvtsi2ss xmm10, r10
cvtsi2sd xmm10, r10


; Between: xmm10 and r11
movss dword ptr [r11 - 4], xmm10
movsd qword ptr [r11 + 4], xmm10

movss xmm10, dword ptr [r11 - 4]
movsd xmm10, qword ptr [r11 + 4]
cvtsi2ss xmm10, qword ptr [r11 - 4]
cvtsi2sd xmm10, dword ptr [r11 + 4]

cvttss2si r11d, xmm10
cvttsd2si r11d, xmm10

cvtsi2ss xmm10, r11d
cvtsi2sd xmm10, r11d

cvttss2si r11, xmm10
cvttsd2si r11, xmm10

cvtsi2ss xmm10, r11
cvtsi2sd xmm10, r11


; Between: xmm10 and r12
movss dword ptr [r12 - 4], xmm10
movsd qword ptr [r12 + 4], xmm10

movss xmm10, dword ptr [r12 - 4]
movsd xmm10, qword ptr [r12 + 4]
cvtsi2ss xmm10, qword ptr [r12 - 4]
cvtsi2sd xmm10, dword ptr [r12 + 4]

cvttss2si r12d, xmm10
cvttsd2si r12d, xmm10

cvtsi2ss xmm10, r12d
cvtsi2sd xmm10, r12d

cvttss2si r12, xmm10
cvttsd2si r12, xmm10

cvtsi2ss xmm10, r12
cvtsi2sd xmm10, r12


; Between: xmm10 and r13
movss dword ptr [r13 - 4], xmm10
movsd qword ptr [r13 + 4], xmm10

movss xmm10, dword ptr [r13 - 4]
movsd xmm10, qword ptr [r13 + 4]
cvtsi2ss xmm10, qword ptr [r13 - 4]
cvtsi2sd xmm10, dword ptr [r13 + 4]

cvttss2si r13d, xmm10
cvttsd2si r13d, xmm10

cvtsi2ss xmm10, r13d
cvtsi2sd xmm10, r13d

cvttss2si r13, xmm10
cvttsd2si r13, xmm10

cvtsi2ss xmm10, r13
cvtsi2sd xmm10, r13


; Between: xmm10 and r14
movss dword ptr [r14 - 4], xmm10
movsd qword ptr [r14 + 4], xmm10

movss xmm10, dword ptr [r14 - 4]
movsd xmm10, qword ptr [r14 + 4]
cvtsi2ss xmm10, qword ptr [r14 - 4]
cvtsi2sd xmm10, dword ptr [r14 + 4]

cvttss2si r14d, xmm10
cvttsd2si r14d, xmm10

cvtsi2ss xmm10, r14d
cvtsi2sd xmm10, r14d

cvttss2si r14, xmm10
cvttsd2si r14, xmm10

cvtsi2ss xmm10, r14
cvtsi2sd xmm10, r14


; Between: xmm10 and r15
movss dword ptr [r15 - 4], xmm10
movsd qword ptr [r15 + 4], xmm10

movss xmm10, dword ptr [r15 - 4]
movsd xmm10, qword ptr [r15 + 4]
cvtsi2ss xmm10, qword ptr [r15 - 4]
cvtsi2sd xmm10, dword ptr [r15 + 4]

cvttss2si r15d, xmm10
cvttsd2si r15d, xmm10

cvtsi2ss xmm10, r15d
cvtsi2sd xmm10, r15d

cvttss2si r15, xmm10
cvttsd2si r15, xmm10

cvtsi2ss xmm10, r15
cvtsi2sd xmm10, r15


; Between: xmm11 and rax
movss dword ptr [rax - 4], xmm11
movsd qword ptr [rax + 4], xmm11

movss xmm11, dword ptr [rax - 4]
movsd xmm11, qword ptr [rax + 4]
cvtsi2ss xmm11, qword ptr [rax - 4]
cvtsi2sd xmm11, dword ptr [rax + 4]

cvttss2si eax, xmm11
cvttsd2si eax, xmm11

cvtsi2ss xmm11, eax
cvtsi2sd xmm11, eax

cvttss2si rax, xmm11
cvttsd2si rax, xmm11

cvtsi2ss xmm11, rax
cvtsi2sd xmm11, rax


; Between: xmm11 and rcx
movss dword ptr [rcx - 4], xmm11
movsd qword ptr [rcx + 4], xmm11

movss xmm11, dword ptr [rcx - 4]
movsd xmm11, qword ptr [rcx + 4]
cvtsi2ss xmm11, qword ptr [rcx - 4]
cvtsi2sd xmm11, dword ptr [rcx + 4]

cvttss2si ecx, xmm11
cvttsd2si ecx, xmm11

cvtsi2ss xmm11, ecx
cvtsi2sd xmm11, ecx

cvttss2si rcx, xmm11
cvttsd2si rcx, xmm11

cvtsi2ss xmm11, rcx
cvtsi2sd xmm11, rcx


; Between: xmm11 and rdx
movss dword ptr [rdx - 4], xmm11
movsd qword ptr [rdx + 4], xmm11

movss xmm11, dword ptr [rdx - 4]
movsd xmm11, qword ptr [rdx + 4]
cvtsi2ss xmm11, qword ptr [rdx - 4]
cvtsi2sd xmm11, dword ptr [rdx + 4]

cvttss2si edx, xmm11
cvttsd2si edx, xmm11

cvtsi2ss xmm11, edx
cvtsi2sd xmm11, edx

cvttss2si rdx, xmm11
cvttsd2si rdx, xmm11

cvtsi2ss xmm11, rdx
cvtsi2sd xmm11, rdx


; Between: xmm11 and rbx
movss dword ptr [rbx - 4], xmm11
movsd qword ptr [rbx + 4], xmm11

movss xmm11, dword ptr [rbx - 4]
movsd xmm11, qword ptr [rbx + 4]
cvtsi2ss xmm11, qword ptr [rbx - 4]
cvtsi2sd xmm11, dword ptr [rbx + 4]

cvttss2si ebx, xmm11
cvttsd2si ebx, xmm11

cvtsi2ss xmm11, ebx
cvtsi2sd xmm11, ebx

cvttss2si rbx, xmm11
cvttsd2si rbx, xmm11

cvtsi2ss xmm11, rbx
cvtsi2sd xmm11, rbx


; Between: xmm11 and rsp
movss dword ptr [rsp - 4], xmm11
movsd qword ptr [rsp + 4], xmm11

movss xmm11, dword ptr [rsp - 4]
movsd xmm11, qword ptr [rsp + 4]
cvtsi2ss xmm11, qword ptr [rsp - 4]
cvtsi2sd xmm11, dword ptr [rsp + 4]

cvttss2si esp, xmm11
cvttsd2si esp, xmm11

cvtsi2ss xmm11, esp
cvtsi2sd xmm11, esp

cvttss2si rsp, xmm11
cvttsd2si rsp, xmm11

cvtsi2ss xmm11, rsp
cvtsi2sd xmm11, rsp


; Between: xmm11 and rbp
movss dword ptr [rbp - 4], xmm11
movsd qword ptr [rbp + 4], xmm11

movss xmm11, dword ptr [rbp - 4]
movsd xmm11, qword ptr [rbp + 4]
cvtsi2ss xmm11, qword ptr [rbp - 4]
cvtsi2sd xmm11, dword ptr [rbp + 4]

cvttss2si ebp, xmm11
cvttsd2si ebp, xmm11

cvtsi2ss xmm11, ebp
cvtsi2sd xmm11, ebp

cvttss2si rbp, xmm11
cvttsd2si rbp, xmm11

cvtsi2ss xmm11, rbp
cvtsi2sd xmm11, rbp


; Between: xmm11 and rsi
movss dword ptr [rsi - 4], xmm11
movsd qword ptr [rsi + 4], xmm11

movss xmm11, dword ptr [rsi - 4]
movsd xmm11, qword ptr [rsi + 4]
cvtsi2ss xmm11, qword ptr [rsi - 4]
cvtsi2sd xmm11, dword ptr [rsi + 4]

cvttss2si esi, xmm11
cvttsd2si esi, xmm11

cvtsi2ss xmm11, esi
cvtsi2sd xmm11, esi

cvttss2si rsi, xmm11
cvttsd2si rsi, xmm11

cvtsi2ss xmm11, rsi
cvtsi2sd xmm11, rsi


; Between: xmm11 and rdi
movss dword ptr [rdi - 4], xmm11
movsd qword ptr [rdi + 4], xmm11

movss xmm11, dword ptr [rdi - 4]
movsd xmm11, qword ptr [rdi + 4]
cvtsi2ss xmm11, qword ptr [rdi - 4]
cvtsi2sd xmm11, dword ptr [rdi + 4]

cvttss2si edi, xmm11
cvttsd2si edi, xmm11

cvtsi2ss xmm11, edi
cvtsi2sd xmm11, edi

cvttss2si rdi, xmm11
cvttsd2si rdi, xmm11

cvtsi2ss xmm11, rdi
cvtsi2sd xmm11, rdi


; Between: xmm11 and r8
movss dword ptr [r8 - 4], xmm11
movsd qword ptr [r8 + 4], xmm11

movss xmm11, dword ptr [r8 - 4]
movsd xmm11, qword ptr [r8 + 4]
cvtsi2ss xmm11, qword ptr [r8 - 4]
cvtsi2sd xmm11, dword ptr [r8 + 4]

cvttss2si r8d, xmm11
cvttsd2si r8d, xmm11

cvtsi2ss xmm11, r8d
cvtsi2sd xmm11, r8d

cvttss2si r8, xmm11
cvttsd2si r8, xmm11

cvtsi2ss xmm11, r8
cvtsi2sd xmm11, r8


; Between: xmm11 and r9
movss dword ptr [r9 - 4], xmm11
movsd qword ptr [r9 + 4], xmm11

movss xmm11, dword ptr [r9 - 4]
movsd xmm11, qword ptr [r9 + 4]
cvtsi2ss xmm11, qword ptr [r9 - 4]
cvtsi2sd xmm11, dword ptr [r9 + 4]

cvttss2si r9d, xmm11
cvttsd2si r9d, xmm11

cvtsi2ss xmm11, r9d
cvtsi2sd xmm11, r9d

cvttss2si r9, xmm11
cvttsd2si r9, xmm11

cvtsi2ss xmm11, r9
cvtsi2sd xmm11, r9


; Between: xmm11 and r10
movss dword ptr [r10 - 4], xmm11
movsd qword ptr [r10 + 4], xmm11

movss xmm11, dword ptr [r10 - 4]
movsd xmm11, qword ptr [r10 + 4]
cvtsi2ss xmm11, qword ptr [r10 - 4]
cvtsi2sd xmm11, dword ptr [r10 + 4]

cvttss2si r10d, xmm11
cvttsd2si r10d, xmm11

cvtsi2ss xmm11, r10d
cvtsi2sd xmm11, r10d

cvttss2si r10, xmm11
cvttsd2si r10, xmm11

cvtsi2ss xmm11, r10
cvtsi2sd xmm11, r10


; Between: xmm11 and r11
movss dword ptr [r11 - 4], xmm11
movsd qword ptr [r11 + 4], xmm11

movss xmm11, dword ptr [r11 - 4]
movsd xmm11, qword ptr [r11 + 4]
cvtsi2ss xmm11, qword ptr [r11 - 4]
cvtsi2sd xmm11, dword ptr [r11 + 4]

cvttss2si r11d, xmm11
cvttsd2si r11d, xmm11

cvtsi2ss xmm11, r11d
cvtsi2sd xmm11, r11d

cvttss2si r11, xmm11
cvttsd2si r11, xmm11

cvtsi2ss xmm11, r11
cvtsi2sd xmm11, r11


; Between: xmm11 and r12
movss dword ptr [r12 - 4], xmm11
movsd qword ptr [r12 + 4], xmm11

movss xmm11, dword ptr [r12 - 4]
movsd xmm11, qword ptr [r12 + 4]
cvtsi2ss xmm11, qword ptr [r12 - 4]
cvtsi2sd xmm11, dword ptr [r12 + 4]

cvttss2si r12d, xmm11
cvttsd2si r12d, xmm11

cvtsi2ss xmm11, r12d
cvtsi2sd xmm11, r12d

cvttss2si r12, xmm11
cvttsd2si r12, xmm11

cvtsi2ss xmm11, r12
cvtsi2sd xmm11, r12


; Between: xmm11 and r13
movss dword ptr [r13 - 4], xmm11
movsd qword ptr [r13 + 4], xmm11

movss xmm11, dword ptr [r13 - 4]
movsd xmm11, qword ptr [r13 + 4]
cvtsi2ss xmm11, qword ptr [r13 - 4]
cvtsi2sd xmm11, dword ptr [r13 + 4]

cvttss2si r13d, xmm11
cvttsd2si r13d, xmm11

cvtsi2ss xmm11, r13d
cvtsi2sd xmm11, r13d

cvttss2si r13, xmm11
cvttsd2si r13, xmm11

cvtsi2ss xmm11, r13
cvtsi2sd xmm11, r13


; Between: xmm11 and r14
movss dword ptr [r14 - 4], xmm11
movsd qword ptr [r14 + 4], xmm11

movss xmm11, dword ptr [r14 - 4]
movsd xmm11, qword ptr [r14 + 4]
cvtsi2ss xmm11, qword ptr [r14 - 4]
cvtsi2sd xmm11, dword ptr [r14 + 4]

cvttss2si r14d, xmm11
cvttsd2si r14d, xmm11

cvtsi2ss xmm11, r14d
cvtsi2sd xmm11, r14d

cvttss2si r14, xmm11
cvttsd2si r14, xmm11

cvtsi2ss xmm11, r14
cvtsi2sd xmm11, r14


; Between: xmm11 and r15
movss dword ptr [r15 - 4], xmm11
movsd qword ptr [r15 + 4], xmm11

movss xmm11, dword ptr [r15 - 4]
movsd xmm11, qword ptr [r15 + 4]
cvtsi2ss xmm11, qword ptr [r15 - 4]
cvtsi2sd xmm11, dword ptr [r15 + 4]

cvttss2si r15d, xmm11
cvttsd2si r15d, xmm11

cvtsi2ss xmm11, r15d
cvtsi2sd xmm11, r15d

cvttss2si r15, xmm11
cvttsd2si r15, xmm11

cvtsi2ss xmm11, r15
cvtsi2sd xmm11, r15


; Between: xmm12 and rax
movss dword ptr [rax - 4], xmm12
movsd qword ptr [rax + 4], xmm12

movss xmm12, dword ptr [rax - 4]
movsd xmm12, qword ptr [rax + 4]
cvtsi2ss xmm12, qword ptr [rax - 4]
cvtsi2sd xmm12, dword ptr [rax + 4]

cvttss2si eax, xmm12
cvttsd2si eax, xmm12

cvtsi2ss xmm12, eax
cvtsi2sd xmm12, eax

cvttss2si rax, xmm12
cvttsd2si rax, xmm12

cvtsi2ss xmm12, rax
cvtsi2sd xmm12, rax


; Between: xmm12 and rcx
movss dword ptr [rcx - 4], xmm12
movsd qword ptr [rcx + 4], xmm12

movss xmm12, dword ptr [rcx - 4]
movsd xmm12, qword ptr [rcx + 4]
cvtsi2ss xmm12, qword ptr [rcx - 4]
cvtsi2sd xmm12, dword ptr [rcx + 4]

cvttss2si ecx, xmm12
cvttsd2si ecx, xmm12

cvtsi2ss xmm12, ecx
cvtsi2sd xmm12, ecx

cvttss2si rcx, xmm12
cvttsd2si rcx, xmm12

cvtsi2ss xmm12, rcx
cvtsi2sd xmm12, rcx


; Between: xmm12 and rdx
movss dword ptr [rdx - 4], xmm12
movsd qword ptr [rdx + 4], xmm12

movss xmm12, dword ptr [rdx - 4]
movsd xmm12, qword ptr [rdx + 4]
cvtsi2ss xmm12, qword ptr [rdx - 4]
cvtsi2sd xmm12, dword ptr [rdx + 4]

cvttss2si edx, xmm12
cvttsd2si edx, xmm12

cvtsi2ss xmm12, edx
cvtsi2sd xmm12, edx

cvttss2si rdx, xmm12
cvttsd2si rdx, xmm12

cvtsi2ss xmm12, rdx
cvtsi2sd xmm12, rdx


; Between: xmm12 and rbx
movss dword ptr [rbx - 4], xmm12
movsd qword ptr [rbx + 4], xmm12

movss xmm12, dword ptr [rbx - 4]
movsd xmm12, qword ptr [rbx + 4]
cvtsi2ss xmm12, qword ptr [rbx - 4]
cvtsi2sd xmm12, dword ptr [rbx + 4]

cvttss2si ebx, xmm12
cvttsd2si ebx, xmm12

cvtsi2ss xmm12, ebx
cvtsi2sd xmm12, ebx

cvttss2si rbx, xmm12
cvttsd2si rbx, xmm12

cvtsi2ss xmm12, rbx
cvtsi2sd xmm12, rbx


; Between: xmm12 and rsp
movss dword ptr [rsp - 4], xmm12
movsd qword ptr [rsp + 4], xmm12

movss xmm12, dword ptr [rsp - 4]
movsd xmm12, qword ptr [rsp + 4]
cvtsi2ss xmm12, qword ptr [rsp - 4]
cvtsi2sd xmm12, dword ptr [rsp + 4]

cvttss2si esp, xmm12
cvttsd2si esp, xmm12

cvtsi2ss xmm12, esp
cvtsi2sd xmm12, esp

cvttss2si rsp, xmm12
cvttsd2si rsp, xmm12

cvtsi2ss xmm12, rsp
cvtsi2sd xmm12, rsp


; Between: xmm12 and rbp
movss dword ptr [rbp - 4], xmm12
movsd qword ptr [rbp + 4], xmm12

movss xmm12, dword ptr [rbp - 4]
movsd xmm12, qword ptr [rbp + 4]
cvtsi2ss xmm12, qword ptr [rbp - 4]
cvtsi2sd xmm12, dword ptr [rbp + 4]

cvttss2si ebp, xmm12
cvttsd2si ebp, xmm12

cvtsi2ss xmm12, ebp
cvtsi2sd xmm12, ebp

cvttss2si rbp, xmm12
cvttsd2si rbp, xmm12

cvtsi2ss xmm12, rbp
cvtsi2sd xmm12, rbp


; Between: xmm12 and rsi
movss dword ptr [rsi - 4], xmm12
movsd qword ptr [rsi + 4], xmm12

movss xmm12, dword ptr [rsi - 4]
movsd xmm12, qword ptr [rsi + 4]
cvtsi2ss xmm12, qword ptr [rsi - 4]
cvtsi2sd xmm12, dword ptr [rsi + 4]

cvttss2si esi, xmm12
cvttsd2si esi, xmm12

cvtsi2ss xmm12, esi
cvtsi2sd xmm12, esi

cvttss2si rsi, xmm12
cvttsd2si rsi, xmm12

cvtsi2ss xmm12, rsi
cvtsi2sd xmm12, rsi


; Between: xmm12 and rdi
movss dword ptr [rdi - 4], xmm12
movsd qword ptr [rdi + 4], xmm12

movss xmm12, dword ptr [rdi - 4]
movsd xmm12, qword ptr [rdi + 4]
cvtsi2ss xmm12, qword ptr [rdi - 4]
cvtsi2sd xmm12, dword ptr [rdi + 4]

cvttss2si edi, xmm12
cvttsd2si edi, xmm12

cvtsi2ss xmm12, edi
cvtsi2sd xmm12, edi

cvttss2si rdi, xmm12
cvttsd2si rdi, xmm12

cvtsi2ss xmm12, rdi
cvtsi2sd xmm12, rdi


; Between: xmm12 and r8
movss dword ptr [r8 - 4], xmm12
movsd qword ptr [r8 + 4], xmm12

movss xmm12, dword ptr [r8 - 4]
movsd xmm12, qword ptr [r8 + 4]
cvtsi2ss xmm12, qword ptr [r8 - 4]
cvtsi2sd xmm12, dword ptr [r8 + 4]

cvttss2si r8d, xmm12
cvttsd2si r8d, xmm12

cvtsi2ss xmm12, r8d
cvtsi2sd xmm12, r8d

cvttss2si r8, xmm12
cvttsd2si r8, xmm12

cvtsi2ss xmm12, r8
cvtsi2sd xmm12, r8


; Between: xmm12 and r9
movss dword ptr [r9 - 4], xmm12
movsd qword ptr [r9 + 4], xmm12

movss xmm12, dword ptr [r9 - 4]
movsd xmm12, qword ptr [r9 + 4]
cvtsi2ss xmm12, qword ptr [r9 - 4]
cvtsi2sd xmm12, dword ptr [r9 + 4]

cvttss2si r9d, xmm12
cvttsd2si r9d, xmm12

cvtsi2ss xmm12, r9d
cvtsi2sd xmm12, r9d

cvttss2si r9, xmm12
cvttsd2si r9, xmm12

cvtsi2ss xmm12, r9
cvtsi2sd xmm12, r9


; Between: xmm12 and r10
movss dword ptr [r10 - 4], xmm12
movsd qword ptr [r10 + 4], xmm12

movss xmm12, dword ptr [r10 - 4]
movsd xmm12, qword ptr [r10 + 4]
cvtsi2ss xmm12, qword ptr [r10 - 4]
cvtsi2sd xmm12, dword ptr [r10 + 4]

cvttss2si r10d, xmm12
cvttsd2si r10d, xmm12

cvtsi2ss xmm12, r10d
cvtsi2sd xmm12, r10d

cvttss2si r10, xmm12
cvttsd2si r10, xmm12

cvtsi2ss xmm12, r10
cvtsi2sd xmm12, r10


; Between: xmm12 and r11
movss dword ptr [r11 - 4], xmm12
movsd qword ptr [r11 + 4], xmm12

movss xmm12, dword ptr [r11 - 4]
movsd xmm12, qword ptr [r11 + 4]
cvtsi2ss xmm12, qword ptr [r11 - 4]
cvtsi2sd xmm12, dword ptr [r11 + 4]

cvttss2si r11d, xmm12
cvttsd2si r11d, xmm12

cvtsi2ss xmm12, r11d
cvtsi2sd xmm12, r11d

cvttss2si r11, xmm12
cvttsd2si r11, xmm12

cvtsi2ss xmm12, r11
cvtsi2sd xmm12, r11


; Between: xmm12 and r12
movss dword ptr [r12 - 4], xmm12
movsd qword ptr [r12 + 4], xmm12

movss xmm12, dword ptr [r12 - 4]
movsd xmm12, qword ptr [r12 + 4]
cvtsi2ss xmm12, qword ptr [r12 - 4]
cvtsi2sd xmm12, dword ptr [r12 + 4]

cvttss2si r12d, xmm12
cvttsd2si r12d, xmm12

cvtsi2ss xmm12, r12d
cvtsi2sd xmm12, r12d

cvttss2si r12, xmm12
cvttsd2si r12, xmm12

cvtsi2ss xmm12, r12
cvtsi2sd xmm12, r12


; Between: xmm12 and r13
movss dword ptr [r13 - 4], xmm12
movsd qword ptr [r13 + 4], xmm12

movss xmm12, dword ptr [r13 - 4]
movsd xmm12, qword ptr [r13 + 4]
cvtsi2ss xmm12, qword ptr [r13 - 4]
cvtsi2sd xmm12, dword ptr [r13 + 4]

cvttss2si r13d, xmm12
cvttsd2si r13d, xmm12

cvtsi2ss xmm12, r13d
cvtsi2sd xmm12, r13d

cvttss2si r13, xmm12
cvttsd2si r13, xmm12

cvtsi2ss xmm12, r13
cvtsi2sd xmm12, r13


; Between: xmm12 and r14
movss dword ptr [r14 - 4], xmm12
movsd qword ptr [r14 + 4], xmm12

movss xmm12, dword ptr [r14 - 4]
movsd xmm12, qword ptr [r14 + 4]
cvtsi2ss xmm12, qword ptr [r14 - 4]
cvtsi2sd xmm12, dword ptr [r14 + 4]

cvttss2si r14d, xmm12
cvttsd2si r14d, xmm12

cvtsi2ss xmm12, r14d
cvtsi2sd xmm12, r14d

cvttss2si r14, xmm12
cvttsd2si r14, xmm12

cvtsi2ss xmm12, r14
cvtsi2sd xmm12, r14


; Between: xmm12 and r15
movss dword ptr [r15 - 4], xmm12
movsd qword ptr [r15 + 4], xmm12

movss xmm12, dword ptr [r15 - 4]
movsd xmm12, qword ptr [r15 + 4]
cvtsi2ss xmm12, qword ptr [r15 - 4]
cvtsi2sd xmm12, dword ptr [r15 + 4]

cvttss2si r15d, xmm12
cvttsd2si r15d, xmm12

cvtsi2ss xmm12, r15d
cvtsi2sd xmm12, r15d

cvttss2si r15, xmm12
cvttsd2si r15, xmm12

cvtsi2ss xmm12, r15
cvtsi2sd xmm12, r15


; Between: xmm13 and rax
movss dword ptr [rax - 4], xmm13
movsd qword ptr [rax + 4], xmm13

movss xmm13, dword ptr [rax - 4]
movsd xmm13, qword ptr [rax + 4]
cvtsi2ss xmm13, qword ptr [rax - 4]
cvtsi2sd xmm13, dword ptr [rax + 4]

cvttss2si eax, xmm13
cvttsd2si eax, xmm13

cvtsi2ss xmm13, eax
cvtsi2sd xmm13, eax

cvttss2si rax, xmm13
cvttsd2si rax, xmm13

cvtsi2ss xmm13, rax
cvtsi2sd xmm13, rax


; Between: xmm13 and rcx
movss dword ptr [rcx - 4], xmm13
movsd qword ptr [rcx + 4], xmm13

movss xmm13, dword ptr [rcx - 4]
movsd xmm13, qword ptr [rcx + 4]
cvtsi2ss xmm13, qword ptr [rcx - 4]
cvtsi2sd xmm13, dword ptr [rcx + 4]

cvttss2si ecx, xmm13
cvttsd2si ecx, xmm13

cvtsi2ss xmm13, ecx
cvtsi2sd xmm13, ecx

cvttss2si rcx, xmm13
cvttsd2si rcx, xmm13

cvtsi2ss xmm13, rcx
cvtsi2sd xmm13, rcx


; Between: xmm13 and rdx
movss dword ptr [rdx - 4], xmm13
movsd qword ptr [rdx + 4], xmm13

movss xmm13, dword ptr [rdx - 4]
movsd xmm13, qword ptr [rdx + 4]
cvtsi2ss xmm13, qword ptr [rdx - 4]
cvtsi2sd xmm13, dword ptr [rdx + 4]

cvttss2si edx, xmm13
cvttsd2si edx, xmm13

cvtsi2ss xmm13, edx
cvtsi2sd xmm13, edx

cvttss2si rdx, xmm13
cvttsd2si rdx, xmm13

cvtsi2ss xmm13, rdx
cvtsi2sd xmm13, rdx


; Between: xmm13 and rbx
movss dword ptr [rbx - 4], xmm13
movsd qword ptr [rbx + 4], xmm13

movss xmm13, dword ptr [rbx - 4]
movsd xmm13, qword ptr [rbx + 4]
cvtsi2ss xmm13, qword ptr [rbx - 4]
cvtsi2sd xmm13, dword ptr [rbx + 4]

cvttss2si ebx, xmm13
cvttsd2si ebx, xmm13

cvtsi2ss xmm13, ebx
cvtsi2sd xmm13, ebx

cvttss2si rbx, xmm13
cvttsd2si rbx, xmm13

cvtsi2ss xmm13, rbx
cvtsi2sd xmm13, rbx


; Between: xmm13 and rsp
movss dword ptr [rsp - 4], xmm13
movsd qword ptr [rsp + 4], xmm13

movss xmm13, dword ptr [rsp - 4]
movsd xmm13, qword ptr [rsp + 4]
cvtsi2ss xmm13, qword ptr [rsp - 4]
cvtsi2sd xmm13, dword ptr [rsp + 4]

cvttss2si esp, xmm13
cvttsd2si esp, xmm13

cvtsi2ss xmm13, esp
cvtsi2sd xmm13, esp

cvttss2si rsp, xmm13
cvttsd2si rsp, xmm13

cvtsi2ss xmm13, rsp
cvtsi2sd xmm13, rsp


; Between: xmm13 and rbp
movss dword ptr [rbp - 4], xmm13
movsd qword ptr [rbp + 4], xmm13

movss xmm13, dword ptr [rbp - 4]
movsd xmm13, qword ptr [rbp + 4]
cvtsi2ss xmm13, qword ptr [rbp - 4]
cvtsi2sd xmm13, dword ptr [rbp + 4]

cvttss2si ebp, xmm13
cvttsd2si ebp, xmm13

cvtsi2ss xmm13, ebp
cvtsi2sd xmm13, ebp

cvttss2si rbp, xmm13
cvttsd2si rbp, xmm13

cvtsi2ss xmm13, rbp
cvtsi2sd xmm13, rbp


; Between: xmm13 and rsi
movss dword ptr [rsi - 4], xmm13
movsd qword ptr [rsi + 4], xmm13

movss xmm13, dword ptr [rsi - 4]
movsd xmm13, qword ptr [rsi + 4]
cvtsi2ss xmm13, qword ptr [rsi - 4]
cvtsi2sd xmm13, dword ptr [rsi + 4]

cvttss2si esi, xmm13
cvttsd2si esi, xmm13

cvtsi2ss xmm13, esi
cvtsi2sd xmm13, esi

cvttss2si rsi, xmm13
cvttsd2si rsi, xmm13

cvtsi2ss xmm13, rsi
cvtsi2sd xmm13, rsi


; Between: xmm13 and rdi
movss dword ptr [rdi - 4], xmm13
movsd qword ptr [rdi + 4], xmm13

movss xmm13, dword ptr [rdi - 4]
movsd xmm13, qword ptr [rdi + 4]
cvtsi2ss xmm13, qword ptr [rdi - 4]
cvtsi2sd xmm13, dword ptr [rdi + 4]

cvttss2si edi, xmm13
cvttsd2si edi, xmm13

cvtsi2ss xmm13, edi
cvtsi2sd xmm13, edi

cvttss2si rdi, xmm13
cvttsd2si rdi, xmm13

cvtsi2ss xmm13, rdi
cvtsi2sd xmm13, rdi


; Between: xmm13 and r8
movss dword ptr [r8 - 4], xmm13
movsd qword ptr [r8 + 4], xmm13

movss xmm13, dword ptr [r8 - 4]
movsd xmm13, qword ptr [r8 + 4]
cvtsi2ss xmm13, qword ptr [r8 - 4]
cvtsi2sd xmm13, dword ptr [r8 + 4]

cvttss2si r8d, xmm13
cvttsd2si r8d, xmm13

cvtsi2ss xmm13, r8d
cvtsi2sd xmm13, r8d

cvttss2si r8, xmm13
cvttsd2si r8, xmm13

cvtsi2ss xmm13, r8
cvtsi2sd xmm13, r8


; Between: xmm13 and r9
movss dword ptr [r9 - 4], xmm13
movsd qword ptr [r9 + 4], xmm13

movss xmm13, dword ptr [r9 - 4]
movsd xmm13, qword ptr [r9 + 4]
cvtsi2ss xmm13, qword ptr [r9 - 4]
cvtsi2sd xmm13, dword ptr [r9 + 4]

cvttss2si r9d, xmm13
cvttsd2si r9d, xmm13

cvtsi2ss xmm13, r9d
cvtsi2sd xmm13, r9d

cvttss2si r9, xmm13
cvttsd2si r9, xmm13

cvtsi2ss xmm13, r9
cvtsi2sd xmm13, r9


; Between: xmm13 and r10
movss dword ptr [r10 - 4], xmm13
movsd qword ptr [r10 + 4], xmm13

movss xmm13, dword ptr [r10 - 4]
movsd xmm13, qword ptr [r10 + 4]
cvtsi2ss xmm13, qword ptr [r10 - 4]
cvtsi2sd xmm13, dword ptr [r10 + 4]

cvttss2si r10d, xmm13
cvttsd2si r10d, xmm13

cvtsi2ss xmm13, r10d
cvtsi2sd xmm13, r10d

cvttss2si r10, xmm13
cvttsd2si r10, xmm13

cvtsi2ss xmm13, r10
cvtsi2sd xmm13, r10


; Between: xmm13 and r11
movss dword ptr [r11 - 4], xmm13
movsd qword ptr [r11 + 4], xmm13

movss xmm13, dword ptr [r11 - 4]
movsd xmm13, qword ptr [r11 + 4]
cvtsi2ss xmm13, qword ptr [r11 - 4]
cvtsi2sd xmm13, dword ptr [r11 + 4]

cvttss2si r11d, xmm13
cvttsd2si r11d, xmm13

cvtsi2ss xmm13, r11d
cvtsi2sd xmm13, r11d

cvttss2si r11, xmm13
cvttsd2si r11, xmm13

cvtsi2ss xmm13, r11
cvtsi2sd xmm13, r11


; Between: xmm13 and r12
movss dword ptr [r12 - 4], xmm13
movsd qword ptr [r12 + 4], xmm13

movss xmm13, dword ptr [r12 - 4]
movsd xmm13, qword ptr [r12 + 4]
cvtsi2ss xmm13, qword ptr [r12 - 4]
cvtsi2sd xmm13, dword ptr [r12 + 4]

cvttss2si r12d, xmm13
cvttsd2si r12d, xmm13

cvtsi2ss xmm13, r12d
cvtsi2sd xmm13, r12d

cvttss2si r12, xmm13
cvttsd2si r12, xmm13

cvtsi2ss xmm13, r12
cvtsi2sd xmm13, r12


; Between: xmm13 and r13
movss dword ptr [r13 - 4], xmm13
movsd qword ptr [r13 + 4], xmm13

movss xmm13, dword ptr [r13 - 4]
movsd xmm13, qword ptr [r13 + 4]
cvtsi2ss xmm13, qword ptr [r13 - 4]
cvtsi2sd xmm13, dword ptr [r13 + 4]

cvttss2si r13d, xmm13
cvttsd2si r13d, xmm13

cvtsi2ss xmm13, r13d
cvtsi2sd xmm13, r13d

cvttss2si r13, xmm13
cvttsd2si r13, xmm13

cvtsi2ss xmm13, r13
cvtsi2sd xmm13, r13


; Between: xmm13 and r14
movss dword ptr [r14 - 4], xmm13
movsd qword ptr [r14 + 4], xmm13

movss xmm13, dword ptr [r14 - 4]
movsd xmm13, qword ptr [r14 + 4]
cvtsi2ss xmm13, qword ptr [r14 - 4]
cvtsi2sd xmm13, dword ptr [r14 + 4]

cvttss2si r14d, xmm13
cvttsd2si r14d, xmm13

cvtsi2ss xmm13, r14d
cvtsi2sd xmm13, r14d

cvttss2si r14, xmm13
cvttsd2si r14, xmm13

cvtsi2ss xmm13, r14
cvtsi2sd xmm13, r14


; Between: xmm13 and r15
movss dword ptr [r15 - 4], xmm13
movsd qword ptr [r15 + 4], xmm13

movss xmm13, dword ptr [r15 - 4]
movsd xmm13, qword ptr [r15 + 4]
cvtsi2ss xmm13, qword ptr [r15 - 4]
cvtsi2sd xmm13, dword ptr [r15 + 4]

cvttss2si r15d, xmm13
cvttsd2si r15d, xmm13

cvtsi2ss xmm13, r15d
cvtsi2sd xmm13, r15d

cvttss2si r15, xmm13
cvttsd2si r15, xmm13

cvtsi2ss xmm13, r15
cvtsi2sd xmm13, r15


; Between: xmm14 and rax
movss dword ptr [rax - 4], xmm14
movsd qword ptr [rax + 4], xmm14

movss xmm14, dword ptr [rax - 4]
movsd xmm14, qword ptr [rax + 4]
cvtsi2ss xmm14, qword ptr [rax - 4]
cvtsi2sd xmm14, dword ptr [rax + 4]

cvttss2si eax, xmm14
cvttsd2si eax, xmm14

cvtsi2ss xmm14, eax
cvtsi2sd xmm14, eax

cvttss2si rax, xmm14
cvttsd2si rax, xmm14

cvtsi2ss xmm14, rax
cvtsi2sd xmm14, rax


; Between: xmm14 and rcx
movss dword ptr [rcx - 4], xmm14
movsd qword ptr [rcx + 4], xmm14

movss xmm14, dword ptr [rcx - 4]
movsd xmm14, qword ptr [rcx + 4]
cvtsi2ss xmm14, qword ptr [rcx - 4]
cvtsi2sd xmm14, dword ptr [rcx + 4]

cvttss2si ecx, xmm14
cvttsd2si ecx, xmm14

cvtsi2ss xmm14, ecx
cvtsi2sd xmm14, ecx

cvttss2si rcx, xmm14
cvttsd2si rcx, xmm14

cvtsi2ss xmm14, rcx
cvtsi2sd xmm14, rcx


; Between: xmm14 and rdx
movss dword ptr [rdx - 4], xmm14
movsd qword ptr [rdx + 4], xmm14

movss xmm14, dword ptr [rdx - 4]
movsd xmm14, qword ptr [rdx + 4]
cvtsi2ss xmm14, qword ptr [rdx - 4]
cvtsi2sd xmm14, dword ptr [rdx + 4]

cvttss2si edx, xmm14
cvttsd2si edx, xmm14

cvtsi2ss xmm14, edx
cvtsi2sd xmm14, edx

cvttss2si rdx, xmm14
cvttsd2si rdx, xmm14

cvtsi2ss xmm14, rdx
cvtsi2sd xmm14, rdx


; Between: xmm14 and rbx
movss dword ptr [rbx - 4], xmm14
movsd qword ptr [rbx + 4], xmm14

movss xmm14, dword ptr [rbx - 4]
movsd xmm14, qword ptr [rbx + 4]
cvtsi2ss xmm14, qword ptr [rbx - 4]
cvtsi2sd xmm14, dword ptr [rbx + 4]

cvttss2si ebx, xmm14
cvttsd2si ebx, xmm14

cvtsi2ss xmm14, ebx
cvtsi2sd xmm14, ebx

cvttss2si rbx, xmm14
cvttsd2si rbx, xmm14

cvtsi2ss xmm14, rbx
cvtsi2sd xmm14, rbx


; Between: xmm14 and rsp
movss dword ptr [rsp - 4], xmm14
movsd qword ptr [rsp + 4], xmm14

movss xmm14, dword ptr [rsp - 4]
movsd xmm14, qword ptr [rsp + 4]
cvtsi2ss xmm14, qword ptr [rsp - 4]
cvtsi2sd xmm14, dword ptr [rsp + 4]

cvttss2si esp, xmm14
cvttsd2si esp, xmm14

cvtsi2ss xmm14, esp
cvtsi2sd xmm14, esp

cvttss2si rsp, xmm14
cvttsd2si rsp, xmm14

cvtsi2ss xmm14, rsp
cvtsi2sd xmm14, rsp


; Between: xmm14 and rbp
movss dword ptr [rbp - 4], xmm14
movsd qword ptr [rbp + 4], xmm14

movss xmm14, dword ptr [rbp - 4]
movsd xmm14, qword ptr [rbp + 4]
cvtsi2ss xmm14, qword ptr [rbp - 4]
cvtsi2sd xmm14, dword ptr [rbp + 4]

cvttss2si ebp, xmm14
cvttsd2si ebp, xmm14

cvtsi2ss xmm14, ebp
cvtsi2sd xmm14, ebp

cvttss2si rbp, xmm14
cvttsd2si rbp, xmm14

cvtsi2ss xmm14, rbp
cvtsi2sd xmm14, rbp


; Between: xmm14 and rsi
movss dword ptr [rsi - 4], xmm14
movsd qword ptr [rsi + 4], xmm14

movss xmm14, dword ptr [rsi - 4]
movsd xmm14, qword ptr [rsi + 4]
cvtsi2ss xmm14, qword ptr [rsi - 4]
cvtsi2sd xmm14, dword ptr [rsi + 4]

cvttss2si esi, xmm14
cvttsd2si esi, xmm14

cvtsi2ss xmm14, esi
cvtsi2sd xmm14, esi

cvttss2si rsi, xmm14
cvttsd2si rsi, xmm14

cvtsi2ss xmm14, rsi
cvtsi2sd xmm14, rsi


; Between: xmm14 and rdi
movss dword ptr [rdi - 4], xmm14
movsd qword ptr [rdi + 4], xmm14

movss xmm14, dword ptr [rdi - 4]
movsd xmm14, qword ptr [rdi + 4]
cvtsi2ss xmm14, qword ptr [rdi - 4]
cvtsi2sd xmm14, dword ptr [rdi + 4]

cvttss2si edi, xmm14
cvttsd2si edi, xmm14

cvtsi2ss xmm14, edi
cvtsi2sd xmm14, edi

cvttss2si rdi, xmm14
cvttsd2si rdi, xmm14

cvtsi2ss xmm14, rdi
cvtsi2sd xmm14, rdi


; Between: xmm14 and r8
movss dword ptr [r8 - 4], xmm14
movsd qword ptr [r8 + 4], xmm14

movss xmm14, dword ptr [r8 - 4]
movsd xmm14, qword ptr [r8 + 4]
cvtsi2ss xmm14, qword ptr [r8 - 4]
cvtsi2sd xmm14, dword ptr [r8 + 4]

cvttss2si r8d, xmm14
cvttsd2si r8d, xmm14

cvtsi2ss xmm14, r8d
cvtsi2sd xmm14, r8d

cvttss2si r8, xmm14
cvttsd2si r8, xmm14

cvtsi2ss xmm14, r8
cvtsi2sd xmm14, r8


; Between: xmm14 and r9
movss dword ptr [r9 - 4], xmm14
movsd qword ptr [r9 + 4], xmm14

movss xmm14, dword ptr [r9 - 4]
movsd xmm14, qword ptr [r9 + 4]
cvtsi2ss xmm14, qword ptr [r9 - 4]
cvtsi2sd xmm14, dword ptr [r9 + 4]

cvttss2si r9d, xmm14
cvttsd2si r9d, xmm14

cvtsi2ss xmm14, r9d
cvtsi2sd xmm14, r9d

cvttss2si r9, xmm14
cvttsd2si r9, xmm14

cvtsi2ss xmm14, r9
cvtsi2sd xmm14, r9


; Between: xmm14 and r10
movss dword ptr [r10 - 4], xmm14
movsd qword ptr [r10 + 4], xmm14

movss xmm14, dword ptr [r10 - 4]
movsd xmm14, qword ptr [r10 + 4]
cvtsi2ss xmm14, qword ptr [r10 - 4]
cvtsi2sd xmm14, dword ptr [r10 + 4]

cvttss2si r10d, xmm14
cvttsd2si r10d, xmm14

cvtsi2ss xmm14, r10d
cvtsi2sd xmm14, r10d

cvttss2si r10, xmm14
cvttsd2si r10, xmm14

cvtsi2ss xmm14, r10
cvtsi2sd xmm14, r10


; Between: xmm14 and r11
movss dword ptr [r11 - 4], xmm14
movsd qword ptr [r11 + 4], xmm14

movss xmm14, dword ptr [r11 - 4]
movsd xmm14, qword ptr [r11 + 4]
cvtsi2ss xmm14, qword ptr [r11 - 4]
cvtsi2sd xmm14, dword ptr [r11 + 4]

cvttss2si r11d, xmm14
cvttsd2si r11d, xmm14

cvtsi2ss xmm14, r11d
cvtsi2sd xmm14, r11d

cvttss2si r11, xmm14
cvttsd2si r11, xmm14

cvtsi2ss xmm14, r11
cvtsi2sd xmm14, r11


; Between: xmm14 and r12
movss dword ptr [r12 - 4], xmm14
movsd qword ptr [r12 + 4], xmm14

movss xmm14, dword ptr [r12 - 4]
movsd xmm14, qword ptr [r12 + 4]
cvtsi2ss xmm14, qword ptr [r12 - 4]
cvtsi2sd xmm14, dword ptr [r12 + 4]

cvttss2si r12d, xmm14
cvttsd2si r12d, xmm14

cvtsi2ss xmm14, r12d
cvtsi2sd xmm14, r12d

cvttss2si r12, xmm14
cvttsd2si r12, xmm14

cvtsi2ss xmm14, r12
cvtsi2sd xmm14, r12


; Between: xmm14 and r13
movss dword ptr [r13 - 4], xmm14
movsd qword ptr [r13 + 4], xmm14

movss xmm14, dword ptr [r13 - 4]
movsd xmm14, qword ptr [r13 + 4]
cvtsi2ss xmm14, qword ptr [r13 - 4]
cvtsi2sd xmm14, dword ptr [r13 + 4]

cvttss2si r13d, xmm14
cvttsd2si r13d, xmm14

cvtsi2ss xmm14, r13d
cvtsi2sd xmm14, r13d

cvttss2si r13, xmm14
cvttsd2si r13, xmm14

cvtsi2ss xmm14, r13
cvtsi2sd xmm14, r13


; Between: xmm14 and r14
movss dword ptr [r14 - 4], xmm14
movsd qword ptr [r14 + 4], xmm14

movss xmm14, dword ptr [r14 - 4]
movsd xmm14, qword ptr [r14 + 4]
cvtsi2ss xmm14, qword ptr [r14 - 4]
cvtsi2sd xmm14, dword ptr [r14 + 4]

cvttss2si r14d, xmm14
cvttsd2si r14d, xmm14

cvtsi2ss xmm14, r14d
cvtsi2sd xmm14, r14d

cvttss2si r14, xmm14
cvttsd2si r14, xmm14

cvtsi2ss xmm14, r14
cvtsi2sd xmm14, r14


; Between: xmm14 and r15
movss dword ptr [r15 - 4], xmm14
movsd qword ptr [r15 + 4], xmm14

movss xmm14, dword ptr [r15 - 4]
movsd xmm14, qword ptr [r15 + 4]
cvtsi2ss xmm14, qword ptr [r15 - 4]
cvtsi2sd xmm14, dword ptr [r15 + 4]

cvttss2si r15d, xmm14
cvttsd2si r15d, xmm14

cvtsi2ss xmm14, r15d
cvtsi2sd xmm14, r15d

cvttss2si r15, xmm14
cvttsd2si r15, xmm14

cvtsi2ss xmm14, r15
cvtsi2sd xmm14, r15


; Between: xmm15 and rax
movss dword ptr [rax - 4], xmm15
movsd qword ptr [rax + 4], xmm15

movss xmm15, dword ptr [rax - 4]
movsd xmm15, qword ptr [rax + 4]
cvtsi2ss xmm15, qword ptr [rax - 4]
cvtsi2sd xmm15, dword ptr [rax + 4]

cvttss2si eax, xmm15
cvttsd2si eax, xmm15

cvtsi2ss xmm15, eax
cvtsi2sd xmm15, eax

cvttss2si rax, xmm15
cvttsd2si rax, xmm15

cvtsi2ss xmm15, rax
cvtsi2sd xmm15, rax


; Between: xmm15 and rcx
movss dword ptr [rcx - 4], xmm15
movsd qword ptr [rcx + 4], xmm15

movss xmm15, dword ptr [rcx - 4]
movsd xmm15, qword ptr [rcx + 4]
cvtsi2ss xmm15, qword ptr [rcx - 4]
cvtsi2sd xmm15, dword ptr [rcx + 4]

cvttss2si ecx, xmm15
cvttsd2si ecx, xmm15

cvtsi2ss xmm15, ecx
cvtsi2sd xmm15, ecx

cvttss2si rcx, xmm15
cvttsd2si rcx, xmm15

cvtsi2ss xmm15, rcx
cvtsi2sd xmm15, rcx


; Between: xmm15 and rdx
movss dword ptr [rdx - 4], xmm15
movsd qword ptr [rdx + 4], xmm15

movss xmm15, dword ptr [rdx - 4]
movsd xmm15, qword ptr [rdx + 4]
cvtsi2ss xmm15, qword ptr [rdx - 4]
cvtsi2sd xmm15, dword ptr [rdx + 4]

cvttss2si edx, xmm15
cvttsd2si edx, xmm15

cvtsi2ss xmm15, edx
cvtsi2sd xmm15, edx

cvttss2si rdx, xmm15
cvttsd2si rdx, xmm15

cvtsi2ss xmm15, rdx
cvtsi2sd xmm15, rdx


; Between: xmm15 and rbx
movss dword ptr [rbx - 4], xmm15
movsd qword ptr [rbx + 4], xmm15

movss xmm15, dword ptr [rbx - 4]
movsd xmm15, qword ptr [rbx + 4]
cvtsi2ss xmm15, qword ptr [rbx - 4]
cvtsi2sd xmm15, dword ptr [rbx + 4]

cvttss2si ebx, xmm15
cvttsd2si ebx, xmm15

cvtsi2ss xmm15, ebx
cvtsi2sd xmm15, ebx

cvttss2si rbx, xmm15
cvttsd2si rbx, xmm15

cvtsi2ss xmm15, rbx
cvtsi2sd xmm15, rbx


; Between: xmm15 and rsp
movss dword ptr [rsp - 4], xmm15
movsd qword ptr [rsp + 4], xmm15

movss xmm15, dword ptr [rsp - 4]
movsd xmm15, qword ptr [rsp + 4]
cvtsi2ss xmm15, qword ptr [rsp - 4]
cvtsi2sd xmm15, dword ptr [rsp + 4]

cvttss2si esp, xmm15
cvttsd2si esp, xmm15

cvtsi2ss xmm15, esp
cvtsi2sd xmm15, esp

cvttss2si rsp, xmm15
cvttsd2si rsp, xmm15

cvtsi2ss xmm15, rsp
cvtsi2sd xmm15, rsp


; Between: xmm15 and rbp
movss dword ptr [rbp - 4], xmm15
movsd qword ptr [rbp + 4], xmm15

movss xmm15, dword ptr [rbp - 4]
movsd xmm15, qword ptr [rbp + 4]
cvtsi2ss xmm15, qword ptr [rbp - 4]
cvtsi2sd xmm15, dword ptr [rbp + 4]

cvttss2si ebp, xmm15
cvttsd2si ebp, xmm15

cvtsi2ss xmm15, ebp
cvtsi2sd xmm15, ebp

cvttss2si rbp, xmm15
cvttsd2si rbp, xmm15

cvtsi2ss xmm15, rbp
cvtsi2sd xmm15, rbp


; Between: xmm15 and rsi
movss dword ptr [rsi - 4], xmm15
movsd qword ptr [rsi + 4], xmm15

movss xmm15, dword ptr [rsi - 4]
movsd xmm15, qword ptr [rsi + 4]
cvtsi2ss xmm15, qword ptr [rsi - 4]
cvtsi2sd xmm15, dword ptr [rsi + 4]

cvttss2si esi, xmm15
cvttsd2si esi, xmm15

cvtsi2ss xmm15, esi
cvtsi2sd xmm15, esi

cvttss2si rsi, xmm15
cvttsd2si rsi, xmm15

cvtsi2ss xmm15, rsi
cvtsi2sd xmm15, rsi


; Between: xmm15 and rdi
movss dword ptr [rdi - 4], xmm15
movsd qword ptr [rdi + 4], xmm15

movss xmm15, dword ptr [rdi - 4]
movsd xmm15, qword ptr [rdi + 4]
cvtsi2ss xmm15, qword ptr [rdi - 4]
cvtsi2sd xmm15, dword ptr [rdi + 4]

cvttss2si edi, xmm15
cvttsd2si edi, xmm15

cvtsi2ss xmm15, edi
cvtsi2sd xmm15, edi

cvttss2si rdi, xmm15
cvttsd2si rdi, xmm15

cvtsi2ss xmm15, rdi
cvtsi2sd xmm15, rdi


; Between: xmm15 and r8
movss dword ptr [r8 - 4], xmm15
movsd qword ptr [r8 + 4], xmm15

movss xmm15, dword ptr [r8 - 4]
movsd xmm15, qword ptr [r8 + 4]
cvtsi2ss xmm15, qword ptr [r8 - 4]
cvtsi2sd xmm15, dword ptr [r8 + 4]

cvttss2si r8d, xmm15
cvttsd2si r8d, xmm15

cvtsi2ss xmm15, r8d
cvtsi2sd xmm15, r8d

cvttss2si r8, xmm15
cvttsd2si r8, xmm15

cvtsi2ss xmm15, r8
cvtsi2sd xmm15, r8


; Between: xmm15 and r9
movss dword ptr [r9 - 4], xmm15
movsd qword ptr [r9 + 4], xmm15

movss xmm15, dword ptr [r9 - 4]
movsd xmm15, qword ptr [r9 + 4]
cvtsi2ss xmm15, qword ptr [r9 - 4]
cvtsi2sd xmm15, dword ptr [r9 + 4]

cvttss2si r9d, xmm15
cvttsd2si r9d, xmm15

cvtsi2ss xmm15, r9d
cvtsi2sd xmm15, r9d

cvttss2si r9, xmm15
cvttsd2si r9, xmm15

cvtsi2ss xmm15, r9
cvtsi2sd xmm15, r9


; Between: xmm15 and r10
movss dword ptr [r10 - 4], xmm15
movsd qword ptr [r10 + 4], xmm15

movss xmm15, dword ptr [r10 - 4]
movsd xmm15, qword ptr [r10 + 4]
cvtsi2ss xmm15, qword ptr [r10 - 4]
cvtsi2sd xmm15, dword ptr [r10 + 4]

cvttss2si r10d, xmm15
cvttsd2si r10d, xmm15

cvtsi2ss xmm15, r10d
cvtsi2sd xmm15, r10d

cvttss2si r10, xmm15
cvttsd2si r10, xmm15

cvtsi2ss xmm15, r10
cvtsi2sd xmm15, r10


; Between: xmm15 and r11
movss dword ptr [r11 - 4], xmm15
movsd qword ptr [r11 + 4], xmm15

movss xmm15, dword ptr [r11 - 4]
movsd xmm15, qword ptr [r11 + 4]
cvtsi2ss xmm15, qword ptr [r11 - 4]
cvtsi2sd xmm15, dword ptr [r11 + 4]

cvttss2si r11d, xmm15
cvttsd2si r11d, xmm15

cvtsi2ss xmm15, r11d
cvtsi2sd xmm15, r11d

cvttss2si r11, xmm15
cvttsd2si r11, xmm15

cvtsi2ss xmm15, r11
cvtsi2sd xmm15, r11


; Between: xmm15 and r12
movss dword ptr [r12 - 4], xmm15
movsd qword ptr [r12 + 4], xmm15

movss xmm15, dword ptr [r12 - 4]
movsd xmm15, qword ptr [r12 + 4]
cvtsi2ss xmm15, qword ptr [r12 - 4]
cvtsi2sd xmm15, dword ptr [r12 + 4]

cvttss2si r12d, xmm15
cvttsd2si r12d, xmm15

cvtsi2ss xmm15, r12d
cvtsi2sd xmm15, r12d

cvttss2si r12, xmm15
cvttsd2si r12, xmm15

cvtsi2ss xmm15, r12
cvtsi2sd xmm15, r12


; Between: xmm15 and r13
movss dword ptr [r13 - 4], xmm15
movsd qword ptr [r13 + 4], xmm15

movss xmm15, dword ptr [r13 - 4]
movsd xmm15, qword ptr [r13 + 4]
cvtsi2ss xmm15, qword ptr [r13 - 4]
cvtsi2sd xmm15, dword ptr [r13 + 4]

cvttss2si r13d, xmm15
cvttsd2si r13d, xmm15

cvtsi2ss xmm15, r13d
cvtsi2sd xmm15, r13d

cvttss2si r13, xmm15
cvttsd2si r13, xmm15

cvtsi2ss xmm15, r13
cvtsi2sd xmm15, r13


; Between: xmm15 and r14
movss dword ptr [r14 - 4], xmm15
movsd qword ptr [r14 + 4], xmm15

movss xmm15, dword ptr [r14 - 4]
movsd xmm15, qword ptr [r14 + 4]
cvtsi2ss xmm15, qword ptr [r14 - 4]
cvtsi2sd xmm15, dword ptr [r14 + 4]

cvttss2si r14d, xmm15
cvttsd2si r14d, xmm15

cvtsi2ss xmm15, r14d
cvtsi2sd xmm15, r14d

cvttss2si r14, xmm15
cvttsd2si r14, xmm15

cvtsi2ss xmm15, r14
cvtsi2sd xmm15, r14


; Between: xmm15 and r15
movss dword ptr [r15 - 4], xmm15
movsd qword ptr [r15 + 4], xmm15

movss xmm15, dword ptr [r15 - 4]
movsd xmm15, qword ptr [r15 + 4]
cvtsi2ss xmm15, qword ptr [r15 - 4]
cvtsi2sd xmm15, dword ptr [r15 + 4]

cvttss2si r15d, xmm15
cvttsd2si r15d, xmm15

cvtsi2ss xmm15, r15d
cvtsi2sd xmm15, r15d

cvttss2si r15, xmm15
cvttsd2si r15, xmm15

cvtsi2ss xmm15, r15
cvtsi2sd xmm15, r15


instructions ENDP

END
