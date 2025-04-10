IDEAL
MODEL LARGE,C
P8086

DATASEG
	modeSet DB 0
	scanLines DW 200 DUP(?)
ENDS
CODESEG

PROC IsInGraphicsMode
	PUBLIC IsInGraphicsMode

	XOR AX, AX
	MOV AL, [modeSet]
	RET
ENDP

PROC SetTandyMediumMode
	PUBLIC SetTandyMediumMode
	USES BX,CX,DX,ES,DI,BP

	; In case we've already set the mode, do nothing and
	; return an error.
	CMP [modeSet], 1
	JE BADNEWS

	MOV AX,0ffffh
	MOV ES,AX

	MOV DI, 0eh
	CMP [BYTE PTR ES:DI],0fdh
	; No PC-Jr support here. Fuck that machine.
	JE BADNEWS

	CMP [BYTE PTR ES:DI], 0ffh
	JNE BADNEWS

	MOV AX,0FC00h
	MOV ES,AX
	XOR DI, DI
	CMP [BYTE PTR ES:DI], 21h
	JNE BADNEWS

; Build a list of scanline offsets for the 320x200 16 colour mode.
	MOV CX, 200
	MOV AX, @DATA
	MOV ES, AX
	LEA DI, [scanLines]
	XOR DX, DX
NEXTSCAN:

	XOR AX,AX
	MOV BX,DX

	SHR BX,1
	RCR AX,1
	SHR BX,1
	RCR AX,1
	SHR AX,1

	; SHL BX, 5
	SHL BX,1
	SHL BX,1
	SHL BX,1
	SHL BX,1
	SHL BX,1

	ADD AX,BX	; yoffset << 5

	; SHL BX, 2
	SHL BX,1
	SHL BX,1

	ADD AX,BX	; yoffset << 7

	STOSW

	INC DX
	LOOP NEXTSCAN

	; Tandy 320x200 16 Colour mode.
	MOV AX, 9h
	INT 10h

	MOV [modeSet], 1
	MOV AX, 1
	JMP MODEDONE

BADNEWS:
	XOR AX, AX
MODEDONE:
	RET
ENDP

PROC SetTextMode
	PUBLIC SetTextMode

	CMP [modeSet], 0
	JE TEXTMODEDONE

	MOV AX, 3h
	INT 10h
	MOV [modeSet], 0

TEXTMODEDONE:
	RET
ENDP

PROC Cls FAR
	PUBLIC Cls
	ARG Buffer:DWORD, Color:BYTE
	USES ES,DI,AX,BX,CX,BP

	LES DI, [Buffer]
	MOV AL, [Color]
	MOV BL, AL

	; SHL AL,4
	SHL AL, 1
	SHL AL, 1
	SHL AL, 1
	SHL AL, 1

	OR AL,BL

	MOV AH, AL
	MOV CX, 4000h
	REP STOSW
	RET
ENDP

PROC CopyPage FAR
	PUBLIC CopyPage
	ARG SRC:DWORD, DEST:DWORD
	USES ES,DI,DS,SI,CX,BP

	LDS SI, [SRC]
	LES DI, [DEST]
	MOV CX, 4000h
	REP MOVSW

	RET
ENDP

PROC Pset FAR
	PUBLIC Pset
	ARG buffer:DWORD, x:WORD, y:WORD, color:BYTE
	USES DS,SI,ES,DI,BX,CX,DX,BP

	MOV DX, [y]
	MOV CX, [x]

	CMP CX, 0
	JL NOPIXEL

	CMP CX, 319
	JNLE NOPIXEL

	CMP DX, 0
	JL NOPIXEL

	CMP DX, 199
	JNLE NOPIXEL

; Load our scanline look up and retrieve the bank offset
; for the y coordinate.
	MOV AX,SEG scanLines
	MOV DS,AX
	LEA SI,[scanLines]
	SHL DX,1							 	; DX * 2 because we're using word values.
	ADD SI,DX

; Add the bank to the buffer offset.
	LES DI,[buffer]
	ADD DI,[WORD PTR DS:SI]

	SHR CX,1
	LAHF			; Remember carry flag for odd/even pixel.
	ADD DI,CX	; xoffset >> 1

	XOR BX,BX
	MOV BL, [color]
	MOV AL, [ES:DI]

	SAHF
	JNC ISODD
	AND AL, 0f0h
	OR AL,BL
	JMP PLOTPIXEL
ISODD:
	AND AL, 0fh

	; SHL BL,4
	SHL BL, 1
	SHL BL, 1
	SHL BL, 1
	SHL BL, 1

	OR AL, BL
PLOTPIXEL:
	STOSB
NOPIXEL:
	RET
ENDP

END