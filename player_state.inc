; ============================================================
; 만든이: KimTaeWoo
; 도와준이: Lee.J.H
;  player_state.inc - 플레이어 화면이 읽는 상태 (지금은 시안과 같은 시험 값)
;  나중에 MIDI 엔진이 이 변수들을 갱신하고 화면은 그대로 둔다 (화면 <-> 엔진 분리)
; ============================================================
; 그리기 도우미가 쓰는 임시 변수
loop_ch:  defb  0
bar_x:    defw  0
bar_lv:   defb  0
bt_x:     defw  0
bt_fill:  defb  0
bt_line:  defb  0
bt_icon:  defb  0
tg_s:     defw  0
tg_bg:    defb  0
tg_fg:    defb  0
sl_x:     defw  0
sl_lbl:   defw  0
sl_val:   defw  0
sl_pos:   defb  0
sl_ctr:   defb  0
row_x:    defw  0
row_y:    defb  0
row_bg:   defb  0
row_sel:  defb  0
row_used: defb  0
num_fg:   defb  0
ms_x:     defw  0
ms_fill:  defb  0
ms_on:    defb  0
bk_c:     defb  0
lab_idx:  defb  0
fk_x:     defw  0
fk_bg:    defb  0
fk_key_fg:defb  0
fk_name_fg:defb 0
fk_key:   defw  0
fk_name:  defw  0
fk_wk:    defw  0
fk_wn:    defw  0
fk_gx:    defw  0

; ---- 화면 상태 ----
btn_active: defb 0              ; 눌린 아이콘 버튼 0..6 (2 = 재생)
tg_ab:    defb  0               ; A-B 반복
tg_rep:   defb  1                ; 기본: 한 곡 반복 (R 키로 끄고 켠다)
tg_shuf:  defb  0
tg_next:  defb  0
sl_tempo: defb  40              ; 손잡이 위치 0..80
sl_key:   defb  40
sl_vol:   defb  64
fkey_active: defb 0             ; 0 = F1 PLAYER
ch_sel:   defb  0               ; 선택된 채널 (0 = CH01)
solo_any: defb  0               ; 단독(S) 채널이 하나라도 있으면 1
seek_px:  defw 0                ; 탐색 막대 진행 칸 수 (0..498)
ab_a_px:  defw 0
ab_b_px:  defw 0
sl_dirty: defb 1                ; 1 = 슬라이더 값을 다시 계산해 그린다
st_dirty:  defb 1               ; 1 = 상태 줄(선택 채널 세부)을 다시 그려야 함
cur_screen: defb 0              ; 0 플레이어, 1 롤, 2 가사, 3 파일, 4 설정

; ---- 설정 (설정 화면이 바꾼다) ----
; 곡이 템포를 정하기 전의 기본 템포 = 500000 us / 4분음표 = 07A120h  (어셈블러가 16비트로 계산하므로 낮은 16비트와 높은 바이트로 나눠 둔다)
TEMPO_DEFAULT_LO equ 0A120h
TEMPO_DEFAULT_HI equ 07h
PORT_MSXMIDI equ 0
PORT_NONE    equ 4            ; (1..3 은 비워 둔다: 옛 번호가 다른 뜻으로 쓰이지 않게)
set_port:  defb PORT_MSXMIDI
set_reset: defb 3               ; 0 안 보냄, 1 GM, 2 GS, 3 곡 파일에 맡김
diag_on:   defb 0               ; 진단 스위치 (명령줄 /D)

