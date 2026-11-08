; ============================================================
; 만든이: KimTaeWoo
; 도와준이: Lee.J.H
;  scr_lyrics.inc - F3 가사 화면.  곡의 가사(메타 05) / 글(메타 01) 사건을 모아 최근 줄을 크게 보여 준다.
;  (한글 가사는 글꼴에 없어 ? 로 나온다)
; ============================================================
; ---- 화면 ----
scr_lyrics:
        TXT     4,13,0,C_LN,C_BG,0,"LYRICS"
        BOX     4,22,504,1,C_LN
        ld      a,2
        ld      (lyr_dirty),a
        jp      lyrics_draw

lyrics_update:
        ld      a,(lyr_dirty)
        or      a
        ret     z
lyrics_draw:
        ld      a,(lyr_dirty)
        ld      c,a
        xor     a
        ld      (lyr_dirty),a
        ld      a,(has_lyrics)
        or      a
        jr      nz,lyd_go
        ld      a,(lyr_n)
        or      a
        jr      nz,lyd_go
        TXT     8,40,0,C_LN,C_BG,0,"(no lyrics in this song)"
        ret
lyd_go:  ld      a,c
        cp      1
        ld      a,LYR_LINES-1   ; 한 줄만: 맨 아래
        jr      z,lyd_one
        xor     a
        ld      (lyd_i),a
        BOX     4,28,504,LYR_H,C_BG      ; 전체 다시 그릴 때만 바탕을 지운다
        jr      lyd_l
lyd_one: ld      (lyd_i),a
        BOX     4,LYR_LAST_Y,504,30,C_BG
lyd_l:   ld      a,(lyd_i)        ; 줄 i 의 y = 30 + 30 * i
        ld      c,a
        add     a,a
        add     a,c             ; *3
        add     a,a             ; *6
        ld      c,a
        add     a,a
        add     a,a             ; *24
        add     a,c             ; *30
        add     a,30
        ld      (t_y),a
        ld      a,(lyd_i)        ; 글자 위치 = lyr_buf + i * (LYR_LEN+1)
        ld      c,a
        ld      hl,lyr_buf
        ld      de,LYR_LEN+1
lyd_m:   ld      a,c
        or      a
        jr      z,lyd_p
        add     hl,de
        dec     c
        jr      lyd_m
lyd_p:   ld      (lyd_str),hl
        ld      a,(lyd_i)        ; 맨 아래 줄 = 노란색, 나머지 = 흐리게
        cp      LYR_LINES-1
        ld      a,C_LN
        jr      nz,lyd_c
        ld      a,C_YEL
lyd_c:   ld      (t_fg),a
        ld      a,C_BG
        ld      (t_bg),a
        ld      hl,4
        ld      (t_x),hl
        ld      hl,504
        ld      (t_w),hl
        ld      a,1
        ld      (t_al),a        ; 가운데
        ld      hl,(lyd_str)
        xor     a               ; 보통 크기 (한 줄 8픽셀)
        call    txtv
        ld      a,(lyd_i)
        inc     a
        ld      (lyd_i),a
        cp      LYR_LINES
        jr      c,lyd_l
        ret
lyd_i:   defb    0
lyd_str: defw    0
