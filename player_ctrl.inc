; ============================================================
; 만든이: KimTaeWoo
; 도와준이: Lee.J.H
;  player_ctrl.inc - 플레이어를 조작하는 동작들 (키 처리는 keys_handle.inc 가 이것만 부른다)
;    템포 / 키 이동 / 볼륨 / 채널 선택 / 소거 / 단독 / 건너뛰기 / A-B 반복 / 곡 끝 처리
; ============================================================
SEEK_CHUNK  equ 8               ; 건너뛰기에서 한 번에 진행하는 프레임 수
seek_tgt:   defs 4              ; 건너뛸 곳 (song_frames 단위)
seek_state: defb 0
seek_sum:   defw 0

; ---- 템포 % : A = 바꿀 양 (부호 있는 값), 25 .. 200 ----
ctl_tempo:
        ld      e,a
        ld      d,0
        bit     7,e
        jr      z,ct_p
        dec     d               ; 부호 확장
ct_p:   ld      a,(tempo_pct)
        ld      l,a
        ld      h,0
        add     hl,de
        bit     7,h
        jr      nz,ct_lo        ; 0 아래
        ld      a,h
        or      a
        jr      nz,ct_hi        ; 255 위
        ld      a,l
        cp      25
        jr      c,ct_lo
        cp      201
        jr      c,ct_set
ct_hi:  ld      a,200
        jr      ct_set
ct_lo:  ld      a,25
ct_set: ld      (tempo_pct),a
        ld      a,1
        ld      (sl_dirty),a
        jp      tempo_apply_main

; ---- 키 이동 : A = 바꿀 양 (-1 / +1).  -12 .. +12 ----
ctl_transpose:
        ld      b,a
        ld      a,(transpose)
        add     a,b
        cp      13
        jr      c,tr_ok
        cp      0F4h            ; -12 이상이면 음수 범위
        ret     c
tr_ok:  ld      (transpose),a
        ld      a,1
        ld      (sl_dirty),a
        jp      ctl_panic       ; 울리던 소리는 옛 높이이므로 끊는다

; ---- 마스터 볼륨 % : A = 바꿀 양 (부호 있는 값) ----
ctl_volume:
        ld      b,a
        ld      a,(vol_pct)
        add     a,b
        bit     7,a             ; 0 아래로
        jr      z,cv_hi
        xor     a
        jr      cv_set
cv_hi:  cp      101
        jr      c,cv_set
        ld      a,100
cv_set: ld      (vol_pct),a
        ld      a,1
        ld      (sl_dirty),a
ctl_apply_volume:               ; 지금 vol_pct 를 장치에 보낸다 (0..100 -> 0..127)
        ld      a,(vol_pct)
        ld      l,a
        ld      h,0
        ld      b,127
        ld      c,100
        call    muldiv
        ld      a,l
        jp      midi_master_vol

; ---- 채널 선택 : A = +1 / -1 ----
ctl_select:
        ld      b,a
        ld      a,(ch_sel)
        ld      e,a
        add     a,b
        and     0Fh
        ld      (ch_sel),a
        ld      d,0
        ld      hl,ch_dirty     ; 옛 줄과 새 줄 모두 다시 그린다
        add     hl,de
        ld      (hl),1
        ld      a,(ch_sel)
        ld      e,a
        ld      hl,ch_dirty
        add     hl,de
        ld      (hl),1
        ld      a,1
        ld      (st_dirty),a
        ret

; ---- 소거 (M) ----
ctl_mute:
        call    sel_index
        ld      hl,ch_mute
        add     hl,de
        ld      a,(hl)
        xor     1
        ld      (hl),a
        push    de
        or      a
        jr      z,cm_d
        call    midi_chan_off_e ; 소거로 바뀌면 그 채널의 소리를 끊는다
        call    keys_clear
cm_d:   pop     de
        ld      hl,ch_dirty
        add     hl,de
        ld      (hl),1
        ret