; ---- 채널별 상태 16개 ----
ch_level: defs  16              ; 0..38 소리 크기 막대 (소리가 나면 오르고 프레임마다 줄어든다)
ch_level_drawn: defs 16         ; 표 줄에 그려 둔 막대 값 (바뀐 것만 다시 그린다)
ch_act_drawn: defs 16           ; 활동 막대에 그려 둔 높이
ch_vol:   defs  16
ch_pan:   defs  16              ; MIDI 팬 0..127 (64 = 가운데)
ch_prg:   defs  16              ; 프로그램 0..127 (화면에는 +1)
ch_bank:  defs  16
ch_mod:   defs  16
ch_exp:   defs  16
ch_rev:   defs  16
ch_cho:   defs  16
ch_bend:  defs  16              ; 피치벤드 위쪽 7비트 (64 = 가운데)
ch_flags: defs  16              ; bit0 = 쓰는 채널, bit1 = 프로그램 확인됨
ch_mute:  defs  16
ch_solo:  defs  16
ch_dirty: defs  16              ; 1 = 표의 이 줄을 다시 그려야 함
ch_row_style: defs 16           ; 이 줄을 마지막에 그린 모양 (선택 * 2 + 쓰는 채널). FFh = 아직 안 그림
nm_none:  defb  "--",0
; 채널 번호 -> 칩 색 (시안: 1~9 는 12색 앞쪽, 10 = 흰색, 11 = 분홍, 12 = 황갈색, 13~16 은 1~4 와 같은 색)
chcol_tbl: defb C_RED,C_ORG,C_YEL,C_LIM,C_GRN,C_TEAL,C_SKY,C_BLU,C_VIO,C_WHT,C_PNK,C_TAN,C_RED,C_ORG,C_YEL,C_LIM

; ---- 건반 상태 88개 : FFh = 안 눌림, 그 밖 = 칠할 색 ----
key_state: defs 88,0FFh
key_dirty: defs 88

; ---------------------------------------------------------------
;  loop_ch 번째 채널의 값을 꺼내는 도우미 (화면은 이것만 부른다)
; ---------------------------------------------------------------
; HL = 16바이트 표 -> A = 표[loop_ch]
ch_pick:
        ld      a,(loop_ch)
        ld      e,a
        ld      d,0
        add     hl,de
        ld      a,(hl)
        ret
channel_level:
        ld      hl,ch_level
        jr      ch_pick
channel_volume:
        ld      hl,ch_vol
        jr      ch_pick
channel_pan:
        ld      hl,ch_pan
        jr      ch_pick
channel_program:
        ld      hl,ch_prg
        call    ch_pick
        inc     a               ; 화면에는 1 부터
        ret
; NZ = 소거
channel_muted:
        ld      hl,ch_mute
        call    ch_pick
        or      a
        ret
channel_solo:
        ld      hl,ch_solo
        call    ch_pick
        or      a
        ret
; A = 0 이면 쓰지 않는 채널
channel_used:
        ld      hl,ch_flags
        call    ch_pick
        and     1
        ret
; HL = 이름 글자 (드럼 채널은 드럼 세트 이름)
channel_name:
        call    channel_used
        ld      hl,nm_none
        ret     z
        ld      hl,ch_prg
        call    ch_pick
        ld      e,a
        ld      a,(loop_ch)
        cp      9
        jr      z,cn_kit
        ld      d,0
        ld      hl,gm_name_tbl
        jr      cn_idx
cn_kit: ld      a,e             ; 드럼 세트 = 프로그램 / 8
        rrca
        rrca
        rrca
        and     7
        ld      e,a
        ld      d,0
        ld      hl,kit_name_tbl
cn_idx: add     hl,de
        add     hl,de
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        ex      de,hl
        ret
; A = 채널 색
channel_color:
        ld      a,(loop_ch)
        ld      e,a
        ld      d,0
        ld      hl,chcol_tbl
        add     hl,de
        ld      a,(hl)
        ret
; A = 막대 높이 0..38 -> A = 0..13  ((A * 3 + 4) / 9)
level_to_height:
        ld      c,a
        add     a,a
        add     a,c
        add     a,4
        ld      c,0
lh_l:   sub     9
        jr      c,lh_d
        inc     c
        jr      lh_l
lh_d:   ld      a,c
        ret

; ---------------------------------------------------------------
;  엔진이 부르는 상태 갱신 (화면 쪽 표시 전용 변수만 바꾼다)
; ---------------------------------------------------------------
; 곡이 바뀌거나 처음부터 연주할 때 채널 상태 초기화
channels_reset:
        ld      hl,ch_level
        ld      de,ch_level+1
        ld      bc,16*3-1
        ld      (hl),0
        ldir                    ; ch_level, ch_level_drawn, ch_act_drawn
        ld      hl,ch_vol
        ld      b,16
