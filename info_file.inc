; ============================================================
; 만든이: KimTaeWoo
; 도와준이: Lee.J.H
;  info_file.inc - X86MIDI.DIR : 폴더마다 파일 정보(제목, 템포, 크기 ...)를 저장해 두고 되살린다
;
;  파일 모양: 64바이트 칸의 줄.  첫 칸 = 머리 ("XMD1", 버전 1), 이후 한 파일에 한 칸:
;     이름 13 (끝 0, 남는 곳 0) + 도장 8 (시각 2, 날짜 2, 크기 4) + 정보 43 (정보 기억 칸의 앞 43바이트)
;  읽기: 폴더를 읽은 직후(dir_scan 끝) 한 번에.  이름과 도장이 같은 파일만 되살린다 (바뀐 파일은 다시 읽는다).
;  쓰기: 새 정보가 있을 때만(info_dirty).  연주 중에는 쓰지 않는다 (정지 상태에서만).
;        폴더를 옮기기 전 / 파일 화면을 나갈 때 / 프로그램을 끝낼 때.  실패해도 조용히 넘어간다 (메모리 기억은 그대로).
;  쓰기 중 디스크 오류(쓰기 방지 등)로 DOS 가 화면에 물음을 띄우지 않도록 _DEFER 루틴으로 "무시"를 돌려 준다.
; ============================================================
INFO_FILE_REC   equ     64
INFO_FILE_IOSZ  equ     512
INFO_SLOT_SAVED equ     43      ; 파일에 저장하는 정보 바이트 수 (INFO_REC 44 중 앞 43)

info_file_name: defb    "X86MIDI.DIR",0
info_magic:     defb    "XMD1"
info_dirty:     defb    0       ; 1 = 아직 저장 안 한 새 정보가 있다
inf_h:          defb    0       ; 열린 파일
inf_ptr:        defw    0       ; 덩어리 안의 다음 칸 (읽기) / 쓸 자리 (쓰기)
inf_cnt:        defb    0       ; 이번 덩어리에 남은 칸 수
inf_first:      defb    0       ; 1 = 아직 첫 덩어리 (머리 확인 전)
inf_k:          defb    0       ; 읽기: 읽은 칸 순서 / 쓰기: 목록 번호
inf_n:          defw    0       ; 이번에 읽은 바이트 수
inf_rec:        defw    0       ; 지금 보는 칸
inf_idx:        defb    0       ; 칸과 이름이 같은 목록 번호

; ---- 도장 ----
; A = 목록 번호 -> HL = 그 도장 자리 (BC 를 쓴다)
info_stamp_addr:
        ld      l,a
        ld      h,0
        add     hl,hl
        add     hl,hl
        add     hl,hl
        ld      bc,INFOS
        add     hl,bc
        ret

; dir_take 가 부른다: 방금 읽은 항목(dir_fib)의 도장을 목록 번호 dir_count 칸에 적는다
info_stamp_take:
        ld      a,(dir_count)
        cp      INFO_MAX
        ret     nc
        call    info_stamp_addr
        ex      de,hl
        ld      hl,dir_fib+15           ; 시각 2 + 날짜 2
        ld      bc,4
        ldir
        ld      hl,dir_fib+FIB_SIZE     ; 크기 4
        ld      c,4
        ldir
        ret

; ---- 읽기 ----
info_file_load:
        ld      a,(dir_count)
        or      a
        ret     z
        ld      de,info_file_name
        ld      a,1                     ; 읽기 전용
        ld      c,_OPEN
        call    BDOS
        or      a
        ret     nz                      ; 파일이 없으면 그냥 지나간다
        ld      a,b
        ld      (inf_h),a
        ld      a,1
        ld      (inf_first),a
        xor     a
        ld      (inf_k),a
ifl_chunk:
        ld      a,(inf_h)
        ld      b,a
        ld      de,INFOIO
        ld      hl,INFO_FILE_IOSZ
        ld      c,_READ
        call    BDOS
        cp      0C7h                    ; .EOF 는 정상 (읽은 바이트 수는 HL)
        jr      z,ifl_got
        or      a
        jr      nz,ifl_done
ifl_got:ld      (inf_n),hl
        ld      b,6
