; ============================================================
; 만든이: KimTaeWoo
; 도와준이: Lee.J.H
;  isr.inc - 인터럽트(H.TIMI)에서 엔진을 돌린다  (V1.3.0)
;
;  왜: 예전에는 메인 루프가 엔진을 불렀다.  화면을 그리느라 한 바퀴가 길어지면 사건이
;      프레임 경계에서 몰려 나가 박자가 들쭉날쭉했다.  이제는 수직귀선 인터럽트마다 정확히 한 번
;      엔진이 돌고, 메인 루프는 화면 / 키 / 디스크 선읽기만 한다.
;  규칙:
;   - 이 파일과 엔진이 쓰는 코드/자료는 모두 8000h 이상 (X86MIDI.ASM 의 높은 영역)
;   - 인터럽트 안에서는 DOS 를 부르지 않는다 (MSX-DOS 는 다시 들어올 수 없다)
;   - 모든 레지스터를 저장하고, 전용 스택으로 바꿔서 돈다
;   - isr_lock 이 0 이 아니면 엔진을 건너뛴다 (곡 열기 / 건너뛰기 / 처음으로 되감기 중)
;  공개:
;   isr_install / isr_remove : 훅 걸기 / 풀기
;   isr_frames               : 인터럽트 횟수 (메인 루프가 시간 지난 것을 알아채는 데 쓴다)
;   engine_lock / engine_unlock : 엔진 건너뛰기 (겹쳐 불러도 된다)
;   ints_save_di / ints_restore : 인터럽트 끄기 / 이전 상태로 되돌리기 (짧은 구간 보호용)
; ============================================================
HOOK_TIMI   equ 0FD9Fh          ; 수직귀선 인터럽트 훅 (5바이트)
ISR_STACK_SZ equ 160

isr_old:    defb 0C9h,0C9h,0C9h,0C9h,0C9h   ; 원래 훅 내용 (다른 프로그램이 걸어 둔 것도 이어서 부른다)
isr_on:     defb 0              ; 1 = 훅이 걸려 있다
isr_lock:   defb 0              ; 0 이 아니면 엔진을 건너뛴다
isr_mode:   defb 0              ; 1 = 지금 인터럽트 안에서 엔진이 돌고 있다 (디스크를 읽을 수 없다)
isr_frames: defw 0              ; 인터럽트 횟수
isr_sp:     defw 0
isr_skip:   defw 0              ; 잠금 때문에 건너뛴 횟수 (진단)
isr_ran:    defw 0              ; 엔진을 돌린 횟수 (진단)
            defs ISR_STACK_SZ,0
isr_stack_top:

; ---- 훅 걸기 / 풀기 ----
isr_install:
        ld      a,(isr_on)
        or      a
        ret     nz
        call    ints_save_di
        push    af
        ld      hl,HOOK_TIMI
        ld      de,isr_old
        ld      bc,5
        ldir
        ld      a,0C3h
        ld      (HOOK_TIMI),a
        ld      hl,isr_entry
        ld      (HOOK_TIMI+1),hl
        ld      a,1
        ld      (isr_on),a
        call    tick_start      ; 1ms 타이머가 있으면 그것도 건다 (없으면 프레임 방식)
        pop     af
        jp      ints_restore

isr_remove:
        ld      a,(isr_on)
        or      a
        ret     z
        call    ints_save_di
        push    af
        call    tick_stop       ; 타이머 인터럽트를 먼저 끈다
        ld      hl,isr_old
        ld      de,HOOK_TIMI
        ld      bc,5
        ldir
        xor     a
        ld      (isr_on),a
        pop     af
        jp      ints_restore

; DOS 가 프로그램을 강제로 끝내기 직전에 부른다: 훅과 타이머를 풀고 돌아가면 DOS 가 마저 끝낸다
abort_cleanup:
        call    isr_remove
        ld      a,(set_port)
        cp      PORT_NONE
        ret     z
        jp      midi_flush      ; 큐에 남은 바이트도 내보낸다

; ---- 잠금 (겹쳐도 된다) ----
engine_lock:
        push    hl
        ld      hl,isr_lock
        inc     (hl)
        pop     hl
        ret
