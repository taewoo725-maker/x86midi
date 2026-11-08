; ============================================================
; 만든이: KimTaeWoo
; 도와준이: Lee.J.H
;  smf.inc - 표준 MIDI 파일(SMF 0/1) 읽기  (V1.2.0: 파일을 통째로 올리지 않고 트랙별 창으로 읽는다)
;
;  [위층: 곡 구조]
;   smf_parse   : 파일 머리와 덩어리 머리만 읽어 트랙 표(TRKS)를 만든다.  Carry = 오류 (err_code 참고)
;   trk_rewind  : 모든 트랙을 처음 상태로 되돌리고 첫 시간 간격까지 읽어 둔다
;   trk_read_delta / trk_read_event : 트랙 하나에서 시간 간격 / 사건 하나를 읽는다 (IX = 트랙 칸)
;  [아래층: 바이트 공급]  -- 파일 위치나 버퍼는 이 층 밖에서 만지지 않는다
;   trk_start   : 트랙 칸 하나를 처음 상태로 (창을 비우고 파일 위치를 자료 시작으로)
;   trk_getb    : 트랙에서 한 바이트 (창이 비면 trk_fill 이 알아서 채운다)
;   trk_read_vlq: 가변 길이 수 하나
;   trk_fill    : 창 채우기 (dos_pread 로 파일에서 읽는다)
;  EVBUF(메타/SysEx 자료 한 건), TBUF(트랙 창 16개) 는 bss.inc
; ============================================================
TRK_SZ  equ     40
TRK_MAX equ     16
TRK_PTR equ     0               ; 창 안에서 다음에 읽을 자리
TRK_END equ     2               ; 창에 들어 있는 자료의 끝
TRK_DLO equ     4
TRK_DHI equ     6
TRK_RUN equ     8
TRK_ST  equ     9               ; 0 쓰지 않음, 1 연주 중, 2 끝남
TRK_USE equ     10              ; 1 = 파일에 있는 트랙
TRK_ABS equ     12              ; 훑을 때 쓰는 누적 틱 (4바이트)
TRK_STA equ     16              ; 트랙 자료의 파일 위치 (4바이트)
TRK_LEN equ     20              ; 트랙 자료 길이 (4바이트)   <- STA, LEN 은 붙어 있어야 한다
TRK_FPOS equ    24              ; 다음에 창으로 읽어 올 파일 위치 (4바이트)
TRK_REM equ     28              ; 아직 창으로 읽지 않은 길이 (4바이트)   <- FPOS, REM 도 붙어 있다
TRK_BUF equ     32              ; 이 트랙 창의 주소 (2바이트)
TRK_NEED equ    34              ; 1 = 인터럽트에서 자료가 모자라 멈췄다: 메인 루프가 서둘러 선읽기
TRK_PEND equ    35              ; 1 = 사건은 보냈는데 다음 시간 간격을 아직 못 읽었다

ERR_OPEN  equ   1
ERR_BIG   equ   2
ERR_NOTMIDI equ 3
ERR_SMPTE equ   4
ERR_EMPTY equ   5

err_code:   defb 0
smf_fmt:    defb 0
smf_ntrk:   defb 0
smf_ppq:    defw 480

smf_head:    defs 14,0           ; 파일 머리 (MThd)
ch_buf:     defs 8,0            ; 덩어리 머리
sp_pos:     defs 4,0            ; 지금 보는 덩어리의 파일 위치
sp_len:     defs 4,0            ; 그 덩어리의 자료 길이
sp_bufp:    defw 0              ; 다음 트랙에 줄 창 주소
mthd_txt:   defm "MThd"
mtrk_txt:   defm "MTrk"

; HL = 4바이트 글자, DE = 비교할 글자 -> Z = 같다
cmp_tag4:
        ld      b,4
ct_l:   ld      a,(de)
        cp      (hl)
        ret     nz
        inc     hl
        inc     de
        djnz    ct_l
        ret

