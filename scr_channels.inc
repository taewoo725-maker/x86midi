; ============================================================
; 만든이: KimTaeWoo
; 도와준이: Lee.J.H
;  scr_channels.inc - 플레이어 화면의 한 부분: 채널 표 (머리줄 + 16줄)
;  scr_player.inc 가 맨 아래에서 이어 붙인다 (그림 부분은 상태 변수만 읽는다)
; ============================================================
; ---------------------------------------------------------------
;  채널 표 : 머리줄 (y 85) + 줄 (y 94) + 16줄 (9줄 간격, 2단)
; ---------------------------------------------------------------
draw_channel_table:
        ld      hl,ch_row_style ; 처음부터 그리니 모든 줄의 바탕을 지운다
        ld      de,ch_row_style+1
        ld      bc,15
        ld      (hl),0FFh
        ldir
        ld      hl,4
        call    draw_table_head
        ld      hl,258
        call    draw_table_head
        BOX     4,94,504,1,C_LN
        xor     a
        ld      (loop_ch),a
dct_l:  call    draw_channel_row
        ld      hl,loop_ch
        inc     (hl)
        ld      a,(hl)
        cp      16
        jr      c,dct_l
        ret

; HL = 단의 왼쪽 x
draw_table_head:
        ld      (row_x),hl
        ld      de,0
        ld      b,C_LN
        ld      c,C_BG
        ld      a,85
        call    text_at
        xor     a
        ld      (t_al),a
        ld      hl,txt_head
        call    txtp
        ld      ix,head_cols
dth_l:  ld      a,(ix+0)
        or      a
        ret     z
        ld      e,a             ; 열 x 오프셋 (1바이트 + 256 보정은 hi 를 따로)
        ld      d,(ix+1)
        ld      hl,(row_x)
        add     hl,de
        ld      e,(ix+2)
        ld      d,0
        ld      b,C_LN
        ld      c,C_BG
        ld      a,85
        call    text_at
        ld      a,1
        ld      (t_al),a
        push    ix
        ld      l,(ix+3)
        ld      h,(ix+4)
        call    txtp
        pop     ix
        ld      de,5
        add     ix,de
        jr      dth_l
txt_head: defb  "CH  INSTRUMENT",0
; 열 : x 오프셋(2바이트) , 너비 , 글자 주소(2바이트)
head_cols:
        defb    92,0,40
        defw    hc_level
        defb    134,0,20
        defw    hc_vol
        defb    156,0,22
        defw    hc_pan
        defb    180,0,22
        defw    hc_prg
        defb    204,0,9
        defw    hc_m
        defb    216,0,9
        defw    hc_s
        defb    0
hc_level: defb  "LEVEL",0
hc_vol:   defb  "VOL",0
hc_pan:   defb  "PAN",0
hc_prg:   defb  "PRG",0
hc_m:     defb  "M",0
hc_s:     defb  "S",0

; loop_ch 번째 채널의 표 한 줄 그리기에 필요한 자리/색을 정한다 (row_x, row_y, row_bg, row_sel, row_used)
row_setup:
        ld      a,(loop_ch)
        ld      b,a
        and     7
        ld      c,a
        add     a,a
        add     a,a
        add     a,a
        add     a,c             ; * 9
        add     a,95
        ld      (row_y),a
        ld      hl,4
        ld      a,b
        cp      8
        jr      c,dcr_x
        ld      hl,258
dcr_x:  ld      (row_x),hl
        ld      a,(loop_ch)     ; 선택된 줄이면 크림색 바탕
        ld      b,a
        ld      a,(ch_sel)
        cp      b
        ld      a,C_BG
        ld      b,0
        jr      nz,dcr_b
        ld      a,C_TX
        ld      b,1
dcr_b:  ld      (row_bg),a
        ld      a,b
        ld      (row_sel),a
        call    channel_used    ; Z = 안 쓰는 채널
        ld      (row_used),a
        ret

; 소리 크기 막대만 다시 그린다 (loop_ch)
redraw_row_level:
        call    row_setup
        ld      a,(row_used)
        or      a
        ret     z
        jp      row_level

