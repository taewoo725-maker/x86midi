; ============================================================
; 만든이: KimTaeWoo
; 도와준이: Lee.J.H
;  screens.inc - 화면 바꾸기 (F1 플레이어, F2 롤, F3 가사, F4 파일, F5 설정) 와 곡 열기 / 다음 곡
;  화면 번호 = cur_screen = F 키 번호 - 1.  연주는 어느 화면에서든 계속된다.
; ============================================================
SCR_PLAYER equ  0
SCR_ROLL   equ  1
SCR_LYRICS equ  2
SCR_FILES  equ  3
SCR_SETUP  equ  4

; A = 새 화면 번호 -> 그 화면을 처음부터 그린다
screen_switch:
        push    af
        ld      a,(cur_screen)
        cp      SCR_FILES
        call    z,info_file_save_if_dirty       ; 파일 화면을 떠날 때 새 정보를 저장 (연주 중이면 미룬다)
        call    page_begin      ; 안 보이는 쪽에 그린다: 그리는 동안 보이는 화면은 그대로 둔다
        pop     af
        call    screen_draw
        jp      page_show       ; 다 그렸으면 한 번에 바꾼다

screen_draw:
        ld      (cur_screen),a
        push    af
        xor     a
        ld      (help_open),a   ; 화면이 바뀌면 도움말 팝업은 닫힌 것으로 본다
        pop     af
        ld      (fkey_active),a
        cp      SCR_FILES
        call    nz,resume_if_files_paused       ; FILES 를 떠나면 FILES 가 멈춘 곡을 이어 재생
        ld      a,(cur_screen)
        or      a
        jr      nz,ss_other
        call    scr_player      ; 제목줄, F 키 줄까지 모두 그린다
        jp      ui_first
ss_other:
        BOX     0,0,512,212,C_BG
        call    draw_titlebar
        call    ui_title
        call    draw_fkeys
        ld      a,(cur_screen)
        cp      SCR_ROLL
        jp      z,scr_roll
        cp      SCR_LYRICS
        jp      z,scr_lyrics
        cp      SCR_FILES
        jr      z,ss_files
        jp      scr_setup
ss_files:
        call    pause_if_playing        ; 목록 읽기 / 그리기가 느려 소리가 끊기므로 먼저 멈춘다
        call    dir_scan
        ld      a,0FFh
        ld      (cur_idx),a
        call    dir_find_cur    ; 지금 곡이 이 폴더에 있으면 그 줄 번호
        ld      a,(cur_idx)     ; 지금 곡이 보이도록 고른 줄을 맞춘다
        cp      0FFh
        jr      z,ss_f0
        ld      b,a
        ld      a,(dir_count)
        cp      b
        jr      c,ss_f0
        ld      a,b
        ld      (dir_sel),a
        cp      FILE_ROWS
        jr      c,ss_f0
        sub     FILE_ROWS-1
        ld      (dir_top),a
ss_f0:  jp      scr_files

; 재생 중이면 일시정지 (소리는 끄고 위치는 그대로).  FILES 가 멈췄다는 표시(files_paused)를 남긴다
pause_if_playing:
        ld      a,(play_state)
        cp      PLAY_RUN
        ret     nz
        ld      a,1
        ld      (files_paused),a
        jp      play_toggle

; FILES 가 멈춘 곡이 아직 멈춰 있으면(= 그 사이 새 곡을 열지 않았으면) 이어서 재생.
;   마스터님이 직접 멈춘 곡은 표시가 없으므로 그대로 둔다.  표시는 항상 지운다.
resume_if_files_paused:
        ld      a,(files_paused)
        or      a
        ret     z
        xor     a
        ld      (files_paused),a
        ld      a,(play_state)
        cp      PLAY_PAUSE
        ret     nz
        jp      play_toggle
files_paused: defb 0

