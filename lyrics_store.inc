; ============================================================
; 만든이: KimTaeWoo
; 도와준이: Lee.J.H
;  lyrics_store.inc - 가사 저장소 (인터럽트 쪽 엔진이 부르므로 8000h 이상의 높은 영역에 둔다)
;  화면 그리기는 scr_lyrics.inc
; ============================================================
LYR_LINES equ   5
LYR_LEN   equ   44
LYR_MOVE  equ   (LYR_LINES-1)*(LYR_LEN+1)
LYR_H     equ   LYR_LINES*30
LYR_LAST_Y equ  28+(LYR_LINES-1)*30
lyr_buf:  defs  LYR_LINES*(LYR_LEN+1)
lyr_n:    defb  0               ; 지금 줄 번호 (0..LYR_LINES-1, 가장 아래)가 아니라 채운 줄 수 (최대 LYR_LINES)
lyr_len:  defb  0               ; 맨 아래 줄의 글자 수
lyr_dirty:defb  0               ; 1 = 맨 아래 줄만, 2 = 전체

lyrics_clear:
        ld      hl,lyr_buf
        ld      de,lyr_buf+1
        ld      bc,LYR_LINES*(LYR_LEN+1)-1
        ld      (hl),0
        ldir
        xor     a
        ld      (lyr_n),a
        ld      (lyr_len),a
        ld      a,2
        ld      (lyr_dirty),a
        ret

; HL = 글자, BC = 길이 : 가사 한 조각을 맨 아래 줄에 붙인다 ( / 또는 \ 로 시작하면 새 줄)
lyrics_add:
        ld      a,b
        or      a
        ret     nz
        ld      a,c
        or      a
        ret     z
        ld      a,(hl)
        cp      '@'             ; KAR 머리표 (@T 제목 ...) 는 건너뛴다
        ret     z
        cp      '/'
        jr      z,la_nl
        cp      '\'
        jr      nz,la_cp
la_nl:  push    hl
        push    bc
        call    lyr_newline
        pop     bc
        pop     hl
        inc     hl
        dec     c
        ret     z
la_cp:  ld      a,(lyr_n)       ; 아직 한 줄도 없으면 첫 줄 시작
        or      a
        jr      nz,la_l
        push    hl
        push    bc
        call    lyr_newline
        pop     bc
        pop     hl
la_l:   ld      a,(hl)
        cp      0Dh
        jr      z,la_cr
        cp      0Ah
        jr      z,la_cr
        ld      d,a
        ld      a,(lyr_len)
        cp      LYR_LEN
        jr      c,la_put
        push    hl
        push    bc
        call    lyr_newline     ; 너무 길면 줄바꿈
        pop     bc
        pop     hl
la_put: push    hl
        push    bc
        ld      a,(lyr_len)
        ld      c,a
        call    lyr_last        ; HL = 맨 아래 줄
        ld      b,0
        add     hl,bc
        ld      (hl),d
        inc     hl
        ld      (hl),0
        ld      hl,lyr_len
        inc     (hl)
        pop     bc
        pop     hl
la_n:   inc     hl
        dec     c
        jr      nz,la_l
        ld      a,(lyr_dirty)
        or      a
        ret     nz
        ld      a,1
        ld      (lyr_dirty),a
        ret
la_cr:  push    hl
        push    bc
        call    lyr_newline
        pop     bc
        pop     hl
        jr      la_n

; 줄을 한 칸 위로 밀고 맨 아래를 비운다
lyr_newline:
        ld      hl,lyr_buf+(LYR_LEN+1)
        ld      de,lyr_buf
        ld      bc,LYR_MOVE
        ldir
        ex      de,hl
        ld      (hl),0
        xor     a
        ld      (lyr_len),a
        ld      a,(lyr_n)
        cp      LYR_LINES
        jr      nc,ln_d
        inc     a
        ld      (lyr_n),a
ln_d:   ld      a,2
        ld      (lyr_dirty),a
        ret

; HL = 맨 아래 줄
lyr_last:
        ld      hl,lyr_buf+(LYR_LINES-1)*(LYR_LEN+1)
        ret