; ============================================================
;  위층 1: 파일 머리 + 트랙 표
; ============================================================
smf_parse:
        ld      hl,0
        ld      (sp_pos),hl
        ld      (sp_pos+2),hl
        ld      hl,sp_pos
        ld      de,smf_head
        ld      bc,14
        call    dos_pread
        jr      c,sp_open_err
        ld      a,b
        or      a
        jr      nz,sp_bad
        ld      a,c
        cp      14
        jr      nz,sp_bad
        ld      hl,smf_head
        ld      de,mthd_txt
        call    cmp_tag4
        jr      nz,sp_bad
        ld      a,(smf_head+9)   ; 형식 (빅엔디언 2바이트 중 아래쪽)
        ld      (smf_fmt),a
        ld      a,(smf_head+11)  ; 트랙 수
        cp      TRK_MAX+1
        jr      c,sp_nt
        ld      a,TRK_MAX
sp_nt:  ld      (smf_ntrk),a
        ld      a,(smf_head+12)  ; 분해능
        bit     7,a
        jr      nz,sp_smpte
        ld      h,a
        ld      a,(smf_head+13)
        ld      l,a
        ld      (smf_ppq),hl
        ld      a,h
        or      l
        jr      z,sp_bad
        call    trk_table_clear
        ld      hl,14           ; 첫 덩어리는 파일 머리 바로 뒤
        ld      (sp_pos),hl
        ld      hl,TBUF
        ld      (sp_bufp),hl
        ld      ix,TRKS
        ld      a,(smf_ntrk)
        ld      b,a
        or      a
        jr      z,sp_empty
sp_chunk:
        push    bc
        call    sp_one_chunk
        pop     bc
        jr      c,sp_done       ; 파일 끝
        djnz    sp_chunk
sp_done:ld      a,(TRKS+TRK_ST)
        or      a
        jr      z,sp_empty
        or      a
        ret
sp_open_err:
        ld      a,ERR_OPEN
        jr      sp_err
sp_bad: ld      a,ERR_NOTMIDI
        jr      sp_err
sp_smpte:
        ld      a,ERR_SMPTE
        jr      sp_err
sp_empty:
        ld      a,ERR_EMPTY
sp_err: ld      (err_code),a
        scf
        ret

trk_table_clear:
        ld      hl,TRKS
        ld      de,TRKS+1
        ld      bc,TRK_SZ*TRK_MAX-1
        ld      (hl),0
        ldir
        ret

; sp_pos 의 덩어리를 읽는다.  MTrk 면 IX 칸을 채우고 IX 를 다음 칸으로.  Carry = 더 읽을 게 없음
sp_one_chunk:
sc_next:ld      hl,sp_pos
        ld      de,ch_buf
        ld      bc,8
        call    dos_pread
        jr      c,sc_none
        ld      a,b
        or      a
        jr      nz,sc_none
        ld      a,c
        cp      8
        jr      nz,sc_none      ; 머리 8바이트가 다 있어야 한다
        call    sp_take_length
        ld      hl,ch_buf
        ld      de,mtrk_txt
        call    cmp_tag4
        jr      nz,sc_skip
        call    sp_fill_slot
        call    sp_advance
        or      a               ; 길이가 이상해도 이 트랙은 받는다 (다음 읽기가 실패해 끝난다)
        ret
sc_skip:call    sp_advance      ; MTrk 가 아닌 덩어리는 건너뛴다
        jr      nc,sc_next
sc_none:scf
        ret

; 덩어리 머리의 빅엔디언 길이 -> sp_len (작은 쪽부터)
sp_take_length:
        ld      a,(ch_buf+7)
        ld      (sp_len),a
        ld      a,(ch_buf+6)
        ld      (sp_len+1),a
        ld      a,(ch_buf+5)
        ld      (sp_len+2),a
        ld      a,(ch_buf+4)
        ld      (sp_len+3),a
        ret

