; ============================================================
; 만든이: KimTaeWoo
; 도와준이: Lee.J.H
;  scr_bottom.inc - 플레이어 화면의 한 부분: 상태 줄 + F 키 줄
;  scr_player.inc 가 맨 아래에서 이어 붙인다 (그림 부분은 상태 변수만 읽는다)
; ============================================================
; ---------------------------------------------------------------
;  상태 줄 (y 189, 높이 10)
; ---------------------------------------------------------------
draw_statusline:
        BOX     0,189,512,10,C_PN
        ret

; ---------------------------------------------------------------
;  F 키 줄 (y 200..211)
; ---------------------------------------------------------------
draw_fkeys:
        BOX     0,200,512,12,C_PN
        xor     a
        ld      (loop_ch),a
dfk_l:  call    draw_fkey_item
        ld      hl,loop_ch
        inc     (hl)
        ld      a,(hl)
        cp      5
        jr      c,dfk_l
        TXT     330,202,178,C_LN,C_PN,2,"H HELP  SPACE PLAY  P PANIC"
        ret

; loop_ch 번째 F 키 (x = 2 + 65 * 번호, 62 x 10)
draw_fkey_item:
        ld      a,(loop_ch)
        ld      l,a
        ld      h,0
        ld      e,l
        ld      d,h
        add     hl,hl
        add     hl,hl
        add     hl,hl
        add     hl,hl
        add     hl,hl
        add     hl,hl           ; * 64
        add     hl,de           ; * 65
        inc     hl
        inc     hl
        ld      (fk_x),hl
        ld      a,(fkey_active)
        ld      b,a
        ld      a,(loop_ch)
        cp      b
        ld      a,C_PN
        ld      b,C_YEL
        ld      c,C_TX
        jr      nz,dfi_c
        ld      a,C_TX          ; 선택된 칸: 크림색 바탕에 어두운 글자
        ld      b,C_BG
        ld      c,C_BG
dfi_c:  ld      (fk_bg),a
        ld      a,b
        ld      (fk_key_fg),a
        ld      a,c
        ld      (fk_name_fg),a
        ld      hl,(fk_x)
        ld      (r_x),hl
        ld      (f_x),hl
        ld      hl,201
        ld      (r_y),hl
        ld      (f_y),hl
        ld      hl,62
        ld      (r_w),hl
        ld      (f_w),hl
        ld      hl,10
        ld      (r_h),hl
        ld      (f_h),hl
        ld      a,(fk_bg)
        ld      (r_c),a
        call    rect_go
        ld      a,(loop_ch)
        ld      b,a
        ld      a,(fkey_active)
        cp      b
        jr      z,dfi_t         ; 선택된 칸은 테두리 없음
        ld      a,C_LN
        ld      (f_c),a
        call    frame_go
dfi_t:  ld      a,(loop_ch)     ; 키 이름 "F1", 기능 이름 "PLAYER"
        ld      l,a
        ld      h,0
        add     hl,hl
        ld      de,fk_names
        add     hl,de
        ld      e,(hl)
        inc     hl
        ld      d,(hl)
        ld      (fk_name),de
        ld      hl,fk_keys
        ld      a,(loop_ch)
        add     a,a
        add     a,a             ; "Fn",0 = 3바이트가 아니라 4칸 간격으로 둔다
        ld      e,a
        ld      d,0
        add     hl,de
        ld      (fk_key),hl
        call    tx_index_hl_key
        ld      hl,(t_tw)
        ld      (fk_wk),hl
        ld      hl,(fk_name)
        call    tx_index
        ld      hl,(t_tw)
        ld      (fk_wn),hl
        ld      de,(fk_wk)      ; 전체 너비 = 키 + 3 + 이름 -> 62 안에서 가운데
        add     hl,de
        ld      de,3
        add     hl,de
        ex      de,hl
        ld      hl,62
        or      a
        sbc     hl,de
        srl     h
        rr      l
        ld      de,(fk_x)
        add     hl,de
        ld      (fk_gx),hl
        ld      hl,(fk_gx)      ; 키 이름
        ld      de,(fk_wk)
        ld      a,(fk_key_fg)
        ld      b,a
        ld      a,(fk_bg)
        ld      c,a
        ld      a,202
        call    text_at
        xor     a
        ld      (t_al),a
        ld      hl,(fk_key)
        call    txtp
        ld      hl,(fk_gx)      ; 기능 이름
        ld      de,(fk_wk)
        add     hl,de
        ld      de,3
        add     hl,de
        ld      de,(fk_wn)
        ld      a,(fk_name_fg)
        ld      b,a
        ld      a,(fk_bg)
        ld      c,a
        ld      a,202
        call    text_at
        xor     a
        ld      (t_al),a
        ld      hl,(fk_name)
        jp      txtp
tx_index_hl_key:
        ld      hl,(fk_key)
        jp      tx_index
fk_keys:  defb  "F1",0,0,"F2",0,0,"F3",0,0,"F4",0,0,"F5",0,0
fk_names: defw  fkn1,fkn2,fkn3,fkn4,fkn5
fkn1:     defb  "PLAYER",0
fkn2:     defb  "ROLL",0
fkn3:     defb  "LYRICS",0
fkn4:     defb  "FILES",0
fkn5:     defb  "SETUP",0
