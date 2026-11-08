; ============================================================
; 만든이: KimTaeWoo
; 도와준이: Lee.J.H
;  player_ui.inc - 플레이어 화면의 살아 있는 부분
;    ui_first  : 처음 그림 위에 값(제목, 곡 정보, 시간 ...)을 채운다
;    ui_update : 프레임마다 부른다.  바뀐 것만 다시 그린다 (글자 그리기는 느려서 꼭 필요할 때만)
;  화면에 그려 둔 값은 ui_*_drawn 에 적어 두었다가 달라졌을 때만 그린다.
; ============================================================
song_loaded:     defb 0         ; 1 = 곡이 열려 있다
ui_tick:         defb 0
ui_sec_drawn:    defw 0FFFFh
ui_state_drawn:  defb 0FFh
ui_bar_drawn:    defw 0FFFFh
ui_beat_drawn:   defb 0FFh
ui_poly_drawn:   defb 0FFh
ui_btn_drawn:    defb 0FFh
ui_tempo_drawn:  defb 0FFh,0FFh,0FFh
tot_sec:         defw 0
bpm_frac:        defb 0
ROWS_PER_FRAME   equ 2          ; 한 프레임에 다시 그리는 표 줄 수 상한

; ---------------------------------------------------------------
;  처음 그림
; ---------------------------------------------------------------
ui_first:
        call    ui_invalidate
        call    ui_title
        call    ui_info
        call    ui_time
        call    ui_bar
        call    ui_seek
        call    ui_poly
        call    ui_status
        call    ui_buttons
        jp      ui_sliders

ui_invalidate:
        ld      hl,0FFFFh
        ld      (ui_sec_drawn),hl
        ld      (ui_bar_drawn),hl
        ld      a,0FFh
        ld      (ui_state_drawn),a
        ld      (ui_beat_drawn),a
        ld      (ui_poly_drawn),a
        ld      (ui_btn_drawn),a
        ld      (ui_tempo_drawn),a
        ld      hl,0FFFFh
        ld      (seek_px),hl
        ret

; ---------------------------------------------------------------
;  프레임마다
; ---------------------------------------------------------------
ui_update:
        call    ui_keys
        call    ui_levels
        call    ui_rows
        call    ui_status_if
        call    ui_buttons
        ld      a,(sl_dirty)
        or      a
        call    nz,ui_sliders
        ld      a,(tempo_dirty)
        or      a
        call    nz,ui_info_tempo
        ld      hl,ui_tick
        inc     (hl)
        ld      a,(hl)
        and     3               ; 4 프레임에 한 번씩, 프레임마다 하나만: 0 시간 / 1 탐색 막대 / 2 동시 발음 수 / 3 마디·박
        jp      z,ui_time       ;  (한 프레임에 몰아 그리면 그 프레임이 길어져 곡이 느려지고 끊긴다)
        dec     a
        jp      z,ui_seek
        dec     a
        jp      z,ui_poly
        jp      ui_bar          ; 3: 마디 / 박

; 눌림이 바뀐 건반만 다시 그린다
ui_keys:
        ld      hl,key_dirty
        ld      b,KEY_COUNT
        ld      c,0
uk_l:   ld      a,(hl)
        or      a
        jr      z,uk_n
        ld      (hl),0
        push    hl
        push    bc
        ld      a,c
        call    piano_redraw_key
        pop     bc
        pop     hl
uk_n:   inc     hl
        inc     c
        djnz    uk_l
        ret

; 소리 크기: 활동 막대(높이가 바뀌면) + 표 줄 막대(값이 바뀌면)
ui_levels:
        xor     a
        ld      (loop_ch),a
ul_l:   ld      a,(loop_ch)
        ld      e,a
        ld      d,0
        ld      hl,ch_level
        add     hl,de
        ld      a,(hl)
        push    af
        call    level_to_height
        ld      c,a             ; C = 새 높이
        ld      hl,ch_act_drawn
        add     hl,de
        ld      a,(hl)
        cp      c
        jr      z,ul_row
        ld      (hl),c
        or      a               ; A = 그려 둔 높이: 0 <-> 0 아님이 바뀌면 번호 색도 바뀐다
        jr      z,ul_full
        inc     c
        dec     c
        jr      z,ul_full
        call    act_bar
        jr      ul_row
