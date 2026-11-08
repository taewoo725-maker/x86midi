; ============================================================
; 만든이: KimTaeWoo
; 도와준이: Lee.J.H
;  tick.inc - 1ms 타이머 틱 엔진 (V1.4.0)
;
;  왜: 프레임(1/60 초) 단위로 엔진이 돌면 사건이 프레임 경계에 모여 나가 박자가 16ms 단위로 떨린다.
;      MSX-MIDI 카트리지의 8253 카운터2 는 1ms 마다 인터럽트를 낼 수 있어서, 이것으로 곡 시간을 센다.
;  방식:
;   - 8253 카운터2 = 모드2, 카운트 4000 (클럭이 4MHz 면 1ms).  클럭이 기종마다 다를 수 있어 시작할 때 프레임 30개 동안
;     깃발이 몇 번 서는지 세어 실제 간격 tick_p10 (0.1us 단위)을 잰다.  8251 명령 비트1(DTR)=1 이면 인터럽트가 나가고,
;     8251 상태 비트7 이 타이머 깃발이다.  OUT (EAh) 로 깃발을 지운다.
;   - BIOS 인터럽트 루틴은 H.KEYI(FD9Ah) 를 먼저 부르므로 거기에 건다.  우리 깃발일 때만 처리하고 항상 원래 훅으로 이어 간다.
;   - 인터럽트마다 ms_acc += ms_add.  ms_add = 템포% * PPQ * tick_p10 / 1000 (간격이 1ms 면 10 * 템포% * PPQ),  한 틱의 길이 = tempo_us 이므로
;     ms_acc >= tempo_us 인 만큼 틱이 지났다 (나눗셈 없이 빼기로 센다).
;   - 프레임 인터럽트(H.TIMI)는 프레임 세기(play_frames, song_frames)만 맡고, 엔진(트랙 진행)은 1ms 쪽이 맡는다.
;   - 타이머가 없는 기종은 깃발이 안 내려가거나 안 올라오므로 감지에 실패하고 예전 프레임 방식으로 돈다.
;  공개:
;   tick_start / tick_stop : isr_install / isr_remove 가 부른다 (인터럽트 끈 채로)
;   tick_on                : 1 = 1ms 틱 방식으로 도는 중
;   tick_calc_add          : 템포가 바뀔 때 ms_add 를 다시 계산 (tempo_apply 가 부른다)
; ============================================================
HOOK_KEYI   equ 0FD9Ah          ; 인터럽트 첫 훅 (5바이트)
T8253_C2    equ 0EEh            ; 8253 카운터2 데이터
T8253_CTL   equ 0EFh            ; 8253 제어
TIMER_ACK   equ 0EAh            ; 아무 값이나 쓰면 타이머 깃발이 지워진다
TICK_COUNT  equ 4000            ; 카운터2 에 주는 값.  클럭이 4MHz 면 1ms (기종에 따라 클럭이 달라서 실제 간격은 아래 보정으로 잰다)
TICK_CAL_FRAMES equ 30         ; 간격을 재는 프레임 수 (약 0.5초)
TICK_SNAP   equ 150            ; 잰 값이 정확한 값에서 이 안(0.1us 단위, 약 1.5%)이면 정확한 값을 쓴다
TICK_P10_358 equ 11174         ; 3.579545MHz / 4000 = 1117.4us
TICK_P10_MIN equ 7000          ; 잰 간격이 이 범위(0.1us 단위: 0.7ms ~ 1.4ms)를 벗어나면 타이머가 아니라고 본다
TICK_P10_MAX equ 14000
TICK_DETECT_WAIT equ 800        ; 깃발이 올라오기를 기다리는 횟수 (1ms 는 약 140번)
CMD_RUN     equ 05h             ; 8251 명령: TxEN + RxEN (타이머 인터럽트 끔)
CMD_RUN_TMR equ 07h             ; 위 + DTR (타이머 인터럽트 켬)

