; ============================================================
; 만든이: KimTaeWoo
; 도와준이: Lee.J.H
;  scr_setup.inc - F5 설정 화면
;    PORT  : MSX-MIDI(E8h) / 없음(소리 안 냄)   (X86 카트리지 · SFG-05 · 내장은 아직 지원 안 함)
;    RESET : 없음 / GM / GS / 곡에 맡김
;    DIAG  : 진단 기록 켜기/끄기 (명령줄 /D 와 같은 스위치)
;  위/아래 = 항목 고르기, 왼쪽/오른쪽 = 값 바꾸기, ESC = 플레이어로
; ============================================================
SETUP_ITEMS equ 3
setup_sel:   defb 0
setup_dirty: defb 2             ; 2 = 전체, 1 = 값만

; 값 이름표: 항목마다 같은 칸 크기로 맞춘다 (번호 * 칸 크기 로 찾는다)
PORT_NAME_LEN equ 24
port_names:
pn0:    defb "MSX-MIDI/x86midi (E8h)",0 ; x86midi 카트리지도 MSX-MIDI 와 같은 방식(E8h)
        defs PORT_NAME_LEN-($-pn0)
pn1:    defb "(x86midi = MSX-MIDI)",0   ; 따로 고르지 않는다 (위 값과 같다)
        defs PORT_NAME_LEN-($-pn1)
pn2:    defb "SFG-05 (unsupported)",0
        defs PORT_NAME_LEN-($-pn2)
pn3:    defb "Internal (unsupported)",0
        defs PORT_NAME_LEN-($-pn3)
pn4:    defb "NONE (silent)",0
        defs PORT_NAME_LEN-($-pn4)
RESET_NAME_LEN equ 8
reset_names:
rn0:    defb "OFF",0
        defs RESET_NAME_LEN-($-rn0)
rn1:    defb "GM",0
        defs RESET_NAME_LEN-($-rn1)
rn2:    defb "GS",0
        defs RESET_NAME_LEN-($-rn2)
rn3:    defb "BY SONG",0
        defs RESET_NAME_LEN-($-rn3)
ONOFF_NAME_LEN equ 4
onoff_names:
on0:    defb "OFF",0
on1:    defb "ON",0
        defs ONOFF_NAME_LEN-($-on1)

timing_tick:  defb "1MS TIMER",0
timing_auto:  defb "FRAME (AUTO: TOO BUSY)",0
timing_frame: defb "FRAME (60/50HZ)",0

scr_setup:
        TXT     4,13,0,C_LN,C_BG,0,"SETUP"
        BOX     4,22,504,1,C_LN
        TXT     40,40,0,C_LN,C_BG,0,"MIDI PORT"
        TXT     40,64,0,C_LN,C_BG,0,"RESET ON SONG START"
        TXT     40,88,0,C_LN,C_BG,0,"DIAGNOSTIC LOG"
        TXT     40,112,0,C_LN,C_BG,0,"TIMING"
        TXT     40,140,0,C_LN,C_BG,0,"UP/DOWN item   LEFT/RIGHT change   ESC back"
        ld      a,2
        ld      (setup_dirty),a
        jp      setup_draw

; 설정 값 글자(오른쪽 열)의 자리와 모양: x = 300, 너비 200, 왼쪽 정렬, 보통 크기, 바탕 C_BG.  y 와 글자색은 부른 쪽이 정한다
setup_value_begin:
        ld      hl,300
        ld      (t_x),hl
        ld      hl,200
        ld      (t_w),hl
        xor     a
        ld      (t_al),a
        ld      (t_sh),a
        ld      a,C_BG
        ld      (t_bg),a
        ret

setup_draw:
        xor     a
        ld      (setup_dirty),a
        xor     a
        ld      (loop_ch),a
