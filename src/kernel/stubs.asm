;PSOS Development Build
;https://github.com/TheBenPerson/PSOS/tree/dev

;Copyright (C) 2016 - 2017 Ben Stockett <thebenstockett@gmail.com>

;Permission is hereby granted, free of charge, to any person obtaining a copy
;of this software and associated documentation files (the "Software"), to deal
;in the Software without restriction, including without limitation the rights
;to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
;copies of the Software, and to permit persons to whom the Software is
;furnished to do so, subject to the following conditions:

;The above copyright notice and this permission notice shall be included in all
;copies or substantial portions of the Software.

;THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
;IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
;FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
;AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
;LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
;OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
;SOFTWARE.

[BITS 16]

extern brkpt
global brkptISR

brkptISR: ;int3 handler

	call dword brkpt

iret

extern syscall
extern sysret
global syscallISR

syscallISR: ;syscall handler

	pop dword [sysret]
	pop dword [sysret + 4]
	pop dword [sysret + 8]
	pop word [sysret + 12]

	call dword syscall

	push word [sysret + 12]
	push dword [sysret + 8]
	push dword [sysret + 4]
	push dword [sysret]

iret