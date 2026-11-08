; ============================================================
; 만든이: KimTaeWoo
; 도와준이: Lee.J.H
;  engine.inc - MIDI 재생 엔진 (화면과 분리: 상태 변수만 바꾸고 화면은 그것을 읽는다)
;  한 프레임(수직귀선 1회)마다 engine_frame 을 부른다.  틱 단위 시간은 고정소수점(8.8) "프레임당 틱"으로 센다.
;    프레임당 틱 = PPQ * 1,000,000 / (템포 * 프레임수) * (템포% / 100)
;  play_state : 0 정지, 1 연주, 2 일시정지
; ============================================================
PLAY_STOP  equ  0
PLAY_RUN   equ  1
PLAY_PAUSE equ  2

play_state: defb 0
fps:        defb 60             ; 화면 갱신 주파수 (NTSC 60 / PAL 50)
tempo_us:   defb 20h,0A1h,07h   ; 현재 템포 (마이크로초 / 4분음표) = 500000, 낮은 바이트부터 3바이트
tempo_pct:  defb 100            ; 템포 % (25..200)
transpose:  defb 0              ; 키 이동 (반음, 부호 있음)
tpf_f:      defb 0              ; 프레임당 틱 (소수 위 8비트)
tpf_i:      defw 16             ; 프레임당 틱 (정수)
tpf_g:      defb 0              ; 프레임당 틱 (소수 아래 8비트: 합쳐서 16비트 소수)
acc_f:      defb 0
acc_g:      defb 0
adv:        defw 0
rem_ticks:  defw 0
now_tick:   defw 0,0            ; 곡 시작부터의 틱 (32비트)
play_frames:defw 0,0            ; 곡 시작부터의 프레임 (템포% 와 상관없는 실제 프레임)
song_frames:defw 0,0            ; 곡 시간 프레임 (템포% 반영, 화면의 시간)
sf_acc:     defb 0
silent:     defb 0              ; 1 이면 건반/소리 사건을 보내지 않는다 (찾아가기 중)
song_ended: defb 0
total_ticks:defw 0,0
total_frames:defw 0,0
total_bars: defw 0
ticks_per_bar: defw 1920
ticks_per_beat:defw 480
ts_num:     defb 4
ts_den_pow: defb 2
key_sf:     defb 0
key_mi:     defb 0
key_known:  defb 0
tsig_known: defb 0
has_lyrics: defb 0
sys_type:   defb 0              ; 0 알 수 없음, 1 GM, 2 GS, 3 XG
cur_ch:     defb 0
title_buf:  defs 42
ab_a:       defw 0,0            ; A-B 반복 위치 (song_frames)
ab_b:       defw 0,0
ab_a_set:   defb 0
ab_b_set:   defb 0

; ---- 템포 -> 프레임당 틱 ----
; 입력 A = 템포% , tmp_tempo(3바이트).  출력 v_n = 프레임당 틱 * 256 (32비트)
tmp_tempo:  defs 3
tpf_shift:  defb 0
calc_tpf88:
        push    af
        call    ppq_for_tpf
        push    hl
        call    load_frame_k    ; v_k = 프레임 시간 (실제 주파수)
        pop     hl
        call    mul32_by_hl     ; v_n = PPQ * K
        ld      hl,tmp_tempo
        ld      de,v_d
        ld      bc,3
        ldir
        xor     a
        ld      (v_d+3),a
        call    div32           ; v_n = PPQ * K / 템포
        ld      a,(tpf_shift)
        or      a
        jr      z,ct_pct
        ld      hl,v_n
        sla     (hl)
        inc     hl
        rl      (hl)
        inc     hl
        rl      (hl)
        inc     hl
        rl      (hl)
ct_pct: pop     af              ; 템포%
        cp      100
        ret     z
        call    mul32_8
        ld      hl,100
        ld      (v_d),hl
        ld      hl,0
        ld      (v_d+2),hl
        jp      div32
; v_k = 256e6 / 실제 화면 주파수 (= 프레임 시간 us * 256).  NTSC 59.9227Hz (21.477MHz/1368/262), PAL 50.1590Hz (313줄).
; 정확히 60/50 으로 잡으면 곡이 0.13% 느려진다.  프레임 방식과 1ms 틱 방식의 시간 기준이 모두 이 값을 쓴다.
load_frame_k:
        ld      hl,frame_k_ntsc
        ld      a,(fps)
        cp      60
        jr      z,lfk_go
        ld      hl,frame_k_pal
lfk_go: ld      de,v_k
        ld      bc,4
        ldir
        ret
frame_k_ntsc:   defw 12327, 65          ; 4272167 (pasmo 는 16비트 계산이라 낮은/높은 값을 직접 적는다)
frame_k_pal:    defw 57500, 77          ; 5103772

; HL = PPQ.  800 이상이면 반으로 줄여 곱한 뒤 결과를 두 배로 한다 (32비트 넘침 방지: tpf_shift = 1)
ppq_for_tpf:
        xor     a
        ld      (tpf_shift),a
        ld      hl,(smf_ppq)
        ld      de,800
        push    hl
        or      a
        sbc     hl,de
        pop     hl
        ret     c
        srl     h
        rr      l
        ld      a,1
        ld      (tpf_shift),a
        ret