ifl_div:srl     h
        rr      l
        djnz    ifl_div                 ; L = 읽은 칸 수 (바이트 / 64)
        ld      a,l
        ld      (inf_cnt),a
        or      a
        jr      z,ifl_done
        ld      hl,INFOIO
        ld      (inf_ptr),hl
        ld      a,(inf_first)
        or      a
        jr      z,ifl_rec
        ld      hl,INFOIO               ; 첫 칸 = 머리: 글자 확인 후 건너뛴다
        ld      de,info_magic
        ld      b,4
ifl_mg: ld      a,(de)
        cp      (hl)
        jr      nz,ifl_done             ; 우리 파일이 아니다
        inc     hl
        inc     de
        djnz    ifl_mg
        ld      hl,INFOIO+INFO_FILE_REC
        ld      (inf_ptr),hl
        ld      hl,inf_cnt
        dec     (hl)
        xor     a
        ld      (inf_first),a
ifl_rec:ld      a,(inf_cnt)
        or      a
        jr      z,ifl_next
        ld      hl,(inf_ptr)
        call    info_file_apply
        ld      hl,(inf_ptr)
        ld      de,INFO_FILE_REC
        add     hl,de
        ld      (inf_ptr),hl
        ld      hl,inf_cnt
        dec     (hl)
        ld      hl,inf_k
        inc     (hl)
        jr      ifl_rec
ifl_next:
        ld      a,(inf_n+1)
        cp      2                       ; 512 바이트를 다 채웠으면 더 있을 수 있다
        jr      z,ifl_chunk
ifl_done:
        ld      a,(inf_h)
        ld      b,a
        ld      c,_CLOSE
        jp      BDOS

; HL = 파일의 칸 -> 이름이 같은 목록 번호를 찾아, 도장이 같으면 정보를 되살린다
info_file_apply:
        ld      (inf_rec),hl
        ld      a,(inf_k)               ; 순서가 같을 가능성이 크니 같은 번호부터 본다
        ld      b,a
        ld      a,(dir_count)
        cp      b
        jr      z,ifa_scan
        jr      c,ifa_scan
        ld      a,b
        call    info_name_is
        ld      a,(inf_k)               ; (ld 는 Z 를 바꾸지 않는다)
        jr      z,ifa_found
ifa_scan:
        ld      a,(dir_count)
        ld      b,a
        ld      c,0
ifa_l:  push    bc
        ld      a,c
        call    info_name_is
        pop     bc
        ld      a,c
        jr      z,ifa_found
        inc     c
        djnz    ifa_l
        ret
ifa_found:
        ld      (inf_idx),a
        cp      INFO_MAX
        ret     nc                      ; 기억 칸이 없는 번호
        ld      hl,(inf_rec)
        ld      de,13
        add     hl,de
        push    hl                      ; 칸의 도장
        ld      a,(inf_idx)
        call    info_stamp_addr         ; HL = 지금 폴더의 도장
        pop     de
        ld      b,8
ifa_c:  ld      a,(de)
        cp      (hl)
        ret     nz                      ; 바뀐 파일: 되살리지 않는다
        inc     de
        inc     hl
        djnz    ifa_c
        ex      de,hl                   ; HL = 칸의 정보 (도장 바로 뒤)
        ld      a,(hl)
        and     1
        ret     z                       ; 유효 표시가 없는 칸
        push    hl
        ld      a,(inf_idx)
        call    info_slot
        ex      de,hl
        pop     hl
        ld      bc,INFO_SLOT_SAVED
        ldir
        xor     a
        ld      (de),a                  ; 44번째 바이트
        ret

; A = 목록 번호 -> Z = 목록의 이름이 inf_rec 칸의 이름과 같다
info_name_is:
        call    dir_rec
        ld      de,(inf_rec)
        ld      b,13
ini_l:  ld      a,(de)
        cp      (hl)
        ret     nz
        or      a
        ret     z
        inc     hl
        inc     de
        djnz    ini_l
        xor     a
        ret

