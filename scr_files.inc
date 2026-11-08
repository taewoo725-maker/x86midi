; ============================================================
; 만든이: KimTaeWoo
; 도와준이: Lee.J.H
;  scr_files.inc - F4 파일 화면: 현재 폴더의 폴더 / MIDI 파일 목록, 고른 파일의 머리 정보, 곡 고르기
;  목록 칸(16바이트): 이름 13 (끝 0) + 속성 1 + 크기 2 (바이트 / 256)
; ============================================================
_FFIRST  equ    40h
_FNEXT   equ    41h
_CHDIR   equ    5Ah
_GETCD   equ    59h
FIB_NAME equ    1
FIB_ATTR equ    14
FIB_SIZE equ    21

dir_count: defb 0
dir_more:  defb 0               ; 1 = 폴더에 항목이 DIR_MAX 개보다 많다 (넘친 것은 목록에 없다)
dir_sel:   defb 0
dir_top:   defb 0
cur_idx:   defb 0FFh            ; 지금 열린 곡의 목록 번호 (FFh = 모름)
dir_fib:   defs 64
dir_spec:  defb "*.*",0
cwd_buf:   defs 66
FILE_ROWS  equ  17

; ---- 폴더 읽기 ----
dir_scan:
        call    info_clear      ; 폴더가 바뀌면 기억해 둔 파일 정보는 모두 버린다
        xor     a
        ld      (dir_count),a
        ld      (dir_more),a
        ld      ix,dir_fib
        ld      de,dir_spec
        ld      b,10h           ; 폴더도 찾는다
        ld      c,_FFIRST
        call    BDOS
        or      a
        jr      nz,dsc_x
dsc_l:  call    dir_take        ; (목록이 가득 차도 끝까지 훑어 넘치는 항목이 있는지 안다)
        ld      ix,dir_fib
        ld      c,_FNEXT
        call    BDOS
        or      a
        jr      z,dsc_l
dsc_x:  ld      de,cwd_buf      ; 지금 폴더 이름
        ld      b,0
        ld      c,_GETCD
        call    BDOS
        jp      info_file_load  ; 이 폴더의 X86MIDI.DIR 에 저장해 둔 파일 정보를 되살린다

; dir_fib 의 항목이 폴더(. 제외) 이거나 MID/KAR 파일이면 목록에 넣는다
dir_take:
        ld      a,(dir_fib+FIB_ATTR)
        and     10h
        jr      z,dt_file
        ld      a,(dir_fib+FIB_NAME)    ; "." 은 건너뛴다
        cp      '.'
        jr      nz,dt_add
        ld      a,(dir_fib+FIB_NAME+1)
        or      a
        ret     z
        jr      dt_add
dt_file:ld      hl,dir_fib+FIB_NAME     ; 확장자 찾기
        ld      de,0
dt_f:   ld      a,(hl)
        or      a
        jr      z,dt_e
        cp      '.'
        jr      nz,dt_n
        ld      d,h
        ld      e,l
dt_n:   inc     hl
        jr      dt_f
dt_e:   ld      a,d
        or      e
        ret     z
        inc     de
        ld      a,(de)
        and     0DFh
        ld      b,a
        inc     de
        ld      a,(de)
        and     0DFh
        ld      c,a
        inc     de
        ld      a,(de)
        and     0DFh
        ld      h,a
        ld      a,b
        cp      'M'
        jr      nz,dt_k
        ld      a,c
        cp      'I'
        ret     nz
        ld      a,h
        cp      'D'
        ret     nz
        jr      dt_add
dt_k:   cp      'K'
        ret     nz
        ld      a,c
        cp      'A'
        ret     nz
        ld      a,h
        cp      'R'
        ret     nz
dt_add: ld      a,(dir_count)
        cp      DIR_MAX
        jr      c,dt_room
        ld      a,1             ; 자리가 없다: 넘쳤다는 표시만 남긴다
        ld      (dir_more),a
        ret
dt_room:ld      l,a
        ld      h,0
        add     hl,hl
        add     hl,hl
        add     hl,hl
        add     hl,hl           ; * 16
        ld      de,DIRLIST
        add     hl,de
        ex      de,hl
        ld      hl,dir_fib+FIB_NAME
        ld      bc,13
        ldir
        ld      a,(dir_fib+FIB_ATTR)
        and     10h
        ld      (de),a          ; 속성: 0 = 파일, 10h = 폴더
        inc     de
        ld      hl,(dir_fib+FIB_SIZE+1)
        ex      de,hl
        ld      (hl),e
        inc     hl
        ld      (hl),d
        call    info_stamp_take ; 바뀜 확인용 도장 (시각, 날짜, 크기)
        ld      hl,dir_count
        inc     (hl)
        ret

