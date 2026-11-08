; ============================================================
; 만든이: KimTaeWoo
; 도와준이: Lee.J.H
;  text7.inc - SCREEN 7 (4비트 화소) 글자 그리기 공용부   글꼴: font.inc (Galmuri7)
;  call txt : defw x / defb y / defw 칸너비(0=글자너비) / defb 글자색,바탕색,정렬 / defb "글",0
;             정렬 0=왼쪽 1=가운데 2=오른쪽.  칸 전체를 바탕색으로 칠하고 그 위에 글자를 얹는다.
;  call txtp: 같은 변수를 미리 채우고 HL = 글자 주소
;  글자 높이 8줄, y 는 글자 맨 윗줄.  x 가 홀수여도 된다(가장자리 화소는 읽어서 섞는다).
; ============================================================
; TXB, TGI 는 bss.inc (프로그램 끝 뒤의 작업 영역)

; A = 글자 -> A = 글꼴 번호
glyph_idx:
        cp      20h
        jr      nc,gi_n
        add     a,60h
        ret
gi_n:   sub     20h
        cp      60h
        ret     c
        ld      a,1Fh           ; '?'
        ret

txt:    xor     a
        jr      txt_in
txt2:   ld      a,1             ; 2배 크기 (가로 세로 2배, 16줄)
txt_in: call    txt_mode
        pop     hl
        call    txt_hdr         ; 머리를 읽는다 (HL = 머리 바로 뒤)
        push    hl              ; 글자 시작
tx_e:   ld      a,(hl)
        inc     hl
        or      a
        jr      nz,tx_e
        ex      (sp),hl         ; 스택 = 돌아갈 주소, HL = 글자
        call    txtp
        ret

; 변수 글자 : HL = 글자(끝 0).  머리(x, y, 칸너비, 글자색, 바탕색, 정렬)만 txt 처럼 바로 뒤에 적는다
;   TXTD / TXTD2 매크로 참고
txtd:   ld      (td_str),hl
        xor     a
        jr      txtd_in
txtd2:  ld      (td_str),hl
        ld      a,1
txtd_in:call    txt_mode
        pop     hl
        call    txt_hdr
        push    hl              ; 돌아갈 주소
        ld      hl,(td_str)
        call    txtp
        ret
td_str: defw    0

; 머리를 t_x,t_y,t_w,t_fg,t_bg,t_al 에 미리 채워 놓고 부르는 글자 : A = 0 보통 / 1 2배, HL = 글자
txtv:   call    txt_mode
        jp      txtp

; A = 0 보통, 1 = 2배 크기 -> t_sh, t_rows
txt_mode:
        ld      (t_sh),a
        ld      a,8
        ld      (t_rows),a
        ld      a,(t_sh)
        or      a
        ret     z
        ld      a,16
        ld      (t_rows),a
        ret

; HL = 머리 -> t_x, t_y, t_w, t_fg, t_bg, t_al 를 채우고 HL = 머리 바로 뒤
txt_hdr:
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        inc     hl
        ld      (t_x),de
        ld      a,(hl)
        inc     hl
        ld      (t_y),a
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        inc     hl
        ld      (t_w),de
        ld      a,(hl)
        inc     hl
        ld      (t_fg),a
        ld      a,(hl)
        inc     hl
        ld      (t_bg),a
        ld      a,(hl)
        inc     hl
        ld      (t_al),a
        ret

txtp:   call    tx_index        ; 번호 목록 + 너비 합
        call    tx_layout
        call    vwait
        xor     a
        ld      (t_r),a
tp_row: call    tx_buildrow
        call    tx_putrow
        ld      hl,t_r
        inc     (hl)
        ld      a,(t_rows)
        cp      (hl)
        jr      nz,tp_row
        ret

; 글자 -> TGI, 너비 합 -> t_tw
tx_index:
        ld      de,TGI
        ld      bc,0
        ld      (t_tw),bc
        ld      a,120           ; 한 번에 120글자까지
        ld      (ti_n),a