; sp_pos += 8 + sp_len.  Carry = 32비트를 넘었다
sp_advance:
        ld      hl,(sp_pos)
        ld      de,(sp_len)
        add     hl,de
        ld      (sp_pos),hl
        ld      hl,(sp_pos+2)
        ld      de,(sp_len+2)
        adc     hl,de
        ld      (sp_pos+2),hl
        ret     c
        ld      hl,(sp_pos)
        ld      de,8
        add     hl,de
        ld      (sp_pos),hl
        ret     nc
        ld      hl,(sp_pos+2)
        inc     hl
        ld      (sp_pos+2),hl
        ld      a,h
        or      l
        ret     nz              ; NC
        scf
        ret

; 지금 덩어리(자료 = sp_pos+8, 길이 = sp_len)로 IX 칸을 채우고 IX 를 다음 칸으로
sp_fill_slot:
        ld      hl,(sp_pos)
        ld      de,8
        add     hl,de
        ld      (ix+TRK_STA),l
        ld      (ix+TRK_STA+1),h
        ld      hl,(sp_pos+2)
        ld      de,0
        adc     hl,de
        ld      (ix+TRK_STA+2),l
        ld      (ix+TRK_STA+3),h
        ld      hl,(sp_len)
        ld      (ix+TRK_LEN),l
        ld      (ix+TRK_LEN+1),h
        ld      hl,(sp_len+2)
        ld      (ix+TRK_LEN+2),l
        ld      (ix+TRK_LEN+3),h
        ld      hl,(sp_bufp)
        ld      (ix+TRK_BUF),l
        ld      (ix+TRK_BUF+1),h
        ld      de,TBUF_SZ
        add     hl,de
        ld      (sp_bufp),hl
        ld      (ix+TRK_USE),1
        ld      (ix+TRK_ST),1
        ld      bc,TRK_SZ
        add     ix,bc
        ret

; ============================================================
;  아래층: 트랙 바이트 공급
; ============================================================
; IX 칸을 처음 상태로: 파일 위치 = 자료 시작, 남은 길이 = 전체, 창 비움, 러닝 스테이터스 없음
trk_start:
        push    ix
        pop     hl
        ld      bc,TRK_STA
        add     hl,bc           ; HL = STA (뒤에 LEN 이 붙어 있다)
        push    ix
        pop     de
        ld      bc,TRK_FPOS
        ex      de,hl
        add     hl,bc
        ex      de,hl           ; DE = FPOS (뒤에 REM 이 붙어 있다)
        ld      bc,8
        ldir
        ld      a,(ix+TRK_BUF)
        ld      (ix+TRK_PTR),a
        ld      (ix+TRK_END),a
        ld      a,(ix+TRK_BUF+1)
        ld      (ix+TRK_PTR+1),a
        ld      (ix+TRK_END+1),a
        ld      (ix+TRK_RUN),0
        ld      (ix+TRK_NEED),0
        ld      (ix+TRK_PEND),0
        ret

; 창을 파일에서 다시 채운다.  Carry = 더 읽을 자료가 없다.  BC, DE 는 보존
; (인터럽트 안에서는 디스크를 읽을 수 없다: 자료가 남았는데 창이 비면 starved 를 세우고 Carry 로 돌아간다.
;  호출한 쪽(track_step)이 하던 일을 되돌리고, 메인 루프의 선읽기가 창을 채워 준다)
starved: defb   0

; BC = min(BC, 이 트랙의 남은 길이).  A, HL 은 망가진다
trk_clamp_len:
        ld      a,(ix+TRK_REM+2)
        or      (ix+TRK_REM+3)
        ret     nz              ; 남은 길이가 64KB 이상이면 BC 가 더 작다
        ld      l,(ix+TRK_REM)
        ld      h,(ix+TRK_REM+1)
        or      a
        sbc     hl,bc
        ret     nc              ; 남은 길이 >= BC
        add     hl,bc
        ld      b,h
        ld      c,l             ; 남은 길이만큼
        ret

; 이 트랙의 파일 위치에서 BC 바이트를 DE 가 가리키는 곳으로 읽는다.  Carry = 실패 (읽기 오류 / 0 바이트).  성공이면 BC = 읽은 바이트
trk_pread:
        push    de
        push    ix
        pop     hl
        ld      de,TRK_FPOS
        add     hl,de           ; HL = 파일 위치가 든 곳
        pop     de
        call    dos_pread
        ret     c
        ld      a,b
        or      c
        ret     nz
        scf
        ret