; ---- 단독 (S) ----
ctl_solo:
        call    sel_index
        ld      hl,ch_solo
        add     hl,de
        ld      a,(hl)
        xor     1
        ld      (hl),a
        push    de
        call    solo_recount
        call    ctl_panic       ; 다른 채널 소리는 끊고 시작
        pop     de
        ld      hl,ch_dirty
        add     hl,de
        ld      (hl),1
        ret

solo_recount:
        ld      hl,ch_solo
        ld      b,16
        xor     a
sr_l:   or      (hl)
        inc     hl
        djnz    sr_l
        or      a
        jr      z,sr_s
        ld      a,1
sr_s:   ld      (solo_any),a
        ret

; DE = ch_sel
sel_index:
        ld      a,(ch_sel)
        ld      e,a
        ld      d,0
        ret
midi_chan_off_e:                ; E = 채널
        jp      midi_chan_off

; ---- 모든 소리 끄기 (P) ----
ctl_panic:
        call    midi_panic
        jp      keys_clear

; ---- 재생 / 일시정지 ----
ctl_play_toggle:
        jp      play_toggle

; ---- 정지 (처음으로) ----
ctl_stop:
        ld      a,(song_loaded)
        or      a
        ret     z
        call    play_start      ; 처음으로 돌리고
        jp      play_stop       ; 멈춘다

; ---- 반복 토글 ----
ctl_repeat:
        ld      a,(tg_rep)
        xor     1
        ld      (tg_rep),a
        jp      draw_transport

; ---- 셔플 / 자동 다음 곡 토글 ----
ctl_shuffle:
        ld      a,(tg_shuf)
        xor     1
        ld      (tg_shuf),a
        jp      draw_transport
ctl_autonext:
        ld      a,(tg_next)
        xor     1
        ld      (tg_next),a
        jp      draw_transport

; ---- A-B 반복: 첫 누름 A, 두 번째 B (반복 시작), 세 번째 해제 ----
ctl_ab:
        ld      a,(song_loaded)
        or      a
        ret     z
        ld      a,(ab_a_set)
        or      a
        jr      nz,ab_second
        ld      hl,song_frames
        ld      de,ab_a
        call    copy32
        ld      a,1
        ld      (ab_a_set),a
        ld      hl,ab_a
        call    px_of_frames
        ld      (ab_a_px),hl
        jr      ab_redraw
ab_second:
        ld      a,(ab_b_set)
        or      a
        jr      nz,ab_clear
        ld      hl,song_frames  ; B 는 A 보다 뒤여야 한다
        ld      de,ab_a
        call    cmp32           ; Carry = 지금 < A
        ret     c
        ld      hl,song_frames
        ld      de,ab_b
        call    copy32
        ld      a,1
        ld      (ab_b_set),a
        ld      (tg_ab),a
        ld      hl,ab_b
        call    px_of_frames
        ld      (ab_b_px),hl
        jr      ab_redraw
ab_clear:
        xor     a
        ld      (ab_a_set),a
        ld      (ab_b_set),a
        ld      (tg_ab),a
ab_redraw:
        ld      hl,0FFFFh
        ld      (seek_px),hl    ; 탐색 막대를 다시 그리게 한다
        jp      draw_transport

; HL = 32비트 프레임 수의 주소 -> HL = 탐색 막대 칸 (0..498)
px_of_frames:
        ld      de,v_k
        ld      bc,4
        ldir
        jp      px_from_vk

; ---- 곡 끝났을 때 (main 이 부른다) ----
song_finished:
        xor     a
        ld      (song_ended),a
        ld      a,(tg_rep)
        or      a
        jp      nz,play_start   ; 반복이면 처음부터 다시
        ld      a,(tg_shuf)
        ld      b,a
        ld      a,(tg_next)
        or      b
        jr      z,sf_halt
        ld      a,(tg_shuf)     ; 순서대로 갈 때는 목록 끝에서 돌지 않고 멈춘다
        xor     1
        ld      (sn_nowrap),a   ; (셔플은 끝이 없으니 그대로)
        xor     a
        ld      (sn_hit_end),a
        call    song_next       ; 다음 곡(또는 무작위) 자동 재생
        xor     a
        ld      (sn_nowrap),a
        ld      a,(sn_hit_end)
        or      a
        ret     z               ; 다음 곡이 열렸다