engine_unlock:
        push    hl
        ld      hl,0            ; 잠긴 동안 트랙이 바뀌었을 수 있다: 1ms 틱 방식이 처음부터 다시 계산하게 한다
        ld      (min_wait),hl
        ld      (pend_ticks),hl
        ld      hl,isr_lock
        dec     (hl)
        pop     hl
        ret

; ---- 짧은 구간 보호: 인터럽트를 끄고, 되돌릴 때 켜야 하는지를 A 에 돌려준다 ----
;  사용:  call ints_save_di / push af / ... / pop af / call ints_restore
;  메인 쪽은 늘 인터럽트가 켜진 채로 도니 A = 1 (되돌리면 켠다).  인터럽트 엔진 안(isr_mode = 1)은
;  이미 꺼져 있으니 A = 0 (되돌려도 그대로).
;  (LD A,I 로 IFF2 를 읽는 방법은 NMOS Z80 에서 그 순간 인터럽트가 걸리면 값이 틀려 영영 꺼진 채로 남을 수 있어 쓰지 않는다)
ints_save_di:
        di
        ld      a,(isr_mode)
        xor     1
        ret
ints_restore:
        or      a
        ret     z
        ei
        ret

; ---- 레지스터 저장 / 복구 + 전용 스택 (프레임 인터럽트와 1ms 인터럽트가 함께 쓴다) ----
ISR_SAVE macro
        push    af
        push    bc
        push    de
        push    hl
        push    ix
        push    iy
        ex      af,af'
        push    af
        exx
        push    bc
        push    de
        push    hl
        ld      (isr_sp),sp
        ld      sp,isr_stack_top
        endm
ISR_RESTORE macro
        ld      sp,(isr_sp)
        pop     hl
        pop     de
        pop     bc
        exx
        pop     af
        ex      af,af'
        pop     iy
        pop     ix
        pop     hl
        pop     de
        pop     bc
        pop     af
        endm

; 엔진을 돌려도 되는지 본다.  Z = 돌려도 된다,  NZ = 쉰다 (잠김 / 재생 중 아님 / 곡 끝 / 출력 큐가 밀림)
isr_gate:
        ld      a,(isr_lock)
        or      a
        jr      z,ig_run
        ld      hl,(isr_skip)
        inc     hl
        ld      (isr_skip),hl
        ret                     ; NZ
ig_run: ld      a,(play_state)
        cp      PLAY_RUN
        ret     nz
        ld      a,(song_ended)
        or      a
        ret     nz              ; 곡이 끝났다: 메인 루프가 처리할 때까지 기다린다
        ld      a,(mq_head)     ; 출력 큐가 많이 밀렸으면 쉰다 (선로 속도 한계: 곡 시간도 같이 멈춘다)
        ld      hl,mq_tail
        sub     (hl)
        cp      MQ_HIGH
        jr      c,ig_ok
        ld      hl,(mq_skip)
        inc     hl
        ld      (mq_skip),hl
        or      1               ; NZ
        ret
ig_ok:  xor     a               ; Z
        ret

; ---- 프레임 인터럽트 본체 (BIOS 가 H.TIMI 를 부른다: 이 때 인터럽트는 꺼져 있다) ----
;  1ms 틱 방식이면 프레임 세기만 하고, 아니면 엔진 전체(시간 세기 + 트랙 진행)를 돌린다
isr_entry:
        ISR_SAVE
        ld      hl,(isr_frames)
        inc     hl
        ld      (isr_frames),hl
        ld      a,(tick_on)
        or      a
        call    nz,tick_frame_watch     ; 1ms 틱 방식: 메인이 멈추지 않았는지 보고, 놓친 인터럽트가 있는지 센다
        call    isr_gate
        jr      nz,isr_done
        ld      a,1
        ld      (isr_mode),a
        ld      a,(tick_on)
        or      a
        jr      nz,isr_ticked
        call    engine_frame
        jr      isr_ran_ok
isr_ticked:
        call    ef_frames
        call    tick_apply_credit       ; 놓친 만큼 시간을 채운다
isr_ran_ok:
        xor     a
        ld      (isr_mode),a
        ld      hl,(isr_ran)
        inc     hl
        ld      (isr_ran),hl
isr_done:
        call    midi_pump       ; 쌓인 MIDI 바이트를 포트로 내보낸다
        ISR_RESTORE
        jp      isr_old         ; 원래 훅(대개 RET 뿐)으로 이어 간다