ul_full:call    draw_activity_bar
ul_row: pop     af              ; A = 새 level
        ld      a,(loop_ch)
        ld      e,a
        ld      d,0
        ld      hl,ch_level
        add     hl,de
        ld      b,(hl)
        ld      hl,ch_level_drawn
        add     hl,de
        ld      a,(hl)
        cp      b
        jr      z,ul_n
        ld      (hl),b
        call    redraw_row_level
ul_n:   ld      hl,loop_ch
        inc     (hl)
        ld      a,(hl)
        cp      16
        jr      c,ul_l
        ret

; 바뀐 표 줄을 다시 그린다 (한 프레임에 ROWS_PER_FRAME 줄까지)
ui_rows:
        ld      a,ROWS_PER_FRAME
        ld      (ur_budget),a
        xor     a
        ld      (loop_ch),a
ur_l:   ld      a,(loop_ch)
        ld      e,a
        ld      d,0
        ld      hl,ch_dirty
        add     hl,de
        ld      a,(hl)
        or      a
        jr      z,ur_n
        ld      (hl),0
        ld      hl,ch_level
        add     hl,de
        ld      a,(hl)
        ld      hl,ch_level_drawn
        add     hl,de
        ld      (hl),a
        ld      a,(ch_sel)      ; 선택 채널이 바뀌었으면 상태 줄도
        ld      b,a
        ld      a,(loop_ch)
        cp      b
        jr      nz,ur_d
        ld      a,1
        ld      (st_dirty),a
ur_d:   call    draw_channel_row
        ld      hl,ur_budget
        dec     (hl)
        ret     z
ur_n:   ld      hl,loop_ch
        inc     (hl)
        ld      a,(hl)
        cp      16
        jr      c,ur_l
        ret
ur_budget: defb 0

; ---------------------------------------------------------------
;  제목 줄: 파일 이름 (경로 빼고)
; ---------------------------------------------------------------
ui_title:
        ld      hl,txt_nofile
        ld      a,(have_name)
        or      a
        jr      z,ut_d
        ld      a,(song_loaded)
        ld      hl,txt_loaderr
        or      a
        jr      z,ut_d          ; 이름은 있는데 못 읽었으면 알린다
        call    base_name
ut_d:   TXTD    120,1,272,C_WHT,C_BLU,1
        ret
txt_nofile: defb "(NO FILE)",0
txt_loaderr:defb "(LOAD ERROR)",0

; file_name 에서 \ 또는 : 뒤의 이름 -> HL
base_name:
        ld      hl,file_name
        ld      d,h
        ld      e,l
bn_l:   ld      a,(hl)
        or      a
        jr      z,bn_x
        inc     hl
        cp      '\'
        jr      z,bn_s
        cp      ':'
        jr      nz,bn_l
bn_s:   ld      d,h
        ld      e,l
        jr      bn_l
bn_x:   ex      de,hl
        ret

; ---------------------------------------------------------------
;  곡 정보 칸 (4줄)
; ---------------------------------------------------------------
ui_info:
        call    ui_info_title
        call    ui_info_format
        call    ui_info_tempo
        jp      ui_info_lyrics

ui_info_title:
        ld      hl,txt_empty
        ld      a,(song_loaded)
        or      a
        jr      z,uit_d
        ld      a,(title_buf)
        or      a
        ld      hl,title_buf
        jr      nz,uit_d
        call    base_name
uit_d:  TXTD    135,13,164,C_TX,C_PN,1
        ret
txt_empty: defb 0

; "SMF-1  12TRK  PPQ480  GM"
ui_info_format:
        call    sb_reset
        ld      a,(song_loaded)
        or      a
        jr      z,uif_d
        ld      hl,txt_smf
        call    sb_str
        ld      a,(smf_fmt)
        call    sb_dec
        ld      hl,txt_2sp
        call    sb_str
        ld      a,(smf_ntrk)
        call    sb_dec
        ld      hl,txt_trk
        call    sb_str
        ld      hl,(smf_ppq)
        call    sb_dec16
        ld      hl,txt_2sp
        call    sb_str
        ld      a,(sys_type)
        add     a,a
        add     a,a             ; * 4
        ld      e,a
        ld      d,0
        ld      hl,sys_names
        add     hl,de
        call    sb_str