sf_halt:call    play_start      ; 더 연주할 곡이 없으면 처음 위치로 돌려 놓고 멈춘다
        jp      play_stop

; A-B 반복 구간 끝에 닿으면 A 로 돌아간다 (main 이 매 프레임 부른다)
ab_check:
        ld      a,(ab_b_set)
        or      a
        ret     z
        ld      a,(play_state)
        cp      PLAY_RUN
        ret     nz
        ld      hl,song_frames
        ld      de,ab_b
        call    cmp32           ; Carry = 아직 B 앞
        ret     c
        ld      hl,ab_a
        ld      de,seek_tgt
        call    copy32
        jp      seek_go

; ---- 상대 위치로 건너뛰기: HL = 바꿀 초 (부호 있는 값, 16비트) ----
ctl_seek_rel:
        ld      a,(song_loaded)
        or      a
        ret     z
        push    hl
        ld      a,(fps)         ; 초 -> 프레임
        ld      c,a
        ld      b,0
        pop     hl
        bit     7,h
        jr      nz,sr_back
        call    mul16_bc        ; HL = 프레임 수 (앞으로)
        ex      de,hl
        ld      hl,(song_frames)
        add     hl,de
        ld      (seek_tgt),hl
        ld      hl,(song_frames+2)
        jr      nc,sr_hi
        inc     hl
sr_hi:  ld      (seek_tgt+2),hl
        jr      sr_clamp
sr_back:xor     a               ; 절댓값
        sub     l
        ld      l,a
        ld      a,0
        sbc     a,h
        ld      h,a
        call    mul16_bc
        ex      de,hl           ; DE = 프레임 수 (뒤로)
        ld      hl,(song_frames)
        or      a
        sbc     hl,de
        ld      (seek_tgt),hl
        ld      hl,(song_frames+2)
        ld      de,0
        sbc     hl,de
        jr      nc,sr_ok
        ld      hl,0            ; 처음보다 앞
        ld      (seek_tgt),hl
sr_ok:  ld      (seek_tgt+2),hl
sr_clamp:
        ld      hl,seek_tgt     ; 끝을 넘으면 끝 직전으로
        ld      de,total_frames
        call    cmp32
        jr      c,seek_go
        ld      hl,total_frames
        ld      de,seek_tgt
        call    copy32
        ld      hl,seek_tgt
        ld      a,(hl)
        sub     1
        ld      (hl),a
        jr      nc,seek_go
        inc     hl
        dec     (hl)
; 아래로 이어짐

; seek_tgt 위치로 건너뛴다.  처음부터 소리 없이 되감아 그 위치까지 사건만 읽는다 (악기/볼륨 상태를 맞추기 위해)
seek_go:
        call    engine_lock             ; 건너뛰는 동안 인터럽트 엔진은 쉰다
        call    seek_go_locked
        jp      engine_unlock
seek_go_locked:
        ld      a,(play_state)
        ld      (seek_state),a
        call    midi_panic
        call    keys_clear
        call    seek_rewind_if_needed
        ld      a,1
        ld      (silent),a
sg_l:   ld      hl,main_beat    ; 오래 걸려도 메인이 살아 있다고 알린다 (안 그러면 워치독이 1ms 타이머를 꺼 버린다)
        inc     (hl)
        ld      hl,song_frames
        ld      de,seek_tgt
        call    cmp32           ; Carry = 아직 목표 앞
        jr      nc,sg_done
        call    seek_chunk
        ld      a,(song_ended)
        or      a
        jr      z,sg_l