; loop_ch 번째 채널의 표 한 줄
; 한 줄을 다시 그린다.  줄 바탕은 모양(선택 / 쓰는 채널)이 바뀌었을 때만 지워서,
; 같은 줄을 자주 고쳐 그려도 줄이 한순간 비어 보이며 깜빡이지 않는다 (칸마다 자기 바탕을 칠한다)
draw_channel_row:
        call    row_setup
        call    row_style_changed
        call    nz,row_clear_background
        call    row_chip
        call    row_name
        ld      a,(row_used)
        or      a
        ret     z
        call    row_level
        call    row_numbers
        jp      row_ms

; NZ = 이 줄을 마지막에 그린 모양과 다르다 (새 모양을 기억해 둔다)
row_style_changed:
        ld      a,(row_sel)
        add     a,a
        ld      b,a
        ld      a,(row_used)
        or      a
        jr      z,rsc_k
        inc     b
rsc_k:  ld      a,(loop_ch)
        ld      e,a
        ld      d,0
        ld      hl,ch_row_style
        add     hl,de
        ld      a,b
        cp      (hl)
        ld      (hl),a
        ret

row_clear_background:
        ld      hl,(row_x)      ; 줄 바탕
        ld      (r_x),hl
        ld      a,(row_y)
        ld      l,a
        ld      h,0
        ld      (r_y),hl
        ld      hl,250
        ld      (r_w),hl
        ld      hl,9
        ld      (r_h),hl
        ld      a,(row_bg)
        ld      (r_c),a
        jp      rect_go

; 색 칩 (11 x 7) + 채널 번호
row_chip:
        ld      a,(row_used)
        or      a
        ld      a,C_PN
        ld      b,C_LN
        jr      z,rc_c
        call    channel_color
        ld      b,C_BG
rc_c:   ld      c,a             ; 바탕색 = 칩색
        ld      hl,(row_x)
        ld      (r_x),hl
        ld      a,(row_y)
        inc     a
        ld      l,a
        ld      h,0
        ld      (r_y),hl
        ld      hl,11
        ld      (r_w),hl
        ld      hl,7
        ld      (r_h),hl
        ld      a,c
        ld      (r_c),a
        push    bc
        call    rect_go
        pop     bc
        ld      hl,(row_x)
        ld      de,11
        ld      a,(row_y)
        inc     a
        call    text_at
        ld      a,7
        ld      (t_rows),a
        ld      a,1
        ld      (t_al),a
        ld      a,(loop_ch)
        inc     a
        ld      de,numbuf
        call    dec2
        ld      hl,numbuf
        jp      txtp

; 악기 이름 (x + 14, 너비 76)
;   안 쓰는 채널 = 흐림, 선택 줄 = 어두운 글자, 소거된 채널 = 흐림, 그 밖에는 크림색
row_name:
        ld      a,(row_used)
        or      a
        ld      b,C_LN
        jr      z,rn_t
        ld      a,(row_sel)
        or      a
        ld      b,C_BG
        jr      nz,rn_t
        call    channel_muted
        ld      b,C_TX
        jr      z,rn_t
        ld      b,C_LN
rn_t:   ld      hl,(row_x)
        ld      de,14
        add     hl,de
        ld      de,76
        ld      a,(row_bg)
        ld      c,a
        ld      a,(row_y)
        call    text_at
        xor     a
        ld      (t_al),a
        call    channel_name
        jp      txtp

; 소리 크기 막대 (x + 92, 40 x 4)
row_level:
        ld      hl,(row_x)
        ld      de,92
        add     hl,de
        ld      (r_x),hl
        ld      (f_x),hl
        ld      a,(row_y)
        add     a,3
        ld      l,a
        ld      h,0
        ld      (r_y),hl
        ld      (f_y),hl
        ld      hl,40
        ld      (r_w),hl
        ld      (f_w),hl
        ld      hl,4
        ld      (r_h),hl
        ld      (f_h),hl
        ld      a,(row_sel)
        or      a
        ld      a,C_PN
        jr      z,rl_c
        ld      a,C_BG