uif_d:  call    sb_done
        TXTD    135,22,164,C_LN,C_PN,1
        ret
txt_smf:  defb  "SMF-",0
txt_2sp:  defb  "  ",0
txt_trk:  defb  "TRK  PPQ",0
sys_names:defb  "--",0,0, "GM",0,0, "GS",0,0, "XG",0,0

; "BPM 120.0   4/4   C maj"
ui_info_tempo:
        xor     a
        ld      (tempo_dirty),a
        ld      hl,tempo_us
        ld      de,ui_tempo_drawn
        ld      bc,3
        ldir
        call    sb_reset
        ld      a,(song_loaded)
        or      a
        jr      z,uit3_d
        call    calc_bpm10      ; bpm_int, bpm_frac
        ld      hl,txt_bpm
        call    sb_str
        ld      hl,(bpm_int)
        call    sb_dec16
        ld      a,'.'
        call    sb_chr
        ld      a,(bpm_frac)
        call    sb_dec
        ld      hl,txt_3sp
        call    sb_str
        ld      a,(ts_num)
        call    sb_dec
        ld      a,'/'
        call    sb_chr
        ld      a,(ts_den_pow)  ; 분모 = 2 ^ ts_den_pow
        ld      b,a
        ld      a,1
        inc     b
        dec     b
        jr      z,uit3_t
uit3_s: add     a,a
        djnz    uit3_s
uit3_t: call    sb_dec
        ld      hl,txt_3sp
        call    sb_str
        call    key_text
uit3_d: call    sb_done
        TXTD    135,31,164,C_TX,C_PN,1
        ret
txt_bpm:  defb  "BPM ",0
txt_3sp:  defb  "   ",0
bpm_int:  defw  0

; 지금 템포 -> bpm_int (정수), bpm_frac (소수 한 자리).  BPM * 10 = 600000000 / 템포(us)
calc_bpm10:
        ld      hl,tempo_us
; HL = 템포(마이크로초, 3바이트 낮은 쪽부터) 의 주소 -> bpm_int, bpm_frac
calc_bpm10_of:
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        inc     hl
        ld      a,(hl)
        ld      (v_d+2),a
        ld      (v_d),de
        xor     a
        ld      (v_d+3),a
        ld      hl,4600h        ; 600000000 = 23C34600h
        ld      (v_n),hl
        ld      hl,23C3h
        ld      (v_n+2),hl
        call    div32           ; v_n = BPM * 10
        ld      hl,10
        ld      (v_d),hl
        ld      hl,0
        ld      (v_d+2),hl
        call    div32
        ld      hl,(v_n)
        ld      (bpm_int),hl
        ld      a,(v_r)
        ld      (bpm_frac),a
        ret

; 조표 글자 붙이기: "C maj" / "A min" (없으면 "---")
key_text:
        ld      a,(key_known)
        or      a
        jr      nz,kt_k
        ld      hl,txt_nokey
        jp      sb_str
kt_k:   ld      a,(key_sf)
        add     a,7
        cp      15
        jr      c,kt_o
        ld      a,7
kt_o:   ld      c,a
        add     a,a
        add     a,c             ; * 3
        ld      e,a
        ld      d,0
        ld      hl,key_maj
        ld      a,(key_mi)
        or      a
        jr      z,kt_n
        ld      hl,key_min
kt_n:   add     hl,de
        push    af
        call    sb_str
        pop     af
        ld      hl,txt_maj
        jr      z,kt_s
        ld      hl,txt_min
kt_s:   jp      sb_str
txt_nokey:defb  "---",0
txt_maj:  defb  " maj",0
txt_min:  defb  " min",0
key_maj:  defb  "Cb",0,"Gb",0,"Db",0,"Ab",0,"Eb",0,"Bb",0,"F",0,0,"C",0,0,"G",0,0,"D",0,0,"A",0,0,"E",0,0,"B",0,0,"F#",0,"C#",0
key_min:  defb  "Ab",0,"Eb",0,"Bb",0,"F",0,0,"C",0,0,"G",0,0,"D",0,0,"A",0,0,"E",0,0,"B",0,0,"F#",0,"C#",0,"G#",0,"D#",0,"A#",0