; 이 트랙은 더 읽을 자료가 없는 것으로 한다 (남은 길이 = 0)
trk_rem_clear:
        ld      (ix+TRK_REM),0
        ld      (ix+TRK_REM+1),0
        ld      (ix+TRK_REM+2),0
        ld      (ix+TRK_REM+3),0
        ret

trk_fill:
        ld      a,(ix+TRK_REM)
        or      (ix+TRK_REM+1)
        or      (ix+TRK_REM+2)
        or      (ix+TRK_REM+3)
        jp      z,tf_none
        ld      a,(isr_mode)
        or      a
        jr      z,tf_real
        ld      a,1
        ld      (starved),a
        scf
        ret
tf_real:
        push    bc
        push    de
        ld      bc,TBUF_SZ      ; 읽을 양 = min(남은 길이, 창 크기)
        call    trk_clamp_len
        ld      e,(ix+TRK_BUF)
        ld      d,(ix+TRK_BUF+1)
        call    trk_pread
        jr      c,tf_fail
        ld      l,(ix+TRK_BUF)
        ld      h,(ix+TRK_BUF+1)
        ld      (ix+TRK_PTR),l
        ld      (ix+TRK_PTR+1),h
        add     hl,bc
        ld      (ix+TRK_END),l
        ld      (ix+TRK_END+1),h
        call    trk_advance_read
        pop     de
        pop     bc
        or      a
        ret
tf_fail:call    trk_rem_clear   ; 읽기 실패 / 파일이 모자람: 여기서 끝낸다
        pop     de
        pop     bc
tf_none:scf
        ret

; 방금 BC 바이트를 읽었다: 파일 위치 += BC, 남은 길이 -= BC
trk_advance_read:
        ld      a,(ix+TRK_FPOS)
        add     a,c
        ld      (ix+TRK_FPOS),a
        ld      a,(ix+TRK_FPOS+1)
        adc     a,b
        ld      (ix+TRK_FPOS+1),a
        ld      a,(ix+TRK_FPOS+2)
        adc     a,0
        ld      (ix+TRK_FPOS+2),a
        ld      a,(ix+TRK_FPOS+3)
        adc     a,0
        ld      (ix+TRK_FPOS+3),a
        ld      a,(ix+TRK_REM)
        sub     c
        ld      (ix+TRK_REM),a
        ld      a,(ix+TRK_REM+1)
        sbc     a,b
        ld      (ix+TRK_REM+1),a
        ld      a,(ix+TRK_REM+2)
        sbc     a,0
        ld      (ix+TRK_REM+2),a
        ld      a,(ix+TRK_REM+3)
        sbc     a,0
        ld      (ix+TRK_REM+3),a
        ret

; 트랙(IX)에서 한 바이트 -> A.  Carry = 더 없음.  BC, DE 는 보존 (HL 은 망가진다)
trk_getb:
tg_retry:
        ld      l,(ix+TRK_PTR)
        ld      h,(ix+TRK_PTR+1)
        ld      a,l
        sub     (ix+TRK_END)
        ld      a,h
        sbc     a,(ix+TRK_END+1)
        jr      nc,tg_empty     ; PTR >= END : 창이 비었다
        ld      a,(hl)
        inc     hl
        ld      (ix+TRK_PTR),l
        ld      (ix+TRK_PTR+1),h
        or      a               ; Carry = 0
        ret
tg_empty:
        call    trk_fill
        jr      nc,tg_retry
        ret

; Carry = 읽을 자리가 없음 (연주 중이 아니거나 자료를 다 읽음)
trk_at_end:
        ld      a,(ix+TRK_ST)
        cp      1
        jr      nz,ae_y
        ld      l,(ix+TRK_PTR)
        ld      h,(ix+TRK_PTR+1)
        ld      a,l
        sub     (ix+TRK_END)
        ld      a,h
        sbc     a,(ix+TRK_END+1)
        jr      nc,ae_fill
        or      a               ; 창에 자료가 남아 있다
        ret
