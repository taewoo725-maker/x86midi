; ============================================================
; 만든이: KimTaeWoo
; 도와준이: Lee.J.H
;  scr_roll.inc - F2 롤 화면: 소리가 나는 음이 오른쪽에서 왼쪽으로 흘러가는 피아노 롤
;  (VDP 의 화면 안 복사 HMMM 으로 2칸씩 밀고, 오른쪽 끝에 지금 눌린 음을 채널 색으로 찍는다)
; ============================================================
ROLL_TOP    equ 26
ROLL_H      equ 150
roll_tick:  defb 0

hm_sx:  defw    2
hm_sy:  defw    ROLL_TOP
hm_dx:  defw    0
hm_dy:  defw    ROLL_TOP
hm_nx:  defw    510
hm_ny:  defw    ROLL_H
        defb    0               ; CLR
        defb    0               ; ARG (오른쪽으로 읽어 나가며 복사)
        defb    0D0h            ; HMMM

hmmm_go:
        call    vwait
        ld      hl,hm_sy+1      ; 원본과 목적지 모두 그리는 쪽에 있다
        call    hm_set_page
        ld      hl,hm_dy+1
        call    hm_set_page
        di
        ld      a,32
        out     (VDP_C),a
        ld      a,91h           ; R#17 = 32 (자동 증가)
        out     (VDP_C),a
        ld      hl,hm_sx
        ld      b,15
        ld      c,VDP_I
        otir
        ei
        ret

; HL 가 가리키는 y 위쪽 바이트에 그리는 쪽을 넣는다
hm_set_page:
        ld      a,(hl)
        and     0FEh
        ld      b,a
        ld      a,(draw_page)
        or      b
        ld      (hl),a
        ret

scr_roll:
        TXT     4,13,0,C_LN,C_BG,0,"PIANO ROLL"
        BOX     4,22,504,1,C_LN
        BOX     0,ROLL_TOP,512,ROLL_H,C_PN
        jp      roll_legend

; 채널 색 안내 (아래쪽 16칸)
roll_legend:
        xor     a
        ld      (loop_ch),a
        ld      hl,8
        ld      (r_x),hl
rl_l:   ld      hl,ROLL_TOP+ROLL_H+6
        ld      (r_y),hl
        ld      hl,15
        ld      (r_w),hl
        ld      hl,7
        ld      (r_h),hl
        ld      a,(loop_ch)
        ld      e,a
        ld      d,0
        ld      hl,chcol_tbl
        add     hl,de
        ld      a,(hl)
        ld      (r_c),a
        call    rect_go
        ld      hl,(r_x)
        ld      de,30
        add     hl,de
        ld      (r_x),hl
        ld      hl,loop_ch
        inc     (hl)
        ld      a,(hl)
        cp      16
        jr      c,rl_l
        ret

; 두 프레임에 한 번: 밀고 새 열을 그린다
roll_update:
        ld      hl,roll_tick
        inc     (hl)
        ld      a,(hl)
        and     1
        ret     nz
        call    hmmm_go
        BOX     510,ROLL_TOP,2,ROLL_H,C_PN      ; 새 열 바탕
        xor     a
        ld      (rl_k),a
ru_l:   ld      a,(rl_k)
        ld      e,a
        ld      d,0
        ld      hl,key_state
        add     hl,de
        ld      a,(hl)
        cp      0FFh
        jr      z,ru_n
        ld      (r_c),a
        ld      a,(rl_k)        ; y = ROLL_TOP + ROLL_H - 4 - 1.5 * 건반
        ld      c,a
        srl     a
        add     a,c
        ld      c,a
        ld      a,ROLL_TOP+ROLL_H-4
        sub     c
        ld      l,a
        ld      h,0
        ld      (r_y),hl
        ld      hl,510
        ld      (r_x),hl
        ld      hl,2
        ld      (r_w),hl
        ld      (r_h),hl
        call    rect_go
ru_n:   ld      hl,rl_k
        inc     (hl)
        ld      a,(hl)
        cp      KEY_COUNT
        jr      c,ru_l
        ret
rl_k:   defb    0
