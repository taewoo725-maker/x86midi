; ============================================================
; 만든이: KimTaeWoo
; 도와준이: Lee.J.H
;  X86MIDI.COM  -  MIDI 파일 재생기 (MSX-DOS2 / Nextor, MSX2+ , SCREEN 7 512x212 16색)
;  빌드: pasmo --bin X86MIDI.ASM x86midi.com x86midi.sym
;  글꼴: Galmuri7 (c) 2019-2025 Lee Minseo, SIL Open Font License 1.1
; ============================================================
        org     0100h

; [테스트모드] 1 = 시험 흔적을 넣는다 (사용자모드로 바꿀 때 0 으로 하거나 IF TESTMODE 묶음을 지운다)
TESTMODE equ    1

BDOS    equ     0005h
JIFFY   equ     0FC9Eh
EXPTBL  equ     0FCC1h
CALSLT  equ     001Ch
CHGMOD  equ     005Fh
INIPLT  equ     0141h           ; 서브롬 안의 주소 (본체 롬이 아님)
EXBRSA  equ     0FAF8h          ; 서브롬 슬롯 번호

start:  ld      (sp0),sp
        call    cmd_parse
        call    abort_register  ; 강제로 끝날 때 훅을 풀도록 DOS 에 알려 둔다
        call    marks_clear
        call    midi_init_if_needed
        call    vdp_init7       ; 화면 틀을 먼저 그려 둔다: 큰 곡은 훑는 데 오래 걸려도 빈 화면이 아니다
        call    scr_player
        call    song_open
        call    ui_first
        call    isr_install     ; 곡이 열렸으니 이제부터 인터럽트가 엔진을 돌린다
main_loop:
        ld      hl,main_beat    ; 메인이 살아 있다는 표시 (인터럽트 쪽 워치독이 본다)
        inc     (hl)
        call    wait_frame
        ld      a,b             ; B = 지난 프레임 수 (1..MAX_CATCHUP)
        ld      (frames_due),a
ml_step:call    levels_decay    ; 엔진은 인터럽트에서 돈다: 여기서는 막대 줄이기만 프레임 수만큼
        ld      hl,frames_due
        dec     (hl)
        jr      nz,ml_step
        ld      a,(song_ended)
        or      a
        call    nz,song_finished
        call    song_start_if_pending   ; 화면이 나온 다음 프레임: 기다리던 곡 시작
        call    ab_check
        call    prefetch_pass   ; 트랙 창이 모자라기 전에 디스크에서 채워 둔다 (인터럽트 엔진은 디스크를 못 읽는다)
        call    key_scan
        call    global_keys
        call    screen_frame
        ld      a,(quit_flag)
        or      a
        jr      z,main_loop
        call    isr_remove      ; 훅을 먼저 풀고
        call    play_stop
        call    midi_flush      ; 큐에 남은 바이트(올 노트 오프 포함)를 다 내보낸다
        call    song_file_close
        call    info_file_save_if_dirty ; 연주를 멈춘 뒤: 새 파일 정보를 X86MIDI.DIR 에 저장
        ld      sp,(sp0)
        call    keys_release_and_flush  ; 눌린 키를 다 뗄 때까지 기다린 뒤 BIOS 키 버퍼를 비운다
        call    scr_restore
        jp      0

; 지금 화면의 키 처리와 갱신
screen_frame:
        ld      a,(cur_screen)
        or      a
        jr      nz,sf_o
        call    keys_handle
        ld      a,(help_open)   ; 도움말 팝업이 떠 있으면 화면 갱신을 쉰다 (팝업을 지우지 않게)
        or      a
        ret     nz
        jp      ui_update
sf_o:   cp      SCR_FILES
        jp      z,files_keys
        cp      SCR_SETUP
        jp      z,setup_keys
        cp      SCR_LYRICS
        jp      z,lyrics_update
        jp      roll_update

sp0:    defw    0
quit_flag: defb 0
frames_due: defb 0
last_frames: defw 0

midi_init_if_needed:
        jp      midi_init

