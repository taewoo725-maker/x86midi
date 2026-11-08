; ============================================================
; 만든이: KimTaeWoo
; 도와준이: Lee.J.H
;  scr_seek.inc - 플레이어 화면의 한 부분: 탐색 막대 + A/B 표지
;  scr_player.inc 가 맨 아래에서 이어 붙인다 (그림 부분은 상태 변수만 읽는다)
; ============================================================
; ---------------------------------------------------------------
;  탐색 막대 (y 58) + A/B 표지
; ---------------------------------------------------------------
draw_seekbar:
        BOX     0,50,512,16,C_BG       ; 손잡이 / 표지 자국 지우기 (y 50..65)
        PANEL   4,58,504,6,C_PN,C_LN
        call    seek_fill       ; 진행 (seek_px 칸)
        call    seek_handle
        jp      seek_marks

; 막대를 지우지 않고 앞으로 조금 움직인 만큼만 덧그린다 (깜빡임 없음, VDP 일도 적다)
;  (seek_old) = 옛 칸 (>= 2), (seek_px) = 새 칸 (> 옛 칸).  옛 손잡이 자리(5칸)를 막대 모양으로 되메우고 새 손잡이를 그린다
draw_seekbar_step:
        ld      hl,(seek_px)
        ld      de,(seek_old)
        or      a
        sbc     hl,de
        ld      (sk_delta),hl
        ld      hl,(seek_old)
        ld      de,3
        add     hl,de
        ld      (sk_x0),hl              ; 옛 손잡이 왼쪽 x
        ld      hl,(sk_x0)              ; 손잡이 위 / 아래 (막대 밖)는 바탕색
        ld      de,5
        call    sk_rect
        defb    56,2,C_BG
        ld      hl,(sk_x0)
        ld      de,5
        call    sk_rect
        defb    64,2,C_BG
        ld      hl,(sk_x0)              ; 막대 위 / 아래 테두리
        ld      de,5
        call    sk_rect
        defb    58,1,C_LN
        ld      hl,(sk_x0)
        ld      de,5
        call    sk_rect
        defb    63,1,C_LN
        ld      hl,(sk_delta)           ; 새 진행 끝 오른쪽에 남는 빈 곳 (폭 3 - 이동량)
        ld      a,h
        or      a
        jr      nz,ss_fill
        ld      a,l
        cp      3
        jr      nc,ss_fill
        ld      e,a
        ld      a,3
        sub     e
        ld      e,a
        ld      d,0
        ld      hl,(seek_px)
        ld      bc,5
        add     hl,bc
        call    sk_rect
        defb    59,4,C_PN
ss_fill:ld      hl,(sk_delta)           ; 옛 손잡이 왼쪽 ~ 새 진행 끝까지 노랗게
        ld      de,2
        add     hl,de
        ex      de,hl
        ld      hl,(sk_x0)
        call    sk_rect
        defb    59,4,C_YEL
        call    seek_handle
        ld      a,(ab_a_set)
        ld      hl,ab_b_set
        or      (hl)
        ret     z
        jp      seek_marks              ; 표지가 있으면 다시 그려 지워진 곳을 메운다

; HL = x, DE = 폭, 이어지는 3바이트 = y, 높이, 색  -> 채운 사각형 (호출 뒤 데이터 다음으로 돌아간다)
sk_rect:
        ld      (r_x),hl
        ld      (r_w),de
        pop     hl                      ; 돌아갈 주소 = 데이터 위치
        ld      a,(hl)
        inc     hl
        ld      (r_y),a
        ld      a,(hl)
        inc     hl
        ld      (r_h),a
        ld      a,(hl)
        inc     hl
        ld      (r_c),a
        xor     a
        ld      (r_y+1),a
        ld      (r_h+1),a
        push    hl
        jp      rect_go
sk_delta: defw  0
sk_x0:    defw  0
seek_old: defw  0

; 진행 부분 (5,59  seek_px x 4)
seek_fill:
        ld      hl,(seek_px)
        ld      a,h
        or      l
        ret     z
        ld      (r_w),hl
        ld      hl,5
        ld      (r_x),hl
        ld      hl,59
        ld      (r_y),hl
        ld      hl,4
        ld      (r_h),hl
        ld      a,C_YEL
        ld      (r_c),a
        jp      rect_go

; 손잡이 (x = 3 + seek_px, 5 x 10)
seek_handle:
        ld      hl,(seek_px)
        ld      de,3
        add     hl,de
        ld      (r_x),hl
        ld      (f_x),hl
        ld      hl,56
        ld      (r_y),hl
        ld      (f_y),hl
        ld      hl,5
        ld      (r_w),hl
        ld      (f_w),hl
        ld      hl,10
        ld      (r_h),hl
        ld      (f_h),hl
        ld      a,C_TX
        ld      (r_c),a
        call    rect_go
        ld      a,C_BG
        ld      (f_c),a
        jp      frame_go

; A / B 표지 (정해졌을 때만)
seek_marks:
        ld      a,(ab_a_set)
        or      a
        jr      z,sm_b
        ld      hl,(ab_a_px)
        ld      a,C_LIM
        ld      de,"A"
        call    seek_mark
sm_b:   ld      a,(ab_b_set)
        or      a
        ret     z
        ld      hl,(ab_b_px)
        ld      a,C_ORG
        ld      de,"B"
        jp      seek_mark

; HL = 표지 x 칸, A = 색, E = 글자 : 세로선 + 글자
seek_mark:
        ld      (sm_col),a
        ld      a,e
        ld      (sm_chr),a
        ld      de,5
        add     hl,de
        ld      (sm_x),hl
        ld      (r_x),hl
        ld      hl,55
        ld      (r_y),hl
        ld      hl,1
        ld      (r_w),hl
        ld      hl,11
        ld      (r_h),hl
        ld      a,(sm_col)
        ld      (r_c),a
        call    rect_go
        ld      hl,(sm_x)
        inc     hl
        inc     hl
        ld      de,5
        ld      a,(sm_col)
        ld      b,a
        ld      c,C_BG
        ld      a,50
        call    text_at
        xor     a
        ld      (t_al),a
        ld      a,(sm_chr)
        ld      (sm_str),a
        ld      hl,sm_str
        jp      txtp
sm_col: defb    0
sm_chr: defb    0
sm_x:   defw    0
sm_str: defb    "A",0