; A = 목록 번호 -> HL = 칸 주소
dir_rec:
        ld      l,a
        ld      h,0
        add     hl,hl
        add     hl,hl
        add     hl,hl
        add     hl,hl
        ld      de,DIRLIST
        add     hl,de
        ret

; ---- 화면 ----
scr_files:
        BOX     0,10,512,190,C_BG
        TXT     4,13,0,C_LN,C_BG,0,"FILES"
        ld      hl,cwd_buf
        TXTD    40,13,300,C_TX,C_BG,0
        TXT     8,24,0,C_LN,C_BG,0,"NAME"
        ld      a,(dir_more)
        or      a
        jr      z,sf_nomore
        TXT     170,24,0,C_TX,C_BG,0,"255 MAX - MORE NOT SHOWN"
sf_nomore:
        TXT     120,24,36,C_LN,C_BG,2,"SIZE"
        BOX     4,32,296,1,C_LN
        PANEL   306,12,202,174,C_PN,C_LN
        TXT     310,14,0,C_LN,C_PN,0,"FILE INFO"
        ld      a,(dir_count)
        or      a
        jr      nz,sf_rows
        TXT     8,40,0,C_LN,C_BG,0,"(no MIDI files or folders)"
sf_rows:call    files_rows
        call    files_mark_total
        call    files_info
        jp      files_hint

files_hint:
        BOX     0,189,512,10,C_PN
        TXT     4,190,508,C_LN,C_PN,0,"UP/DOWN move  SPACE mark  RET play  ESC back or play marked  A-Z jump  BS parent"
        ret

; 보이는 줄 전부
files_rows:
        xor     a
        ld      (fr_i),a
fr_l:   call    files_row
        ld      hl,fr_i
        inc     (hl)
        ld      a,(hl)
        cp      FILE_ROWS
        jr      c,fr_l
        ret
fr_i:   defb    0

; 화면의 fr_i 번째 줄 (y = 34 + 9 * i)
files_row:
        ld      a,(fr_i)
        ld      c,a
        add     a,a
        add     a,a
        add     a,a
        add     a,c
        add     a,34
        ld      (fr_y),a
        ld      a,(dir_top)
        ld      hl,fr_i
        add     a,(hl)
        ld      (fr_idx),a
        ld      hl,dir_count
        cp      (hl)
        jr      c,fw_has
        ld      a,(fr_y)        ; 빈 줄
        ld      l,a
        ld      h,0
        ld      (r_y),hl
        ld      hl,4
        ld      (r_x),hl
        ld      hl,296
        ld      (r_w),hl
        ld      hl,9
        ld      (r_h),hl
        ld      a,C_BG
        ld      (r_c),a
        jp      rect_go
fw_has: ld      a,(dir_sel)     ; 고른 줄은 크림색 바탕
        ld      b,a
        ld      a,(fr_idx)
        cp      b
        ld      a,C_BG
        ld      b,C_TX
        jr      nz,fw_c
        ld      a,C_TX
        ld      b,C_BG
fw_c:   ld      (fr_bg),a
        ld      a,b
        ld      (fr_fg),a
        ld      hl,4
        ld      (r_x),hl
        ld      a,(fr_y)
        ld      l,a
        ld      h,0
        ld      (r_y),hl
        ld      hl,296
        ld      (r_w),hl
        ld      hl,9
        ld      (r_h),hl
        ld      a,(fr_bg)
        ld      (r_c),a
        call    rect_go
        ld      a,(fr_idx)
        call    dir_rec
        ld      (fr_rec),hl
        ld      de,13
        add     hl,de
        ld      a,(hl)          ; 폴더는 노란색, 파일은 본문색
        or      a
        ld      a,(fr_fg)
        jr      z,fw_n
        ld      a,(fr_bg)
        cp      C_TX
        ld      a,C_YEL
        jr      nz,fw_n
        ld      a,C_BG
fw_n:   ld      b,a
        ld      a,(fr_bg)
        ld      c,a
        ld      hl,8
        ld      a,(fr_y)
        ld      de,100
        call    text_at
        xor     a
        ld      (t_al),a
        ld      hl,(fr_rec)
        call    txtp
        ld      hl,(fr_rec)     ; 크기 (폴더는 <DIR>)
        ld      de,13
        add     hl,de
        ld      a,(hl)
        or      a
        jr      z,fw_sz
        ld      hl,txt_dir
        jr      fw_sd
fw_sz:  ld      hl,(fr_rec)
        ld      de,14
        add     hl,de
        ld      e,(hl)
        inc     hl
        ld      d,(hl)          ; 크기 / 256
        ex      de,hl
        srl     h
        rr      l
        srl     h
        rr      l               ; / 4 -> KB
        ld      a,h
        or      l
        jr      nz,fw_kb
        inc     hl              ; 1KB 보다 작아도 1K 로 보인다