keyi_old:   defb 0C9h,0C9h,0C9h,0C9h,0C9h
tick_want:  defb 1              ; 0 = 명령줄 /F 로 1ms 틱을 쓰지 않기로 했다
tick_on:    defb 0              ; 1 = 1ms 틱 방식으로 돌고 있다
tick_state: defb 0              ; 진단: 0 시도 안 함, 1 사용 중, 2 감지 실패, 3 스위치로 끔, 4 포트 없음
tick_irqs:  defw 0              ; 진단: 1ms 인터럽트 횟수
tick_ran:   defw 0              ; 진단: 엔진이 틱을 진행시킨 횟수
tick_adv_max: defb 0            ; 진단: 한 번에 가장 많이 진행한 틱
tick_p10_raw: defw 0            ; 진단: 맞추기 전에 잰 간격
tick_p10:   defw 10000          ; 잰 인터럽트 간격 (0.1us 단위, 1ms = 10000)
ms_add:     defs 4              ; 인터럽트마다 더하는 값 (템포% * PPQ * tick_p10 / 1000)
ms_acc:     defs 4              ; 쌓인 값
tick_len:   defs 4              ; 한 틱의 길이 = tempo_us (4바이트로 늘린 것)

; ---- 템포가 바뀔 때: ms_add 와 tick_len 을 다시 계산 (tempo_apply 안에서: v_n 등 작업 칸은 이미 보호되어 있다) ----
tick_calc_add:
        ld      a,(tempo_pct)
        ld      l,a
        ld      h,0
        ld      (v_k),hl
        ld      hl,0
        ld      (v_k+2),hl
        ld      hl,(smf_ppq)
        call    mul32_by_hl     ; v_n = 템포% * PPQ
        ld      hl,v_n          ; v_k = v_n
        ld      de,v_k
        ld      bc,4
        ldir
        ld      hl,(tick_p10)
        call    mul32_by_hl     ; v_n = 템포% * PPQ * 간격
        ld      hl,1000
        ld      (v_d),hl
        ld      hl,0
        ld      (v_d+2),hl
        call    div32           ; / 1000
        ld      hl,v_n
        ld      de,ms_add
        ld      bc,4
        ldir
        ld      hl,tmp_tempo    ; tempo_us (3바이트) -> 4바이트
        ld      de,tick_len
        ld      bc,3
        ldir
        xor     a
        ld      (tick_len+3),a
        ret

; ---- 시작: 감지에 성공하면 훅을 걸고 타이머 인터럽트를 켠다 (인터럽트 끈 채로 부른다) ----
tick_start:
        ld      a,(tick_on)
        or      a
        ret     nz
        ld      a,(tick_want)
        or      a
        ld      a,3
        jr      z,ts_set
        ld      a,(set_port)
        cp      PORT_NONE
        ld      a,4
        jr      z,ts_set
        call    tick_detect
        ld      a,2
        jr      c,ts_set        ; 감지 실패
        ld      hl,HOOK_KEYI    ; 원래 훅을 보관하고 우리 것을 건다
        ld      de,keyi_old
        ld      bc,5
        ldir
        ld      a,0C3h
        ld      (HOOK_KEYI),a
        ld      hl,keyi_entry
        ld      (HOOK_KEYI+1),hl
        ld      hl,0
        ld      (ms_acc),hl
        ld      (ms_acc+2),hl
        ld      (tick_balance),hl
        ld      (pend_ticks),hl
        xor     a
        ld      (tick_exp_frac),a
        ld      (tick_serviced),a
        ld      (tick_credit),a
        call    tempo_apply     ; ms_add / tick_len 준비 (인터럽트가 꺼져 있고 메인 쪽 호출이다)
        out     (TIMER_ACK),a   ; 깃발을 지우고
        ld      a,CMD_RUN_TMR
        out     (MIDI_STAT),a   ; 타이머 인터럽트를 켠다
        ld      a,1
        ld      (tick_on),a
        ld      hl,0            ; 워치독 처음부터
        ld      (wd_frames),hl
        ld      a,MQ_BURST_TICK
        ld      (pump_burst),a
        ld      a,1
ts_set: ld      (tick_state),a
        ret

; ---- 끝: 타이머 인터럽트를 끄고 카운터를 세우고 훅을 되돌린다 (인터럽트 끈 채로) ----
tick_stop:
        ld      a,(tick_on)
        or      a
        ret     z
        ld      a,CMD_RUN
        out     (MIDI_STAT),a   ; 인터럽트 선을 먼저 끈다 (안 끄면 BIOS 가 끝없이 인터럽트에 빠진다)
        out     (TIMER_ACK),a
        ld      a,0B0h          ; 카운터2: 모드0, 카운트를 안 주면 멈춘다
        out     (T8253_CTL),a
        ld      hl,keyi_old
        ld      de,HOOK_KEYI
        ld      bc,5
        ldir
        xor     a
        ld      (tick_on),a
        ld      a,MQ_BURST
        ld      (pump_burst),a
        ret