ui_info_lyrics:
        call    sb_reset
        ld      a,(song_loaded)
        or      a
        jr      z,uil_d
        ld      hl,txt_lyr
        call    sb_str
        ld      hl,txt_yes
        ld      a,(has_lyrics)
        or      a
        jr      nz,uil_s
        ld      hl,txt_no
uil_s:  call    sb_str
uil_d:  call    sb_done
        TXTD    135,40,164,C_LN,C_PN,1
        ret
txt_lyr:  defb  "LYRICS ",0
txt_yes:  defb  "yes",0
txt_no:   defb  "no",0

; ---------------------------------------------------------------
;  시간 칸: 재생 상태, mm:ss, 전체 시간, BAR
; ---------------------------------------------------------------
ui_time:
        ld      a,(play_state)
        ld      hl,ui_state_drawn
        cp      (hl)
        jr      z,utm_t
        ld      (hl),a
        ld      hl,txt_stopped
        or      a
        jr      z,utm_s
        ld      hl,txt_playing
        cp      PLAY_RUN
        jr      z,utm_s
        ld      hl,txt_paused
utm_s:  TXTD    66,13,60,C_YEL,C_PN,2
utm_t:  ld      hl,song_frames  ; 지금 위치
        call    frames_to_sec
        ld      de,(ui_sec_drawn)
        ld      a,l
        cp      e
        jr      nz,utm_n
        ld      a,h
        cp      d
        jr      z,utm_b
utm_n:  ld      (ui_sec_drawn),hl
        call    fmt_time
        TXTD2   30,21,46,C_TX,C_PN,0
        ld      hl,(tot_sec)    ; 전체 시간 "/ mm:ss"
        call    fmt_total
        TXTD    76,30,48,C_LN,C_PN,0
utm_b:  ret                     ; (마디 / 박은 ui_bar 가 다른 프레임에 따로 그린다)

; ---------------------------------------------------------------
;  곡을 읽는 동안 (훑기 진행 표시): 상태 칸에 LOADING, 진행 칸에 트랙 수만큼 채운다
; ---------------------------------------------------------------
ui_loading_begin:
        ld      hl,txt_loading
        TXTD    66,13,60,C_YEL,C_PN,2
        ld      hl,0
        jp      draw_seekbar

; 알림 루틴: ps_done_n / smf_ntrk 만큼 진행 칸을 채운다
ui_loading_progress:
        ld      a,(smf_ntrk)
        or      a
        ret     z
        ld      e,a
        ld      d,0
        ld      hl,498
        ld      bc,0
ulp_d:  or      a
        sbc     hl,de
        jr      c,ulp_m
        inc     bc
        jr      ulp_d
ulp_m:  ld      a,(ps_done_n)
        ld      hl,0
        or      a
        jr      z,ulp_go
ulp_a:  add     hl,bc
        dec     a
        jr      nz,ulp_a
ulp_go: ld      de,498
        push    hl
        or      a
        sbc     hl,de
        pop     hl
        jr      c,ulp_ok
        ld      hl,498
ulp_ok: jp      draw_seekbar
txt_loading: defb "LOADING",0

txt_playing: defb "PLAYING",0
txt_paused:  defb "PAUSED",0
txt_stopped: defb "STOPPED",0

; BAR 034.2 / 096
ui_bar:
        ld      a,(song_loaded)
        or      a
        ret     z
        call    calc_bar_beat   ; HL = 마디 (1 부터), A = 박 (1 부터)
        ld      (ub_bar),hl
        ld      (ub_beat),a
        ld      de,(ui_bar_drawn)
        ld      a,l
        cp      e
        jr      nz,ub_d
        ld      a,h
        cp      d
        jr      nz,ub_d
        ld      a,(ui_beat_drawn)
        ld      hl,ub_beat
        cp      (hl)
        ret     z
ub_d:   ld      hl,(ub_bar)
        ld      (ui_bar_drawn),hl
        ld      a,(ub_beat)
        ld      (ui_beat_drawn),a
        call    sb_reset
        ld      hl,txt_bar
        call    sb_str
        ld      hl,(ub_bar)
        call    sb_pad3_hl
        ld      a,'.'
        call    sb_chr
        ld      a,(ub_beat)
        call    sb_dec
        ld      hl,txt_of
        call    sb_str
        ld      hl,(total_bars)
        call    sb_pad3_hl
        call    sb_done
        TXTD    4,40,126,C_TX,C_PN,1
        ret