; 프레임당 틱을 16비트 소수까지 정확히 구한다 (tmp_tempo, tempo_pct -> tpf_i / tpf_f / tpf_g)
;   8비트 소수로 자르면 한 프레임마다 최대 1/256 틱이 사라져 곡이 0.1% 가까이 느려지므로, 나머지로 8비트를 더 구한다
;   프레임당 틱 = PPQ * 프레임시간 / 템포2,  템포2 = 템포 * 100 / 템포%
tpf_t2:     defs 4
tpf_cnt:    defb 0
tpf_q:      defs 5              ; 프레임당 틱 * 65536 (낮은 바이트가 먼저, 5바이트)
calc_tpf_precise:
        ld      hl,tmp_tempo
        ld      de,v_k
        ld      bc,3
        ldir
        xor     a
        ld      (v_k+3),a
        ld      hl,100
        call    mul32_by_hl     ; v_n = 템포 * 100
        ld      a,(tempo_pct)
        ld      l,a
        ld      h,0
        ld      (v_d),hl
        ld      hl,0
        ld      (v_d+2),hl
        call    div32           ; v_n = 템포2
        ld      hl,v_n
        ld      de,tpf_t2
        ld      bc,4
        ldir
        call    ppq_for_tpf
        push    hl
        call    load_frame_k
        pop     hl
        call    mul32_by_hl     ; v_n = PPQ * 프레임시간 * 256
        ld      hl,tpf_t2
        ld      de,v_d
        ld      bc,4
        ldir
        call    div32           ; v_n = 몫 (틱 * 256),  v_r = 나머지
        ld      hl,v_n
        ld      de,tpf_q+1
        ld      bc,4
        ldir
        xor     a               ; 나머지로 몫을 8비트 더 이어서 구한다 (긴 나눗셈을 8번만 더 돈다: 새로 32번 나누는 것보다 훨씬 빠르다)
        ld      (tpf_q),a
        ld      a,8
        ld      (tpf_cnt),a
tp_bit: ld      hl,v_r          ; 나머지 *= 2
        or      a
        ld      b,4
tp_s:   rl      (hl)
        inc     hl
        djnz    tp_s
        ld      hl,tpf_q
        sla     (hl)
        ld      hl,v_r          ; 나머지 >= 템포2 이면 빼고 몫의 맨 아래 비트를 켠다
        ld      de,tpf_t2
        call    cmp32
        jr      c,tp_nx
        ld      hl,v_r
        ld      de,tpf_t2
        call    sub32
        ld      hl,tpf_q
        set     0,(hl)
tp_nx:  ld      hl,tpf_cnt
        dec     (hl)
        jr      nz,tp_bit
        ld      a,(tpf_shift)
        or      a
        jr      z,tp_sat
        ld      hl,tpf_q        ; PPQ 를 반으로 했으면 두 배
        ld      b,5
tp_sh:  rl      (hl)
        inc     hl
        djnz    tp_sh
tp_sat: ld      a,(tpf_q+4)
        or      a
        jr      nz,tp_max
        ld      a,(tpf_q)
        ld      (tpf_g),a
        ld      a,(tpf_q+1)
        ld      (tpf_f),a
        ld      hl,(tpf_q+2)
        ld      (tpf_i),hl
        ret
tp_max: ld      a,0FFh
        ld      (tpf_g),a
        ld      (tpf_f),a
        ld      hl,0FFFFh
        ld      (tpf_i),hl
        ret

; 현재 템포와 템포% 로 프레임당 틱을 다시 계산
tempo_apply:
        ld      hl,tempo_us
        ld      de,tmp_tempo
        ld      bc,3
        ldir
        ld      a,(tick_on)     ; 1ms 틱 방식은 프레임당 틱(tpf)을 쓰지 않는다: 인터럽트 안에서 시간을 아끼려고 건너뛴다
        or      a
        jr      nz,ta_tick      ;   (곡을 열 때 인터럽트가 걸리기 전에 한 번 구해 두므로 프레임 방식으로 바뀌어도 값이 있다)
        call    calc_tpf_precise
ta_tick:jp      tick_calc_add   ; 1ms 틱 방식에 쓰는 값도 같이 다시 계산

; 메인에서 부를 때: 계산 도중 인터럽트의 템포 사건이 같은 작업 칸을 건드리지 않게 잠깐 끈다
tempo_apply_main:
        call    ints_save_di
        push    af
        call    tempo_apply
        pop     af
        jp      ints_restore

; 인터럽트(엔진)에서 부를 때: 메인이 쓰던 계산 작업 칸(v_n..v_cnt, m_mul)을 보존한다
tas_save:   defs 21
tas_mul:    defw 0
tempo_apply_isr:
        ld      hl,v_n
        ld      de,tas_save
        ld      bc,21
        ldir
        ld      hl,(m_mul)
        ld      (tas_mul),hl
        call    tempo_apply
        ld      hl,tas_save
        ld      de,v_n
        ld      bc,21
        ldir
        ld      hl,(tas_mul)
        ld      (m_mul),hl
        ret

; ---- 곡 정보 훑기 ----
ps_tidx:    defb 0
ps_tmap_trk:defb 0FFh
tmap_n:     defb 0

; 훑기 진행 알림: progress_hook 에 루틴 주소가 있으면 트랙 하나를 끝낼 때마다 부른다 (엔진은 화면을 모른다)
progress_hook: defw 0
ps_done_n:     defb 0           ; 훑기를 끝낸 트랙 수