ti_l:   ld      a,(hl)
        inc     hl
        or      a
        jr      z,ti_d
        call    glyph_idx
        ld      (de),a
        inc     de
        push    hl
        push    de
        ld      l,a
        ld      h,0
        ld      de,fontw
        add     hl,de
        ld      c,(hl)
        ld      b,0
        ld      hl,(t_tw)
        add     hl,bc
        ld      (t_tw),hl
        pop     de
        pop     hl
        ld      a,(ti_n)
        dec     a
        ld      (ti_n),a
        jr      nz,ti_l
ti_d:   ld      a,0FFh
        ld      (de),a
        ld      a,(t_sh)
        or      a
        ret     z
        ld      hl,(t_tw)
        add     hl,hl
        ld      (t_tw),hl
        ret

; 칸 너비, 정렬 위치, 앞뒤 덧화소, 전체 화소 수 계산
tx_layout:
        ld      hl,(t_w)
        ld      a,h
        or      l
        jr      nz,tl_w
        ld      hl,(t_tw)
tl_w:   ld      de,512          ; 화면 오른쪽 끝(512)을 넘지 않게 칸을 줄인다
        ex      de,hl
        ld      bc,(t_x)
        or      a
        sbc     hl,bc           ; HL = 512 - x
        ex      de,hl           ; HL = 칸, DE = 남은 너비
        or      a
        sbc     hl,de
        jr      c,tl_keep       ; 칸 < 남은 너비 : 그대로
        ex      de,hl           ; 남은 너비로 줄임
        jr      tl_ok
tl_keep:add     hl,de
tl_ok:  ld      (t_cw),hl
        ld      de,(t_tw)
        or      a
        sbc     hl,de           ; HL = 칸 - 글자 (여유)
        jr      nc,tl_pos
        ld      hl,0
tl_pos: ld      a,(t_al)
        or      a
        jr      nz,tl_c
        ld      hl,0
        jr      tl_o
tl_c:   cp      1
        jr      nz,tl_o
        srl     h
        rr      l
tl_o:   ld      a,(t_x)
        and     1
        ld      (t_tl),a
        ld      e,a
        ld      d,0
        add     hl,de
        ld      (t_cur),hl      ; 글자 시작 화소
        ld      hl,(t_cw)
        add     hl,de
        ld      a,l
        and     1
        ld      (t_tr),a
        ld      e,a
        ld      d,0
        add     hl,de
        ld      (t_n),hl
        ret

; ---------------------------------------------------------------
;  줄 버퍼 TXB: 화소 2개 = 1바이트 (위쪽 4비트 = 왼쪽 화소).  VRAM(SCREEN 7)과 같은 모양이라 그대로 내보낸다.
;  tx_row_begin 이 바탕으로 채우고, tx_paint_1x / tx_paint_2x 가 글자·그림 화소를 얹고, tx_putrow 가 내보낸다.
; ---------------------------------------------------------------
; 바탕색으로 한 줄을 채우고 양 끝 화소는 VRAM 에서 읽어 둔다 (글자와 그림이 함께 쓴다)
tx_rowbase:
        ld      a,(t_bg)
        and     0Fh
        ld      b,a
        add     a,a
        add     a,a
        add     a,a
        add     a,a
        or      b               ; A = 바탕색 두 화소
        ld      hl,TXB
        ld      (hl),a
        ld      de,TXB+1
        ld      bc,(t_n)
        srl     b
        rr      c               ; BC = 바이트 수
        dec     bc
        ld      a,b
        or      c
        jr      z,tb_edge       ; 한 바이트뿐이면 채울 것이 없다 (BC = 0 이면 LDIR 은 64K 를 복사한다)
        ldir
tb_edge:ld      a,(t_tl)
        or      a
        jr      z,tb_r
        call    tx_addr
        call    vrd
        and     0F0h            ; 왼쪽 이웃 화소(위쪽 4비트)는 그대로,
        ld      b,a
        ld      a,(t_bg)
        and     0Fh
        or      b               ; 오른쪽 화소는 바탕색
        ld      (TXB),a