fw_kb:  push    hl
        call    sb_reset
        pop     hl
        call    sb_dec16
        ld      a,'K'
        call    sb_chr
        call    sb_done
fw_sd:  push    hl
        ld      a,(fr_fg)
        ld      b,a
        ld      a,(fr_bg)
        ld      c,a
        ld      hl,120
        ld      a,(fr_y)
        ld      de,36
        call    text_at
        ld      a,2
        ld      (t_al),a
        pop     hl
        call    txtp
        call    files_row_title
        jp      files_row_mark
txt_dir: defb   "<DIR>",0
fr_y:   defb    0
fr_idx: defb    0
fr_bg:  defb    0
fr_fg:  defb    0
fr_rec: defw    0

hdr_buf: defs 164           ; FI_READ + 여유

; 고른 파일의 머리 정보 (SMF 머리 14바이트를 읽는다)
files_info:
        BOX     308,24,198,160,C_PN
        ld      a,(dir_count)
        or      a
        ret     z
        ld      a,(dir_sel)
        call    dir_rec
        ld      (fi_rec),hl
        TXTD    310,26,194,C_TX,C_PN,0
        ld      hl,(fi_rec)
        ld      de,13
        add     hl,de
        ld      a,(hl)
        or      a
        jr      z,fi_file
        TXT     310,40,0,C_LN,C_PN,0,"folder"
        ret
fi_file:
        ld      a,(dir_sel)
        call    info_load       ; 이미 읽은 파일이면 디스크를 열지 않는다
        jp      c,fi_draw_lines
        IF TESTMODE
        ld      hl,(JIFFY)      ; [테스트모드] 파일 열기 ~ 닫기 시간
        ld      (tm_d0),hl
        ENDIF
        ld      hl,(fi_rec)
        ex      de,hl
        ld      a,1
        ld      c,_OPEN
        call    BDOS
        or      a
        ret     nz
        ld      a,b
        ld      (fi_h),a
        call    fi_read_head    ; 머리글 + 첫 트랙 앞부분, 파일 크기
        ld      a,(fi_h)
        ld      b,a
        ld      c,_CLOSE
        call    BDOS
        IF TESTMODE
        ld      hl,(JIFFY)
        ld      de,(tm_d0)
        or      a
        sbc     hl,de
        ld      (tm_dos),hl
        ENDIF
        ld      a,(hdr_buf)     ; 'MThd' 확인
        cp      'M'
        jr      nz,fi_bad
        call    fi_parse_track0 ; 제목, 템포
        ld      a,(dir_sel)
        call    info_store      ; 다음에는 디스크를 열지 않도록 기억
        call    c,files_row_selected    ; 목록 줄에도 제목이 보이게 그 줄을 다시 그린다
        jp      fi_draw_lines
fi_bad: TXT     310,40,0,C_RED,C_PN,0,"not a MIDI file"
        ret

; ---- 파일 앞부분 읽기 (열려 있는 fi_h) ----
FI_READ equ     160             ; 머리글 14 + 트랙 머리 8 + 첫 사건들 (제목 / 템포 메타를 찾을 만큼)
fi_read_head:
        ld      hl,0
        ld      (fi_got),hl
        ld      (fi_size),hl
        ld      (fi_size+2),hl
        ld      (hdr_buf),hl    ; 못 읽었을 때 'M' 이 아니게
        ld      a,(fi_h)
        ld      b,a
        ld      de,hdr_buf
        ld      hl,FI_READ
        ld      c,_READ
        call    BDOS
        or      a
        jr      nz,frh_size
        ld      (fi_got),hl     ; 읽은 바이트 수
frh_size:
        ld      a,(fi_h)        ; 파일 크기 = 끝으로 _SEEK 한 위치
        ld      b,a
        ld      a,2
        ld      de,0
        ld      hl,0
        ld      c,_SEEK
        call    BDOS
        or      a
        ret     nz
        ld      (fi_size),hl
        ld      (fi_size+2),de
        ret

; ---- 첫 트랙의 맨 앞 메타 사건에서 제목(FF 03)과 템포(FF 51)를 찾는다 ----
;  읽어 둔 부분 안에서만 찾고, 메타가 아닌 사건이 나오면 거기서 멈춘다
fi_parse_track0:
        xor     a
        ld      (fi_title),a
        ld      (fi_has_tempo),a
        ld      hl,(fi_got)
        ld      de,hdr_buf
        add     hl,de
        ld      (fp_end),hl
        ld      hl,hdr_buf+22   ; 머리글 14 + 'MTrk' 와 길이 8