; midi_init 가 8251 명령을 05h 로 되돌린 뒤 불린다: 1ms 틱을 쓰는 중이면 타이머 인터럽트를 다시 켠다
tick_reassert:
        ld      a,(tick_on)
        or      a
        ret     z
        ld      a,CMD_RUN_TMR
        out     (MIDI_STAT),a
        ret

; 감지 + 간격 재기: 카운터2 를 맞추고 (1) 깃발이 "지우면 내려가고 곧 다시 올라오는지" 두 번 확인한 뒤
;  (2) 프레임 TICK_CAL_FRAMES 개 동안 깃발이 선 횟수로 tick_p10 을 잰다.  Carry = 실패
;  (장치가 없으면 포트가 FFh 로 읽혀 지워도 안 내려가므로 실패한다)
tick_detect:
        ld      a,0B4h          ; 카운터2: 아래/위 바이트, 모드2, 이진
        out     (T8253_CTL),a
        ld      a,TICK_COUNT mod 256
        out     (T8253_C2),a
        ld      a,TICK_COUNT/256
        out     (T8253_C2),a
        ld      a,CMD_RUN_TMR
        out     (MIDI_STAT),a
        call    td_cycle
        jr      c,td_fail
        call    td_cycle
        jr      c,td_fail
        call    td_measure
        ret     nc
td_fail:ld      a,CMD_RUN       ; 실패: 인터럽트 선을 끄고 카운터를 세운다
        out     (MIDI_STAT),a
        out     (TIMER_ACK),a
        ld      a,0B0h
        out     (T8253_CTL),a
        scf
        ret
; 깃발 지움 -> 내려갔나 -> 다시 올라오나.  Carry = 실패
td_cycle:
        out     (TIMER_ACK),a
        in      a,(MIDI_STAT)
        rla
        jr      c,td_bad        ; 지웠는데 안 내려간다
        ld      hl,TICK_DETECT_WAIT
td_w:   in      a,(MIDI_STAT)
        rla
        jr      c,td_ok         ; 올라왔다
        dec     hl
        ld      a,h
        or      l
        jr      nz,td_w
td_bad: scf
        ret
td_ok:  or      a               ; Carry 끔 = 성공
        ret

; HL = 잰 간격, DE = 알려진 정확한 값, BC = 허용 오차.  |HL-DE| <= BC 이면 HL = DE (프레임 30개로는 0.2% 정도 오차가 나서 그만큼 박자가 어긋나기 때문)
td_snap:
        push    hl
        or      a
        sbc     hl,de
        jr      nc,tsn_p
        ld      a,l             ; 음수: 부호를 뒤집는다
        cpl
        ld      l,a
        ld      a,h
        cpl
        ld      h,a
        inc     hl
tsn_p:  or      a
        sbc     hl,bc
        pop     hl
        ret     nc              ; 오차보다 멀다: 잰 값 그대로
        ex      de,hl           ; 가깝다: 정확한 값으로
        ret

; 프레임 TICK_CAL_FRAMES 개 동안 깃발이 선 횟수를 세어 tick_p10 = 프레임 시간 / 횟수 를 구한다.  Carry = 범위 밖
td_measure:
td_sync:in      a,(99h)         ; 프레임이 막 시작될 때까지 기다린다 (VDP 상태 비트7)
        rla
        jr      nc,td_sync
        out     (TIMER_ACK),a
        ld      hl,0            ; HL = 깃발이 선 횟수
        ld      b,TICK_CAL_FRAMES
td_m:   in      a,(MIDI_STAT)
        rla
        jr      nc,td_f
        out     (TIMER_ACK),a
        inc     hl