; 인터럽트 횟수(isr_frames)가 바뀔 때까지 기다린다.  B = 지난 프레임 수 (최대 MAX_CATCHUP)
; (화면 갱신이 무거워 한 바퀴가 길어져도 막대 줄이기는 지난 프레임 수만큼 따라잡는다.
;  곡 시간은 인터럽트 엔진이 직접 세므로 여기와 상관없다)
MAX_CATCHUP equ 16
wait_frame:
wf_l:   ld      hl,(isr_frames)
        ld      de,(last_frames)
        or      a
        sbc     hl,de
        jr      z,wf_l
        ld      a,h
        or      a
        ld      a,l
        jr      nz,wf_cap
        cp      MAX_CATCHUP+1
        jr      c,wf_ok
wf_cap: ld      a,MAX_CATCHUP
wf_ok:  ld      b,a
        ld      hl,(isr_frames)
        ld      (last_frames),hl
        ret

; ---------------------------------------------------------------
;  끝낼 때 BIOS 키 버퍼 비우기
;   이 프로그램은 키보드 행렬을 직접 읽어서 BIOS 키 버퍼는 한 번도 비우지 않는다.
;   그런데 BIOS 의 키 인터럽트는 계속 돌아서, 연주 중에 누른 키(RET, 위/아래 화살표, ESC ...)가
;   버퍼에 쌓여 있다가 DOS 로 돌아가면 명령 줄에 입력된 것처럼 실행된다
;   (위 화살표 = 이전 명령 불러오기 + RET = 다시 실행).  그래서 나가기 직전에 버퍼를 비운다.
; ---------------------------------------------------------------
KILBUF  equ     0156h
RELEASE_WAIT_FRAMES equ 120     ; 키를 안 떼는 경우를 대비한 최대 대기 (2 초)

keys_release_and_flush:
        ld      b,RELEASE_WAIT_FRAMES
krf_wait:
        halt
        push    bc
        call    key_scan
        call    any_key_down    ; (B 를 쓰므로 대기 횟수는 PUSH 로 지켜 둔다)
        pop     bc              ; POP 은 플래그를 바꾸지 않는다
        jr      z,krf_flush
        djnz    krf_wait
krf_flush:
        ld      ix,KILBUF
        ld      iy,(EXPTBL-1)
        jp      CALSLT

; 눌린 키가 하나도 없으면 Z
any_key_down:
        ld      hl,key_now
        ld      b,KEY_ROWS
        xor     a
akd_l:  or      (hl)
        inc     hl
        djnz    akd_l
        or      a
        ret

; 끝낼 때 화면을 SCREEN 0 으로 되돌린다
scr_restore:
        xor     a
        ld      ix,CHGMOD
        ld      iy,(EXPTBL-1)
        call    CALSLT
        ld      ix,INIPLT       ; 팔레트도 원래대로 (안 하면 우리 색이 남아 DOS 글자가 분홍색으로 나온다)
        ld      iy,(EXBRSA-1)   ; INIPLT 는 서브롬 루틴이라 서브롬 슬롯으로 부른다 (MSX2 이상)
        jp      CALSLT

        include "ui_macros.inc"
        include "vdp7.inc"
        include "text7.inc"
        include "bmp7.inc"
        include "numfmt.inc"
        include "scr_player.inc"
        include "font.inc"
        include "dos.inc"
        include "keys.inc"
        include "strfmt.inc"
        include "gm_names.inc"
        include "player_ui.inc"
        include "player_ctrl.inc"
        include "screens.inc"
        include "scr_files.inc"
        include "info_file.inc"
        include "scr_setup.inc"
        include "scr_lyrics.inc"
        include "scr_roll.inc"
        include "keys_handle.inc"
        include "data_player.inc"
low_end:
; ---- 높은 영역: 인터럽트 엔진이 쓰는 코드와 자료 (8000h 이상) ----
HI_BASE equ     8000h
        org     HI_BASE
        include "isr.inc"
        include "tick.inc"
        include "math32.inc"
        include "smf.inc"
        include "midi_out.inc"
        include "engine.inc"
        include "player_state.inc"
        include "lyrics_store.inc"
hi_end:
        include "bss.inc"