fp_next:call    fp_skip_delta
        ret     c
        ld      a,(hl)
        inc     hl
        cp      0FFh
        ret     nz              ; 메타가 아닌 사건: 더 찾지 않는다
        call    fp_chk
        ret     c
        ld      b,(hl)          ; 메타 종류
        inc     hl
        call    fp_chk
        ret     c
        ld      c,(hl)          ; 길이 (128 이상이면 멈춘다)
        inc     hl
        bit     7,c
        ret     nz
        ld      a,b
        cp      2Fh
        ret     z
        cp      03h
        jr      z,fp_title
        cp      51h
        jr      z,fp_tempo
fp_skip:ld      b,0
        add     hl,bc
        jr      fp_next
fp_title:
        ld      a,(fi_title)    ; 처음 나온 제목만
        or      a
        jr      nz,fp_skip
        ld      a,c
        or      a
        jr      z,fp_skip
        cp      FI_TITLE_MAX
        jr      c,fp_tn
        ld      a,FI_TITLE_MAX
fp_tn:  push    hl
        push    bc
        ld      b,a
        ld      de,fi_title
fp_tc:  ld      a,(hl)
        cp      32
        jr      nc,fp_tp
        ld      a,'.'           ; 제어 글자는 점으로
fp_tp:  ld      (de),a
        inc     hl
        inc     de
        djnz    fp_tc
        xor     a
        ld      (de),a
        pop     bc
        pop     hl
        jr      fp_skip
fp_tempo:
        ld      a,c
        cp      3
        jr      nz,fp_skip
        ld      a,(fi_has_tempo)
        or      a
        jr      nz,fp_skip      ; 처음 나온 템포만
        ld      a,(hl)          ; 파일은 높은 바이트부터, tempo_us 는 낮은 바이트부터
        ld      (fi_tempo+2),a
        inc     hl
        ld      a,(hl)
        ld      (fi_tempo+1),a
        inc     hl
        ld      a,(hl)
        ld      (fi_tempo),a
        inc     hl
        ld      a,1
        ld      (fi_has_tempo),a
        jr      fp_next
; 시간 간격(가변 길이 수)을 건너뛴다.  Carry = 읽어 둔 부분이 끝났다
fp_skip_delta:
fp_sd:  call    fp_chk
        ret     c
        ld      a,(hl)
        inc     hl
        bit     7,a
        jr      nz,fp_sd
        jr      fp_chk
; Carry = HL 이 읽어 둔 부분의 끝 이상
fp_chk: push    hl
        ld      de,(fp_end)
        or      a
        sbc     hl,de
        pop     hl
        ccf
        ret
fp_end: defw    0

; ---- 정보 줄 그리기 ----
fi_draw_lines:
        call    sb_reset        ; SMF-1  3 TRK  PPQ 480
        ld      hl,txt_smf
        call    sb_str
        ld      a,(hdr_buf+9)
        call    sb_dec
        ld      hl,txt_2sp
        call    sb_str
        ld      a,(hdr_buf+11)
        call    sb_dec
        ld      hl,txt_trk
        call    sb_str
        ld      a,(hdr_buf+12)
        ld      h,a
        ld      a,(hdr_buf+13)
        ld      l,a
        call    sb_dec16
        call    sb_done
        TXTD    310,40,194,C_LN,C_PN,0
        call    sb_reset        ; 제목
        ld      hl,txt_title
        call    sb_str
        ld      hl,fi_title
        ld      a,(hl)
        or      a
        jr      nz,fdl_t
        ld      hl,txt_dash
fdl_t:  call    sb_str
        call    sb_done
        TXTD    310,54,194,C_TX,C_PN,0
        call    sb_reset        ; 템포
        ld      hl,txt_bpm
        call    sb_str
        ld      a,(fi_has_tempo)
        or      a
        jr      nz,fdl_b
        ld      hl,txt_dash
        call    sb_str
        jr      fdl_bd
fdl_b:  ld      hl,fi_tempo
        call    calc_bpm10_of
        ld      hl,(bpm_int)
        call    sb_dec16
        ld      a,'.'
        call    sb_chr
        ld      a,(bpm_frac)
        call    sb_dec
fdl_bd: call    sb_done
        TXTD    310,68,194,C_TX,C_PN,0
        call    sb_reset        ; 크기
        ld      hl,txt_size
        call    sb_str
        ld      a,(fi_size+2)
        ld      hl,fi_size+3
        or      (hl)
        jr      nz,fdl_kb
        ld      hl,(fi_size)
        call    sb_dec16
        ld      hl,txt_bytes
        call    sb_str
        jr      fdl_sd
