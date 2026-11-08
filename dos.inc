; ============================================================
; 만든이: KimTaeWoo
; 도와준이: Lee.J.H
;  dos.inc - MSX-DOS2 (Nextor) 호출: 명령줄 읽기, 곡 파일 열기 / 위치 지정 읽기
;  명령줄: X86MIDI [파일이름.MID] [/D]      /D = 진단 기록을 남긴다 (진단 스위치)
; ============================================================
_OPEN   equ     43h
_CREATE equ     44h             ; 파일 만들기 (이미 있는 보통 파일은 비운다)
_CLOSE  equ     45h
_READ   equ     48h
_WRITE  equ     49h
_DEFER  equ     64h             ; 디스크 오류 때 불러 줄 루틴 지정 (DE = 0 이면 원래대로)
_DEFAB  equ     63h             ; 프로그램이 강제로 끝날 때(디스크 오류 중단, Ctrl-STOP) 부르는 루틴 지정 (MSX-DOS2)
_SEEK   equ     4Ah

CMD_LEN equ     0080h
CMD_TXT equ     0081h

file_name:  defs 64,0           ; 명령줄에서 가져온 파일 이름 (끝 = 0)
have_name:  defb 0
file_handle:defb 0FFh           ; 열려 있는 곡 파일 (FFh = 없음)
dp_next:    defs 4,0FFh         ; 직전 읽기가 끝난 파일 위치 (FFFFFFFF = 모름).  같은 곳에서 이어 읽으면 _SEEK 를 건너뛴다

; 곡 창(TBUF) 전체가 쓸 수 있는 메모리 안에 들어가는지.  (0006h) = DOS 가 쓰는 자리의 시작.  스택을 위해 256바이트 남긴다.
; Carry = 모자람
check_track_memory:
        ld      hl,(0006h)
        ld      de,0100h
        or      a
        sbc     hl,de
        ld      de,MEM_END
        or      a
        sbc     hl,de
        ret                     ; Carry = (0006h)-100h < MEM_END

; 명령줄을 훑어서 파일 이름과 스위치를 읽는다
cmd_parse:
        xor     a
        ld      (have_name),a
        ld      a,(CMD_LEN)
        or      a
        ret     z
        ld      b,a
        ld      hl,CMD_TXT
cp_next:
        call    cp_skip_space
        ret     z
        ld      a,(hl)
        cp      '/'
        jr      z,cp_switch
        ld      de,file_name
        ld      c,63
cp_copy:ld      a,(hl)
        cp      ' '
        jr      z,cp_done
        cp      '/'
        jr      z,cp_done
        ld      (de),a
        inc     de
        inc     hl
        dec     b               ; 명령줄에서 한 글자 소비
        jr      z,cp_done       ; 줄 끝
        dec     c
        jr      nz,cp_copy      ; 이름이 너무 길면 여기서 끊는다
cp_done:xor     a
        ld      (de),a
        ld      a,1
        ld      (have_name),a
        ld      a,b
        or      a
        ret     z
        jr      cp_next
cp_switch:
        inc     hl
        dec     b
        ret     z
        ld      a,(hl)
        and     0DFh            ; 대문자
        cp      'D'
        jr      nz,cp_sw_not_d
        ld      a,1
        ld      (diag_on),a
        jr      cp_sw_skip
cp_sw_not_d:
        cp      'F'             ; /F = 1ms 틱을 쓰지 않고 프레임 방식으로 (비교용)
        jr      nz,cp_sw_skip
        xor     a
        ld      (tick_want),a
cp_sw_skip:
        inc     hl
        djnz    cp_next
        ret
; 공백을 건너뛴다.  Z = 줄 끝
cp_skip_space:
        ld      a,b
        or      a
        ret     z
cps_l:  ld      a,(hl)
        cp      ' '
        jr      nz,cps_nz
        inc     hl
        djnz    cps_l
        xor     a               ; 줄 끝 -> Z
        ret
cps_nz: or      1               ; NZ
        ret

; 열려 있는 곡 파일을 닫는다 (없으면 아무것도 안 한다)
song_file_close:
        ld      a,(file_handle)
        inc     a
        ret     z
        dec     a
        ld      b,a
        ld      c,_CLOSE
        call    BDOS
        ld      a,0FFh
        ld      (file_handle),a
        ret

