; ============================================================
; 만든이: KimTaeWoo
; 도와준이: Lee.J.H
;  keys_handle.inc - 눌린 키를 동작(player_ctrl.inc)에 이어 준다
;    SPACE 재생/일시정지   RET 정지   ESC 끝내기
;    UP / DOWN 채널 고르기   M 소거   S 단독   P 모든 소리 끄기
;    LEFT / RIGHT 5초 뒤로/앞으로 (SHIFT 와 함께 30초)
;    - / =  템포 -5% / +5%      [ / ]  키 -1 / +1 반음      , / .  볼륨 -5% / +5%
;    A  A-B 반복 지정/해제     R  곡 반복     H  도움말 팝업
; ============================================================
SEEK_SHORT equ  5
SEEK_LONG  equ  30

keys_handle:
        call    help_keys       ; H = 도움말 팝업.  떠 있는 동안은 닫는 키만 본다 (그 키는 다른 동작을 하지 않는다)
        ret     c
        ld      a,(help_open)
        or      a
        ret     nz
        ld      a,(key_new+8)           ; 행 8: SPACE HOME INS DEL LEFT UP DOWN RIGHT
        ld      c,a
        bit     0,c
        call    nz,ctl_play_toggle      ; (이 루틴은 C 를 망가뜨린다: 아래 검사 전에 다시 읽는다)
        ld      a,(key_rpt8)            ; 채널 고르기는 누르고 있으면 반복
        bit     5,a
        jr      z,kh_dn
        ld      a,-1
        call    ctl_select
kh_dn:  ld      a,(key_rpt8)
        bit     6,a
        jr      z,kh_lr
        ld      a,1
        call    ctl_select
kh_lr:  ld      a,(key_new+8)
        ld      c,a
        bit     4,c
        jr      z,kh_rt
        ld      hl,-SEEK_SHORT
        ld      de,-SEEK_LONG
        call    kh_pick_step
        call    ctl_seek_rel
kh_rt:  ld      a,(key_new+8)
        ld      c,a
        bit     7,c
        jr      z,kh_r7
        ld      hl,SEEK_SHORT
        ld      de,SEEK_LONG
        call    kh_pick_step
        call    ctl_seek_rel
kh_r7:  ld      a,(key_new+7)           ; 행 7: F4 F5 ESC TAB STOP BS SELECT RET
        ld      c,a
        bit     2,c
        jr      z,kh_ret
        ld      a,1
        ld      (quit_flag),a
kh_ret: ld      a,(key_new+7)
        bit     7,a
        call    nz,ctl_stop
        ld      a,(key_new+4)           ; 행 4: K L M N O P Q R
        ld      c,a
        bit     2,c
        call    nz,ctl_mute
        ld      a,(key_new+4)
        bit     5,a
        call    nz,ctl_panic
        ld      a,(key_new+4)
        bit     7,a
        call    nz,ctl_repeat
        ld      a,(key_new+4)           ; N = 다음 곡
        bit     3,a
        call    nz,song_next
        ld      a,(key_new+5)           ; X = 셔플, T = 자동 다음 곡
        bit     5,a
        call    nz,ctl_shuffle
        ld      a,(key_new+5)
        bit     1,a
        call    nz,ctl_autonext
        ld      a,(key_new+5)           ; 행 5: S T U V W X Y Z
        bit     0,a
        call    nz,ctl_solo
        ld      a,(key_new+2)           ; 행 2: ' ` , . / _ A B
        ld      c,a
        bit     6,c
        call    nz,ctl_ab
        ld      a,(key_new+2)
        bit     2,a
        jr      z,kh_dot
        ld      a,-5
        call    ctl_volume
kh_dot: ld      a,(key_new+2)
        bit     3,a
        jr      z,kh_r1
        ld      a,5
        call    ctl_volume
kh_r1:  ld      a,(key_new+1)           ; 행 1: 8 9 - = \ [ ] ;
        ld      c,a
        bit     2,c
        jr      z,kh_eq
        ld      a,-5
        call    ctl_tempo
kh_eq:  ld      a,(key_new+1)
        bit     3,a
        jr      z,kh_lb
        ld      a,5
        call    ctl_tempo
kh_lb:  ld      a,(key_new+1)
        bit     5,a
        jr      z,kh_rb
        ld      a,-1
        call    ctl_transpose
kh_rb:  ld      a,(key_new+1)
        bit     6,a
        ret     z
        ld      a,1
        jp      ctl_transpose

; SHIFT 가 눌려 있으면 DE, 아니면 HL -> HL
kh_pick_step:
        ld      a,(key_now+6)
        bit     0,a
        ret     z
        ex      de,hl
        ret