fdl_kb: ld      hl,(fi_size+1)  ; 64KB 이상: 1024 로 나눠 KB (24비트를 2번 밀기)
        ld      a,(fi_size+3)
        srl     a
        rr      h
        rr      l
        srl     a
        rr      h
        rr      l
        call    sb_dec16
        ld      hl,txt_kb
        call    sb_str
fdl_sd: call    sb_done
        TXTD    310,82,194,C_TX,C_PN,0
        ret
txt_title: defb "TITLE ",0
txt_size:  defb "SIZE ",0
txt_bytes: defb " B",0
txt_kb:    defb " KB",0
txt_dash:  defb "---",0
FI_TITLE_MAX equ 30
fi_title:  defs FI_TITLE_MAX+1
fi_tempo:  defs 3
fi_has_tempo: defb 0
fi_got:    defw 0
fi_size:   defw 0,0
fi_rec: defw    0
fi_h:   defb    0

; ---- 키 ----
files_keys:                             ; 화살표는 누르고 있으면 반복 (key_rpt8)
        ld      a,(key_rpt8)
        ld      c,a
        bit     5,c
        jr      z,fk_dn
        ld      a,-1
        call    files_move
fk_dn:  ld      a,(key_rpt8)
        bit     6,a
        jr      z,fk_pl
        ld      a,1
        call    files_move
fk_pl:  ld      a,(key_rpt8)
        bit     4,a
        jr      z,fk_pr
        ld      a,-FILE_ROWS
        call    files_move
fk_pr:  ld      a,(key_rpt8)
        bit     7,a
        jr      z,fk_jmp
        ld      a,FILE_ROWS
        call    files_move
fk_jmp: call    files_jump_key          ; 글자 / 숫자 키 = 그 글자로 시작하는 곳으로
        ld      a,(key_new+8)           ; SPACE = 고른 줄에 선택 표시를 켜고 끈다 (재생이 아니다)
        bit     0,a
        call    nz,files_toggle_mark
        ld      a,(key_new+7)
        bit     7,a             ; RET = 폴더면 들어가고, 파일이면 ESC 와 같다
        jp      nz,files_open
        ld      a,(key_new+7)
        bit     5,a             ; BS = 위 폴더
        jp      nz,files_parent
        ld      a,(key_new+7)
        bit     2,a             ; ESC = 표시한 곡들을 재생하며 플레이어로 (표시가 없으면 그냥 나감)
        ret     z
        jp      files_leave

; ---- 글자 / 숫자 키: 이름이 그 글자로 시작하는 다음 항목으로 (끝나면 처음부터 돌아서 찾는다) ----
; 방금 눌린 키(key_new 행 0..5) 하나를 글자로 바꿔 files_jump_to 에 넘긴다
files_jump_key:
        ld      hl,key_new
        ld      b,0                     ; B = 행
fj_row: ld      a,(hl)
        or      a
        jr      nz,fj_hit
        inc     hl
        inc     b
        ld      a,b
        cp      6
        jr      c,fj_row
        ret
fj_hit: ld      c,0                     ; C = 눌린 비트 번호
fj_bit: rrca
        jr      c,fj_got
        inc     c
        jr      fj_bit
fj_got: ld      a,b
        cp      2
        jr      c,fj_digit
        jr      nz,fj_row3
        ld      a,c                     ; 행 2: 비트 6 = A, 비트 7 = B
        sub     6
        ret     c
        add     a,'A'
        jr      files_jump_to
fj_row3:sub     3                       ; 행 3.. : C D E ... Z 가 8개씩
        add     a,a
        add     a,a
        add     a,a
        add     a,c
        add     a,'C'
        jr      files_jump_to
fj_digit:                               ; 행 0 = 0..7, 행 1 = 8, 9
        ld      a,b
        or      a
        ld      a,c
        jr      z,fj_d0
        cp      2
        ret     nc
        add     a,8
fj_d0:  add     a,'0'
; A = 글자 -> 지금 고른 줄 다음부터 그 글자로 시작하는 첫 항목으로 이동
files_jump_to:
        call    files_find_initial      ; 찾기 (화면과 상관없는 계산)
        ret     nc                      ; 없으면 그대로
        ld      d,a
        ld      a,(dir_sel)
        ld      c,a
        ld      a,d
        sub     c                       ; 이동할 양 (부호 있는 값)
        ret     z
        jp      files_move              ; 화면 갱신은 files_move 가 맡는다

; A = 글자 -> 지금 고른 줄 다음부터 (끝나면 처음부터 돌아서) 이름이 그 글자로 시작하는 항목을 찾는다
; 돌려주는 값: Carry = 찾음, A = 항목 번호 (못 찾으면 Carry 없음)
files_find_initial:
        ld      e,a
        ld      a,(dir_count)
        or      a
        ret     z                       ; (Carry 없음)
        ld      b,a                     ; B = 항목 수 = 살펴볼 횟수
        ld      a,(dir_sel)
        ld      c,a                     ; C = 살펴보는 번호
