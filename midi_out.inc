; ============================================================
; 만든이: KimTaeWoo
; 도와준이: Lee.J.H
;  midi_out.inc - MIDI 출력 공용부 (MSX-MIDI 8251: 데이터 E8h / 상태·명령 E9h)
;  midi_init  : 8251 초기화 (reset 00 00 00 40 -> mode 4Eh -> command 05h), 8253 카운터0 = 8 분주
;  midi_send  : A = 한 바이트 보내기 (TxRDY 가 될 때까지 기다리되 시간 제한이 있다)
;  midi_msg3  : B,C,D = 상태,데이터1,데이터2 / midi_msg2 : B,C = 상태,데이터1
;  midi_panic : 16채널 All Notes Off + All Sound Off
;  midi_log   : 진단 스위치가 켜져 있으면 보낸 바이트를 ring 에 기록 (diag 스위치, 1바이트 = 0/1)
; ============================================================
MIDI_DATA equ   0E8h
MIDI_STAT equ   0E9h
MIDI_T8253C equ 0EFh            ; 8253 제어
MIDI_T8253D equ 0ECh            ; 8253 카운터 0

midi_init:
        xor     a               ; 출력 큐 / 러닝 스테이터스 처음 상태
        ld      (mq_head),a
        ld      (mq_tail),a
        ld      (rs_last),a
        ld      a,(set_port)
        cp      PORT_NONE
        ret     z
        xor     a
        out     (MIDI_STAT),a
        call    midi_wait16
        out     (MIDI_STAT),a
        call    midi_wait16
        out     (MIDI_STAT),a
        call    midi_wait16
        ld      a,40h
        out     (MIDI_STAT),a   ; reset
        call    midi_wait16
        ld      a,16h           ; 8253 카운터0: 모드3, 아래 바이트만
        out     (MIDI_T8253C),a
        ld      a,8             ; 4MHz / 8 = 500kHz (8251 x16 -> 31.25kbps)
        out     (MIDI_T8253D),a
        ld      a,4Eh           ; 8251 모드: x16, 8비트, 정지 1, 패리티 없음
        out     (MIDI_STAT),a
        call    midi_wait16
        ld      a,05h           ; 명령: TxEN + RxEN
        out     (MIDI_STAT),a
        call    midi_wait16
        jp      tick_reassert   ; 1ms 타이머를 쓰고 있었다면 8251 을 초기화하면서 꺼졌으니 다시 켠다
midi_wait16:
        push    af
        ld      a,8
mw_l:   dec     a
        jr      nz,mw_l
        pop     af
        ret

; ------------------------------------------------------------
;  출력 큐 (V1.3.1)
;   엔진/메인은 바이트를 큐(MQ, 256바이트 고리)에 넣기만 하고, 인터럽트가 매 프레임 midi_pump 로
;   포트에 내보낸다.  그래서 엔진이 포트(TxRDY) 때문에 오래 막히지 않는다.
;   러닝 스테이터스: 직전과 같은 상태 바이트는 생략한다.  노트 오프(8n)는 노트 온 속도 0(9n)으로 보내
;   상태 바이트를 더 많이 생략한다.  SysEx 등 midi_send 로 보내는 것은 러닝 스테이터스를 끊는다.
;   모든 입구는 인터럽트가 꺼진 상태에서 부른다 (메시지 단위 보호).
; ------------------------------------------------------------
MQ_HI       equ MQ/256
MQ_BURST    equ 52              ; 프레임 방식: 인터럽트 한 번에 내보내는 최대 바이트 (31250bps 는 1프레임(60Hz)에 약 52바이트, TxRDY 를 기다리며 보낸다)
MQ_BURST_TICK equ 3             ; 1ms 틱 방식: 31250bps 는 1ms 에 약 3바이트
MQ_HIGH     equ 160             ; 큐에 이만큼 이상 쌓였으면 이번 프레임 엔진은 쉰다 (곡 시간도 같이 멈춘다)
pump_burst: defb MQ_BURST       ; midi_pump 가 한 번에 내보내는 최대 바이트 (1ms 틱 방식이면 MQ_BURST_TICK)
mq_head:    defb 0
mq_tail:    defb 0
rs_last:    defb 0              ; 직전에 보낸 채널 상태 바이트 (0 = 없음)
mq_max:     defb 0              ; 큐가 가장 많이 찼던 양 (진단)
mq_skip:    defw 0              ; 큐가 차서 엔진이 쉰 프레임 수 (진단)
rs_saved:   defw 0              ; 러닝 스테이터스로 아낀 바이트 수 (진단)
mq_out:     defw 0              ; 내보낸 바이트 수 (진단)

; A = 보낼 바이트 (SysEx 등): 러닝 스테이터스를 끊고 큐에 넣는다 (레지스터 모두 보존)
midi_send:
        push    af
        xor     a
        ld      (rs_last),a
        pop     af
        jp      mq_put

; A = 큐에 넣을 바이트 (BC, DE 보존).  큐가 가득 차면 하나를 먼저 내보내 자리를 만든다
mq_put:
        push    hl
        push    bc
        ld      c,a
        ld      a,(set_port)
        cp      PORT_NONE
        jr      z,mqp_none
        ld      a,(mq_head)
        inc     a
        ld      b,a
        ld      a,(mq_tail)
        cp      b
        call    z,midi_out_one  ; 가득 참
        ld      a,(mq_head)
        ld      l,a
        ld      h,MQ_HI
        ld      (hl),c
        inc     a
        ld      (mq_head),a
        pop     bc
        pop     hl
        ret
mqp_none:
        ld      a,c
        call    diag_put
        pop     bc
        pop     hl
        ret