smf_prescan:
        xor     a
        ld      (ps_done_n),a
        ld      (has_lyrics),a
        ld      (sys_type),a
        ld      (key_known),a
        ld      (tsig_known),a
        ld      (tmap_n),a
        ld      (title_buf),a
        ld      a,0FFh
        ld      (ps_tmap_trk),a
        ld      a,4
        ld      (ts_num),a
        ld      a,2
        ld      (ts_den_pow),a
        ld      hl,0
        ld      (total_ticks),hl
        ld      (total_ticks+2),hl
        xor     a
        ld      (ps_tidx),a
        ld      ix,TRKS
ps_trk: ld      a,(ix+TRK_USE)
        or      a
        jr      z,ps_next
        call    trk_start
        xor     a
        ld      (ix+TRK_ABS),a
        ld      (ix+TRK_ABS+1),a
        ld      (ix+TRK_ABS+2),a
        ld      (ix+TRK_ABS+3),a
        inc     a
        ld      (ix+TRK_ST),a
ps_ev:  call    trk_read_vlq    ; 시간 간격을 누적 틱에 더한다 (자료가 없으면 Carry)
        jr      c,ps_fin
        ld      a,(ix+TRK_ABS)
        add     a,c
        ld      (ix+TRK_ABS),a
        ld      a,(ix+TRK_ABS+1)
        adc     a,b
        ld      (ix+TRK_ABS+1),a
        ld      a,(ix+TRK_ABS+2)
        adc     a,e
        ld      (ix+TRK_ABS+2),a
        ld      a,(ix+TRK_ABS+3)
        adc     a,d
        ld      (ix+TRK_ABS+3),a
        call    trk_read_event
        ld      a,(ev_kind)
        cp      EV_END
        jr      z,ps_fin
        cp      EV_META
        jr      z,ps_meta
        cp      EV_SYSX
        jr      z,ps_sys
        call    ps_chan
        jr      ps_ev
ps_meta:call    ps_meta_event
        jr      ps_ev
ps_sys: call    ps_sysex_event
        jr      ps_ev
ps_fin: call    ps_track_end
        call    ps_notify
ps_next:ld      hl,ps_tidx
        inc     (hl)
        ld      bc,TRK_SZ
        add     ix,bc
        ld      a,(hl)
        cp      TRK_MAX
        jp      c,ps_trk
        call    ps_summary
        jp      trk_rewind

; 트랙 하나 훑기를 끝냈다: 끝낸 수를 올리고 알림 루틴이 있으면 부른다.  IX 는 보존
ps_notify:
        ld      hl,ps_done_n
        inc     (hl)
        ld      hl,(progress_hook)
        ld      a,h
        or      l
        ret     z
        push    ix
        ld      de,psn_back
        push    de
        jp      (hl)
psn_back:
        pop     ix
        ret

; total_ticks = max(total_ticks, 이 트랙의 누적 틱)
ps_track_end:
        push    ix
        pop     hl
        ld      de,TRK_ABS
        add     hl,de
        ld      de,total_ticks
        call    cmp32           ; 이 트랙 < total ?
        ret     c
        ld      de,total_ticks
        jp      copy32

; 채널 사건: 쓰는 채널 표시, 처음 프로그램 기억
ps_chan:ld      a,(ev_st)
        and     0F0h
        ld      b,a
        ld      a,(ev_st)
        and     0Fh
        ld      e,a
        ld      d,0
        ld      hl,ch_flags
        add     hl,de
        ld      a,b
        cp      90h
        jr      nz,pc_prg
        ld      a,(ev_d2)
        or      a
        ret     z
        set     0,(hl)          ; 쓰는 채널
        ret
pc_prg: cp      0C0h
        ret     nz
        bit     1,(hl)
        ret     nz
        set     1,(hl)          ; 프로그램 확인됨
        ld      hl,ch_prg
        add     hl,de
        ld      a,(ev_d1)
        ld      (hl),a
        ret

ps_meta_event:
        ld      a,(ev_mtype)
        cp      51h
        jr      z,pm_tempo
        cp      58h
        jr      z,pm_tsig
        cp      59h
        jr      z,pm_key
        cp      03h
        jr      z,pm_title
        cp      05h
        jr      nz,pm_x
        ld      a,1
        ld      (has_lyrics),a
pm_x:   ret
pm_tsig:ld      a,(tsig_known)
        or      a
        ret     nz
        ld      hl,(ev_mptr)
        ld      a,(hl)
        ld      (ts_num),a
        inc     hl
        ld      a,(hl)
        ld      (ts_den_pow),a
        ld      a,1
        ld      (tsig_known),a
        ret
pm_key: ld      a,(key_known)
        or      a
        ret     nz
        ld      hl,(ev_mptr)
        ld      a,(hl)
        ld      (key_sf),a
        inc     hl
        ld      a,(hl)
        ld      (key_mi),a
        ld      a,1
        ld      (key_known),a
        ret
pm_title:
        ld      a,(title_buf)
        or      a
        ret     nz
        ld      hl,(ev_mptr)
        ld      bc,(ev_mlen)
        ld      a,b
        or      a
        jr      nz,pt_cap
        ld      a,c
        cp      40
        jr      c,pt_len