cr_vol: ld      (hl),100
        inc     hl
        djnz    cr_vol
        ld      hl,ch_pan
        ld      b,16
cr_pan: ld      (hl),64
        inc     hl
        djnz    cr_pan
        ld      hl,ch_bend
        ld      b,16
cr_bnd: ld      (hl),64
        inc     hl
        djnz    cr_bnd
        ld      hl,ch_exp
        ld      b,16
cr_exp: ld      (hl),127
        inc     hl
        djnz    cr_exp
        ld      hl,ch_rev
        ld      b,16
cr_rev: ld      (hl),40
        inc     hl
        djnz    cr_rev
        ld      hl,ch_bank
        ld      de,ch_bank+1
        ld      bc,16-1
        ld      (hl),0
        ldir
        ld      hl,ch_mod
        ld      de,ch_mod+1
        ld      bc,16-1
        ld      (hl),0
        ldir
        ld      hl,ch_cho
        ld      de,ch_cho+1
        ld      bc,16-1
        ld      (hl),0
        ldir
        ld      hl,ch_dirty
        ld      b,16
cr_dty: ld      (hl),1
        inc     hl
        djnz    cr_dty
        jr      keys_clear

; 눌린 건반 모두 해제 (다시 그리기 표시)
keys_clear:
        ld      hl,key_state
        ld      b,88
kc_l:   ld      a,(hl)
        cp      0FFh
        jr      z,kc_n
        ld      (hl),0FFh
        push    hl
        ld      de,key_dirty-key_state
        add     hl,de
        ld      (hl),1
        pop     hl
kc_n:   inc     hl
        djnz    kc_l
        ret

; A = 소리 낸 건반 음번호 -> 지금 채널 색으로 켠다
key_light:
        sub     21
        ret     c
        cp      88
        ret     nc
        ld      e,a
        ld      d,0
        ld      a,(cur_ch)
        ld      c,a
        ld      hl,chcol_tbl
        ld      b,0
        add     hl,bc
        ld      c,(hl)
        ld      hl,key_state
        add     hl,de
        ld      (hl),c
        ld      hl,key_dirty
        add     hl,de
        ld      (hl),1
        ret
key_dark:
        sub     21
        ret     c
        cp      88
        ret     nc
        ld      e,a
        ld      d,0
        ld      hl,key_state
        add     hl,de
        ld      (hl),0FFh
        ld      hl,key_dirty
        add     hl,de
        ld      (hl),1
        ret

; A = 벨로시티 -> cur_ch 막대 = max(현재, (A * 3 + 5) / 10)
level_hit:
        ld      c,a
        add     a,a
        add     a,c
        add     a,5
        ld      c,0
lhit_l: sub     10
        jr      c,lhit_d
        inc     c
        jr      lhit_l
lhit_d: ld      a,c
        cp      39
        jr      c,lhit_c
        ld      a,38
lhit_c: ld      b,a
        ld      a,(cur_ch)
        ld      e,a
        ld      d,0
        ld      hl,ch_level
        add     hl,de
        ld      a,(hl)
        cp      b
        jr      nc,lhit_f
        ld      (hl),b
lhit_f: ld      hl,ch_flags     ; 처음 소리가 난 채널이면 표 줄을 다시 그린다
        add     hl,de
        bit     0,(hl)
        ret     nz
        set     0,(hl)
        ld      hl,ch_dirty
        add     hl,de
        ld      (hl),1
        ret

; 프레임마다 막대를 한 칸씩 줄인다
levels_decay:
        ld      hl,ch_level
        ld      b,16
ld_l:   ld      a,(hl)
        or      a
        jr      z,ld_n
        dec     (hl)
ld_n:   inc     hl
        djnz    ld_l
        ret

; M / S 글자 5 x 5 점그림
bmp_m:  defw    08800h,0D800h,0A800h,08800h,08800h
bmp_s:  defw    07800h,08000h,07000h,00800h,0F000h

icon_tbl:
        defw    icon_prev,icon_rew,icon_play,icon_pause,icon_stop,icon_ff,icon_next
