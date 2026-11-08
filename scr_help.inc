; ============================================================
; 만든이: KimTaeWoo
; 도와준이: Lee.J.H
;  scr_help.inc - 플레이어 화면의 도움말 팝업: H 키를 누르면 단축키 목록이 뜬다
;  (플레이어에 H 로 정해진 동작이 없으므로 H 가 도움말이다)  아무 키나 누르면 닫히고 플레이어를 다시 그린다
;  팝업이 떠 있는 동안에는 화면 갱신(ui_update)을 쉬어 팝업이 지워지지 않게 한다 (screen_frame 이 help_open 을 본다)
; ============================================================
help_open: defb 0               ; 1 = 도움말 팝업이 떠 있다

; keys_handle 맨 앞: 팝업이 떠 있으면 아무 키로 닫는다.  Carry = 팝업을 다뤘으니 다른 키 처리는 건너뛴다
help_keys:
        ld      a,(help_open)
        or      a
        jr      nz,hk_close
        ld      a,(key_new+3)           ; 행 3: C D E F G H I J  (H = 비트 5)
        bit     5,a
        ret     z                       ; H 가 아니면 Carry 없이 돌아간다
        call    help_show
        scf
        ret
hk_close:
        call    keys_any_new
        ret     z                       ; 아직 아무 키도 안 눌렸다 (그래도 팝업이 떠 있으니 처리는 건너뛴다)
        xor     a
        ld      (help_open),a
        ld      a,SCR_PLAYER
        call    screen_switch           ; 플레이어를 처음부터 다시 그린다
        scf
        ret

; 이번 프레임에 새로 눌린 키가 있으면 NZ
keys_any_new:
        ld      hl,key_new
        ld      b,KEY_ROWS
        xor     a
kan_l:  or      (hl)
        inc     hl
        djnz    kan_l
        or      a
        ret

help_show:
        ld      a,1
        ld      (help_open),a
        PANEL   56,22,400,158,C_PN,C_LN
        TXT     72,28,0,C_WHT,C_PN,0,"KEYS    (any key to close)"
        TXT     72,40,0,C_TX,C_PN,0,"SPACE  play / pause       RET   stop"
        TXT     72,50,0,C_TX,C_PN,0,"LEFT / RIGHT  back / forward 5 sec  (SHIFT 30 sec)"
        TXT     72,60,0,C_TX,C_PN,0,"UP / DOWN     choose channel"
        TXT     72,70,0,C_TX,C_PN,0,"M mute channel   S solo channel   P all notes off"
        TXT     72,80,0,C_TX,C_PN,0,"-  /  =   tempo  -5% / +5%"
        TXT     72,90,0,C_TX,C_PN,0,"[  /  ]   key  -1 / +1 semitone"
        TXT     72,100,0,C_TX,C_PN,0,",  /  .   volume  -5% / +5%"
        TXT     72,110,0,C_TX,C_PN,0,"A   A-B repeat  (set A, set B, clear)"
        TXT     72,120,0,C_TX,C_PN,0,"R   repeat song      X   shuffle"
        TXT     72,130,0,C_TX,C_PN,0,"T   auto next song   N   next song"
        TXT     72,140,0,C_TX,C_PN,0,"F1 player  F2 roll  F3 lyrics  F4 files  F5 setup"
        TXT     72,150,0,C_TX,C_PN,0,"ESC  quit"
        TXT     72,164,0,C_LN,C_PN,0,"H   this help"
        ret