td_f:   in      a,(99h)
        rla
        jr      nc,td_m
        djnz    td_m
        ld      a,h
        or      l
        jp      z,td_range_bad
        ex      de,hl           ; v_d = 횟수 * 256
        xor     a
        ld      (v_d),a
        ld      (v_d+3),a
        ld      a,e
        ld      (v_d+1),a
        ld      a,d
        ld      (v_d+2),a
        call    load_frame_k    ; v_n = 프레임 시간 (0.1us 단위) * 256 * 프레임 수
        ld      hl,v_k
        ld      de,v_n
        ld      bc,4
        ldir
        ld      a,TICK_CAL_FRAMES
        call    mul32_8
        ld      a,10
        call    mul32_8
        call    div32           ; v_n = 프레임 시간 * 프레임 수 / 횟수 = 간격 (0.1us 단위)
        ld      hl,(v_n+2)
        ld      a,h
        or      l
        jp      nz,td_range_bad
        ld      hl,(v_n)
        ld      de,TICK_P10_MIN
        or      a
        sbc     hl,de
        jp      c,td_range_bad
        ld      hl,(v_n)
        ld      de,TICK_P10_MAX+1
        or      a
        sbc     hl,de
        jp      nc,td_range_bad
        ld      hl,(v_n)
        ld      (tick_p10_raw),hl       ; 진단: 맞추기 전에 잰 값
        ld      de,10000        ; 클럭이 4MHz 인 기종 (카운트 4000 -> 정확히 1ms): 잰 값이 가까우면 정확한 값으로 맞춘다
        ld      bc,TICK_SNAP
        call    td_snap
        ld      de,TICK_P10_358 ; 클럭이 3.579545MHz 인 기종
        call    td_snap
        ld      (tick_p10),hl
        ld      (v_d),hl        ; 한 프레임에 올 횟수 (8.8) = 프레임 시간 * 256 / 간격
        ld      hl,0
        ld      (v_d+2),hl
        call    load_frame_k
        ld      hl,v_k
        ld      de,v_n
        ld      bc,4
        ldir
        ld      a,10
        call    mul32_8
        call    div32
        ld      hl,(v_n)
        ld      (tick_e88),hl
        or      a
        ret
td_range_bad:
        scf
        ret

; ---- H.KEYI 훅: 1ms 타이머 인터럽트일 때만 처리하고, 언제나 원래 훅으로 이어 간다 ----
;  BIOS 는 훅을 부르기 전에 모든 레지스터를 이미 저장했지만, 훅이 어떻게 불리는지는 기종마다 달라질 수 있으니
;  쓰는 레지스터는 직접 지킨다.  대부분의 1ms 는 시간만 세고 끝나는 짧은 길(AF,BC,DE,HL 만 보존)로 처리하고,
;  사건이 올 때가 되었을 때만 모든 레지스터를 저장하고 전용 스택으로 바꾸는 무거운 길로 간다.
keyi_entry:
        push    af
        in      a,(MIDI_STAT)
        rla
        jr      c,ke_mine       ; 상태 비트7 = 타이머 깃발
        pop     af
        jp      keyi_old
ke_mine:out     (TIMER_ACK),a   ; 깃발을 지운다
        push    bc
        push    de
        push    hl
        ld      hl,(tick_irqs)
        inc     hl
        ld      (tick_irqs),hl
        ld      hl,tick_serviced
        inc     (hl)
        call    tick_count      ; 시간을 세고, 트랙을 진행시켜야 하면 A = 1
        or      a
        jr      nz,ke_heavy
        call    midi_pump       ; 쌓인 바이트를 조금씩 내보낸다 (MQ_BURST_TICK)
        pop     hl
        pop     de
        pop     bc
        pop     af
        jp      keyi_old
ke_heavy:                       ; 사건이 올 때가 되었다: 모든 레지스터를 저장하고 엔진을 돌린다
        pop     hl
        pop     de
        pop     bc
        pop     af
        ISR_SAVE
        ld      a,1
        ld      (isr_mode),a
        call    tick_run_engine
        xor     a
        ld      (isr_mode),a
        call    midi_pump
        ISR_RESTORE
        jp      keyi_old

; 인터럽트 한 번(약 1ms)이 흘렀다: 쌓은 시간으로 틱을 센다.  A = 1 이면 트랙을 진행시킬 때가 되었다
;  트랙마다 "다음 사건까지 남은 틱"이 있으니 그 중 가장 작은 값(min_wait)보다 적게 쌓였으면 트랙을 건드릴 필요가 없다.
;  (쌓인 틱을 한꺼번에 주어도 결과는 같다.  이 덕분에 대부분의 1ms 는 아주 짧게 끝난다)
tick_count:
        call    isr_gate        ; 잠김 / 재생 중 아님 / 곡 끝 / 출력 큐 밀림 이면 쉰다
        jr      z,tc_go
        xor     a
        ret
tc_go:  ld      hl,(ms_acc)     ; ms_acc += ms_add
        ld      de,(ms_add)
        add     hl,de
        ld      (ms_acc),hl
        ld      hl,(ms_acc+2)
        ld      de,(ms_add+2)
        adc     hl,de
        ld      (ms_acc+2),hl
