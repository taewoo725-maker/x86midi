; ============================================================
; 만든이: KimTaeWoo
; 도와준이: Lee.J.H
;  strfmt.inc - 글자 조립 도우미 (strbuf 에 이어 붙인다)
;  sb_reset : 처음부터    sb_str : HL = 글자 붙이기    sb_chr : A = 한 글자    sb_dec : A = 숫자(앞 0 없음)
;  sb_pad2 / sb_pad3 : A = 숫자를 2 / 3자리(앞을 0 으로 채움)    sb_done : 끝 표시, HL = strbuf
; ============================================================
sb_p:   defw    0

sb_reset:
        ld      hl,strbuf
        ld      (sb_p),hl
        ret

sb_chr: ld      hl,(sb_p)
        ld      (hl),a
        inc     hl
        ld      (sb_p),hl
        ret

sb_str: ex      de,hl
        ld      hl,(sb_p)
ss_l:   ld      a,(de)
        or      a
        jr      z,ss_d
        ld      (hl),a
        inc     hl
        inc     de
        jr      ss_l
ss_d:   ld      (sb_p),hl
        ret

sb_done:xor     a
        call    sb_chr
        ld      hl,strbuf
        ret

sb_dec: call    dec_trim        ; HL = numbuf 안의 앞 0 없는 글자
        jp      sb_str
sb_pad3:ld      de,numbuf
        call    dec3
        ld      hl,numbuf
        jp      sb_str
sb_pad2:ld      de,numbuf
        call    dec2
        ld      hl,numbuf
        jp      sb_str

; 16비트 값 HL 을 앞 0 없이 붙인다 (0..65535)
sb_dec16:
        ld      (v_n),hl
        ld      hl,0
        ld      (v_n+2),hl
        jp      sb_dec32
; v_n (32비트) 을 앞 0 없이 붙인다
sb_dec32:
        ld      hl,0
        ld      (v_d+2),hl
        ld      hl,10
        ld      (v_d),hl
        ld      hl,db_end       ; 아래쪽에서부터 거꾸로 채운다
        ld      (hl),0
        ld      b,0
d32_l:  push    bc
        call    div32           ; v_n = 몫, v_r = 나머지 (0..9)
        pop     bc
        ld      a,(v_r)
        add     a,'0'
        ld      hl,db_end
        ld      c,b
        ld      e,c
        ld      d,0
        or      a
        sbc     hl,de
        dec     hl
        ld      (hl),a
        inc     b
        ld      a,(v_n)
        ld      hl,v_n+1
        or      (hl)
        inc     hl
        or      (hl)
        inc     hl
        or      (hl)
        jr      z,d32_x
        ld      a,b
        cp      10
        jr      c,d32_l
d32_x:  ld      hl,db_end       ; 쓴 글자 수 b 만큼 거꾸로 위치를 구한다
        ld      e,b
        ld      d,0
        or      a
        sbc     hl,de
        jp      sb_str
db_buf: defs    12
db_end  equ     db_buf+11