pt_cap: ld      bc,40
pt_len: ld      a,b
        or      c
        ret     z
        ld      de,title_buf
        ldir
        xor     a
        ld      (de),a
        ret
; A = 박자표 칸 번호 -> HL = 그 칸의 주소 (칸 하나 = 7바이트: 틱 4 + 템포 3).  DE 는 망가진다
tmap_slot_addr:
        ld      l,a
        ld      h,0
        ld      e,l
        ld      d,h
        add     hl,hl
        add     hl,hl
        add     hl,hl           ; * 8
        or      a
        sbc     hl,de           ; * 7
        ld      de,TMAP
        add     hl,de
        ret

pm_tempo:
        ld      a,(ps_tmap_trk) ; 박자표(템포 지도)는 템포가 처음 나온 트랙에서만 모은다
        cp      0FFh
        jr      nz,pt_have
        ld      a,(ps_tidx)
        ld      (ps_tmap_trk),a
pt_have:ld      b,a
        ld      a,(ps_tidx)
        cp      b
        ret     nz
        ld      a,(tmap_n)
        cp      TMAP_MAX
        ret     nc
        call    tmap_slot_addr  ; HL = 박자표 칸 n
        ld      a,(ix+TRK_ABS)
        ld      (hl),a
        inc     hl
        ld      a,(ix+TRK_ABS+1)
        ld      (hl),a
        inc     hl
        ld      a,(ix+TRK_ABS+2)
        ld      (hl),a
        inc     hl
        ld      a,(ix+TRK_ABS+3)
        ld      (hl),a
        inc     hl
        push    hl
        ld      hl,(ev_mptr)    ; 템포는 빅엔디언 3바이트 -> 낮은 바이트부터 저장
        ld      a,(hl)
        inc     hl
        ld      b,a
        ld      a,(hl)
        inc     hl
        ld      c,a
        ld      a,(hl)
        pop     hl
        ld      (hl),a
        inc     hl
        ld      (hl),c
        inc     hl
        ld      (hl),b
        ld      hl,tmap_n
        inc     (hl)
        ret

ps_sysex_event:
        ld      a,(sys_type)
        or      a
        ret     nz
        ld      a,(ev_st)
        cp      0F0h
        ret     nz
        ld      hl,(ev_mptr)
        ld      a,(hl)
        inc     hl
        ld      b,a
        inc     hl              ; 두 번째 바이트는 건너뛴다
        ld      c,(hl)
        inc     hl
        ld      d,(hl)
        ld      a,b
        cp      7Eh             ; 유니버설: 7E dd 09 01/03 = GM
        jr      nz,px_gs
        ld      a,c
        cp      09h
        ret     nz
        ld      a,1
        ld      (sys_type),a
        ret
px_gs:  cp      41h             ; 롤랜드: 41 dd 42 12 = GS
        jr      nz,px_xg
        ld      a,c
        cp      42h
        ret     nz
        ld      a,2
        ld      (sys_type),a
        ret
px_xg:  cp      43h             ; 야마하: 43 1n 4C = XG
        ret     nz
        ld      a,c
        cp      4Ch
        ret     nz
        ld      a,3
        ld      (sys_type),a
        ret

; 전체 길이(프레임), 박자 길이, 마디 수
ps_prev_tick: defs 4
ps_acc:       defs 4
seg_end:      defs 4
seg_tpf:      defs 4
ps_i:         defb 0
ps_summary:
        ld      hl,ps_prev_tick
        ld      de,ps_prev_tick+1
        ld      bc,7            ; ps_prev_tick, ps_acc 8바이트를 0 으로
        ld      (hl),0
        ldir
        ld      hl,TEMPO_DEFAULT_LO     ; 기본 템포 500000
        ld      (tmp_tempo),hl
        ld      a,TEMPO_DEFAULT_HI
        ld      (tmp_tempo+2),a
        xor     a
        ld      (ps_i),a
pu_l:   ld      a,(ps_i)
        ld      hl,tmap_n
        cp      (hl)
        jr      nc,pu_last
        call    tmap_slot_addr  ; HL = 박자표 칸 i
        push    hl
        ld      de,seg_end      ; 이 지점까지의 구간을 지금 템포로 더한다
        call    copy32
        call    ps_add_segment
        pop     hl
        ld      de,4
        add     hl,de
        ld      de,tmp_tempo    ; 새 템포
        ld      bc,3
        ldir
        ld      hl,ps_i
        inc     (hl)
        jr      pu_l
pu_last:ld      hl,total_ticks
        ld      de,seg_end
        call    copy32
        call    ps_add_segment
        ld      hl,ps_acc
        ld      de,total_frames
        call    copy32
        jp      calc_bar_sizes

; 구간 (ps_prev_tick .. seg_end) 의 길이를 프레임으로 바꿔 ps_acc 에 더하고 ps_prev_tick = seg_end
seg_len:      defs 4
ps_add_segment:
        ld      a,100
        call    calc_tpf88      ; v_n = 프레임당 틱 * 256 (tmp_tempo 기준)
        ld      hl,v_n
        ld      de,seg_tpf
        call    copy32
        ld      hl,seg_end      ; 구간 틱 = seg_end - 이전 끝
        ld      de,seg_len
        call    copy32
        ld      hl,seg_len
        ld      de,ps_prev_tick
        call    sub32
        ld      hl,seg_end
        ld      de,ps_prev_tick
        call    copy32
        xor     a               ; v_n = 구간 틱 * 256
        ld      (v_n),a
        ld      hl,seg_len
        ld      de,v_n+1
        ld      bc,3
        ldir
        ld      hl,seg_tpf      ; 제수 = 프레임당 틱 * 256 (0 이면 1)
        ld      de,v_d
        call    copy32
        ld      a,(v_d)
        ld      hl,v_d+1
        or      (hl)
        inc     hl
        or      (hl)
        inc     hl
        or      (hl)
        jr      nz,pa_ok
        ld      a,1
        ld      (v_d),a