tick_core:                      ; (놓친 인터럽트를 보충할 때는 여기로 들어온다: ms_acc 에 이미 더했다)
        ld      bc,0            ; BC = 이번에 지난 틱 수
ta_l:   ld      hl,(ms_acc)     ; ms_acc - tick_len  (한 틱 길이 이상 쌓였으면 한 틱 지났다)
        ld      de,(tick_len)
        or      a
        sbc     hl,de
        push    hl
        ld      hl,(ms_acc+2)
        ld      de,(tick_len+2)
        sbc     hl,de
        jr      c,ta_end        ; 모자란다
        ld      (ms_acc+2),hl
        pop     hl
        ld      (ms_acc),hl
        inc     bc
        ld      a,b
        or      a
        jr      z,ta_l          ; 한 번에 최대 255틱 (너무 빠른 템포에서 오래 돌지 않게)
        dec     bc
        push    hl              ; 아래 ta_end 의 pop 과 맞춘다
ta_end: pop     hl
        ld      a,b
        or      c
        ret     z               ; 아직 한 틱도 안 지났다 (A = 0)
        ld      hl,(now_tick)   ; now_tick += BC
        add     hl,bc
        ld      (now_tick),hl
        jr      nc,ta_nc
        ld      hl,(now_tick+2)
        inc     hl
        ld      (now_tick+2),hl
ta_nc:  ld      hl,(pend_ticks) ; 아직 트랙에 주지 않은 틱
        add     hl,bc
        jr      nc,ta_pn
        ld      hl,0FFFFh
ta_pn:  ld      (pend_ticks),hl
        ld      a,(starve_hold) ; 모자란 트랙이 있으면 프레임 인터럽트가 풀어 줄 때까지 틱만 쌓는다
        or      a
        jr      z,ta_sh
        xor     a
        ret
ta_sh:  ld      de,(min_wait)   ; 가장 빠른 사건까지 남은 틱보다 적으면 건너뛴다
        or      a
        sbc     hl,de
        ld      a,0
        ret     c
        inc     a               ; A = 1: 트랙을 진행시킬 때
        ret

; 쌓인 틱을 트랙에 주고 모든 트랙을 진행시킨다 (isr_mode = 1, 모든 레지스터 저장한 상태에서)
tick_run_engine:
        ld      hl,(pend_ticks)
        ld      (adv),hl
        ld      hl,0
        ld      (pend_ticks),hl
        ld      a,(adv)         ; 진단: 한 번에 준 틱 (아래 바이트)
        ld      hl,tick_adv_max
        cp      (hl)
        jr      c,tr_nm
        ld      (hl),a
tr_nm:  ld      hl,(tick_ran)
        inc     hl
        ld      (tick_ran),hl
        jp      ef_tracks
pend_ticks: defw 0

; ------------------------------------------------------------
;  멈춤 방지 (워치독)
;   1ms 인터럽트가 CPU 를 거의 다 써서 메인(키 / 화면)이 못 돌면 키가 먹지 않는다.
;   메인 루프가 한 바퀴 돌 때마다 main_beat 가 늘어나므로, 프레임 인터럽트가 이것이 TICK_WATCHDOG_FRAMES 동안
;   그대로이면 1ms 틱을 끄고 프레임 방식으로 돌아간다 (SETUP 에 STATE 5 로 나온다).
; ------------------------------------------------------------
TICK_WATCHDOG_FRAMES equ 600    ; 약 10초 (화면 전환 때 메인이 몇 초씩 걸리는 것은 정상이다)
main_beat:  defb 0              ; 메인 루프가 돌 때마다 +1
wd_last:    defb 0
wd_frames:  defw 0

; 프레임 인터럽트마다 (1ms 틱 방식일 때): 메인이 살아 있는지 보고, 놓친 인터럽트도 센다
tick_frame_watch:
        call    tick_watchdog
        ld      a,(tick_on)
        or      a
        ret     z               ; 방금 껐다
        jp      tick_frame_balance

tick_watchdog:
        ld      a,(main_beat)
        ld      hl,wd_last
        cp      (hl)
        jr      z,tw_still
        ld      (hl),a          ; 메인이 돌았다
        ld      hl,0
        ld      (wd_frames),hl
        ret
tw_still:
        ld      hl,(wd_frames)
        inc     hl
        ld      (wd_frames),hl
        ld      de,TICK_WATCHDOG_FRAMES
        or      a
        sbc     hl,de
        ret     c
        call    tick_stop       ; 타이머 인터럽트를 끄고 훅을 되돌린다 (tick_on = 0)
        ld      a,5
        ld      (tick_state),a
        jp      tempo_apply_isr ; 프레임 방식이 쓰는 프레임당 틱을 다시 구한다 (1ms 방식에서는 건너뛰었다)