ffi_l:  inc     c
        ld      a,c
        cp      b                       ; 끝을 넘으면 처음으로
        jr      c,ffi_in
        ld      c,0
ffi_in: push    bc
        push    de
        ld      a,c
        call    dir_rec                 ; (BC, DE 를 지킨다)
        pop     de
        pop     bc
        ld      a,(hl)
        cp      e
        jr      z,ffi_hit
        ld      a,(ffi_n)
        inc     a
        ld      (ffi_n),a
        cp      b
        jr      c,ffi_l
        xor     a
        ld      (ffi_n),a
        ret                             ; Carry 없음 (a=0 이므로 cp 결과가 아니라 xor 로 지워진 상태)
ffi_hit:xor     a
        ld      (ffi_n),a
        ld      a,c
        scf
        ret
ffi_n:  defb    0

; A = 바꿀 양 (부호 있는 값): 고른 줄 이동, 필요하면 스크롤
files_move:
        IF TESTMODE
        ld      hl,(JIFFY)      ; [테스트모드] 이동 한 번에 걸린 시간을 잰다
        ld      (tm_mv0),hl
        ld      hl,0
        ld      (tm_dos),hl
        call    files_move_body
        jp      tm_show
files_move_body:
        ENDIF
        ld      c,a
        ld      a,(dir_count)
        or      a
        ret     z
        ld      b,a
        ld      a,(dir_sel)
        ld      d,a             ; 옛 선택
        bit     7,c
        jr      z,fm_pos
        add     a,c
        jr      c,fm_set        ; 위로 넘치지 않음
        xor     a
        jr      fm_set
fm_pos: add     a,c
        jr      c,fm_end        ; (번호가 255 에 가까우면 더하다 넘칠 수 있다)
        cp      b
        jr      c,fm_set
fm_end: ld      a,b
        dec     a
fm_set: ld      (dir_sel),a
        cp      d
        ret     z
        ld      a,(dir_top)     ; 화면 밖이면 스크롤
        ld      e,a
        ld      a,(dir_sel)
        cp      e
        jr      c,fm_up
        ld      a,e
        add     a,FILE_ROWS
        ld      b,a
        ld      a,(dir_sel)
        cp      b
        jr      c,fm_same
        sub     FILE_ROWS-1
        ld      (dir_top),a
        jr      fm_all
fm_up:  ld      (dir_top),a
fm_all: call    files_rows
        jp      files_info
fm_same:ld      a,d             ; 같은 화면 안: 옛 줄과 새 줄만
        ld      hl,dir_top
        sub     (hl)
        ld      (fr_i),a
        call    files_row
        ld      a,(dir_sel)
        ld      hl,dir_top
        sub     (hl)
        ld      (fr_i),a
        call    files_row
        jp      files_info

; RET: 폴더면 들어가고, 파일이면 (표시한 곡이 있으면 그 곡들, 없으면 이 곡 하나를) 재생하며 플레이어로
files_open:
        ld      a,(dir_count)
        or      a
        ret     z
        ld      a,(dir_sel)
        call    dir_rec
        ld      (fo_rec),hl
        ld      de,13
        add     hl,de
        ld      a,(hl)
        ld      de,(fo_rec)
        or      a
        jr      z,fo_file
        push    de
        call    info_file_save_if_dirty ; 폴더를 옮기기 전에 이 폴더의 새 정보를 저장
        pop     de
        ld      c,_CHDIR        ; 폴더로 들어간다 (DE = 이름)
        call    BDOS
        jr      files_rescan
fo_file:ld      a,(mark_count)  ; 표시한 곡이 있으면 ESC 와 같다 (표시한 곡들 재생)
        or      a
        jp      nz,files_leave
        ld      a,(dir_sel)     ; 표시한 곡이 없으면 RET 를 친 그 곡 하나를 재생한다
        call    open_list_entry
        xor     a
        jp      screen_switch

files_parent:
        call    info_file_save_if_dirty
        ld      de,txt_dotdot
        ld      c,_CHDIR
        call    BDOS
files_rescan:
        call    marks_clear     ; 폴더가 바뀌면 목록 번호가 달라진다
        xor     a
        ld      (dir_sel),a
        ld      (dir_top),a
        ld      a,0FFh
        ld      (cur_idx),a
        call    dir_scan
        jp      scr_files
txt_dotdot: defb "..",0
fo_rec: defw    0

; ============================================================
;  선택 표시 (SPACE) 와 표시한 곡 재생 (ESC / RET)
;  표시는 DIRMARK[목록 번호] = 1.  폴더를 바꾸면 지운다.
; ============================================================
mark_count: defb 0

