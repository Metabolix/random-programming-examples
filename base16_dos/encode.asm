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
	mov dx, bin_file
	int 0x21
	mov si, ax

	; avataan (luodaan) kohdetiedosto
	mov ah, 0x3c     ; ax, 0x3c00
	xor cx, cx
	mov dx, txt_file
	int 0x21
	mov di, ax

	; käytetään tiedostonimen paikkaa apumuistina
	mov dx, txt_file
	inc cx
	.loop:
		; luetaan yksi tavu (cx = 1)
		mov ah, 0x3f
		mov bx, si
		int 0x21

		; jos lukeminen epäonnistui, lopetetaan
		test ax, ax
		jz .loop_end

		; haetaan luettu tavu
		mov al, [txt_file]
		; hajotetaan tavu kahdeksi (ab => 0a 0b)
		xor ah, ah
		shl ax, 4
		shr al, 4
		; lisätään molempiin merkin 'a' ASCII-arvo
		add ax, 0x6161
		; laitetaan tulos muistiin
		mov [txt_file], ax

		; kirjoitetaan kaksi tavua (huomaa cx:n muutos)
		mov ah, 0x40
		mov bx, di
		inc cx
		int 0x21
		dec cx
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