; ------------------------------------------------------------
;  놓친 인터럽트 보충
;   다른 곳에서 인터럽트가 오래 꺼져 있으면 1ms 인터럽트가 겹쳐서(깃발 하나로) 몇 번 사라진다.  그대로 두면 곡이 느려진다.
;   프레임 인터럽트(정확히 1/fps 초)를 기준으로 "한 프레임에 와야 할 횟수" 와 "실제로 처리한 횟수" 를 비교해
;   모자란 만큼을 시간으로 채워 넣는다.  (프레임 경계에서 한두 번 어긋나는 것은 balance 로 모아서 무시한다)
; ------------------------------------------------------------
tick_e88:       defw 0          ; 한 프레임에 올 인터럽트 수 (8.8 고정소수점)
tick_exp_frac:  defb 0
tick_serviced:  defb 0          ; 이번 프레임 동안 처리한 1ms 인터럽트 수
tick_balance:   defw 0          ; (와야 할 수 - 처리한 수) 의 누적.  -4 아래로는 내려가지 않는다
tick_credit:    defb 0          ; 이번 프레임에 보충할 횟수
tick_lost:      defw 0          ; 진단: 보충한 횟수 합계
TICK_CREDIT_MAX equ 24

; 프레임 인터럽트마다 (1ms 틱 방식일 때, 엔진 가능 여부와 상관없이): 보충할 횟수 tick_credit 를 정한다
tick_frame_balance:
        ld      hl,(tick_e88)   ; L = 소수, H = 정수
        ld      a,(tick_exp_frac)
        add     a,l
        ld      (tick_exp_frac),a
        ld      a,h
        adc     a,0
        ld      e,a             ; E = 이번 프레임에 와야 할 횟수
        ld      hl,tick_serviced
        ld      a,e
        sub     (hl)            ; 와야 할 수 - 처리한 수 (부호 있음)
        ld      (hl),0
        ld      c,a
        sbc     a,a
        ld      b,a             ; BC = 부호 확장
        ld      hl,(tick_balance)
        add     hl,bc
        bit     7,h
        jr      z,tfb_pos
        ld      a,h             ; 음수: -4 아래로는 내리지 않는다
        inc     a
        jr      nz,tfb_clamp
        ld      a,l
        cp      0FCh
        jr      nc,tfb_store
tfb_clamp:
        ld      hl,0FFFCh
        jr      tfb_store
tfb_pos:ld      a,h
        or      a
        jr      nz,tfb_big
        ld      a,l
        cp      2
        jr      c,tfb_store     ; 1 이하: 위상 어긋남으로 보고 모아 둔다
        dec     a               ; A = balance - 1 만큼 보충
        cp      TICK_CREDIT_MAX+1
        jr      c,tfb_set
tfb_big:ld      a,TICK_CREDIT_MAX
tfb_set:ld      (tick_credit),a
        ld      hl,1            ; balance 는 1 로 되돌린다
        ld      (tick_balance),hl
        ret
tfb_store:
        ld      (tick_balance),hl
        xor     a
        ld      (tick_credit),a
        ret

; 엔진을 돌려도 되는 프레임에서: 정해진 만큼 ms_add 를 더해 시간을 채우고 틱을 센다 (isr_mode = 1 에서 부른다)
tick_apply_credit:
        ld      a,(starve_hold) ; 프레임마다 한 번은 모자란 트랙을 다시 시도하게 푼다
        or      a
        jr      z,tac_nh
        xor     a
        ld      (starve_hold),a
        ld      hl,0
        ld      (min_wait),hl
tac_nh: ld      a,(tick_credit)
        or      a
        ret     z
        ld      b,a
        ld      hl,(tick_lost)
        ld      e,a
        ld      d,0
        add     hl,de
        ld      (tick_lost),hl
        xor     a
        ld      (tick_credit),a
tac_l:  ld      hl,(ms_acc)
        ld      de,(ms_add)
        add     hl,de
        ld      (ms_acc),hl
        ld      hl,(ms_acc+2)
        ld      de,(ms_add+2)
        adc     hl,de
        ld      (ms_acc+2),hl
        djnz    tac_l
        call    tick_core
        or      a
        ret     z
        jp      tick_run_engine