marks_clear:
        ld      hl,DIRMARK
        ld      de,DIRMARK+1
        ld      bc,DIR_MAX-1
        ld      (hl),0
        ldir
        xor     a
        ld      (mark_count),a
        ret

; A = 목록 번호 -> Z 가 아니면 표시됨.  (A, HL 를 바꾼다)
mark_get:
        ld      hl,DIRMARK
        ld      e,a
        ld      d,0
        add     hl,de
        ld      a,(hl)
        or      a
        ret

; 고른 줄이 파일이면 표시를 뒤집고 그 줄과 개수를 다시 그린다
files_toggle_mark:
        ld      a,(dir_count)
        or      a
        ret     z
        ld      a,(dir_sel)
        call    dir_rec
        ld      de,13
        add     hl,de
        ld      a,(hl)
        or      a
        ret     nz              ; 폴더는 표시하지 않는다
        ld      hl,DIRMARK
        ld      a,(dir_sel)
        ld      e,a
        ld      d,0
        add     hl,de
        ld      a,(hl)
        xor     1
        ld      (hl),a
        ld      hl,mark_count
        jr      z,ftm_off
        inc     (hl)
        jr      ftm_draw
ftm_off:dec     (hl)
ftm_draw:
        ld      a,(dir_sel)
        ld      hl,dir_top
        sub     (hl)
        ld      (fr_i),a
        call    files_row
        jp      files_mark_total

; 줄 앞(x 4)에 초록 표시: 선택된 곡
files_row_mark:
        ld      a,(fr_idx)
        call    mark_get
        ret     z
        ld      hl,4
        ld      (r_x),hl
        ld      a,(fr_y)
        add     a,2
        ld      l,a
        ld      h,0
        ld      (r_y),hl
        ld      hl,3
        ld      (r_w),hl
        ld      hl,5
        ld      (r_h),hl
        ld      a,C_GRN
        ld      (r_c),a
        jp      rect_go

; 정보창 머리 오른쪽에 "MARK n"
files_mark_total:
        BOX     440,13,66,9,C_PN
        ld      a,(mark_count)
        or      a
        ret     z
        call    sb_reset
        ld      hl,txt_mark
        call    sb_str
        ld      a,(mark_count)
        call    sb_dec
        call    sb_done
        TXTD    440,14,64,C_GRN,C_PN,0
        ret
txt_mark: defb  "MARK ",0

; A = 목록 번호의 파일을 열어 연주를 시작한다 (cur_idx 도 맞춘다)
open_list_entry:
        ld      (cur_idx),a
        call    dir_rec
        ld      de,file_name
        ld      bc,13
        ldir
        ld      a,1
        ld      (have_name),a
        jp      song_open

; 첫 표시된 번호 -> A, 없으면 Carry
mark_first:
        xor     a
mf_l:   ld      hl,dir_count
        cp      (hl)
        jr      nc,mf_none
        push    af
        call    mark_get
        jr      nz,mf_hit
        pop     af
        inc     a
        jr      mf_l
mf_hit: pop     af
        or      a
        ret
mf_none:scf
        ret

; ESC / RET: 표시한 곡이 있으면 그 곡들만 재생(첫 곡부터), 없으면 그냥 나간다
files_leave:
        ld      a,(mark_count)
        or      a
        jr      z,fl_out
        call    mark_first
        jr      c,fl_out
        call    open_list_entry
fl_out: xor     a
        jp      screen_switch

        IF TESTMODE
; ---- [테스트모드] 커서 이동 시간 표시: MOVE = 이동 전체, DOS = 파일 열기~닫기 (단위 1/fps 초) ----
tm_mv0: defw    0
tm_d0:  defw    0
tm_dos: defw    0
tm_mv:  defw    0
tm_show:
        ld      hl,(JIFFY)
        ld      de,(tm_mv0)
        or      a
        sbc     hl,de
        ld      (tm_mv),hl
        call    sb_reset
        ld      hl,tm_t1
        call    sb_str
        ld      hl,(tm_mv)
        call    sb_dec16
        ld      hl,tm_t2
        call    sb_str
        ld      hl,(tm_dos)
        call    sb_dec16
        ld      hl,tm_t3
        call    sb_str
        ld      a,(fps)
        call    sb_dec
        call    sb_done
        TXTD    310,176,194,C_ORG,C_PN,0
        ret
tm_t1:  defb    "T MOVE ",0
tm_t2:  defb    " DOS ",0
tm_t3:  defb    " /",0
        ENDIF