pa_ok:  call    div32           ; 몫 = 프레임 수
        ld      hl,ps_acc
        ld      de,v_n
        jp      add32

; 한 마디 / 한 박의 틱 수, 전체 마디 수
calc_bar_sizes:
        ld      hl,(smf_ppq)    ; 박(4분음표) = PPQ,  박자표 분모가 다르면 PPQ * 4 / 분모
        add     hl,hl
        add     hl,hl           ; PPQ * 4 (온음표)
        ld      a,(ts_den_pow)
        ld      b,a
        inc     b
        dec     b
        jr      z,cb_beat
cb_s:   srl     h
        rr      l
        djnz    cb_s
cb_beat:ld      a,h
        or      l
        jr      nz,cb_b1
        inc     l
cb_b1:  ld      (ticks_per_beat),hl
        ld      a,(ts_num)
        or      a
        jr      nz,cb_n
        ld      a,4
cb_n:   ld      c,a             ; 마디 = 박 * 분자
        ld      hl,0
        ld      de,(ticks_per_beat)
cb_m:   add     hl,de
        dec     c
        jr      nz,cb_m
        ld      (ticks_per_bar),hl
        ex      de,hl           ; DE = 마디 틱
        ld      hl,(total_ticks)        ; 전체 마디 = (전체 틱 + 마디 - 1) / 마디
        ld      (v_n),hl
        ld      hl,(total_ticks+2)
        ld      (v_n+2),hl
        ld      hl,(v_n)
        add     hl,de
        dec     hl
        ld      (v_n),hl
        jr      nc,cb_nc
        ld      hl,(v_n+2)
        inc     hl
        ld      (v_n+2),hl
cb_nc:  ld      (v_d),de
        ld      hl,0
        ld      (v_d+2),hl
        call    div32
        ld      hl,(v_n)
        ld      (total_bars),hl
        ret

; ============================================================
;  연주
; ============================================================
; 곡 처음부터 연주 시작
play_start:
        call    engine_lock             ; 인터럽트 엔진이 되감는 도중에 끼어들지 않게
        call    play_start_locked
        jp      engine_unlock
; 곡 처음으로 되돌린다: 트랙, 시계(틱 / 프레임), 템포(500000), 채널 상태.  연주 시작과 되감기 이동이 함께 쓴다
song_rewind_all:
        call    trk_rewind
        ld      hl,0
        ld      (now_tick),hl
        ld      (now_tick+2),hl
        ld      (play_frames),hl
        ld      (play_frames+2),hl
        ld      (song_frames),hl
        ld      (song_frames+2),hl
        xor     a
        ld      (sf_acc),a
        ld      (acc_f),a
        ld      (acc_g),a
        ld      (song_ended),a
        ld      hl,TEMPO_DEFAULT_LO
        ld      (tempo_us),hl
        ld      a,TEMPO_DEFAULT_HI
        ld      (tempo_us+2),a
        call    tempo_apply
        jp      channels_reset

play_start_locked:
        xor     a
        ld      (start_pending),a       ; 직접 시작하면 기다리던 시작은 필요 없다
        call    song_rewind_all
        call    midi_reset_module
        ld      a,PLAY_RUN
        ld      (play_state),a
        ret

play_stop:
        ld      a,PLAY_STOP
        ld      (play_state),a
        call    midi_panic
        jp      keys_clear

play_toggle:
        ld      a,(play_state)
        cp      PLAY_RUN
        jr      z,pt_pause
        cp      PLAY_PAUSE
        jr      z,pt_resume
        jp      play_start
pt_pause:
        ld      a,PLAY_PAUSE
        ld      (play_state),a
        jp      midi_panic
pt_resume:
        ld      a,PLAY_RUN
        ld      (play_state),a
        ret

; ---- 한 프레임 ----
engine_frame:
        ld      a,(play_state)
        cp      PLAY_RUN
        ret     nz
        call    ef_count
        jp      ef_tracks

; 한 프레임분의 시간을 센다: adv(이번 프레임 틱), now_tick, play_frames, song_frames 를 갱신
ef_count:
        ld      a,(acc_g)       ; 이번 프레임에 진행할 틱 (정수): 16비트 소수를 더해 넘침을 본다
        ld      hl,tpf_g
        add     a,(hl)
        ld      (acc_g),a
        ld      a,(acc_f)
        ld      hl,tpf_f
        adc     a,(hl)
        ld      (acc_f),a
        ld      hl,(tpf_i)
        jr      nc,ef_a
        inc     hl
ef_a:   ld      (adv),hl
        ld      de,(now_tick)   ; now_tick += adv
        add     hl,de
        ld      (now_tick),hl
        jr      nc,ef_b
        ld      hl,(now_tick+2)
        inc     hl
        ld      (now_tick+2),hl
