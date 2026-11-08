; ============================================================
; 만든이: KimTaeWoo
; 도와준이: Lee.J.H
;  keys.inc - MSX 키보드 행렬을 직접 읽는다 (BIOS 버퍼를 건드리지 않음)
;  key_scan 을 프레임마다 한 번 부르면
;    key_now[행] = 지금 눌려 있는 키 (1 = 눌림),  key_new[행] = 이번에 새로 눌린 키
;  key_hit 쓰는 법:  ld a,(key_new+행) / bit 비트,a
; ============================================================
PPI_C   equ     0AAh
PPI_B   equ     0A9h
KEY_ROWS equ    9

key_now:  defs  KEY_ROWS
key_prev: defs  KEY_ROWS
key_new:  defs  KEY_ROWS

; ---- 화살표 키 반복 ----
; key_rpt8 = 행 8 의 화살표(LEFT UP DOWN RIGHT: 비트 4..7) 중 "이번 프레임에 동작해야 하는 것".
; 새로 눌린 순간에 한 번, 계속 누르고 있으면 KEY_REP_DELAY 프레임 뒤부터 KEY_REP_GAP 프레임마다 한 번씩.
; 목록 이동처럼 눌러 두면 계속 움직여야 하는 곳에서만 쓴다 (SPACE 같은 토글 키는 key_new 를 쓴다).
KEY_REP_DELAY equ 15
KEY_REP_GAP   equ 3
ARROW_BITS    equ 0F0h
key_rpt8:       defb 0
key_rpt_timer:  defb 0

; 행 / 비트 이름 (MSX 국제 배열)
KR_SPACE equ 8          ; bit0
KR_LEFT  equ 8          ; bit4
KR_UP    equ 8          ; bit5
KR_DOWN  equ 8          ; bit6
KR_RIGHT equ 8          ; bit7
KR_ESC   equ 7          ; bit2
KR_RET   equ 7          ; bit7
KR_F4    equ 7          ; bit0
KR_F5    equ 7          ; bit1
KR_F1    equ 6          ; bit5
KR_F2    equ 6          ; bit6
KR_F3    equ 6          ; bit7
KR_M     equ 4          ; bit2
KR_L     equ 4          ; bit1
KR_P     equ 4          ; bit5
KR_R     equ 4          ; bit7
KR_S     equ 5          ; bit0
KR_A     equ 2          ; bit6
KR_B     equ 2          ; bit7
KR_MINUS equ 1          ; bit2
KR_PLUS  equ 1          ; bit3  ( = 키 )
KR_LBR   equ 1          ; bit5  ( [ )
KR_RBR   equ 1          ; bit6  ( ] )

key_scan:
        ld      hl,key_now
        ld      de,key_prev
        ld      bc,KEY_ROWS
        ldir                    ; 이전 상태 보관
        di
        in      a,(PPI_C)
        and     0F0h
        ld      d,a             ; 윗 4비트 보존
        ld      hl,key_now
        ld      b,KEY_ROWS
        ld      c,0
ks_row: ld      a,d
        or      c
        out     (PPI_C),a
        in      a,(PPI_B)
        cpl
        ld      (hl),a
        inc     hl
        inc     c
        djnz    ks_row
        ei
        ld      hl,key_now      ; 새로 눌린 키 = 지금 & ~이전
        ld      de,key_prev
        ld      bc,key_new
        ld      a,KEY_ROWS
ks_new: push    af
        ld      a,(de)
        cpl
        and     (hl)
        ld      (bc),a
        inc     hl
        inc     de
        inc     bc
        pop     af
        dec     a
        jr      nz,ks_new
        jp      key_repeat_update

; 화살표 키 반복 상태를 갱신해 key_rpt8 을 만든다
key_repeat_update:
        ld      a,(key_new+8)
        and     ARROW_BITS
        ld      b,a                     ; B = 새로 눌린 화살표
        ld      a,(key_now+8)
        and     ARROW_BITS
        ld      c,a                     ; C = 눌려 있는 화살표
        ld      a,b
        or      a
        jr      nz,kr_restart           ; 새로 눌렀다: 시간을 처음부터
        ld      a,c
        or      a
        jr      z,kr_restart            ; 아무것도 안 눌렸다
        ld      hl,key_rpt_timer
        inc     (hl)
        ld      a,(hl)
        cp      KEY_REP_DELAY
        jr      c,kr_out                ; 아직 기다리는 중 (B = 0)
        ld      (hl),KEY_REP_DELAY-KEY_REP_GAP  ; 다음 반복은 GAP 프레임 뒤
        ld      b,c                     ; 눌려 있는 키를 한 번 더 알린다
        jr      kr_out
kr_restart:
        xor     a
        ld      (key_rpt_timer),a
kr_out: ld      a,b
        ld      (key_rpt8),a
        ret
