; ============================================================
; 만든이: KimTaeWoo
; 도와준이: Lee.J.H
;  scr_piano.inc - 플레이어 화면의 한 부분: 88건반
;  scr_player.inc 가 맨 아래에서 이어 붙인다 (그림 부분은 상태 변수만 읽는다)
; ============================================================
; ---------------------------------------------------------------
;  88건반 (y 169, 높이 20)
; ---------------------------------------------------------------
draw_piano:
        BOX     0,169,512,20,C_BG
        ld      c,0
dpi_w:  push    bc
        call    piano_white
        pop     bc
        inc     c
        ld      a,c
        cp      KEY_COUNT
        jr      c,dpi_w
        ld      c,0
dpi_b:  push    bc
        call    piano_black
        pop     bc
        inc     c
        ld      a,c
        cp      KEY_COUNT
        jr      c,dpi_b
        ld      c,0             ; C 건반 글자 C1..C8
dpi_l:  push    bc
        call    piano_label
        pop     bc
        inc     c
        ld      a,c
        cp      8
        jr      c,dpi_l
        ret

; C = 건반 번호 -> HL = key_geom 칸 (종류 1바이트 + x 2바이트)
key_geom_ptr:
        ld      l,c
        ld      h,0
        ld      e,l
        ld      d,h
        add     hl,hl
        add     hl,de
        ld      de,key_geom
        add     hl,de
        ret

; C = 건반 번호: 흰건반이면 안쪽을 칠한다 (아니면 아무것도 안 함)
piano_white:
        call    key_geom_ptr
        ld      a,(hl)
        or      a
        ret     nz
        inc     hl
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        inc     de              ; 안쪽 7칸 (테두리 1칸 제외)
        ld      (r_x),de
        call    key_color_c
        ld      (r_c),a
        ld      hl,170
        ld      (r_y),hl
        ld      hl,7
        ld      (r_w),hl
        ld      hl,18
        ld      (r_h),hl
        jp      rect_go

; C = 건반 번호: 검은건반이면 칠하고 (눌린 것만) 테두리를 그린다
piano_black:
        call    key_geom_ptr
        ld      a,(hl)
        or      a
        ret     z
        inc     hl
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        ld      (r_x),de
        ld      (f_x),de
        call    key_color_c
        ld      (r_c),a
        ld      (bk_c),a
        ld      hl,169
        ld      (r_y),hl
        ld      (f_y),hl
        ld      hl,6
        ld      (r_w),hl
        ld      (f_w),hl
        ld      hl,12
        ld      (r_h),hl
        ld      (f_h),hl
        call    rect_go
        ld      a,(bk_c)
        cp      C_BG
        ret     z               ; 꺼진 검은건반은 테두리 없음
        ld      a,C_LN
        ld      (f_c),a
        jp      frame_go

; C = C 건반 글자 번호 (0..7): 건반 번호 = 3 + 12 * C
piano_label:
        ld      a,c
        ld      e,a
        add     a,a
        add     a,e
        add     a,a
        add     a,a             ; 12 * c
        add     a,3
        ld      (lab_idx),a
        ld      c,a
        call    key_color_c
        ld      (t_bg),a
        ld      a,C_BG
        ld      (t_fg),a
        ld      a,(lab_idx)
        ld      c,a
        call    key_geom_ptr
        inc     hl
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        inc     de
        ld      (t_x),de
        ld      a,182
        ld      (t_y),a
        ld      a,(lab_idx)     ; 글자 번호 = (건반 - 3) / 12
        sub     3
        ld      c,0
pl_d:   sub     12
        jr      c,pl_x
        inc     c
        jr      pl_d
pl_x:   ld      a,c
        add     a,a
        ld      e,a
        ld      d,0
        ld      hl,keylab_tbl
        add     hl,de
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        ex      de,hl
        ld      bc,5*256+7
        jp      bmp

; C = 건반 번호: 그 건반 하나만 다시 그린다 (흰건반이면 이웃 검은건반과 C 글자도)
piano_redraw_key:
        ld      (prk_i),a
        ld      c,a
        call    key_geom_ptr
        ld      a,(hl)
        or      a
        jr      nz,prk_blk
        ld      a,(prk_i)
        ld      c,a
        call    piano_white
        ld      a,(prk_i)       ; 왼쪽 이웃
        or      a
        jr      z,prk_r
        dec     a
        ld      c,a
        call    piano_black
prk_r:  ld      a,(prk_i)       ; 오른쪽 이웃
        inc     a
        cp      KEY_COUNT
        jr      nc,prk_l
        ld      c,a
        call    piano_black
prk_l:  ld      a,(prk_i)       ; C 건반이면 글자
        sub     3
        ret     c
        ld      c,0
prk_d:  sub     12
        jr      c,prk_t
        inc     c
        jr      prk_d
prk_t:  add     a,12            ; 나머지 0 이어야 C 건반
        ret     nz
        ld      a,c
        cp      8
        ret     nc
        ld      c,a
        jp      piano_label
prk_blk:ld      a,(prk_i)
        ld      c,a
        jp      piano_black
prk_i:  defb    0

; C = 건반 번호 (0..87) -> A = 칠할 색 (눌린 건반이면 그 색, 아니면 흰건반 TX / 검은건반 BG)
; (HL, BC 보존.  종류는 key_geom 에서 다시 읽는다)
key_color_c:
        push    hl
        push    bc
        ld      b,0
        ld      hl,key_state
        add     hl,bc
        ld      a,(hl)
        cp      0FFh
        jr      nz,kc_x
        ld      l,c             ; 눌리지 않음: 종류로 결정
        ld      h,0
        ld      e,l
        ld      d,h
        add     hl,hl
        add     hl,de
        ld      de,key_geom
        add     hl,de
        ld      a,(hl)
        or      a
        ld      a,C_TX
        jr      z,kc_x
        ld      a,C_BG
kc_x:   pop     bc
        pop     hl
        ret