sd_l:   ld      a,(loop_ch)     ; 줄 y = 40 + 24 * i
        ld      c,a
        add     a,a
        add     a,c
        add     a,a
        add     a,a
        add     a,a
        ld      (t_y),a         ; 일단 24 * i 의 절반 (i*24 = i*3*8)
        ld      a,(t_y)
        add     a,40
        ld      (t_y),a
        call    setup_value_begin
        ld      a,(loop_ch)     ; 고른 항목은 노란색
        ld      c,a
        ld      a,(setup_sel)
        cp      c
        ld      a,C_TX
        jr      nz,sd_c
        ld      a,C_YEL
sd_c:   ld      (t_fg),a
        call    setup_value_text        ; HL = 글자
        xor     a
        call    txtv
        ld      hl,loop_ch
        inc     (hl)
        ld      a,(hl)
        cp      SETUP_ITEMS
        jr      c,sd_l
        ; 고르는 항목은 아니고 보여 주기만 하는 줄: 박자를 세는 방식 (1ms 타이머 / 프레임)
        ld      a,112
        ld      (t_y),a
        call    setup_value_begin
        ld      a,C_TX
        ld      (t_fg),a
        ld      hl,timing_frame
        ld      a,(tick_on)
        or      a
        jr      z,sd_off
        ld      hl,timing_tick
        jr      sd_tm
sd_off: ld      a,(tick_state)          ; 워치독이 껐으면 그렇다고 알린다
        cp      5
        jr      nz,sd_tm
        ld      hl,timing_auto
sd_tm:  xor     a
        call    txtv
        jp      setup_stats_draw

; 1ms 타이머 점검 숫자 (실기에서 타이머가 제대로 도는지 화면에서 바로 본다)
;   STATE 1=사용 중 2=감지 실패 3=/F 로 끔 4=포트 없음,  P10 = 잰 간격 (0.1us, 1ms = 10000),
;   IRQ = 처리한 인터럽트 수 (1초에 약 1000 씩 늘어야 한다, 65535 를 넘으면 처음부터), LOST = 놓쳐서 채워 넣은 수
txt_state:  defb "STATE ",0
txt_p10:    defb "   P10 ",0
txt_raw:    defb "   MEASURED ",0
txt_irq:    defb "IRQ ",0
txt_lost:   defb "   LOST ",0
txt_lostp:  defb "LOST NOW ",0
txt_pct:    defb " %",0
stats_last: defb 0
stats_prev_irq:  defw 0
stats_prev_lost: defw 0
stats_d_irq:     defw 0
stats_d_lost:    defw 0
stats_pct:       defb 0
setup_stats_draw:
        call    sb_reset
        ld      hl,txt_state
        call    sb_str
        ld      a,(tick_state)
        call    sb_dec
        ld      hl,txt_p10
        call    sb_str
        ld      hl,(tick_p10)
        call    sb_dec16
        ld      hl,txt_raw
        call    sb_str
        ld      hl,(tick_p10_raw)
        call    sb_dec16
        call    sb_done
        TXTD    40,154,420,C_TX,C_BG,0
        call    sb_reset
        ld      hl,txt_irq
        call    sb_str
        ld      hl,(tick_irqs)
        call    sb_dec16
        ld      hl,txt_lost
        call    sb_str
        ld      hl,(tick_lost)
        call    sb_dec16
        call    sb_done
        TXTD    40,166,420,C_TX,C_BG,0
        call    sb_reset
        ld      hl,txt_lostp
        call    sb_str
        ld      a,(stats_pct)
        call    sb_dec
        ld      hl,txt_pct
        call    sb_str
        call    sb_done
        TXTD    40,178,420,C_TX,C_BG,0
        ret