; 큐 맨 앞 바이트를 TxRDY 를 기다렸다가 포트로 보낸다 (큐가 비어 있지 않아야 한다).  A, HL 망가짐
midi_out_one:
        push    bc
        ld      a,(mq_tail)
        ld      l,a
        ld      h,MQ_HI
        ld      c,(hl)
        inc     a
        ld      (mq_tail),a
        ld      hl,2000         ; 시간 제한 (TxRDY 가 영영 안 올 때 멈추지 않게)
mo_w:   in      a,(MIDI_STAT)
        and     01h
        jr      nz,mo_o
        dec     hl
        ld      a,h
        or      l
        jr      nz,mo_w
mo_o:   ld      a,c
        out     (MIDI_DATA),a
        ld      hl,(mq_out)
        inc     hl
        ld      (mq_out),hl
        ld      a,c
        call    diag_put
        pop     bc
        ret

; 인터럽트에서 매 프레임: 큐에서 최대 MQ_BURST 바이트를 내보낸다
midi_pump:
        ld      a,(mq_head)     ; 진단: 가장 많이 찼던 양
        ld      hl,mq_tail
        sub     (hl)
        ld      hl,mq_max
        cp      (hl)
        jr      c,mpu_s
        ld      (hl),a
mpu_s:  ld      a,(pump_burst)
        ld      b,a
mpu_l:  ld      a,(mq_tail)
        ld      hl,mq_head
        cp      (hl)
        ret     z
        push    bc
        call    midi_out_one
        pop     bc
        djnz    mpu_l
        ret

; 메인에서 끝낼 때(인터럽트 훅을 푼 뒤): 큐를 다 비운다
midi_flush:
mfl_q:   ld      a,(mq_tail)
        ld      hl,mq_head
        cp      (hl)
        ret     z
        call    midi_out_one
        jr      mfl_q

; 진단: A = 포트로 나간 바이트를 기록 (진단 스위치가 켜져 있을 때만)
diag_put:
        push    hl
        push    de
        push    af
        ld      a,(diag_on)
        or      a
        jr      z,dp_end
        ld      hl,(midi_log_ptr)
        ld      de,diag_log+DIAG_LOG_SZ
        or      a
        sbc     hl,de
        add     hl,de
        jr      nc,dp_end       ; 기록 영역이 차면 그만 쓴다
        pop     af
        push    af
        ld      (hl),a
        inc     hl
        ld      (midi_log_ptr),hl
dp_end: pop     af
        pop     de
        pop     hl
        ret
midi_log_ptr: defw diag_log

; B = 상태, C = 데이터1, D = 데이터2   (한 메시지는 인터럽트를 끈 채로 통째로 큐에 넣는다)
midi_msg3:
        call    ints_save_di
        push    af
        ld      a,b
        and     0F0h
        cp      80h
        jr      nz,m3_k
        ld      a,b             ; 노트 오프 -> 노트 온 속도 0 (같은 상태 바이트가 이어져 생략된다)
        or      10h
        ld      b,a
        ld      d,0
m3_k:   call    mq_status
        ld      a,c
        call    mq_put
        ld      a,d
        call    mq_put
        pop     af
        jp      ints_restore
; B = 상태, C = 데이터1
midi_msg2:
        call    ints_save_di
        push    af
        call    mq_status
        ld      a,c
        call    mq_put
        pop     af
        jp      ints_restore

; 상태 바이트 B 를 (직전과 다를 때만) 큐에 넣는다.  BC, DE 보존
mq_status:
        ld      a,(rs_last)
        cp      b
        jr      z,mqs_same
        ld      a,b
        ld      (rs_last),a
        jp      mq_put
mqs_same:
        push    hl
        ld      hl,(rs_saved)
        inc     hl
        ld      (rs_saved),hl
        pop     hl
        ret

; 모든 채널 소리 끄기 (CC123 + CC120)
midi_panic:
        ld      e,0
mp_l:   ld      a,e
        or      0B0h
        ld      b,a
        ld      c,123
        ld      d,0
        call    midi_msg3
        ld      a,e
        or      0B0h
        ld      b,a
        ld      c,120
        ld      d,0
        call    midi_msg3
        inc     e
        ld      a,e
        cp      16
        jr      c,mp_l
        ret

; 채널 E 하나만 소리 끄기
midi_chan_off:
        ld      a,e
        or      0B0h
        ld      b,a
        ld      c,123
        ld      d,0
        jp      midi_msg3

; 곡 시작 때 모듈 초기화 : set_reset = 0 없음 / 1 GM / 2 GS / 3 곡 파일에 맡김
midi_reset_module:
        ld      a,(set_reset)
        cp      1
        jr      z,mr_gm
        cp      2
        jr      z,mr_gs
        ret
mr_gm:  ld      hl,sx_gm
        jr      mr_go
mr_gs:  ld      hl,sx_gs
mr_go:  call    ints_save_di
        push    af
        ld      b,(hl)
        inc     hl
mr_l:   ld      a,(hl)
        inc     hl
        call    midi_send
        djnz    mr_l
        pop     af
        jp      ints_restore
sx_gm:  defb    6,0F0h,07Eh,07Fh,009h,001h,0F7h
sx_gs:  defb    11,0F0h,041h,010h,042h,012h,040h,000h,07Fh,000h,041h,0F7h

; 마스터 볼륨 (GM SysEx): A = 0..127
midi_master_vol:
        ld      (mmv_v),a
        call    ints_save_di
        push    af
        ld      hl,mmv_msg
        ld      b,6
mmv_l:  ld      a,(hl)
        call    midi_send
        inc     hl
        djnz    mmv_l
        ld      a,(mmv_v)
        call    midi_send
        ld      a,0F7h
        call    midi_send
        pop     af
        jp      ints_restore
mmv_msg: defb   0F0h,07Fh,07Fh,004h,001h,000h
mmv_v:   defb   0