ef_frames:                      ; 프레임 세기만 (1ms 틱 방식에서 프레임 인터럽트가 부른다)
ef_b:   ld      hl,(play_frames)        ; 프레임 세기
        inc     hl
        ld      (play_frames),hl
        ld      a,h
        or      l
        jr      nz,ef_c
        ld      hl,(play_frames+2)
        inc     hl
        ld      (play_frames+2),hl
ef_c:   ld      a,(tempo_pct)   ; 곡 시간 = 템포% 만큼 흐른다
        ld      hl,sf_acc
        add     a,(hl)
        ld      (hl),a
        cp      100
        jr      c,ef_d
        sub     100
        ld      (hl),a
        ld      hl,(song_frames)
        inc     hl
        ld      (song_frames),hl
        ld      a,h
        or      l
        jr      nz,ef_d
        ld      hl,(song_frames+2)
        inc     hl
        ld      (song_frames+2),hl
ef_d:   ret

; 모든 트랙을 adv 틱만큼 진행한다 (건너뛰기에서는 여러 프레임 분량을 한꺼번에 준다)
ef_tracks:
        ld      ix,TRKS
        ld      b,TRK_MAX
        xor     a
        ld      (any_active),a
        ld      hl,0FFFFh       ; 가장 빠른 사건까지 남은 틱을 새로 구한다 (1ms 틱 방식이 트랙을 건드릴 때를 정하는 데 쓴다)
        ld      (min_wait),hl
        xor     a
        ld      (starve_hold),a
ef_t:   push    bc
        call    track_step
        ld      a,(ix+TRK_ST)
        cp      1
        jr      nz,ef_n
        ld      a,1
        ld      (any_active),a
        call    ef_wait_note
ef_n:   ld      bc,TRK_SZ
        add     ix,bc
        pop     bc
        djnz    ef_t
        ld      a,(any_active)
        or      a
        ret     nz
        ld      a,1             ; 모든 트랙이 끝남
        ld      (song_ended),a
        ret
any_active: defb 0
min_wait:   defw 0FFFFh         ; 모든 트랙 중 다음 사건까지 가장 적게 남은 틱 (0 = 지금 곧)

; 트랙 하나의 남은 대기 틱을 min_wait 에 반영 (자료를 다시 읽어야 하는 트랙은 starve_hold 로 표시)
ef_wait_note:
        ld      a,(ix+TRK_NEED)
        or      (ix+TRK_PEND)
        jr      nz,ew_zero
        ld      a,(ix+TRK_DHI)
        or      (ix+TRK_DHI+1)
        ret     nz              ; 65536틱 이상 남았다: 최댓값(FFFF)보다 클 일은 없다
        ld      l,(ix+TRK_DLO)
        ld      h,(ix+TRK_DLO+1)
        ld      de,(min_wait)
        or      a
        sbc     hl,de
        ret     nc              ; 이미 더 빠른 트랙이 있다
        ld      l,(ix+TRK_DLO)
        ld      h,(ix+TRK_DLO+1)
        ld      (min_wait),hl
        ret
ew_zero:ld      a,1             ; 자료가 모자라 되돌린 트랙: 디스크를 읽을 수 있는 메인이 채워 줄 때까지 기다린다
        ld      (starve_hold),a ; (1ms 마다 다시 시도하면 메인이 돌 틈이 없다 -> 프레임 인터럽트에서 한 번씩만 다시 시도)
        ret
starve_hold:defb 0              ; 1 = 모자란 트랙이 있다.  1ms 틱 방식은 다음 프레임 인터럽트까지 트랙을 건드리지 않는다

; 트랙 하나를 adv 틱만큼 진행
;  인터럽트 안에서는 디스크를 못 읽으니 창에 자료가 모자라면 하던 일을 되돌리고 (starved) 다음 프레임으로 미룬다:
;   - 사건을 읽다가 모자라면: 읽기 시작 전 자리로 되돌린다 (사건은 아직 안 보냈다)
;   - 사건을 보낸 뒤 시간 간격이 모자라면: TRK_PEND 를 세워 다음 프레임에 간격만 다시 읽는다
track_step:
        ld      a,(ix+TRK_ST)
        cp      1
        ret     nz
        ld      hl,(adv)
        ld      (rem_ticks),hl
        ld      a,(ix+TRK_PEND)
        or      a
        jr      z,ts_loop
        call    ts_read_delta   ; 못 읽었던 시간 간격을 다시 시도
        ld      a,(ix+TRK_ST)
        cp      1
        ret     nz
        ld      a,(ix+TRK_PEND)
        or      a
        ret     nz              ; 아직도 자료가 없다
ts_loop:ld      a,(ix+TRK_DHI)
        or      (ix+TRK_DHI+1)
        jp      nz,ts_sub
        ld      hl,(rem_ticks)
        ld      e,(ix+TRK_DLO)
        ld      d,(ix+TRK_DLO+1)
        or      a
        sbc     hl,de           ; 남은 시간 - 기다림
        jp      c,ts_sub        ; 아직 사건 시각이 아님
        ld      (rem_ticks),hl
        ld      l,(ix+TRK_PTR)  ; 모자라면 되돌릴 자리
        ld      h,(ix+TRK_PTR+1)
        ld      (ts_sv_ptr),hl
        ld      a,(ix+TRK_RUN)
        ld      (ts_sv_run),a
        xor     a
        ld      (starved),a
        call    trk_read_event
        ld      a,(starved)
        or      a
        jr      nz,ts_starve
        ld      a,(ev_kind)
        cp      EV_END
        ret     z
        call    event_dispatch
        call    ts_read_delta
        ld      a,(ix+TRK_ST)
        cp      1
        ret     nz
        ld      a,(ix+TRK_PEND)
        or      a
        ret     nz
        jr      ts_loop