; F1..F5 키: 눌리면 화면을 바꾼다
global_keys:
        ld      a,(key_new+6)
        ld      b,a
        ld      c,SCR_PLAYER
        bit     5,b
        jr      nz,gk_go
        inc     c
        bit     6,b
        jr      nz,gk_go
        inc     c
        bit     7,b
        jr      nz,gk_go
        ld      a,(key_new+7)
        ld      b,a
        ld      c,SCR_FILES
        bit     0,b
        jr      nz,gk_go
        inc     c
        bit     1,b
        ret     z
gk_go:  ld      a,(cur_screen)
        cp      c
        ret     z
        ld      a,c
        jp      screen_switch

; ---------------------------------------------------------------
;  곡 열기 (file_name 의 파일) -> 읽고 풀고 곧바로 연주.  Carry = 실패
; ---------------------------------------------------------------
song_open:
        ld      a,(cur_screen)
        or      a
        jp      nz,song_open_core       ; 플레이어 화면이 아니면 진행 표시 없이
        ld      hl,ui_loading_progress
        ld      (progress_hook),hl
        call    ui_loading_begin
        call    song_open_core
        push    af
        ld      hl,0
        ld      (progress_hook),hl
        call    ui_invalidate           ; LOADING / 진행 칸을 다음 갱신 때 원래대로 다시 그린다
        pop     af
        ret

song_open_core:
        call    engine_lock             ; 곡을 여는 동안 인터럽트 엔진은 쉰다
        call    song_open_inner
        push    af
        call    engine_unlock
        pop     af
        ret
song_open_inner:
        xor     a
        ld      (song_loaded),a
        ld      a,(have_name)
        or      a
        ret     z
        call    play_stop
        ld      hl,ch_flags     ; 곡마다 새로 시작하는 채널 상태
        ld      de,ch_flags+1
        ld      bc,16-1
        ld      (hl),0
        ldir
        ld      hl,ch_mute
        ld      de,ch_mute+1
        ld      bc,16*2-1       ; ch_mute, ch_solo 는 붙어 있다
        ld      (hl),0
        ldir
        xor     a
        ld      (solo_any),a
        ld      (ab_a_set),a
        ld      (ab_b_set),a
        ld      (tg_ab),a
        ld      (has_lyrics),a
        call    lyrics_clear
        call    song_file_open
        ret     c
        call    smf_parse
        ret     c
        call    smf_prescan
        ld      a,1
        ld      (song_loaded),a
        ld      hl,total_frames
        call    frames_to_sec
        ld      (tot_sec),hl
        call    song_make_ready ; 연주는 아직 시작하지 않는다: 화면이 나온 다음 프레임에 main_loop 가 시작한다
        ld      a,(vol_pct)
        cp      100
        call    nz,ctl_apply_volume
        or      a
        ret

; ---------------------------------------------------------------
;  곡 시작 순서: 열기(읽기 / 풀기 / 되감기) -> 새 화면 그리기 -> 앞으로 내보내기 -> 다음 프레임에 연주 시작
;  큰 그리기가 시작 직전에 몰리지 않게 하고, 시간은 연주 중일 때만 세므로 시작이 늦어도 박자는 어긋나지 않는다
; ---------------------------------------------------------------
; 곡을 처음으로 되감고 모듈을 리셋해 놓은 채 멈춰 둔다 (play_state 는 정지 그대로)
song_make_ready:
        call    engine_lock
        call    song_rewind_all
        call    midi_reset_module
        call    engine_unlock
        ld      a,1
        ld      (start_pending),a
        ret

; main_loop 가 프레임 맨 앞에서 부른다: 시작을 기다리는 곡이 있으면 지금 연주 시작
song_start_if_pending:
        ld      a,(start_pending)
        or      a
        ret     z
        xor     a
        ld      (start_pending),a
        ld      a,PLAY_RUN
        ld      (play_state),a
        ret
start_pending: defb 0           ; 1 = 곡이 준비됐고 연주 시작을 기다린다