ae_fill:jp      trk_fill
ae_y:   scf
        ret

; IX 트랙 창에 남은 바이트 -> DE, 읽을 자리 -> HL
trk_window_left:
        ld      e,(ix+TRK_PTR)
        ld      d,(ix+TRK_PTR+1)
        ld      l,(ix+TRK_END)
        ld      h,(ix+TRK_END+1)
        or      a
        sbc     hl,de
        ex      de,hl
        ret

; Carry = 창에 A 바이트 이상 남아 있지 않다.  성공하면 HL = 읽을 자리
trk_has_room:
        ld      b,a
        call    trk_window_left
        ld      a,d
        or      a
        ret     nz              ; 256바이트 이상 남음 (NC)
        ld      a,e
        cp      b
        ret                     ; Carry = 모자람

; HL = 메모리의 가변 길이 수 -> DE:BC, HL = 다음.  (한 바이트짜리는 곧바로 끝낸다)
vlq_byte: defb  0
vlq_cnt:  defb  0
vlq_from_memory:
        ld      a,(hl)
        bit     7,a
        jr      nz,vm_long
        inc     hl
        ld      c,a
        ld      b,0
        ld      d,b
        ld      e,b
        ret
vm_long:ld      de,0
        ld      bc,0
        ld      a,5
        ld      (vlq_cnt),a
vm_l:   ld      a,(hl)
        inc     hl
        ld      (vlq_byte),a
        ld      d,e
        ld      e,b
        ld      b,c
        ld      c,0
        srl     d
        rr      e
        rr      b
        rr      c
        ld      a,(vlq_byte)
        and     7Fh
        or      c
        ld      c,a
        ld      a,(vlq_byte)
        rla
        ret     nc
        ld      a,(vlq_cnt)
        dec     a
        ld      (vlq_cnt),a
        jr      nz,vm_l
        ret

; ---- 가변 길이 수 하나 읽기: IX = 트랙 -> DE:BC = 값 (DE 가 위쪽).  Carry = 파일이 중간에 끊김 ----
trk_read_vlq:
        ld      a,5
        call    trk_has_room
        jr      c,trk_read_vlq_slow
        call    vlq_from_memory ; 창에 5바이트 이상: 곧바로 읽는다
        ld      (ix+TRK_PTR),l
        ld      (ix+TRK_PTR+1),h
        or      a
        ret
trk_read_vlq_slow:
        ld      de,0
        ld      bc,0
        ld      a,5
        ld      (vlq_cnt),a
rv_l:   call    trk_getb
        ret     c
        ld      (vlq_byte),a
        ld      d,e             ; 값 <<= 7  ( = <<8 후 >>1 )
        ld      e,b
        ld      b,c
        ld      c,0
        srl     d
        rr      e
        rr      b
        rr      c
        ld      a,(vlq_byte)
        and     7Fh
        or      c
        ld      c,a
        ld      a,(vlq_byte)
        rla
        ret     nc
        ld      a,(vlq_cnt)
        dec     a
        ld      (vlq_cnt),a
        jr      nz,rv_l
        ret                     ; 5바이트를 넘게 이어지면 깨진 수: Carry 로 알린다

; ============================================================
;  위층 2: 시간 간격 / 사건
; ============================================================
; 트랙(IX)의 다음 시간 간격을 읽어 delay 에 넣는다.  끝이면 상태 = 2
trk_read_delta:
        ld      a,(ix+TRK_ST)
        cp      1
        jr      nz,td_end
        call    trk_read_vlq    ; (자료가 없으면 Carry 로 돌아온다)
        jr      c,td_end
        ld      (ix+TRK_DLO),c
        ld      (ix+TRK_DLO+1),b
        ld      (ix+TRK_DHI),e
        ld      (ix+TRK_DHI+1),d
        ret
td_end: ld      (ix+TRK_ST),2
        ret