sg_done:xor     a
        ld      (silent),a
        ld      (song_ended),a
        call    restore_channels
        ld      a,(seek_state)
        cp      PLAY_RUN
        jr      z,sg_run
        ld      a,PLAY_PAUSE
sg_run: ld      (play_state),a
        ld      a,1
        ld      (tempo_dirty),a
        ld      (st_dirty),a
        jp      ui_invalidate

; 목표가 지금 위치보다 앞(뒤로)이거나 곡이 이미 끝났으면 처음으로 되감는다 (템포도 처음 값부터 다시 따라간다).
; 목표가 앞쪽(앞으로)이면 되감지 않고 지금 위치에서 소리 없이 이어 간다: 걸리는 시간이 곡 위치가 아니라 건너뛸 길이에 따른다
seek_rewind_if_needed:
        ld      a,(song_ended)
        or      a
        jp      nz,song_rewind_all
        ld      hl,song_frames
        ld      de,seek_tgt
        call    cmp32           ; Carry = 목표가 아직 앞
        ret     c
        jp      song_rewind_all

; SEEK_CHUNK 프레임 분량을 한 번에 진행
seek_chunk:
        ld      hl,0
        ld      (seek_sum),hl
        ld      b,SEEK_CHUNK
sc_l:   push    bc
        call    ef_count
        ld      hl,(seek_sum)
        ld      de,(adv)
        add     hl,de
        ld      (seek_sum),hl
        pop     bc
        djnz    sc_l
        ld      hl,(seek_sum)
        ld      (adv),hl
        jp      ef_tracks

; 건너뛰기를 마친 뒤 각 채널의 악기 / 볼륨 / 팬 ... 을 장치에 다시 보낸다
restore_channels:
        xor     a
        ld      (rc_ch),a
rc_l:   ld      a,(rc_ch)
        ld      e,a
        ld      d,0
        ld      hl,ch_flags
        add     hl,de
        bit     0,(hl)
        jr      z,rc_n
        ld      a,(rc_ch)
        or      0B0h
        ld      (rc_st),a
        ld      hl,ch_bank
        ld      c,0
        call    rc_cc
        ld      hl,ch_vol
        ld      c,7
        call    rc_cc
        ld      hl,ch_pan
        ld      c,10
        call    rc_cc
        ld      hl,ch_exp
        ld      c,11
        call    rc_cc
        ld      hl,ch_mod
        ld      c,1
        call    rc_cc
        ld      hl,ch_rev
        ld      c,91
        call    rc_cc
        ld      hl,ch_cho
        ld      c,93
        call    rc_cc
        ld      a,(rc_ch)       ; 프로그램
        ld      e,a
        ld      d,0
        ld      hl,ch_prg
        add     hl,de
        ld      c,(hl)
        or      0C0h
        ld      b,a
        call    midi_msg2
        ld      a,(rc_ch)       ; 피치벤드 (위쪽 7비트만 기억하므로 아래쪽은 0)
        ld      e,a
        ld      d,0
        ld      hl,ch_bend
        add     hl,de
        ld      d,(hl)
        ld      c,0
        or      0E0h
        ld      b,a
        call    midi_msg3
rc_n:   ld      hl,rc_ch
        inc     (hl)
        ld      a,(hl)
        cp      16
        jr      c,rc_l
        ret
; HL = 채널 표, C = 컨트롤 번호 -> CC 전송
rc_cc:  ld      a,(rc_ch)
        ld      e,a
        ld      d,0
        add     hl,de
        ld      a,(hl)
        ld      d,a
        ld      a,(rc_st)
        ld      b,a
        jp      midi_msg3
rc_ch:  defb    0
rc_st:  defb    0

; HL * BC -> HL (둘 다 16비트, 작은 값용)
mul16_bc:
        ld      d,h
        ld      e,l
        ld      hl,0
m16_l:  ld      a,b
        or      c
        ret     z
        add     hl,de
        dec     bc
        jr      m16_l