ub_bar:  defw   0
ub_beat: defb   0
txt_bar: defb "BAR ",0
txt_of:  defb " / ",0

; HL (0..999) 를 3자리(앞 0 포함)로 붙인다
sb_pad3_hl:
        ld      de,1000
        or      a
        sbc     hl,de
        add     hl,de
        jr      c,sp3_a
        ld      hl,999
sp3_a:  push    hl
        ld      de,100
        or      a
        sbc     hl,de
        jr      nc,sp3_n
        ld      a,'0'
        call    sb_chr
        pop     hl
        push    hl
        ld      de,10
        or      a
        sbc     hl,de
        jr      nc,sp3_n
        ld      a,'0'
        call    sb_chr
sp3_n:  pop     hl
        jp      sb_dec16

; 지금 위치(now_tick) -> HL = 마디, A = 박
calc_bar_beat:
        ld      hl,(now_tick)
        ld      (v_n),hl
        ld      hl,(now_tick+2)
        ld      (v_n+2),hl
        ld      hl,(ticks_per_bar)
        ld      (v_d),hl
        ld      hl,0
        ld      (v_d+2),hl
        call    div32
        ld      hl,(v_n)
        inc     hl
        push    hl              ; 마디 (1 부터)
        ld      hl,(v_r)        ; 마디 안의 틱 -> 박
        ld      (v_n),hl
        ld      hl,(v_r+2)
        ld      (v_n+2),hl
        ld      hl,(ticks_per_beat)
        ld      (v_d),hl
        ld      hl,0
        ld      (v_d+2),hl
        call    div32
        ld      a,(v_n)
        inc     a
        pop     hl
        ret

; HL = 32비트 프레임 수의 주소 -> HL = 초 (0..5999)
frames_to_sec:
        ld      de,v_n
        ld      bc,4
        ldir
        xor     a
        ld      (v_d+1),a
        ld      (v_d+2),a
        ld      (v_d+3),a
        ld      a,(fps)
        ld      (v_d),a
        call    div32
        ld      hl,(v_n+2)
        ld      a,h
        or      l
        jr      nz,fs_max
        ld      hl,(v_n)
        ld      de,6000
        or      a
        sbc     hl,de
        jr      nc,fs_max
        add     hl,de
        ret
fs_max: ld      hl,5999
        ret

; HL = 초 -> strbuf "mm:ss"
fmt_time:
        push    hl
        call    sb_reset
        pop     hl
fmt_mmss:
        ld      b,0
fm_l:   ld      de,60
        or      a
        sbc     hl,de
        jr      c,fm_d
        inc     b
        jr      fm_l
fm_d:   add     hl,de
        push    hl
        ld      a,b
        call    sb_pad2
        ld      a,':'
        call    sb_chr
        pop     hl
        ld      a,l
        call    sb_pad2
        jp      sb_done
; HL = 초 -> "/ mm:ss"
fmt_total:
        push    hl
        call    sb_reset
        ld      a,'/'
        call    sb_chr
        ld      a,' '
        call    sb_chr
        pop     hl
        jr      fmt_mmss

; ---------------------------------------------------------------
;  탐색 막대
; ---------------------------------------------------------------
ui_seek:
        call    calc_seek_px
        ld      de,(seek_px)
        ld      a,l
        cp      e
        jr      nz,us_d
        ld      a,h
        cp      d
        ret     z
us_d:   ld      (seek_old),de
        ld      (seek_px),hl
        ld      hl,(seek_old)           ; 막대를 지우지 않고 바뀐 곳만 덧그릴 수 있는 경우:
        ld      de,0FFFFh               ;  - 처음 그리는 게 아니고
        or      a
        sbc     hl,de
        jr      z,us_full
        ld      hl,(seek_old)           ;  - 왼쪽 테두리와 겹치지 않고 (옛 칸 >= 2)
        ld      de,2
        or      a
        sbc     hl,de
        jr      c,us_full
        ld      hl,(seek_old)           ;  - 앞으로 갔을 때 (되감기 / 건너뛰기는 전체 다시)
        ld      de,(seek_px)
        or      a
        sbc     hl,de
        jr      nc,us_full
        jp      draw_seekbar_step