; ---- 사건 하나 읽기 (IX = 트랙) ----
EV_CHAN equ     0
EV_META equ     1
EV_SYSX equ     2
EV_END  equ     3
ev_kind:  defb  0
ev_st:    defb  0
ev_d1:    defb  0
ev_d2:    defb  0
ev_mtype: defb  0
ev_mptr:  defw  0               ; 메타/SysEx 자료: 언제나 EVBUF (앞쪽 256바이트까지만 들어 있다)
ev_mlen:  defw  0               ; 자료의 실제 길이 (256보다 클 수 있다)

trk_read_event:
        ld      a,(ix+TRK_ST)
        cp      1
        jr      nz,te_end
        ld      a,3
        call    trk_has_room    ; 상태 + 자료 2바이트가 창 안에 다 있으면
        jr      c,te_slow
        ld      a,(hl)
        cp      0F0h
        jr      c,te_fast       ; 채널 사건은 곧바로 읽는다
te_slow:call    trk_at_end      ; 느린 길: 창을 채워 가며 한 바이트씩
        jr      c,te_end
        call    trk_getb
        jr      c,te_end
        cp      80h
        jr      c,te_run
        cp      0F0h
        jr      nc,te_sys
        ld      (ix+TRK_RUN),a  ; 새 상태 바이트
        ld      (ev_st),a
        call    trk_getb
        jr      c,te_end
        jr      te_d1
te_run: ld      c,a             ; 상태 생략(러닝 스테이터스): 읽은 바이트가 첫 자료
        ld      a,(ix+TRK_RUN)
        or      a
        jr      z,te_end        ; 상태가 없는데 자료 바이트: 깨진 파일
        ld      (ev_st),a
        ld      a,c
te_d1:  ld      (ev_d1),a
        ld      a,(ev_st)
        and     0F0h
        cp      0C0h
        jr      z,te_one
        cp      0D0h
        jr      z,te_one
        call    trk_getb
        jr      c,te_end
        ld      (ev_d2),a
        jr      te_ch
te_one: xor     a
        ld      (ev_d2),a
te_ch:  ld      a,EV_CHAN
        ld      (ev_kind),a
        ret
te_sys: ld      (ev_st),a
        cp      0FFh
        jr      z,te_meta
        call    trk_meta_body   ; F0 / F7 : 길이 + 자료
        ld      a,EV_SYSX
        jr      te_fin
te_meta:call    trk_getb
        jr      c,te_end
        ld      (ev_mtype),a
        call    trk_meta_body
        ld      a,EV_META
te_fin: ld      (ev_kind),a
        ld      (ix+TRK_RUN),0  ; 메타 / 시스템 사건은 러닝 스테이터스를 끊는다
        ret
te_end: ld      a,EV_END
        ld      (ev_kind),a
        ld      (ix+TRK_ST),2
        ret

; 채널 사건 빠른 길: HL = 읽을 자리 (창에 3바이트 이상 있다)
te_fast:ld      a,(hl)
        cp      80h
        jr      c,tf_run
        inc     hl
        ld      (ix+TRK_RUN),a
        jr      tf_have
tf_run: ld      a,(ix+TRK_RUN)
        or      a
        jr      z,te_end        ; 상태가 없는데 자료 바이트: 깨진 파일
tf_have:ld      (ev_st),a
        and     0F0h
        cp      0C0h
        jr      z,tf_one
        cp      0D0h
        jr      z,tf_one
        ld      a,(hl)
        inc     hl
        ld      (ev_d1),a
        ld      a,(hl)
        inc     hl
        ld      (ev_d2),a
        jr      tf_fin
tf_one: ld      a,(hl)
        inc     hl
        ld      (ev_d1),a
        xor     a
        ld      (ev_d2),a
tf_fin: ld      (ix+TRK_PTR),l
        ld      (ix+TRK_PTR+1),h
        ld      a,EV_CHAN
        ld      (ev_kind),a
        ret

; 길이 + 자료를 읽어 EVBUF 에 담는다 (256바이트를 넘는 뒷부분은 읽지 않고 건너뛴다: trk_skip)
trk_meta_body:
        call    trk_read_vlq
        jr      c,mb_bad
        ld      a,d
        or      e
        jr      z,mb_len
