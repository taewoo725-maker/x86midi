; ============================================================
; 만든이: KimTaeWoo
; 도와준이: Lee.J.H
;  vdp7.inc - SCREEN 7 (512 x 212, 16 colours) VDP 공용부
;  rect : 사각형 채우기      call rect / defw x,y,w,h / defb color
;  hline/vline/frame : 선과 테두리 (rect 위에 얹은 얇은 함수)
; ============================================================
VDP_D   equ     98h
VDP_C   equ     99h
VDP_P   equ     9Ah
VDP_I   equ     9Bh
RG9SAV  equ     0FFE8h

; 색 번호 (팔레트 pal16 과 같은 순서)
C_BG    equ     0               ; 바탕
C_PN    equ     1               ; 패널
C_LN    equ     2               ; 선
C_TX    equ     3               ; 글자
C_RED   equ     4
C_ORG   equ     5
C_YEL   equ     6
C_LIM   equ     7
C_GRN   equ     8
C_TEAL  equ     9
C_SKY   equ     10
C_BLU   equ     11
C_VIO   equ     12
C_PNK   equ     13
C_WHT   equ     14
C_TAN   equ     15

; VDP 명령이 끝날 때까지 기다린다
;  기다리는 동안 인터럽트를 계속 막지 않는다: 상태 레지스터를 읽는 짧은 순간(R#15 = 2)만 막고,
;  읽을 때마다 풀어 준다.  큰 사각형은 몇 ms 걸리는데, 그동안 막아 두면 1ms 타이머를 놓친다.
vwait:
vw1:    di
        ld      a,2
        out     (VDP_C),a
        ld      a,8Fh
        out     (VDP_C),a
        in      a,(VDP_C)
        rrca                    ; 캐리 = 명령 실행 중 (CE)
        ld      a,0             ; (플래그를 건드리지 않는다)
        out     (VDP_C),a
        ld      a,8Fh
        out     (VDP_C),a
        ei
        ret     nc
        jr      vw1

; 화면 주파수 알아내기: VDP 레지스터 9 의 PAL 비트 (1 = 50Hz, 0 = 60Hz) -> fps
; 곡 속도는 이 값으로 계산하므로 50Hz 화면에서도 제 속도로 연주된다
detect_refresh_rate:
        ld      a,(RG9SAV)
        and     02h
        ld      a,60
        jr      z,drr_set
        ld      a,50
drr_set:ld      (fps),a
        ret

; SCREEN 7 켜기 + 팔레트 + 화면 지우기 (BIOS 를 거치지 않고 VDP 레지스터를 직접 쓴다 - X86MP3 와 같은 방식)
vdp_init7:
        call    detect_refresh_rate
        ld      a,(RG9SAV)
        and     02h             ; PAL 비트만 유지
        or      80h             ; 212줄
        ld      (rtab9),a
        di
        ld      hl,rtab
        ld      b,rcount
vi1:    ld      a,(hl)
        out     (VDP_C),a
        inc     hl
        ld      a,(hl)
        out     (VDP_C),a
        inc     hl
        djnz    vi1
        xor     a
        out     (VDP_C),a
        ld      a,90h
        out     (VDP_C),a
        ld      hl,pal16
        ld      c,VDP_P
        ld      b,32
        otir
        ei
        call    rect
        defw    0,0,512,212
        defb    C_BG
        ret

rtab:   defb    0Ah,80h         ; R#0  Graphic 6 (SCREEN 7: 512 x 212, 16색)
        defb    60h,81h         ; R#1  화면 켜기, 수직귀선 인터럽트
        defb    1Fh,82h         ; R#2  이름표 0000h
        defb    00h,87h         ; R#7  테두리 = 색 0
        defb    0Ah,88h         ; R#8  스프라이트 끔, 64K 램
rtab9:  defb    80h,89h         ; R#9  212줄 (PAL 비트는 위에서 채움)
        defb    00h,92h         ; R#18 화면 위치 0
        defb    00h,97h         ; R#23 세로 스크롤 0
        defb    00h,8Eh         ; R#14 VRAM 상위 주소 0
rcount  equ     9

; call rect: defw x,y,w,h / defb color
rect:   pop     hl
        ld      de,r_x
        ld      bc,8
        ldir
        ld      a,(hl)
        inc     hl
        ld      (r_c),a
        push    hl
rect_go:
        ld      a,(r_w)
        or      a
        jr      nz,rg_ok
        ld      a,(r_w+1)
        or      a
        ret     z
rg_ok:  ld      a,(r_h)
        or      a
        ret     z
        call    vwait
        ld      a,(r_y+1)       ; 그리는 쪽 쪽(0/1)을 y 위쪽 비트로 넣는다
        and     0FEh
        ld      b,a
        ld      a,(draw_page)
        or      b
        ld      (r_y+1),a
        ld      a,(r_c)
        and     0Fh
        ld      b,a
        rlca
        rlca
        rlca
        rlca
        or      b
        ld      (r_cc),a
        ld      a,(r_x)
        ld      b,a
        ld      a,(r_w)
        or      b
        and     1
        ld      a,0C0h          ; HMMV
        jr      z,rg_cmd
        ld      a,(r_c)
        ld      (r_cc),a        ; LMMV 는 아래쪽 4비트 사용
        ld      a,80h           ; LMMV
rg_cmd: ld      (r_op),a
        di
        ld      a,36
        out     (VDP_C),a
        ld      a,91h           ; R#17 = 36 (자동 증가)
        out     (VDP_C),a
        ld      hl,r_x
        ld      b,4                     ; DX,DY 4바이트 -> R36..R39
        ld      c,VDP_I
        otir
        ld      hl,r_w          ; NX,NY -> R40..R43
        ld      b,4
        ld      c,VDP_I
        otir
        ld      a,(r_cc)
        out     (VDP_I),a       ; R44
        xor     a
        out     (VDP_I),a       ; R45
        ld      a,(r_op)
        out     (VDP_I),a       ; R46 실행
        ei
        ret

; 테두리 : call frame / defw x,y,w,h / defb color
frame:  pop     hl
        ld      de,f_x
        ld      bc,9
        ldir
        push    hl
frame_go:
        ld      hl,(f_x)
        ld      (r_x),hl
        ld      hl,(f_y)
        ld      (r_y),hl
        ld      hl,(f_w)
        ld      (r_w),hl
        ld      hl,1
        ld      (r_h),hl
        ld      a,(f_c)
        ld      (r_c),a
        call    rect_go                 ; 위
        ld      hl,(f_y)
        ld      de,(f_h)
        add     hl,de
        dec     hl
        ld      (r_y),hl
        call    rect_go                 ; 아래
        ld      hl,(f_y)
        ld      (r_y),hl
        ld      hl,(f_h)
        ld      (r_h),hl
        ld      hl,1
        ld      (r_w),hl
        call    rect_go                 ; 왼쪽
        ld      hl,(f_x)
        ld      de,(f_w)
        add     hl,de
        dec     hl
        ld      (r_x),hl
        jp      rect_go                 ; 오른쪽

; rect 가 읽는 8바이트 (x,y,w,h 순서 고정)
r_x:    defw    0
r_y:    defw    0
r_w:    defw    0
r_h:    defw    0
r_c:    defb    0
r_cc:   defb    0
r_op:   defb    0
f_x:    defw    0
f_y:    defw    0
f_w:    defw    0
f_h:    defw    0
f_c:    defb    0

; ---------------------------------------------------------------
;  두 쪽 그리기: 보이는 쪽은 그대로 두고 안 보이는 쪽에 새 화면을 다 그린 뒤 한 번에 바꾼다
;    화면 0 쪽 = VRAM 00000h, 1 쪽 = 10000h.  draw_page 는 항상 지금 보이는 쪽과 같고,
;    새 화면을 그리는 동안만 안 보이는 쪽을 가리킨다.
; ---------------------------------------------------------------
; 그릴 쪽을 반대편으로 돌린다 (보이는 쪽은 그대로)
page_begin:
        ld      a,(draw_page)
        xor     1
        jr      page_set

; 그릴 쪽을 보이는 쪽에 맞춘다 (화면 틀을 만든 직후 / 새 화면을 다 그린 직후)
page_show:
        ld      a,(draw_page)
        di
        rrca
        rrca
        rrca                    ; 쪽 1 -> 20h (R#2 비트5)
        or      1Fh
        out     (VDP_C),a
        ld      a,82h
        out     (VDP_C),a
        ei
        ret

page_set:
        ld      (draw_page),a
        add     a,a
        add     a,a             ; A16 = R#14 비트2
        ld      (vpage_r14),a
        ret

draw_page: defb 0               ; 지금 그리는 쪽 (0 / 1)
vpage_r14: defb 0               ; draw_page 를 R#14 에 넣을 값 (0 / 4)

        include "pal.inc"