; ---- 쓰기 ----
; 새 정보가 있고 연주 중이 아니면 X86MIDI.DIR 을 새로 쓴다.  실패해도 아무 일 없다
info_file_save_if_dirty:
        ld      a,(info_dirty)
        or      a
        ret     z
        ld      a,(play_state)
        or      a
        ret     nz                      ; 연주 중에는 쓰지 않는다 (나중에 다시 시도)
        xor     a
        ld      (info_dirty),a
        ld      de,info_disk_error
        ld      c,_DEFER
        call    BDOS
        call    info_file_write
        ld      de,0
        ld      c,_DEFER
        jp      BDOS

info_disk_error:
        ld      a,3                     ; 3 = 무시: 쓰기 방지 같은 오류로 DOS 가 물음을 띄우지 않게
        ret

info_file_write:
        ld      de,info_file_name
        xor     a                       ; 읽기/쓰기
        ld      b,a                     ; 보통 파일
        ld      c,_CREATE
        call    BDOS
        or      a
        ret     nz
        ld      a,b
        ld      (inf_h),a
        ld      hl,INFOIO               ; 머리 칸
        ld      d,h
        ld      e,l
        inc     de
        ld      (hl),0
        ld      bc,INFO_FILE_REC-1
        ldir
        ld      hl,info_magic
        ld      de,INFOIO
        ld      bc,4
        ldir
        ld      a,1
        ld      (INFOIO+4),a            ; 버전
        ld      hl,INFOIO+INFO_FILE_REC
        ld      (inf_ptr),hl
        xor     a
        ld      (inf_k),a
ifw_l:  ld      a,(inf_k)
        ld      b,a
        ld      a,(dir_count)
        cp      b
        jr      z,ifw_end
        jr      c,ifw_end
        ld      a,b
        cp      INFO_MAX
        jr      nc,ifw_end
        call    info_file_pack
        jr      c,ifw_close
        ld      hl,inf_k
        inc     (hl)
        jr      ifw_l
ifw_end:call    info_file_flush
ifw_close:
        ld      a,(inf_h)
        ld      b,a
        ld      c,_CLOSE
        jp      BDOS

; 목록 번호 inf_k 의 정보가 유효하면 inf_ptr 자리에 칸을 만들어 붙인다 (덩어리가 차면 쓴다).  Carry = 쓰기 오류
info_file_pack:
        ld      a,(inf_k)
        call    info_slot
        ld      a,(hl)
        and     1
        ret     z                       ; 정보 없음 (Carry 없음)
        ld      hl,(inf_ptr)            ; 칸을 0 으로 채운다
        ld      d,h
        ld      e,l
        inc     de
        ld      (hl),0
        ld      bc,INFO_FILE_REC-1
        ldir
        ld      a,(inf_k)
        call    dir_rec                 ; 이름 (끝 0 까지만)
        ld      de,(inf_ptr)
        ld      b,13
ifp_n:  ld      a,(hl)
        or      a
        jr      z,ifp_nd
        ld      (de),a
        inc     hl
        inc     de
        djnz    ifp_n
ifp_nd: ld      hl,(inf_ptr)
        ld      de,13
        add     hl,de
        ex      de,hl                   ; DE = 칸의 도장 자리
        ld      a,(inf_k)
        call    info_stamp_addr
        ld      bc,8
        ldir                            ; DE = 칸의 정보 자리
        push    de
        ld      a,(inf_k)
        call    info_slot
        pop     de
        ld      bc,INFO_SLOT_SAVED
        ldir
        ld      hl,(inf_ptr)
        ld      de,INFO_FILE_REC
        add     hl,de
        ld      (inf_ptr),hl
        ld      de,INFOIO+INFO_FILE_IOSZ
        or      a
        sbc     hl,de
        jr      c,ifp_ok                ; 아직 덩어리가 안 찼다
        jr      info_file_flush
ifp_ok: or      a
        ret

; 덩어리에 모인 칸을 파일에 쓰고 비운다.  Carry = 쓰기 오류
info_file_flush:
        ld      hl,(inf_ptr)
        ld      de,INFOIO
        or      a
        sbc     hl,de
        jr      z,iff_none
        ld      a,(inf_h)
        ld      b,a
        ld      de,INFOIO
        ld      c,_WRITE
        call    BDOS
        ld      hl,INFOIO
        ld      (inf_ptr),hl
        or      a
        ret     z
        scf
        ret
iff_none:
        or      a
        ret