mb_bad: ld      bc,0FFFFh       ; 4바이트 길이는 있을 수 없다: 끝까지 버린다
mb_len: ld      (ev_mlen),bc
        ld      hl,EVBUF
        ld      (ev_mptr),hl
        ld      a,b
        or      a
        jr      nz,mb_cap
        ld      d,b
        ld      e,c             ; DE = 담을 양 = 길이
        ld      bc,0            ; BC = 건너뛸 양
        jr      mb_copy
mb_cap: dec     b               ; 길이 >= 256: 앞 256바이트만 담고
        ld      de,EVBUF_SZ     ; 나머지(길이-256)는 건너뛴다
mb_copy:push    bc
mb_l:   ld      a,d
        or      e
        jr      z,mb_rest
        dec     de
        push    hl
        call    trk_getb
        pop     hl
        jr      c,mb_short
        ld      (hl),a
        inc     hl
        jr      mb_l
mb_rest:pop     bc
        ld      a,b
        or      c
        ret     z
        jp      trk_skip
mb_short:
        pop     bc
        ret

; IX 트랙에서 BC 바이트를 건너뛴다.  창 안이면 포인터만 옮기고, 창을 넘으면 창을 비우고
; 파일 위치 / 남은 길이만 조정한다 (디스크는 읽지 않으므로 인터럽트 안에서도 된다).  Carry = 파일 끝을 넘음
trk_skip:
        call    trk_window_left ; DE = 창에 남은 바이트
        ld      h,b
        ld      l,c
        or      a
        sbc     hl,de           ; HL = BC - 남은 바이트
        jr      nc,sk_past
        ld      l,(ix+TRK_PTR)
        ld      h,(ix+TRK_PTR+1)
        add     hl,bc
        ld      (ix+TRK_PTR),l
        ld      (ix+TRK_PTR+1),h
        or      a
        ret
sk_past:ld      a,(ix+TRK_END)  ; 창을 다 쓰고
        ld      (ix+TRK_PTR),a
        ld      a,(ix+TRK_END+1)
        ld      (ix+TRK_PTR+1),a
        ld      b,h             ; BC = 창 밖으로 넘치는 양
        ld      c,l
        ld      a,b
        or      c
        ret     z
        ld      a,(ix+TRK_REM+2)
        or      (ix+TRK_REM+3)
        jr      nz,sk_adv
        ld      l,(ix+TRK_REM)
        ld      h,(ix+TRK_REM+1)
        or      a
        sbc     hl,bc
        jr      nc,sk_adv       ; 남은 길이 >= 넘치는 양
        ld      (ix+TRK_REM),0  ; 파일 끝을 넘는다: 더 읽을 것이 없다
        ld      (ix+TRK_REM+1),0
        ld      (ix+TRK_REM+2),0
        ld      (ix+TRK_REM+3),0
        scf
        ret
sk_adv: call    trk_advance_read
        or      a
        ret

; ---- 모든 트랙을 처음으로 (첫 시간 간격까지 읽어 둔다) ----
trk_rewind:
        ld      ix,TRKS
        ld      b,TRK_MAX
tr_l:   push    bc
        ld      a,(ix+TRK_USE)
        or      a
        jr      z,tr_n
        call    trk_start
        ld      (ix+TRK_ST),1
        call    trk_read_delta
tr_n:   ld      bc,TRK_SZ
        add     ix,bc
        pop     bc
        djnz    tr_l
        ret

; ============================================================
;  위층 3: 선읽기 (메인 루프에서만 부른다)
;  인터럽트 엔진이 창에서 자료를 꺼내는 동안, 모자라기 전에 창 뒷부분을 파일에서 채워 둔다.
;   1) 인터럽트를 잠깐 끄고 남은 자료를 창 맨 앞으로 모은다 (최대 PF_LOW 바이트)
;   2) 인터럽트를 켠 채 디스크에서 창의 빈 곳으로 읽어 온다 (엔진은 END 뒤를 보지 않으므로 안전)
;   3) 인터럽트를 잠깐 끄고 END / 파일 위치 / 남은 길이를 갱신한다
; ============================================================
PF_LOW      equ 160             ; 창에 이만큼보다 적게 남으면 채운다
PF_PER_PASS equ 2               ; 메인 루프 한 바퀴에 읽는 트랙 수 (화면 갱신이 늦어지지 않게)
pf_budget:  defb 0
pf_left:    defw 0
pf_dest:    defw 0

