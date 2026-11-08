; ============================================================
; 만든이: KimTaeWoo
; 도와준이: Lee.J.H
;  scr_player.inc - 플레이어 화면 (시안 Main.dc.html 과 같은 좌표)
;  그림 부분(draw_*)은 상태 변수만 읽는다.  상태는 player_state.inc 에 모여 있다.
;  이 파일 = 공용 도우미 + 맨 위 제목 줄 / 시간 / 곡 정보 / 채널 활동 칸.  나머지는 부분별 파일 (맨 아래 include):
;    scr_seek.inc 탐색 막대,  scr_transport.inc 버튼 줄,  scr_sliders.inc 슬라이더,
;    scr_channels.inc 채널 표,  scr_piano.inc 건반,  scr_bottom.inc 상태 줄 + F 키 줄
; ============================================================
scr_player:
        BOX     0,0,512,212,C_BG        ; 안 보이는 쪽에 먼저 바닥을 칠한다 (이전 화면 글씨가 틈으로 비치지 않게)
        call    draw_titlebar
        call    draw_time_panel
        call    draw_info_panel
        call    draw_activity_panel
        call    draw_seekbar
        call    draw_transport
        call    draw_sliders
        call    draw_channel_table
        call    draw_piano
        call    draw_statusline
        call    draw_fkeys
        ret

; ---------------------------------------------------------------
;  작은 도우미
; ---------------------------------------------------------------
; 한 줄(높이 1) 칠하기: HL = x, D = y, E = 너비, A = 색
fill_run:
        ld      (r_x),hl
        ld      (r_c),a
        ld      l,d
        ld      h,0
        ld      (r_y),hl
        ld      l,e
        ld      h,0
        ld      (r_w),hl
        ld      hl,1
        ld      (r_h),hl
        jp      rect_go

; 글자 자리 정하기: HL = x, A = y, DE = 칸 너비(0=글자너비), B = 글자색, C = 바탕색, 정렬은 t_al 에 미리
text_at:
        ld      (t_x),hl
        ld      (t_y),a
        ld      (t_w),de
        ld      a,b
        ld      (t_fg),a
        ld      a,c
        ld      (t_bg),a
        ld      a,8
        ld      (t_rows),a
        xor     a
        ld      (t_sh),a
        ret

; ---------------------------------------------------------------
;  맨 위 제목 줄 (y 0..9)
; ---------------------------------------------------------------
draw_titlebar:
        BOX     0,0,512,10,C_BLU
        TXT     4,1,0,C_WHT,C_BLU,0,"X86MIDI  V1.4.26T 261108"
        TXT     396,1,0,C_WHT,C_BLU,0,"OUT"
        LINEBOX 412,1,8,8,C_WHT
        BOX     414,3,4,4,C_YEL
        TXT     424,1,0,C_WHT,C_BLU,0,"MSX-MIDI E8h"
        ld      hl,(t_tw)
        ld      de,424+4
        add     hl,de           ; 글자 끝 + 4 에 아래쪽 삼각형 (▼)
        push    hl
        ld      d,3
        ld      e,6
        ld      a,C_WHT
        call    fill_run
        pop     hl
        push    hl
        inc     hl
        ld      d,4
        ld      e,4
        ld      a,C_WHT
        call    fill_run
        pop     hl
        inc     hl
        inc     hl
        ld      d,5
        ld      e,2
        ld      a,C_WHT
        jp      fill_run

; ---------------------------------------------------------------
;  시간 칸 (4,12  126 x 38)
; ---------------------------------------------------------------
draw_time_panel:
        PANEL   4,12,126,38,C_PN,C_LN
        TXT     8,13,0,C_LN,C_PN,0,"TIME"
        ret

; ---------------------------------------------------------------
;  곡 정보 칸 (134,12  166 x 38) : 4줄 가운데 정렬
; ---------------------------------------------------------------
draw_info_panel:
        PANEL   134,12,166,38,C_PN,C_LN
        ret

; ---------------------------------------------------------------
;  채널 활동 칸 (304,12  204 x 38) : 16채널 막대
; ---------------------------------------------------------------
draw_activity_panel:
        PANEL   304,12,204,38,C_PN,C_LN
        TXT     308,13,0,C_LN,C_PN,0,"CHANNEL ACTIVITY"
        xor     a
        ld      (loop_ch),a
dap_l:  call    draw_activity_bar
        ld      hl,loop_ch
        inc     (hl)
        ld      a,(hl)
        cp      16
        jr      c,dap_l
        ret

; loop_ch 번 채널의 막대 하나 + 번호 (x = 308 + 12 * ch, 바닥 y = 38)
draw_activity_bar:
        call    act_bar
        jp      act_num

; 막대만 (번호는 그대로)
act_bar:
        ld      a,(loop_ch)
        ld      l,a
        ld      h,0
        ld      e,l
        ld      d,h
        add     hl,hl
        add     hl,de
        add     hl,hl
        add     hl,hl           ; * 12
        ld      de,308
        add     hl,de
        ld      (bar_x),hl
        ld      (r_x),hl
        ld      hl,25           ; 막대 자리를 먼저 패널색으로 지운다
        ld      (r_y),hl
        ld      hl,9
        ld      (r_w),hl
        ld      hl,13
        ld      (r_h),hl
        ld      a,C_PN
        ld      (r_c),a
        call    rect_go
        call    channel_level   ; A = 0..38
        ld      (bar_lv),a
        call    level_to_height ; A = 0..13
        or      a
        ret     z
        ld      c,a
        ld      a,38
        sub     c
        ld      l,a
        ld      h,0
        ld      (r_y),hl
        ld      a,c
        ld      l,a
        ld      (r_h),hl
        ld      hl,(bar_x)
        ld      (r_x),hl
        ld      hl,9
        ld      (r_w),hl
        call    channel_color
        ld      (r_c),a
        jp      rect_go

; 채널 번호 (소리가 나면 채널 색, 아니면 흐린색).  act_bar 가 정한 bar_x, bar_lv 를 쓴다
act_num:ld      b,C_LN
        ld      a,(bar_lv)
        or      a
        jr      z,dab_c
        call    channel_color
        ld      b,a
dab_c:  ld      c,C_PN
        ld      hl,(bar_x)
        ld      a,39
        ld      de,10
        call    text_at
        ld      a,7             ; 숫자 높이 7줄
        ld      (t_rows),a
        ld      a,1
        ld      (t_al),a
        ld      a,(loop_ch)
        inc     a
        ld      de,numbuf
        call    dec2
        ld      hl,numbuf
        jp      txtp

; ---- 플레이어 화면의 나머지 부분 (위에서 아래 순서) ----
        include "scr_seek.inc"
        include "scr_transport.inc"
        include "scr_sliders.inc"
        include "scr_channels.inc"
        include "scr_piano.inc"
        include "scr_bottom.inc"
        include "scr_help.inc"