tb_r:   ld      a,(t_tr)
        or      a
        ret     z
        call    tx_addr
        ld      de,(t_n)
        srl     d
        rr      e
        dec     de
        add     hl,de
        call    vrd
        and     0Fh             ; 오른쪽 이웃 화소(아래쪽 4비트)는 그대로,
        ld      b,a
        ld      a,(t_bg)
        add     a,a
        add     a,a
        add     a,a
        add     a,a
        or      b               ; 왼쪽 화소는 바탕색 (A 위쪽 4비트의 위는 버려진다)
        ld      hl,(t_n)
        srl     h
        rr      l
        ld      de,TXB-1
        add     hl,de
        ld      (hl),a
        ret

; 줄 하나를 시작한다: 바탕을 깔고, 글자 화소를 놓을 자리와 글자색(위/아래 4비트)을 정한다
tx_row_begin:
        call    tx_rowbase
        ld      hl,(t_cur)      ; 글자 시작 화소
        ld      a,l
        and     1
        ld      (tb_par),a      ; 0 = 바이트의 위쪽 화소, 1 = 아래쪽 화소
        srl     h
        rr      l
        ld      de,TXB
        add     hl,de
        ld      (tb_ptr),hl
        ld      a,(t_fg)
        and     0Fh
        ld      (tb_fg),a       ; 아래쪽 화소용
        add     a,a
        add     a,a
        add     a,a
        add     a,a
        ld      (tb_fg+1),a     ; 위쪽 화소용
        ret

; B 개의 화소를 놓는다 (보통 크기): C = 점 모양 (맨 왼쪽 = 비트7), 자리 = tb_ptr / tb_par.  놓은 만큼 자리가 나아간다
tx_paint_1x:
        ld      a,b
        or      a
        ret     z
        ld      hl,(tb_ptr)
        ld      de,(tb_fg)      ; E = 아래쪽 화소용, D = 위쪽 화소용
        ld      a,(tb_par)
        or      a
        jr      nz,tp1_lo
tp1_hi: rlc     c
        jr      nc,tp1_h2
        ld      a,(hl)
        and     0Fh
        or      d
        ld      (hl),a
tp1_h2: djnz    tp1_lo
        ld      a,1             ; 위쪽 화소에서 끝남: 다음은 같은 바이트의 아래쪽
        jr      tp1_end
tp1_lo: rlc     c
        jr      nc,tp1_l2
        ld      a,(hl)
        and     0F0h
        or      e
        ld      (hl),a
tp1_l2: inc     hl
        djnz    tp1_hi
        xor     a               ; 아래쪽 화소에서 끝남: 다음은 다음 바이트의 위쪽
tp1_end:ld      (tb_par),a
        ld      (tb_ptr),hl
        ret

; B 개의 화소를 놓는다 (2배 크기: 점 하나가 가로 두 화소).  자리 어긋남(tb_par)은 한 줄 내내 같다
tx_paint_2x:
        ld      a,b
        or      a
        ret     z
        ld      hl,(tb_ptr)
        ld      de,(tb_fg)
        ld      a,(tb_par)
        or      a
        jr      nz,tp2_o
        ld      a,d
        or      e
        ld      d,a             ; D = 글자색 두 화소 (한 바이트)
tp2_e:  rlc     c
        jr      nc,tp2_es
        ld      (hl),d
tp2_es: inc     hl
        djnz    tp2_e
        jr      tp2_end
tp2_o:  rlc     c               ; 점 하나 = 이 바이트의 아래쪽 + 다음 바이트의 위쪽
        jr      nc,tp2_os
        ld      a,(hl)
        and     0F0h
        or      e
        ld      (hl),a
        inc     hl
        ld      a,(hl)
        and     0Fh
        or      d
        ld      (hl),a
        djnz    tp2_o
        jr      tp2_end
