; ============================================================
; 만든이: KimTaeWoo
; 도와준이: Lee.J.H
;  numfmt.inc - 숫자를 글자로 바꾸는 공용부
;  dec3    : A = 0..255 -> DE 가 가리키는 곳에 3자리(앞을 0으로 채움) + 0 을 쓴다.  DE 는 끝(0)을 가리키게 된다
;  dec2    : A = 0..99  -> 2자리
;  dec_trim: A = 0..255 -> numbuf 에 앞의 0 을 뺀 글자, HL = 그 주소
;  pan_text: A = 팬 값 0..127 -> numbuf 에 "C" / "L20" / "R30", HL = 그 주소
; ============================================================
; numbuf 는 bss.inc

dec2:   ld      c,a
        ld      b,10
        call    dg_one
        jr      dg_last

dec3:   ld      c,a
        ld      b,100
        call    dg_one
        ld      b,10
        call    dg_one
dg_last:ld      a,c             ; 1 의 자리 + 끝 표시(0)
        add     a,'0'
        ld      (de),a
        inc     de
        xor     a
        ld      (de),a
        ret

dg_one: ld      a,'0'-1         ; C 에서 B 를 몇 번 뺄 수 있는지 -> 글자 한 자리
dg_l:   inc     a
        ld      l,a
        ld      a,c
        sub     b
        jr      c,dg_d
        ld      c,a
        ld      a,l
        jr      dg_l
dg_d:   ld      a,l
        ld      (de),a
        inc     de
        ret

dec_trim:
        ld      de,numbuf
        call    dec3
        ld      hl,numbuf
dt_l:   ld      a,(hl)
        cp      '0'
        ret     nz
        inc     hl
        ld      a,(hl)
        or      a
        jr      nz,dt_l
        dec     hl              ; 값이 0 이면 "0" 한 글자는 남긴다
        ret

pan_text:
        ld      hl,numbuf
        cp      64
        jr      nz,pt_s
        ld      (hl),'C'
        inc     hl
        ld      (hl),0
        ld      hl,numbuf
        ret
pt_s:   jr      c,pt_l
        sub     64
        ld      c,a
        ld      (hl),'R'
        jr      pt_n
pt_l:   ld      c,a
        ld      a,64
        sub     c
        ld      c,a
        ld      (hl),'L'
pt_n:   inc     hl
        ex      de,hl
        ld      a,c
        call    dec_trim_to
        ld      hl,numbuf
        ret
; A = 값 -> DE 에 앞 0 없는 글자 + 0
dec_trim_to:
        push    de
        ld      de,numbuf+4
        call    dec3
        ld      hl,numbuf+4
dtt_l:  ld      a,(hl)
        cp      '0'
        jr      nz,dtt_c
        inc     hl
        ld      a,(hl)
        or      a
        jr      nz,dtt_l
        dec     hl
dtt_c:  pop     de
dtt_m:  ld      a,(hl)
        ld      (de),a
        inc     hl
        inc     de
        or      a
        jr      nz,dtt_m
        ret