; ============================================================
;  파일 정보 기억 (메모리): 한 번 읽은 파일의 정보를 목록 번호 칸에 둔다.  폴더가 바뀌면 지운다.
;  칸(INFO_REC = 44바이트): 표시 1 (비트0 = 유효, 비트1 = 템포 있음), 형식 1, 트랙 수 1, 분해능 2 (파일 순서),
;                           템포 3, 크기 4, 제목 31 (FI_TITLE_MAX+1), 남는 1
;  번호가 INFO_MAX 이상이면 기억하지 않고 그때그때 읽는다.
; ============================================================
I_FLAG  equ     0
I_FMT   equ     1
I_TRK   equ     2
I_PPQ   equ     3
I_TEMPO equ     5
I_SIZE  equ     8
I_TITLE equ     12

; 기억을 모두 지운다
info_clear:
        xor     a
        ld      (info_dirty),a
        ld      hl,INFOC
        ld      de,INFOC+1
        ld      bc,INFO_MAX*INFO_REC-1
        ld      (hl),0
        ldir
        ret

; A = 목록 번호 -> Carry = 칸이 있다, HL = 칸 주소 (Carry 없음 = 번호가 너무 큼)
info_slot:
        cp      INFO_MAX
        jr      nc,is_none
        ld      l,a
        ld      h,0
        add     hl,hl
        add     hl,hl           ; 4a
        ld      d,h
        ld      e,l             ; DE = 4a
        add     hl,hl           ; 8a
        push    hl
        add     hl,hl
        add     hl,hl           ; 32a
        pop     bc
        add     hl,bc           ; 40a
        add     hl,de           ; 44a
        ld      bc,INFOC
        add     hl,bc
        scf
        ret
is_none:or      a
        ret

; A = 목록 번호 -> 기억해 둔 정보가 있으면 hdr_buf / fi_* 에 되살린다.  Carry = 되살림
info_load:
        call    info_slot
        ret     nc
        ld      a,(hl)
        and     1
        jr      nz,il_has
        or      a               ; 기억 없음 (Carry 없음)
        ret
il_has: ld      a,(hl)
        rrca
        and     1               ; 비트1 = 템포 있음
        ld      (fi_has_tempo),a
        inc     hl
        ld      a,(hl)
        ld      (hdr_buf+9),a
        inc     hl
        ld      a,(hl)
        ld      (hdr_buf+11),a
        inc     hl
        ld      de,hdr_buf+12
        ld      bc,2
        ldir
        ld      de,fi_tempo
        ld      c,3
        ldir
        ld      de,fi_size
        ld      c,4
        ldir
        ld      de,fi_title
        ld      bc,FI_TITLE_MAX+1
        ldir
        scf
        ret

; A = 목록 번호 -> hdr_buf / fi_* 의 정보를 그 칸에 기억한다.  Carry = 기억함
info_store:
        call    info_slot
        ret     nc
        ld      a,(fi_has_tempo)
        add     a,a
        or      1
        ld      (hl),a
        inc     hl
        ld      a,(hdr_buf+9)
        ld      (hl),a
        inc     hl
        ld      a,(hdr_buf+11)
        ld      (hl),a
        inc     hl
        ex      de,hl
        ld      hl,hdr_buf+12
        ld      bc,2
        ldir
        ld      hl,fi_tempo
        ld      c,3
        ldir
        ld      hl,fi_size
        ld      c,4
        ldir
        ld      hl,fi_title
        ld      bc,FI_TITLE_MAX+1
        ldir
        ld      a,1
        ld      (info_dirty),a  ; 저장할 새 정보가 생겼다
        scf
        ret

; 고른 줄만 다시 그린다 (정보를 처음 기억했을 때 제목을 보이게)
files_row_selected:
        ld      a,(dir_sel)
        ld      hl,dir_top
        sub     (hl)
        ld      (fr_i),a
        jp      files_row

; 목록 줄 오른쪽에 곡 제목 (기억해 둔 것만): fr_rec = 이 줄의 칸, fr_idx = 목록 번호
files_row_title:
        ld      hl,(fr_rec)
        ld      de,13
        add     hl,de
        ld      a,(hl)
        or      a
        ret     nz              ; 폴더는 제목이 없다
        ld      a,(fr_idx)
        call    info_slot
        ret     nc
        ld      a,(hl)
        and     1
        ret     z
        ld      de,I_TITLE
        add     hl,de
        ld      a,(hl)
        or      a
        ret     z               ; 제목이 없는 파일
        push    hl
        ld      a,(fr_bg)
        ld      c,a
        ld      b,C_LN          ; 보통 줄은 흐린 글자색
        cp      C_TX
        jr      nz,frt_go
        ld      b,C_BG          ; 고른 줄(크림색 바탕)은 어두운 글자색
frt_go: ld      hl,160
        ld      a,(fr_y)
        ld      de,140
        call    text_at
        xor     a
        ld      (t_al),a
        pop     hl
        jp      txtp