tp2_os: inc     hl
        djnz    tp2_o
tp2_end:ld      (tb_ptr),hl
        ret

; t_r 번째 줄의 글자 화소를 TXB 에 만든다
tx_buildrow:
        call    tx_row_begin
        ld      a,(t_sh)
        or      a
        ld      a,(t_r)
        jr      z,tb_gr
        srl     a
tb_gr:  ld      (t_gr),a
        ld      hl,TGI
tb_c:   ld      a,(hl)
        cp      0FFh
        ret     z
        inc     hl
        push    hl
        ld      l,a
        ld      h,0
        push    hl
        add     hl,hl
        add     hl,hl
        add     hl,hl
        ld      bc,fontc
        add     hl,bc
        ld      a,(t_gr)
        ld      c,a
        ld      b,0
        add     hl,bc
        ld      a,(hl)
        pop     hl
        ld      bc,fontw
        add     hl,bc
        ld      b,(hl)
        ld      c,a
        ld      a,(t_sh)
        or      a
        jr      nz,tb_big
        call    tx_paint_1x
        jr      tb_nx
tb_big: call    tx_paint_2x
tb_nx:  pop     hl
        jr      tb_c

tb_ptr: defw    0               ; 다음 화소를 놓을 TXB 안 바이트
tb_par: defb    0               ; 그 바이트 안 자리 (0 = 위쪽, 1 = 아래쪽)
tb_fg:  defb    0,0             ; 글자색: 아래쪽 화소용, 위쪽 화소용 (이 차례 그대로 한 쌍)

; HL = 이 줄의 VRAM 주소 (y+r) * 256 + x / 2
tx_addr:
        ld      a,(t_x+1)
        ld      b,a
        ld      a,(t_x)
        srl     b
        rra
        ld      l,a
        ld      a,(t_y)
        ld      c,a
        ld      a,(t_r)
        add     a,c
        ld      h,a
        ret

; VRAM 주소 지정: HL = 주소 (0..FFFFh), 쓰기면 E = 40h, 읽기면 E = 0
;   위쪽 주소(A16..A14)는 R#14 로 넣는다 (줄 y >= 64 부터 필요)
vset:   di
        ld      a,h
        rlca
        rlca
        and     3
        ld      d,a             ; (D 는 호출하는 쪽이 쓰지 않는다)
        ld      a,(vpage_r14)   ; 그리는 쪽의 A16
        or      d
        out     (VDP_C),a
        ld      a,8Eh
        out     (VDP_C),a
        ld      a,l
        out     (VDP_C),a
        ld      a,h
        and     3Fh
        or      e
        out     (VDP_C),a
        ret

; HL 주소의 VRAM 1바이트 읽기 -> A
vrd:    ld      e,0
        call    vset
        nop
        nop
        in      a,(VDP_D)
        ei
        ret

; TXB 를 VRAM 에 쓴다 (이미 VRAM 모양이라 바이트 그대로 내보낸다)
tx_putrow:
        call    tx_addr
        ld      e,40h
        call    vset
        ei                      ; 주소가 정해졌으니 한 줄을 쓰는 동안은 인터럽트를 받아도 된다 (1ms 타이머를 놓치지 않는다)
        ld      hl,(t_n)
        srl     h
        rr      l
        ld      b,l             ; 256 바이트면 B = 0 (256번)
        ld      hl,TXB
        ld      c,VDP_D
        otir
        ret

t_x:    defw    0
t_y:    defb    0
t_w:    defw    0
t_fg:   defb    0
t_bg:   defb    0
t_al:   defb    0
t_tw:   defw    0
t_cw:   defw    0
t_n:    defw    0
t_cur:  defw    0
t_tl:   defb    0
t_tr:   defb    0
t_r:    defb    0
ti_n:   defb    0
t_sh:   defb    0               ; 0 = 보통, 1 = 2배
t_rows: defb    8               ; 그릴 줄 수 (칸 높이가 8보다 작을 때 줄인다)
t_gr:   defb    0