rl_c:   ld      (r_c),a
        call    rect_go
        ld      a,C_LN
        ld      (f_c),a
        call    frame_go
        call    channel_level
        or      a
        ret     z
        ld      (r_w),a
        xor     a
        ld      (r_w+1),a
        ld      hl,(row_x)
        ld      de,93
        add     hl,de
        ld      (r_x),hl
        ld      a,(row_y)
        add     a,4
        ld      l,a
        ld      h,0
        ld      (r_y),hl
        ld      hl,2
        ld      (r_h),hl
        call    channel_color
        ld      (r_c),a
        jp      rect_go

; VOL / PAN / PRG 숫자 (가운데 정렬)
row_numbers:
        ld      a,(row_sel)
        or      a
        ld      b,C_BG
        jr      nz,rnb_c
        call    channel_muted
        ld      b,C_TX
        jr      z,rnb_c
        ld      b,C_LN
rnb_c:  ld      a,b
        ld      (num_fg),a
        call    channel_volume
        call    dec_trim
        push    hl
        ld      hl,(row_x)
        ld      de,134
        add     hl,de
        ld      de,20
        call    num_cell
        pop     hl
        call    txtp
        call    channel_pan
        call    pan_text
        push    hl
        ld      hl,(row_x)
        ld      de,156
        add     hl,de
        ld      de,22
        call    num_cell
        pop     hl
        call    txtp
        call    channel_program
        ld      de,numbuf
        call    dec3
        ld      a,(row_sel)
        or      a
        ld      b,C_LN
        jr      z,rnb_p
        ld      b,C_BG
rnb_p:  ld      a,(row_bg)
        ld      c,a
        ld      hl,(row_x)
        ld      de,180
        add     hl,de
        ld      de,22
        ld      a,(row_y)
        call    text_at
        ld      a,1
        ld      (t_al),a
        ld      hl,numbuf
        jp      txtp
; 숫자 칸 준비: HL = x, DE = 너비
num_cell:
        ld      a,(num_fg)
        ld      b,a
        ld      a,(row_bg)
        ld      c,a
        ld      a,(row_y)
        call    text_at
        ld      a,1
        ld      (t_al),a
        ret

; M / S 단추 (x + 204, x + 216 : 9 x 7)
row_ms: call    channel_muted
        ld      c,a
        ld      hl,(row_x)
        ld      de,204
        add     hl,de
        ld      b,C_RED
        ld      iy,bmp_m
        call    ms_box
        call    channel_solo
        ld      c,a
        ld      hl,(row_x)
        ld      de,216
        add     hl,de
        ld      b,C_YEL
        ld      iy,bmp_s
        jp      ms_box

; HL = x, C = 켜짐(0 이면 꺼짐), B = 켜졌을 때의 색, IY = 글자 점그림 (5 x 5)
ms_box: ld      (ms_x),hl
        ld      a,c
        ld      (ms_on),a       ; (rect_go / frame_go 가 레지스터를 쓰므로 켜짐 값은 변수에 보관)
        ld      a,(row_bg)
        inc     c
        dec     c
        jr      z,mb_c
        ld      a,b
mb_c:   ld      (r_c),a
        ld      (ms_fill),a
        ld      hl,(ms_x)
        ld      (r_x),hl
        ld      (f_x),hl
        ld      a,(row_y)
        inc     a
        ld      l,a
        ld      h,0
        ld      (r_y),hl
        ld      (f_y),hl
        ld      hl,9
        ld      (r_w),hl
        ld      (f_w),hl
        ld      hl,7
        ld      (r_h),hl
        ld      (f_h),hl
        call    rect_go
        ld      a,C_LN
        ld      (f_c),a
        call    frame_go
        ld      a,(row_sel)     ; 글자색: 켜짐 = 어두운색, 꺼짐 = 줄 글자색
        ld      b,C_TX
        or      a
        jr      z,mb_t
        ld      b,C_BG
mb_t:   ld      a,(ms_on)
        or      a
        jr      z,mb_f
        ld      b,C_BG
mb_f:   ld      a,b
        ld      (t_fg),a
        ld      a,(ms_fill)
        ld      (t_bg),a
        ld      hl,(ms_x)
        inc     hl
        inc     hl              ; 안쪽 7칸 가운데에 5칸 글자
        ld      (t_x),hl
        ld      a,(row_y)
        add     a,2
        ld      (t_y),a
        push    iy
        pop     hl
        ld      bc,5*256+5
        jp      bmp
