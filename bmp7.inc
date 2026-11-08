; ============================================================
; 만든이: KimTaeWoo
; 도와준이: Lee.J.H
;  bmp7.inc - 1비트 점그림(아이콘, 건반 글자) 그리기 공용부
;  입력: HL = 그림 자료 (한 줄 = 2바이트, 왼쪽 맞춤, 비트15 가 맨 왼쪽)
;        B = 줄 수, C = 너비(1..16), t_x/t_y = 왼쪽 위, t_fg/t_bg = 그림색/바탕색
;  x 가 홀수여도 된다 (글자 엔진과 같은 가장자리 처리)
; ============================================================
bmp:    ld      (bm_src),hl
        ld      a,b
        ld      (bm_rows),a
        ld      a,c
        ld      (t_w),a
        xor     a
        ld      (t_w+1),a
        ld      (t_al),a
        ld      (t_tw),a
        ld      (t_tw+1),a
        call    tx_layout
        call    vwait
        xor     a
        ld      (t_r),a
bm_row: call    tx_row_begin
        ld      hl,(bm_src)
        ld      e,(hl)          ; defw 는 낮은 바이트가 먼저
        inc     hl
        ld      d,(hl)
        inc     hl
        ld      (bm_src),hl
        ld      a,(t_w)
        ld      b,a
        ld      c,d             ; 왼쪽 8점 (비트15..8)
        cp      9
        jr      c,bm_last
        ld      b,8
        push    de
        call    tx_paint_1x
        pop     de
        ld      a,(t_w)
        sub     8
        ld      b,a
        ld      c,e             ; 나머지 점 (비트7..0)
bm_last:call    tx_paint_1x
        call    tx_putrow
        ld      hl,t_r
        inc     (hl)
        ld      a,(bm_rows)
        cp      (hl)
        jr      nz,bm_row
        ret
bm_src: defw    0
bm_rows:defb    0