prefetch_pass:
        ld      a,PF_PER_PASS
        ld      (pf_budget),a
        ld      ix,TRKS
        ld      b,TRK_MAX
pp_l:   push    bc
        call    prefetch_if_needed
        pop     bc
        ld      a,(pf_budget)
        or      a
        ret     z
        ld      de,TRK_SZ
        add     ix,de
        djnz    pp_l
        ret

; IX 트랙의 창이 모자라면 채운다 (IX 보존)
prefetch_if_needed:
        ld      a,(ix+TRK_ST)
        cp      1
        ret     nz
        ld      a,(ix+TRK_REM)
        or      (ix+TRK_REM+1)
        or      (ix+TRK_REM+2)
        or      (ix+TRK_REM+3)
        ret     z               ; 파일에서 더 읽을 자료가 없다
        ld      a,(ix+TRK_NEED)
        or      a
        jr      nz,pf_go        ; 엔진이 모자라서 멈춘 트랙은 무조건
        call    ints_save_di    ; 창에 남은 양을 잴 때 PTR 이 도중에 바뀌지 않게
        ld      c,a
        call    trk_window_left ; DE = 남은 바이트
        ld      a,c
        call    ints_restore
        ld      a,d
        or      a
        ret     nz              ; 256 바이트 이상 남았다
        ld      a,e
        cp      PF_LOW
        ret     nc
pf_go:  ld      hl,pf_budget
        dec     (hl)
        ; ---- 1) 남은 자료를 창 맨 앞으로 ----
        call    ints_save_di
        push    af
        ld      l,(ix+TRK_PTR)
        ld      h,(ix+TRK_PTR+1)
        ld      e,(ix+TRK_BUF)
        ld      d,(ix+TRK_BUF+1)
        ld      a,(ix+TRK_END)
        sub     l
        ld      c,a
        ld      a,(ix+TRK_END+1)
        sbc     a,h
        ld      b,a             ; BC = 남은 바이트
        ld      (pf_left),bc
        or      c
        jr      z,pf_moved
        ldir
pf_moved:
        ld      l,(ix+TRK_BUF)
        ld      h,(ix+TRK_BUF+1)
        ld      (ix+TRK_PTR),l
        ld      (ix+TRK_PTR+1),h
        ld      de,(pf_left)
        add     hl,de
        ld      (ix+TRK_END),l
        ld      (ix+TRK_END+1),h
        ld      (pf_dest),hl    ; 새로 읽은 자료를 붙일 자리
        pop     af
        call    ints_restore
        ; ---- 2) 디스크에서 읽기: 읽을 양 = min(남은 길이, 창의 빈 곳) ----
        ld      hl,TBUF_SZ
        ld      de,(pf_left)
        or      a
        sbc     hl,de
        ld      b,h
        ld      c,l             ; BC = 창의 빈 곳
        ld      a,b
        or      c
        ret     z               ; 창이 이미 가득
        call    trk_clamp_len
        ld      de,(pf_dest)
        call    trk_pread       ; BC = 읽은 바이트
        jr      c,pf_fail
        ; ---- 3) 갱신 ----
        call    ints_save_di
        push    af
        ld      l,(ix+TRK_END)
        ld      h,(ix+TRK_END+1)
        add     hl,bc
        ld      (ix+TRK_END),l
        ld      (ix+TRK_END+1),h
        call    trk_advance_read        ; 파일 위치 += BC, 남은 길이 -= BC
        ld      (ix+TRK_NEED),0
        pop     af
        jp      ints_restore
pf_fail:call    trk_rem_clear   ; 읽기 실패 / 파일이 모자람: 더 읽지 않는다 (창이 비면 트랙이 끝난다)
        ld      (ix+TRK_NEED),0
        ret