; ---------------------------------------------------------------
;  다음 곡 (폴더 안 차례대로, SHUF 가 켜져 있으면 아무거나)
; ---------------------------------------------------------------
song_next:
        ld      a,(dir_count)
        or      a
        call    z,dir_scan
        ld      a,(dir_count)
        or      a
        ret     z
        ld      a,(cur_idx)
        cp      0FFh
        call    z,dir_find_cur
        ld      a,(tg_shuf)
        or      a
        jr      z,sn_seq
        call    rnd8            ; 아무 곳에서나 시작
        ld      b,a
        ld      a,(dir_count)
        ld      c,a
        ld      a,b
sn_m:   cp      c
        jr      c,sn_go
        sub     c
        jr      sn_m
sn_seq: ld      a,(cur_idx)
        inc     a
sn_go:  ld      (sn_i),a
        ld      a,(dir_count)
        ld      (sn_left),a
sn_l:   ld      a,(sn_i)
        ld      hl,dir_count
        cp      (hl)
        jr      c,sn_ok
        ld      a,(sn_nowrap)   ; 자동 진행이면 목록 끝에서 멈춘다 (처음으로 돌지 않는다)
        or      a
        jr      z,sn_wrap
        ld      (sn_hit_end),a
        ret
sn_wrap:xor     a
        ld      (sn_i),a
sn_ok:  ld      a,(sn_i)
        call    dir_rec
        ld      de,13
        add     hl,de
        ld      a,(hl)
        or      a
        jr      nz,sn_skip      ; 폴더
        ld      a,(mark_count)
        or      a
        jr      z,sn_file       ; 표시가 없으면 파일 전부
        ld      a,(sn_i)
        call    mark_get
        jr      nz,sn_file      ; 표시가 있으면 표시한 곡만
sn_skip:
        ld      hl,sn_i
        inc     (hl)
        ld      hl,sn_left
        dec     (hl)
        jr      nz,sn_l
        ld      a,1
        ld      (sn_hit_end),a  ; 이어 연주할 파일이 하나도 없다
        ret
sn_file:ld      a,(sn_i)
        call    open_list_entry
        ld      a,(cur_screen)
        or      a
        jp      z,ui_first      ; 플레이어 화면이면 값만 새로 채운다
        ret
sn_i:   defb    0
sn_nowrap: defb 0               ; 1 = 목록 끝에서 돌지 않고 멈춘다 (곡이 끝나 자동으로 넘어갈 때)
sn_hit_end:defb 0               ; 1 = 더 이어 연주할 곡이 없었다
sn_left:defb    0

; file_name 과 같은 이름을 목록에서 찾아 cur_idx 로 (없으면 FFh 그대로)
dir_find_cur:
        xor     a
        ld      (sn_i),a
dfc_l:  ld      a,(sn_i)
        ld      hl,dir_count
        cp      (hl)
        ret     nc
        call    dir_rec
        push    hl
        call    base_name       ; HL = 지금 곡 이름
        pop     de              ; DE = 목록의 이름
        call    str_eq          ; HL, DE 글자가 같은가: Z
        jr      z,dfc_f
        ld      hl,sn_i
        inc     (hl)
        jr      dfc_l
dfc_f:  ld      a,(sn_i)
        ld      (cur_idx),a
        ret
; HL, DE 가 가리키는 글자가 같으면 Z
str_eq: ld      a,(de)
        cp      (hl)
        ret     nz
        or      a
        ret     z
        inc     hl
        inc     de
        jr      str_eq

rnd_seed: defw  1234h
; 0..255 어림수 (JIFFY 를 섞는 선형 합동)
rnd8:   ld      hl,(rnd_seed)
        ld      d,h
        ld      e,l
        add     hl,hl
        add     hl,hl
        add     hl,de
        ld      de,(JIFFY)
        add     hl,de
        inc     hl
        ld      (rnd_seed),hl
        ld      a,h
        xor     l
        ret