; 숫자를 약 0.5초마다 다시 그린다.  "LOST NOW" = 바로 앞 0.5초 동안 놓친 비율 (놓침 / (처리 + 놓침))
setup_stats_refresh:
        ld      a,(play_frames)
        and     0E0h
        ld      hl,stats_last
        cp      (hl)
        ret     z
        ld      (hl),a
        ld      hl,(tick_irqs)          ; 이번 구간에 늘어난 수
        ld      de,(stats_prev_irq)
        ld      (stats_prev_irq),hl
        or      a
        sbc     hl,de
        ld      (stats_d_irq),hl
        ld      hl,(tick_lost)
        ld      de,(stats_prev_lost)
        ld      (stats_prev_lost),hl
        or      a
        sbc     hl,de
        ld      (stats_d_lost),hl
        ld      de,(stats_d_irq)
        add     hl,de                   ; HL = 처리 + 놓침
        xor     a
        ld      b,a
        or      h
        or      l
        jr      z,ssr_set               ; 아무것도 없으면 0 %
        ld      (v_d),hl
        ld      hl,0
        ld      (v_d+2),hl
        ld      hl,(stats_d_lost)
        ld      (v_k),hl
        ld      hl,0
        ld      (v_k+2),hl
        ld      hl,100
        call    mul32_by_hl
        call    div32
        ld      a,(v_n)
        ld      b,a
ssr_set:ld      a,b
        ld      (stats_pct),a
        jp      setup_stats_draw

; loop_ch 번째 항목의 값 글자 -> HL
setup_value_text:
        ld      a,(loop_ch)
        or      a
        jr      z,sv_port
        dec     a
        jr      z,sv_reset
        ld      a,(diag_on)
        ld      b,ONOFF_NAME_LEN
        ld      hl,onoff_names
        jr      sv_pick
sv_port:ld      a,(set_port)
        ld      b,PORT_NAME_LEN
        ld      hl,port_names
        jr      sv_pick
sv_reset:
        ld      a,(set_reset)
        ld      b,RESET_NAME_LEN
        ld      hl,reset_names
sv_pick:ld      e,b
        ld      d,0
        or      a
sv_m:   ret     z
        add     hl,de
        dec     a
        jr      sv_m

setup_keys:
        ld      a,(setup_dirty)
        or      a
        call    nz,setup_draw
        call    setup_stats_refresh
        ld      a,(key_rpt8)            ; 위 / 아래는 누르고 있으면 반복
        bit     5,a
        ld      c,-1
        jr      nz,sk_move
        bit     6,a
        ld      c,1
        jr      nz,sk_move
        ld      a,(key_new+8)           ; 왼쪽 / 오른쪽은 값을 바꾸므로 한 번씩만
        bit     4,a
        ld      c,-1
        jr      nz,sk_change
        bit     7,a
        ld      c,1
        jr      nz,sk_change
        ld      a,(key_new+7)
        bit     2,a
        ret     z
        xor     a
        jp      screen_switch

sk_move:ld      a,(setup_sel)
        add     a,c
        jp      m,sk_n
        cp      SETUP_ITEMS
        jr      c,sk_s
        ld      a,SETUP_ITEMS-1
        jr      sk_s
sk_n:   xor     a
sk_s:   ld      (setup_sel),a
        ld      a,1
        ld      (setup_dirty),a
        ret

; C = -1 / +1
sk_change:
        ld      a,(setup_sel)
        or      a
        jr      z,sk_port
        dec     a
        jr      z,sk_reset
        ld      a,(diag_on)     ; DIAG 켜기/끄기
        xor     1
        ld      (diag_on),a
        jr      sk_ok
sk_port:ld      a,(set_port)    ; 값은 두 가지: MSX-MIDI/x86midi(0) 와 NONE(4)
        cp      PORT_MSXMIDI
        ld      a,PORT_NONE
        jr      z,sk_pp
        ld      a,PORT_MSXMIDI
sk_pp:  ld      (set_port),a
        call    ctl_panic
        call    midi_init
        jr      sk_ok
sk_reset:
        ld      a,(set_reset)
        add     a,c
        jp      m,sk_r0
        cp      4
        jr      c,sk_rs
        ld      a,3
        jr      sk_rs
sk_r0:  xor     a
sk_rs:  ld      (set_reset),a
sk_ok:  ld      a,1
        ld      (setup_dirty),a
        jp      setup_draw
