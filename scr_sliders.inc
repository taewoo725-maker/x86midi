; ============================================================
; 만든이: KimTaeWoo
; 도와준이: Lee.J.H
;  scr_sliders.inc - 플레이어 화면의 한 부분: 슬라이더 3개 (TEMPO / KEY / VOL)
;  scr_player.inc 가 맨 아래에서 이어 붙인다 (그림 부분은 상태 변수만 읽는다)
; ============================================================
; ---------------------------------------------------------------
;  슬라이더 3개 : TEMPO / KEY / VOL   (x = 262, 348, 434)
; ---------------------------------------------------------------
draw_sliders:
        ld      hl,258
        ld      de,txt_tempo
        ld      bc,tempo_val
        ld      a,(sl_tempo)
        call    draw_slider_c
        ld      hl,344
        ld      de,txt_key
        ld      bc,key_val
        ld      a,(sl_key)
        call    draw_slider_c
        ld      hl,428
        ld      de,txt_vol
        ld      bc,vol_val
        ld      a,(sl_vol)
        jp      draw_slider
txt_tempo: defb "TEMPO",0
txt_key:   defb "KEY",0
txt_vol:   defb "VOL",0
tempo_val: defb "100%",0,0
key_val:   defb "+0",0,0,0
vol_val:   defb "100",0,0

; 슬라이더 : HL = x, DE = 이름 글자, BC = 값 글자, A = 손잡이 위치 0..80
;   draw_slider_c = 가운데(40) 기준으로 채움 (TEMPO, KEY),  draw_slider = 왼쪽부터 채움 (VOL)
draw_slider_c:
        ld      (sl_pos),a
        ld      a,1
        jr      ds_go2
draw_slider:
        ld      (sl_pos),a
        xor     a
ds_go2: ld      (sl_ctr),a
        ld      (sl_x),hl
        ld      (sl_lbl),de
        ld      (sl_val),bc
        ld      hl,(sl_x)       ; 이름 (왼쪽) 과 값 (오른쪽)
        ld      de,36
        ld      b,C_LN
        ld      c,C_BG
        ld      a,66
        call    text_at
        xor     a
        ld      (t_al),a
        ld      hl,(sl_lbl)
        call    txtp
        ld      hl,(sl_x)
        ld      de,36
        add     hl,de
        ld      de,44
        ld      b,C_TX
        ld      c,C_BG
        ld      a,66
        call    text_at
        ld      a,2
        ld      (t_al),a
        ld      hl,(sl_val)
        call    txtp
        call    slider_erase_thumb_strip
        ld      hl,(sl_x)       ; 막대
        ld      (r_x),hl
        ld      (f_x),hl
        ld      hl,77
        ld      (r_y),hl
        ld      (f_y),hl
        ld      hl,80
        ld      (r_w),hl
        ld      (f_w),hl
        ld      hl,4
        ld      (r_h),hl
        ld      (f_h),hl
        ld      a,C_PN
        ld      (r_c),a
        call    rect_go
        ld      a,C_LN
        ld      (f_c),a
        call    frame_go
        ld      a,(sl_ctr)
        or      a
        jr      z,ds_vol
        ld      hl,(sl_x)       ; 가운데 눈금
        ld      de,40
        add     hl,de
        ld      (r_x),hl
        ld      hl,76
        ld      (r_y),hl
        ld      hl,1
        ld      (r_w),hl
        ld      hl,6
        ld      (r_h),hl
        ld      a,C_LN
        ld      (r_c),a
        call    rect_go
        ld      a,(sl_pos)
        cp      40
        jr      z,ds_thumb
        jr      c,ds_left
        sub     40              ; 가운데 오른쪽으로 채움
        ld      c,a
        ld      hl,(sl_x)
        ld      de,41
        add     hl,de
        jr      ds_fill
ds_left:ld      c,a             ; 가운데 왼쪽으로 채움
        ld      a,40
        sub     c
        ld      b,a
        ld      e,c
        ld      d,0
        ld      hl,(sl_x)
        add     hl,de
        ld      c,b
        jr      ds_fill
ds_vol: ld      a,(sl_pos)      ; 폭 = 위치 - (위치+39)/40
        ld      c,a
        ld      b,0
        ld      hl,39
        ld      e,a
        ld      d,0
        add     hl,de
        xor     a
ds_d:   ld      de,40
        sbc     hl,de
        jr      c,ds_dd
        inc     a
        jr      ds_d
ds_dd:  ld      b,a
        ld      a,c
        sub     b
        ld      c,a
        ld      hl,(sl_x)
        inc     hl
ds_fill:ld      (r_x),hl
        ld      hl,78
        ld      (r_y),hl
        ld      a,c
        ld      l,a
        ld      h,0
        ld      (r_w),hl
        ld      hl,2
        ld      (r_h),hl
        ld      a,C_YEL
        ld      (r_c),a
        call    rect_go
ds_thumb:
        ld      a,(sl_pos)      ; 손잡이 3 x 10 (x + 위치 - 1)
        ld      e,a
        ld      d,0
        ld      hl,(sl_x)
        add     hl,de
        dec     hl
        ld      (r_x),hl
        ld      hl,74
        ld      (r_y),hl
        ld      hl,3
        ld      (r_w),hl
        ld      hl,10
        ld      (r_h),hl
        ld      a,C_TX
        ld      (r_c),a
        jp      rect_go

; 슬라이더 손잡이가 지나다니는 줄 (x-1 .. x+81, y 74..83) 을 배경색으로 지운다.
;   옛 손잡이가 남아 그림이 깨지는 것을 막는다. 입력 : sl_x
slider_erase_thumb_strip:
        ld      hl,(sl_x)
        dec     hl
        ld      (r_x),hl
        ld      hl,74
        ld      (r_y),hl
        ld      hl,83
        ld      (r_w),hl
        ld      hl,10
        ld      (r_h),hl
        ld      a,C_BG
        ld      (r_c),a
        jp      rect_go