; file_name 을 열어 핸들을 붙들어 둔다 (내용은 smf 가 필요할 때마다 읽는다).  Carry = 실패 (err_code 에 이유)
song_file_open:
        xor     a
        ld      (err_code),a
        call    song_file_close
        call    dp_forget_position
        call    check_track_memory
        jr      nc,sf_mem_ok
        ld      a,ERR_BIG
        jr      sf_fail
sf_mem_ok:
        ld      de,file_name
        ld      a,1             ; 읽기 전용
        ld      c,_OPEN
        call    BDOS
        or      a
        ld      a,ERR_OPEN
        jr      nz,sf_fail
        ld      a,b
        ld      (file_handle),a
        or      a
        ret
sf_fail:ld      (err_code),a
        scf
        ret

; 이어 읽기 기억을 지운다 (파일을 새로 열었거나 오류가 났을 때)
dp_forget_position:
        ld      hl,0FFFFh
        ld      (dp_next),hl
        ld      (dp_next+2),hl
        ret

; HL = 4바이트 파일 위치 (작은 쪽부터) 가 든 곳, DE = 버퍼, BC = 읽을 바이트
; -> BC = 읽은 바이트, Carry = 오류.  (파일 끝에서 모자라게 읽는 것은 오류가 아니다.)  IX 는 보존
; 직전 읽기가 끝난 곳에서 이어 읽으면 _SEEK 를 하지 않는다.
dp_want:    defs 4,0            ; 이번에 읽으려는 위치

dos_pread:
        push    ix
        push    de
        push    bc
        ld      de,dp_want
        ld      bc,4
        ldir
        call    dp_seek_if_needed
        pop     hl              ; HL = 읽을 바이트
        pop     de              ; DE = 버퍼
        jr      c,dp_err
        ld      a,(file_handle)
        ld      b,a
        ld      c,_READ
        call    BDOS
        ld      b,h
        ld      c,l
        cp      0C7h            ; .EOF 도 정상 (읽은 바이트 수는 BC)
        jr      z,dp_ok
        or      a
        jr      nz,dp_err
dp_ok:  call    dp_remember_end
        pop     ix
        or      a
        ret
dp_err: call    dp_forget_position
        pop     ix
        ld      bc,0
        scf
        ret

; dp_want 가 직전 읽기가 끝난 자리(dp_next)와 다르면 그 자리로 _SEEK.  Carry = 오류
dp_seek_if_needed:
        ld      hl,dp_want
        ld      de,dp_next
        ld      b,4
dsn_l:  ld      a,(de)
        cp      (hl)
        jr      nz,dsn_seek
        inc     hl
        inc     de
        djnz    dsn_l
        or      a               ; 이어 읽기: 건너뛴다
        ret
dsn_seek:
        ld      hl,(dp_want)
        ld      de,(dp_want+2)
        ld      a,(file_handle)
        ld      b,a
        xor     a               ; 처음부터 잰 위치
        ld      c,_SEEK
        call    BDOS
        or      a
        jr      nz,dsn_fail
        ld      hl,(dp_want)
        ld      (dp_next),hl
        ld      hl,(dp_want+2)
        ld      (dp_next+2),hl
        or      a
        ret
dsn_fail:
        scf
        ret

; 방금 BC 바이트를 읽었다: 다음 이어 읽기 자리 = dp_want + BC
dp_remember_end:
        ld      hl,(dp_want)
        add     hl,bc
        ld      (dp_next),hl
        ld      hl,(dp_want+2)
        ld      de,0
        adc     hl,de
        ld      (dp_next+2),hl
        ret

; DOS 가 프로그램을 강제로 끝내기 전에 불러 주도록 정리 루틴을 알린다 (DOS1 에는 이 호출이 없어 아무 일도 안 일어난다)
;  인터럽트 훅과 1ms 타이머를 안 풀고 끝나면 풀린 메모리를 향해 인터럽트가 계속 들어와 기계가 멈춘다
abort_register:
        ld      c,_DEFAB
        ld      de,abort_cleanup
        jp      BDOS
