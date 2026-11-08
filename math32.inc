; ============================================================
; 만든이: KimTaeWoo
; 도와준이: Lee.J.H
;  math32.inc - 32비트 곱셈/나눗셈 공용부 (박자 계산용. div32 는 제수가 16비트 이하일 때 빠른 길을 탄다)
;  v_n (4바이트) = 피제수/몫,  v_d (4바이트) = 제수,  v_r (4바이트) = 나머지,  모두 낮은 바이트가 먼저
;  div32      : v_n = v_n / v_d,  v_r = 나머지   (v_d = 0 이면 v_n = FFFFFFFF)
;  mul32_by_hl: v_n = v_k * HL,  mul32_8 : v_n = v_n * A
; ============================================================
v_n:    defs 4
v_d:    defs 4
v_r:    defs 4
v_k:    defs 4
v_cnt:  defb 0

; 제수가 16비트 이하이면 레지스터로 계산하는 빠른 길 (화면 갱신이 매 프레임 부르는 나눗셈은 모두 이쪽)
; 한 비트씩 메모리에서 미는 옛 방식은 약 2만 클럭이라 한 프레임의 1/3 을 썼다 -> 약 3천 클럭
div32:
        ld      hl,(v_d+2)
        ld      a,h
        or      l
        jr      nz,div32_wide   ; 제수가 17비트 이상이면 옛 방식
        ld      de,(v_d)        ; DE = 제수 (16비트)
        ld      a,d
        or      e
        jr      z,div32_wide    ; 0 이면 옛 방식이 처리 (FFFFFFFF)
        ld      bc,(v_n+2)      ; 1단계: 윗 16비트 / 제수
        ld      hl,0
        ld      a,b
        or      c
        jr      z,dq_hi0        ; 윗부분이 0 이면 건너뜀 (몫 0, 나머지 0)
        call    div16_step      ; BC = 몫, HL = 나머지
dq_hi0: ld      (v_n+2),bc
        ld      bc,(v_n)        ; 2단계: (나머지 : 아랫 16비트) / 제수
        call    div16_step
        ld      (v_n),bc
        ld      (v_r),hl
        ld      hl,0
        ld      (v_r+2),hl
        ret

; BC = 피제수 16비트, HL = 지금까지의 나머지 (< DE), DE = 제수
; -> BC = 몫, HL = 나머지  (A 를 세는 데 쓴다)
div16_step:
        ld      a,16
d16_l:  sla     c
        rl      b
        adc     hl,hl
        jr      c,d16_s         ; 나머지가 17비트로 넘치면 무조건 뺀다
        or      a
        sbc     hl,de
        jr      nc,d16_q
        add     hl,de           ; 못 뺐으면 되돌린다
        jr      d16_n
d16_s:  or      a
        sbc     hl,de
d16_q:  inc     c               ; 몫의 맨 아래 비트
d16_n:  dec     a
        jr      nz,d16_l
        ret

div32_wide:
        ld      hl,v_d          ; 0 으로 나누기 방지
        ld      a,(hl)
        inc     hl
        or      (hl)
        inc     hl
        or      (hl)
        inc     hl
        or      (hl)
        jr      nz,dv_go
        ld      hl,v_n
        ld      b,4
dv_ff:  ld      (hl),0FFh
        inc     hl
        djnz    dv_ff
        ret
dv_go:  xor     a
        ld      hl,v_r
        ld      b,4
dv_c:   ld      (hl),a
        inc     hl
        djnz    dv_c
        ld      a,32
        ld      (v_cnt),a
dv_l:   ld      hl,v_n          ; v_n 을 왼쪽으로 한 칸 밀고, 넘친 비트를 v_r 로 넣는다
        or      a
        ld      b,4
dv_s1:  rl      (hl)
        inc     hl
        djnz    dv_s1
        ld      hl,v_r
        ld      b,4
dv_s2:  rl      (hl)
        inc     hl
        djnz    dv_s2
        ld      hl,v_r          ; v_r >= v_d 이면 빼고 몫의 맨 아래 비트를 켠다
        ld      de,v_d
        call    cmp32
        jr      c,dv_nx
        ld      hl,v_r
        ld      de,v_d
        call    sub32
        ld      hl,v_n
        set     0,(hl)
dv_nx:  ld      hl,v_cnt
        dec     (hl)
        jr      nz,dv_l
        ret

mul32_by_hl:
        ld      (m_mul),hl
        xor     a
        ld      hl,v_n
        ld      (hl),a
        inc     hl
        ld      (hl),a
        inc     hl
        ld      (hl),a
        inc     hl
        ld      (hl),a
        ld      a,16
        ld      (v_cnt),a
mm_l:   ld      hl,m_mul+1
        srl     (hl)            ; 곱하는 수의 맨 아래 비트를 꺼낸다 (2바이트 오른쪽 이동)
        dec     hl
        rr      (hl)
        jr      nc,mm_s
        ld      hl,v_n          ; 켜져 있으면 v_n += v_k
        ld      de,v_k
        or      a
        ld      b,4
mm_a:   ld      a,(de)
        adc     a,(hl)
        ld      (hl),a
        inc     hl
        inc     de
        djnz    mm_a
mm_s:   ld      hl,v_k          ; v_k *= 2
        sla     (hl)
        inc     hl
        rl      (hl)
        inc     hl
        rl      (hl)
        inc     hl
        rl      (hl)
        ld      hl,v_cnt
        dec     (hl)
        jr      nz,mm_l
        ret
m_mul:  defw    0

; v_n = v_n * A
mul32_8:
        ld      (m_mul),a
        ld      hl,v_n          ; v_k = v_n
        ld      de,v_k
        ld      bc,4
        ldir
        xor     a
        ld      (m_mul+1),a
        ld      hl,(m_mul)
        jp      mul32_by_hl

; ---- 4바이트 수 도우미 (HL, DE 는 4바이트 수의 주소) ----
; (HL) += (DE)
add32:  ld      b,4
        or      a
a32_l:  ld      a,(de)
        adc     a,(hl)
        ld      (hl),a
        inc     hl
        inc     de
        djnz    a32_l
        ret
; (HL) -= (DE)
sub32:  ld      b,4
        or      a
s32_l:  ld      a,(de)
        ld      c,a
        ld      a,(hl)
        sbc     a,c
        ld      (hl),a
        inc     hl
        inc     de
        djnz    s32_l
        ret
; Carry = (HL) < (DE)  (값은 바뀌지 않는다)
cmp32:  push    hl
        push    de
        ld      b,4
        or      a
c32_l:  ld      a,(de)
        ld      c,a
        ld      a,(hl)
        sbc     a,c
        inc     hl
        inc     de
        djnz    c32_l
        pop     de
        pop     hl
        ret
; (DE) = (HL)
copy32: ld      bc,4
        ldir
        ret