; 사건을 읽다가 자료가 모자랐다: 읽기 전으로 되돌리고 선읽기를 재촉한다.  (시간은 되돌리지 않는다: 다음 프레임에 이어서)
ts_starve:
        ld      hl,(ts_sv_ptr)
        ld      (ix+TRK_PTR),l
        ld      (ix+TRK_PTR+1),h
        ld      a,(ts_sv_run)
        ld      (ix+TRK_RUN),a
        ld      (ix+TRK_ST),1
        ld      (ix+TRK_NEED),1
        ld      hl,(starve_count)
        inc     hl
        ld      (starve_count),hl
        ret
ts_sv_ptr:   defw 0
ts_sv_run:   defb 0
ts_dv_ptr:   defw 0
starve_count:defw 0             ; 자료가 모자라 미룬 횟수 (진단)

; 다음 시간 간격을 읽는다.  인터럽트에서 자료가 모자라면 TRK_PEND 를 세우고 돌아온다
ts_read_delta:
        ld      l,(ix+TRK_PTR)
        ld      h,(ix+TRK_PTR+1)
        ld      (ts_dv_ptr),hl
        xor     a
        ld      (starved),a
        ld      (ix+TRK_PEND),a
        call    trk_read_delta
        ld      a,(starved)
        or      a
        ret     z
        ld      hl,(ts_dv_ptr)
        ld      (ix+TRK_PTR),l
        ld      (ix+TRK_PTR+1),h
        ld      a,1
        ld      (ix+TRK_ST),a
        ld      (ix+TRK_PEND),a
        ld      (ix+TRK_NEED),a
        ld      hl,(starve_count)
        inc     hl
        ld      (starve_count),hl
        ret
ts_sub: ld      l,(ix+TRK_DLO)  ; 기다림 -= 남은 시간
        ld      h,(ix+TRK_DLO+1)
        ld      de,(rem_ticks)
        or      a
        sbc     hl,de
        ld      (ix+TRK_DLO),l
        ld      (ix+TRK_DLO+1),h
        ret     nc
        ld      l,(ix+TRK_DHI)
        ld      h,(ix+TRK_DHI+1)
        dec     hl
        ld      (ix+TRK_DHI),l
        ld      (ix+TRK_DHI+1),h
        ret

; ---- 사건 처리 ----
event_dispatch:
        ld      a,(ev_kind)
        cp      EV_CHAN
        jr      z,ed_chan
        cp      EV_META
        jp      z,ed_meta
        cp      EV_SYSX
        jp      z,ed_sysex
        ret
ed_chan:ld      a,(ev_st)
        and     0Fh
        ld      (cur_ch),a
        ld      a,(ev_st)
        and     0F0h
        cp      90h
        jp      z,ev_note_on
        cp      80h
        jp      z,ev_note_off
        cp      0B0h
        jp      z,ev_control
        cp      0C0h
        jp      z,ev_program
        cp      0E0h
        jp      z,ev_bend
        ld      a,(silent)      ; 폴리/채널 압력은 그대로 전달
        or      a
        ret     nz
        ld      a,(ev_st)
        ld      b,a
        ld      a,(ev_d1)
        ld      c,a
        ld      a,(ev_d2)
        ld      d,a
        ld      a,b
        and     0F0h
        cp      0D0h
        jp      z,midi_msg2
        jp      midi_msg3

; 연주할 채널인가 (소거 / 단독)  NZ = 연주, Z = 연주 안 함
ch_audible:
        ld      a,(cur_ch)
        ld      e,a
        ld      d,0
        ld      hl,ch_mute
        add     hl,de
        ld      a,(hl)
        or      a
        jr      nz,au_no
        ld      a,(solo_any)
        or      a
        jr      z,au_yes        ; 단독 채널이 없으면 모두 연주
        ld      hl,ch_solo
        add     hl,de
        ld      a,(hl)
        or      a
        jr      z,au_no
au_yes: ld      a,1
        or      a
        ret
au_no:  xor     a
        ret

; 건반 번호 = ev_d1 + 키 이동 (드럼 채널 제외).  Carry = 범위 밖.  A = 소리 낼 음
note_shifted:
        ld      a,(cur_ch)
        cp      9
        ld      a,(ev_d1)
        ret     z               ; 드럼은 그대로 (Carry 없음)
        ld      b,a
        ld      a,(transpose)
        add     a,b
        bit     7,a
        jr      nz,ns_bad
        or      a               ; 0..127 이면 Carry 없음
        ret
ns_bad: scf
        ret

ev_note_on:
        ld      a,(ev_d2)
        or      a
        jr      z,ev_note_off   ; 벨로시티 0 = 노트 오프
        ld      a,(silent)
        or      a
        ret     nz
        call    ch_audible
        ret     z
        call    note_shifted
        ret     c
        ld      (note_out),a
        ld      a,(ev_st)
        ld      b,a
        ld      a,(note_out)
        ld      c,a
        ld      a,(ev_d2)
        ld      d,a
        call    midi_msg3
        ld      a,(note_out)    ; 화면: 건반 + 채널 막대
        call    key_light
        ld      a,(ev_d2)
        jp      level_hit

