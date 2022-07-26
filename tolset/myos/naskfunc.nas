; naskfunc
; TAB=4

[FORMAT "WCOFF"]		; format of object		
[INSTRSET "i486p"]		; use 486 command
[BITS 32]				; 32 mechine code


[FILE "naskfunc.nas"]	; file name

		GLOBAL  _write_mem8
		GLOBAL	_io_hlt, _io_cli, _io_sti, _io_stihlt
		GLOBAL 	_io_in8, _io_in16, _io_in32
		GLOBAL  _io_out8, _io_out16, _io_out32
		GLOBAL  _io_load_eflags, _io_store_eflags
		GLOBAL 	_load_gdtr, _load_idtr
		GLOBAL	_asm_inthandler20, _asm_inthandler21, _asm_inthandler27, _asm_inthandler2c
		GLOBAL  _load_cr0, _store_cr0
		GLOBAL  _memtest_sub
		EXTERN  _inthandler20, _inthandler21, _inthandler27, _inthandler2c


[SECTION .text]		

_write_mem8:	; void write_mem8(int addr, int data)
	MOV ECX, [ESP+4]
	MOV AL, [ESP+8]
	MOV [ECX], AL
	RET

_io_hlt:	; void io_hlt(void)
		HLT
		RET

_io_cli:	; void io_cli(void)
	CLI
	RET

_io_sti:	; void io_sti(void)
	STI
	RET

_io_stihlt:		; void io_stihlt(void)
	STI
	HLT
	RET

_io_in8:		; int io_in8(int port)
	MOV EDX, [ESP+4]
	MOV EAX, 0
	IN AL, DX
	RET

_io_in16:		; int io_in16(int port)
	MOV EDX, [ESP+4]
	MOV EAX, 0
	IN AX, DX
	RET

_io_in32:		; int io_in32(int port)
	MOV EDX, [ESP+4]
	IN EAX, DX
	RET

_io_out8:		; void io_out8(int port, int data)
	MOV EDX, [ESP+4]
	MOV AL, [ESP+8]
	OUT DX, AL
	RET

_io_out16:		; void io_out16(int port, int data)
	MOV EDX, [ESP+4]
	MOV EAX, [ESP+8]
	OUT DX, AX
	RET

_io_out32:		; void io_out32(int port, int data)
	MOV EDX, [ESP+4]
	MOV EAX, [ESP+8]
	OUT DX, EAX
	RET

_io_load_eflags:	; int load_eflags()
	PUSHFD
	POP EAX
	RET

_io_store_eflags:	; void io_store_eflags(int eflags)
	MOV EAX, [ESP+4]
	PUSH EAX
	POPFD
	RET

_load_gdtr:		; void load_gdtr(int limit, int addr)
	MOV AX, [ESP + 4]
	MOV [ESP + 6], AX
	LGDT [ESP+6]
	RET

_load_idtr:		; void load_idtr(int limit, int addr)
	MOV AX, [ESP + 4]
	MOV [ESP + 6], AX
	LIDT [ESP+6]
	RET

_asm_inthandler20:	; void inthandler20(int *esp)
	PUSH ES
	PUSH DS
	PUSHAD
	MOV EAX, ESP
	PUSH EAX
	MOV AX, SS
	MOV DS, AX
	MOV ES, AX
	CALL _inthandler20
	POP EAX
	POPAD
	POP DS
	POP ES
	IRETD

_asm_inthandler21:	; void inthandler21(int *esp)
	PUSH ES
	PUSH DS
	PUSHAD
	MOV EAX, ESP
	PUSH EAX
	MOV AX, SS
	MOV DS, AX
	MOV ES, AX
	CALL _inthandler21
	POP EAX
	POPAD
	POP DS
	POP ES
	IRETD

_asm_inthandler27:	; void inthandler27(int *esp)
	PUSH ES
	PUSH DS
	PUSHAD
	MOV EAX, ESP
	PUSH EAX
	MOV AX, SS
	MOV DS, AX
	MOV ES, AX
	CALL _inthandler27
	POP EAX
	POPAD
	POP DS
	POP ES
	IRETD

_asm_inthandler2c:	; void inthandler2c(int *esp)
	PUSH ES
	PUSH DS
	PUSHAD
	MOV EAX, ESP
	PUSH EAX
	MOV AX, SS
	MOV DS, AX
	MOV ES, AX
	CALL _inthandler2c
	POP EAX
	POPAD
	POP DS
	POP ES
	IRETD

_load_cr0:	; int load_cr0(void)
	MOV EAX, CR0
	RET

_store_cr0:	; void store_cr0(int cr0)
	MOV EAX, [ESP+4]
	MOV CR0, EAX
	RET

_memtest_sub:	; unsigned int memtest_sub(unsigned int start, unsigned int end)
	PUSH EDI
	PUSH ESI
	PUSH EBX
	MOV ESI, 0xaa55aa55
	MOV EDI, 0x55aa55aa
	MOV EAX, [ESP+12+4] ; EDI, ESI, EBX = 3 * 4, return address = 4, so 3 * 4 + 4
mts_loop:
	MOV EBX, EAX
	ADD EBX, 0xffc
	MOV EDX, [EBX]
	MOV [EBX], ESI
	XOR DWORD [EBX], 0xffffffff
	CMP [EBX], EDI
	JNE mts_fin
	XOR DWORD [EBX], 0xffffffff
	CMP [EBX], ESI
	JNE mts_fin
	MOV [EBX], EDX
	ADD EAX, 0x1000
	CMP EAX,[ESP+12+8]
	JBE mts_loop
	POP EBX
	POP ESI
	POP EDI
	RET
mts_fin:
	MOV [EBX], EDX
	POP EBX
	POP ESI
	POP EDI
	RET