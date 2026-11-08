; ============================================================
; 만든이: KimTaeWoo
; 도와준이: Lee.J.H
;  scr_transport.inc - 플레이어 화면의 한 부분: 재생 버튼 줄 (아이콘 7개 + 토글 4개)
;  scr_player.inc 가 맨 아래에서 이어 붙인다 (그림 부분은 상태 변수만 읽는다)
; ============================================================
; ---------------------------------------------------------------
;  재생 버튼 줄 (y 68) : 아이콘 7개 + 토글 4개
; ---------------------------------------------------------------
draw_transport:
        call    draw_transport_icons
        ld      bc,132
        ld      e,22
        ld      a,(tg_ab)
        ld      d,a
        ld      hl,txt_ab
        call    draw_toggle
        ld      bc,157
        ld      e,22
        ld      a,(tg_rep)
        ld      d,a
        ld      hl,txt_rep
        call    draw_toggle
        ld      bc,182
        ld      e,26
        ld      a,(tg_shuf)
        ld      d,a
        ld      hl,txt_shuf
        call    draw_toggle
        ld      bc,211
        ld      e,26
        ld      a,(tg_next)
        ld      d,a
        ld      hl,txt_next
        jp      draw_toggle
; 아이콘 버튼 7개만
draw_transport_icons:
        xor     a
        ld      (loop_ch),a
dtr_l:  call    draw_icon_button
        ld      hl,loop_ch
        inc     (hl)
        ld      a,(hl)
        cp      7
        jr      c,dtr_l
        ret
txt_ab:   defb  "A-B",0
txt_rep:  defb  "REP",0
txt_shuf: defb  "SHUF",0
txt_next: defb  "NEXT",0

; loop_ch 번째 버튼 (x = 4 + 18 * 번호, 16 x 12), 눌린 버튼은 노란색
draw_icon_button:
        ld      a,(loop_ch)
        ld      l,a
        ld      h,0
        ld      e,l
        ld      d,h
        add     hl,hl
        add     hl,hl
        add     hl,hl
        add     hl,hl
        add     hl,de
        add     hl,de           ; * 18
        ld      de,4
        add     hl,de
        ld      (bt_x),hl
        ld      a,(btn_active)
        ld      c,a
        ld      a,(loop_ch)
        cp      c
        ld      a,C_PN
        ld      b,C_LN
        ld      c,C_TX          ; 아이콘색
        jr      nz,dib_c
        ld      a,C_YEL
        ld      b,C_YEL
        ld      c,C_BG
dib_c:  ld      (bt_fill),a
        ld      a,b
        ld      (bt_line),a
        ld      a,c
        ld      (bt_icon),a
        ld      hl,(bt_x)       ; 바탕 + 테두리
        ld      (r_x),hl
        ld      (f_x),hl
        ld      hl,68
        ld      (r_y),hl
        ld      (f_y),hl
        ld      hl,16
        ld      (r_w),hl
        ld      (f_w),hl
        ld      hl,12
        ld      (r_h),hl
        ld      (f_h),hl
        ld      a,(bt_fill)
        ld      (r_c),a
        call    rect_go
        ld      a,(bt_line)
        ld      (f_c),a
        call    frame_go
        ld      hl,(bt_x)       ; 아이콘 점그림 (안쪽 14 x 10)
        inc     hl
        ld      (t_x),hl
        ld      a,69
        ld      (t_y),a
        ld      a,(bt_icon)
        ld      (t_fg),a
        ld      a,(bt_fill)
        ld      (t_bg),a
        ld      a,(loop_ch)
        add     a,a
        ld      e,a
        ld      d,0
        ld      hl,icon_tbl
        add     hl,de
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        ex      de,hl
        ld      bc,10*256+14
        jp      bmp

; 토글 단추: BC = x, E = 너비, D = 켜짐(0/1), HL = 글자
draw_toggle:
        ld      (tg_s),hl
        ld      (r_x),bc
        ld      (f_x),bc
        ld      a,e
        ld      (r_w),a
        ld      (f_w),a
        xor     a
        ld      (r_w+1),a
        ld      (f_w+1),a
        ld      hl,68
        ld      (r_y),hl
        ld      (f_y),hl
        ld      hl,12
        ld      (r_h),hl
        ld      (f_h),hl
        ld      a,d
        or      a
        ld      a,C_PN
        ld      b,C_LN
        ld      c,C_TX
        jr      z,dt_c
        ld      a,C_YEL
        ld      b,C_YEL
        ld      c,C_BG
dt_c:   ld      (r_c),a
        ld      (tg_bg),a
        ld      a,b
        ld      (f_c),a
        ld      a,c
        ld      (tg_fg),a
        call    rect_go
        call    frame_go
        ld      hl,(f_x)        ; (frame 이 r_x, r_w 를 바꾸므로 f_ 쪽을 읽는다)
        inc     hl
        ld      a,(f_w)
        sub     2
        ld      e,a
        ld      d,0
        ld      a,(tg_fg)
        ld      b,a
        ld      a,(tg_bg)
        ld      c,a
        ld      a,70
        call    text_at
        ld      a,1
        ld      (t_al),a
        ld      hl,(tg_s)
        jp      txtp
