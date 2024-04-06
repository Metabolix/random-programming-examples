; 27.10.2008
ORG 100h
BITS 16

SECTION .text

main:
	; laitetaan nollatavut tiedostonimien loppuun
	xor al, al
	mov [txt_file_end], al
	mov [bin_file_end], al

	; avataan lähdetiedosto
	mov ah, 0x3d     ; ax, 0x3d00
	mov dx, txt_file
	int 0x21
	mov si, ax

	; avataan (luodaan) kohdetiedosto
	mov ah, 0x3c     ; ax, 0x3c00
	xor cx, cx
	mov dx, bin_file
	int 0x21
	mov di, ax

	; käytetään tiedostonimen paikkaa apumuistina
	mov dx, txt_file
	inc cx
	.loop:
		; luetaan kaksi tavua (huomaa cx:n muutos)
		mov ah, 0x3f
		mov bx, si
		inc cx
		int 0x21
		dec cx

		; jos lukeminen epäonnistui, lopetetaan
		test ax, ax
		jz .loop_end

		; haetaan luetut tavut
		mov ax, [txt_file]
		; vähennetään molemmista merkin 'a' ASCII-arvo
		sub ax, 0x6161
		; yhdistetään tavut (0a 0b => ab)
		shl al, 4
		shr ax, 4
		; laitetaan tulos muistiin
		mov [txt_file], al
		xor ah, ah ; turha rivi

		; kirjoitetaan yksi tavu (cx = 1)
		mov ah, 0x40
		mov bx, di
		int 0x21
		jmp .loop
	.loop_end:

	; suljetaan lähdetiedosto
	mov ah, 0x3e
	mov bx, si
	int 0x21

	; suljetaan kohdetiedosto
	mov ah, 0x3e
	mov bx, di
	int 0x21

	; lopetetaan ohjelma
	int 0x20
	nop

SECTION .data

txt_file:
	db "prog.txt"
txt_file_end:
	db '$' ; $:t muutetaan ohjelman aikana nollatavuiksi
bin_file:
	db "prog.exe"
bin_file_end:
	db '$'