ev_note_off:
        ld      a,(silent)
        or      a
        ret     nz
        call    note_shifted
        ret     c
        ld      (note_out),a
        ld      a,(ev_st)
        and     0Fh
        or      80h
        ld      b,a
        ld      a,(note_out)
        ld      c,a
        ld      d,0
        call    midi_msg3
        ld      a,(note_out)
        jp      key_dark
note_out: defb  0

ev_control:
        ld      a,(cur_ch)
        ld      e,a
        ld      d,0
        ld      a,(ev_d1)
        ld      hl,ch_vol
        cp      7
        jr      z,ec_set
        ld      hl,ch_pan
        cp      10
        jr      z,ec_set
        ld      hl,ch_bank
        or      a
        jr      z,ec_set
        ld      hl,ch_mod
        cp      1
        jr      z,ec_set
        ld      hl,ch_exp
        cp      11
        jr      z,ec_set
        ld      hl,ch_rev
        cp      91
        jr      z,ec_set
        ld      hl,ch_cho
        cp      93
        jr      nz,ec_send
ec_set: add     hl,de
        ld      a,(ev_d2)
        ld      (hl),a
        ld      a,1
        ld      (st_dirty),a    ; 상태 줄 다시 그리기
        ld      a,(ev_d1)
        cp      7
        jr      z,ec_row
        cp      10
        jr      nz,ec_send
ec_row: ld      hl,ch_dirty     ; 볼륨 / 팬은 표의 줄도 다시 그린다
        add     hl,de
        ld      (hl),1
ec_send:ld      a,(silent)      ; 건너뛰는 중에는 값만 기억 (끝나면 한 번에 보낸다)
        or      a
        ret     nz
        ld      a,(ev_st)
        ld      b,a
        ld      a,(ev_d1)
        ld      c,a
        ld      a,(ev_d2)
        ld      d,a
        jp      midi_msg3

ev_program:
        ld      a,(cur_ch)
        ld      e,a
        ld      d,0
        ld      hl,ch_prg
        add     hl,de
        ld      a,(ev_d1)
        ld      (hl),a
        ld      hl,ch_flags
        add     hl,de
        set     0,(hl)
        set     1,(hl)
        ld      hl,ch_dirty
        add     hl,de
        ld      (hl),1
        ld      a,(silent)
        or      a
        ret     nz
        ld      a,(ev_st)
        ld      b,a
        ld      a,(ev_d1)
        ld      c,a
        jp      midi_msg2

ev_bend:ld      a,(cur_ch)
        ld      e,a
        ld      d,0
        ld      hl,ch_bend
        add     hl,de
        ld      a,(ev_d2)
        ld      (hl),a
        ld      a,1
        ld      (st_dirty),a
        ld      a,(silent)
        or      a
        ret     nz
        ld      a,(ev_st)
        ld      b,a
        ld      a,(ev_d1)
        ld      c,a
        ld      a,(ev_d2)
        ld      d,a
        jp      midi_msg3

ed_meta:ld      a,(ev_mtype)
        cp      51h
        jr      z,em_tempo
        cp      2Fh
        jr      z,em_end
        cp      05h
        jr      z,em_lyr
        cp      01h
        jr      z,em_text
        ret
em_text:ld      a,(has_lyrics)  ; 05 가사가 없는 곡(KAR)은 01 글을 가사로 본다
        or      a
        ret     nz
em_lyr: ld      a,(silent)
        or      a
        ret     nz
        ld      hl,(ev_mptr)
        ld      bc,(ev_mlen)
        ld      a,b             ; EVBUF 에는 앞쪽 256바이트뿐: 가사 한 건은 255자까지
        or      a
        jr      z,em_lyr_go
        ld      bc,255
em_lyr_go:
        push    ix
        call    lyrics_add
        pop     ix
        ret
em_end: ld      (ix+TRK_ST),2
        ret
em_tempo:
        ld      hl,(ev_mptr)    ; 빅엔디언 3바이트 -> 낮은 바이트부터
        ld      a,(hl)
        inc     hl
        ld      (tempo_us+2),a
        ld      a,(hl)
        inc     hl
        ld      (tempo_us+1),a
        ld      a,(hl)
        ld      (tempo_us),a
        push    ix
        call    tempo_apply_isr
        pop     ix
        ld      a,1
        ld      (tempo_dirty),a
        ret
tempo_dirty: defb 0

ed_sysex:                       ; (건너뛰는 중에도 보낸다: 곡이 쓰는 GS / XG 설정을 놓치지 않도록)
        ld      bc,(ev_mlen)
        ld      a,b
        or      a
        ret     nz              ; 너무 긴 SysEx 는 보내지 않는다
        ld      a,c
        cp      200
        ret     nc
        ld      a,(ev_st)
        cp      0F0h
        jr      nz,es_d
        ld      a,0F0h
        call    midi_send
es_d:   ld      hl,(ev_mptr)
        ld      a,(ev_mlen)
        ld      b,a
        or      a
        ret     z
es_l:   ld      a,(hl)
        inc     hl
        call    midi_send
        djnz    es_l
        ret