us_full:jp      draw_seekbar

; 진행 칸 수 = 지금 프레임 * 498 / 전체 프레임 -> HL
calc_seek_px:
        ld      hl,song_frames
        jp      px_of_frames

; v_k (32비트 프레임) -> HL = 칸 수 (v_k * 498 / total_frames, 0..498)
px_from_vk:
        ld      hl,(total_frames)
        ld      a,h
        or      l
        ld      hl,(total_frames+2)
        or      h
        or      l
        ld      hl,0
        ret     z
        ld      hl,498
        call    mul32_by_hl
        ld      hl,(total_frames)
        ld      (v_d),hl
        ld      hl,(total_frames+2)
        ld      (v_d+2),hl
        call    div32
        ld      hl,(v_n+2)
        ld      a,h
        or      l
        jr      nz,csp_m
        ld      hl,(v_n)
        ld      de,499
        or      a
        sbc     hl,de
        jr      nc,csp_m
        add     hl,de
        ret
csp_m:  ld      hl,498
        ret

; ---------------------------------------------------------------
;  동시에 울리는 소리 수
; ---------------------------------------------------------------
ui_poly:
        ld      hl,key_state
        ld      b,KEY_COUNT
        ld      c,0
up_l:   ld      a,(hl)
        cp      0FFh
        jr      z,up_n
        inc     c
up_n:   inc     hl
        djnz    up_l
        ld      a,(ui_poly_drawn)
        cp      c
        ret     z
        ld      a,c
        ld      (ui_poly_drawn),a
        call    sb_reset
        ld      hl,txt_poly
        call    sb_str
        ld      a,(ui_poly_drawn)
        call    sb_pad3
        call    sb_done
        TXTD    430,13,74,C_TX,C_PN,2
        ret
txt_poly: defb  "POLY ",0

; ---------------------------------------------------------------
;  재생 버튼: 재생 상태에 맞춰 눌린 모양을 바꾼다 (재생 2, 일시정지 3, 정지 4)
; ---------------------------------------------------------------
ui_buttons:
        ld      a,(play_state)
        ld      b,4
        or      a
        jr      z,ub_s
        ld      b,2
        cp      PLAY_RUN
        jr      z,ub_s
        ld      b,3
ub_s:   ld      a,(ui_btn_drawn)
        cp      b
        ret     z
        ld      a,b
        ld      (ui_btn_drawn),a
        ld      (btn_active),a
        jp      draw_transport_icons

; ---------------------------------------------------------------
;  상태 줄: 선택한 채널의 세부 값
; ---------------------------------------------------------------
ui_status_if:
        ld      a,(st_dirty)
        or      a
        ret     z
ui_status:
        xor     a
        ld      (st_dirty),a
        ld      a,(ch_sel)
        ld      (loop_ch),a
        call    sb_reset
        ld      a,'C'
        call    sb_chr
        ld      a,'H'
        call    sb_chr
        ld      a,(ch_sel)
        inc     a
        call    sb_pad2
        call    sb_done
        TXTD    4,190,24,C_YEL,C_PN,0
        call    channel_name
        TXTD    30,190,78,C_TX,C_PN,0
        call    sb_reset
        ld      hl,txt_bank
        call    sb_str
        ld      hl,ch_bank
        call    ch_pick
        call    sb_pad3
        ld      hl,txt_prg
        call    sb_str
        call    channel_program
        call    sb_pad3
        ld      hl,txt_rev
        call    sb_str
        ld      hl,ch_rev
        call    ch_pick
        call    sb_dec
        ld      hl,txt_cho
        call    sb_str
        ld      hl,ch_cho
        call    ch_pick
        call    sb_dec
        ld      hl,txt_bend
        call    sb_str
        ld      hl,ch_bend
        call    ch_pick
        sub     64
        call    sb_signed
        ld      hl,txt_mod
        call    sb_str
        ld      hl,ch_mod
        call    ch_pick
        call    sb_dec
        ld      hl,txt_exp
        call    sb_str
        ld      hl,ch_exp
        call    ch_pick
        call    sb_dec
        call    sb_done
        TXTD    110,190,398,C_LN,C_PN,0
        ret
txt_bank: defb  "BANK ",0
txt_prg:  defb  "  PRG ",0
txt_rev:  defb  "  REV ",0
txt_cho:  defb  "  CHO ",0
txt_bend: defb  "  BEND ",0
txt_mod:  defb  "  MOD ",0
txt_exp:  defb  "  EXP ",0

; A = 부호 있는 값 -> "+n" / "-n"
sb_signed:
        bit     7,a
        jr      nz,ss_neg
        push    af
        ld      a,'+'
        call    sb_chr
        pop     af
        jp      sb_dec
ss_neg: neg
        push    af
        ld      a,'-'
        call    sb_chr
        pop     af
        jp      sb_dec

; ---------------------------------------------------------------
;  슬라이더 (TEMPO / KEY / VOL): 값 -> 손잡이 위치와 글자
; ---------------------------------------------------------------
vol_pct: defb   100             ; 마스터 볼륨 0..100 %

ui_sliders:
        xor     a
        ld      (sl_dirty),a
        ld      a,(tempo_pct)   ; TEMPO 25..200 % : 100 이 가운데(40)
        ld      hl,tempo_val
        call    put_dec3
        ld      (hl),'%'
        inc     hl
        ld      (hl),0
        call    tempo_to_pos
        ld      (sl_tempo),a
        ld      a,(transpose)   ; KEY -12..+12 : 0 이 가운데
        ld      hl,key_val
        call    put_signed
        call    key_to_pos
        ld      (sl_key),a
        ld      a,(vol_pct)     ; VOL 0..100 %
        ld      hl,vol_val
        call    put_dec3
        ld      a,(vol_pct)
        ld      l,a
        ld      h,0
        ld      b,4
        ld      c,5
        call    muldiv          ; pct * 4 / 5 = 0..80
        ld      a,l
        ld      (sl_vol),a
        jp      draw_sliders

; 템포 % (25..200) -> 손잡이 0..80 : 100 % = 40,  위쪽 100..200 -> 40..80,  아래쪽 100..25 -> 40..0
tempo_to_pos:
        ld      a,(tempo_pct)
        cp      100
        jr      c,ttp_lo
        sub     100
        ld      l,a
        ld      h,0
        ld      b,2
        ld      c,5
        call    muldiv          ; x * 2 / 5
        ld      a,40
        add     a,l
        cp      81
        ret     c
        ld      a,80
        ret
ttp_lo: ld      c,a
        ld      a,100
        sub     c
        ld      l,a
        ld      h,0
        ld      b,8
        ld      c,15
        call    muldiv          ; x * 8 / 15
        ld      a,40
        sub     l
        ret     nc
        xor     a
        ret

; 키 이동 -12..+12 -> 손잡이 : 40 + k * 10 / 3
key_to_pos:
        ld      a,(transpose)
        bit     7,a
        jr      nz,ktp_n
        ld      l,a
        ld      h,0
        ld      b,10
        ld      c,3
        call    muldiv
        ld      a,40
        add     a,l
        ret
ktp_n:  neg
        ld      l,a
        ld      h,0
        ld      b,10
        ld      c,3
        call    muldiv
        ld      a,40
        sub     l
        ret

; HL = HL * B / C  (B = 1..255, C = 1..255)
muldiv: ld      a,c
        ld      (md_c),a
        ld      e,l
        ld      d,h
        ld      hl,0
md_m:   add     hl,de
        djnz    md_m
        ld      a,(md_c)
        ld      c,a
        ld      b,0
        ld      de,0
md_d:   or      a
        sbc     hl,bc
        jr      c,md_x
        inc     de
        jr      md_d
md_x:   ex      de,hl
        ret
md_c:   defb    1

; A = 0..255 -> HL 가 가리키는 곳에 앞 0 없이 쓰고 HL 은 글자 끝 다음
put_dec3:
        push    hl
        call    dec_trim
        pop     de
pd_l:   ld      a,(hl)
        ld      (de),a
        or      a
        jr      z,pd_x
        inc     hl
        inc     de
        jr      pd_l
pd_x:   ex      de,hl
        ret

; A = 부호 있는 값 -> HL 위치에 "+n" / "-n"
put_signed:
        ld      b,'+'
        bit     7,a
        jr      z,ps_p
        neg
        ld      b,'-'
ps_p:   ld      (hl),b
        inc     hl
        jp      put_dec3
