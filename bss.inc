; ============================================================
; 만든이: KimTaeWoo
; 도와준이: Lee.J.H
;  bss.inc - 프로그램 뒤의 작업 영역 (파일에는 들어가지 않는다: 주소만 정한다)
;  [낮은 영역] low_end 뒤: 화면/글자/목록처럼 메인 쪽에서만 쓰는 것
;  [높은 영역] hi_end 뒤 : 인터럽트 엔진이 쓰는 것 (트랙 칸, 트랙 창, 메타 자료 ...)
;  (MSX-DOS 는 인터럽트가 걸리면 0000h~3FFFh 에 BIOS ROM 을 넣고 디스크 ROM 이 4000h~7FFFh 에 들어올 수
;   있으므로, 인터럽트에서 부르는 코드와 자료는 모두 8000h 이상에 둔다)
; ============================================================
TXB       equ   low_end                 ; 글자/그림 한 줄 (화소 2개 = 1바이트, VRAM 모양)
TXB_SZ    equ   0240h
TGI       equ   TXB+TXB_SZ              ; 글자 번호 목록 (끝 = FFh), 128바이트
NUMBUF_AT equ   TGI+128
numbuf    equ   NUMBUF_AT               ; 16바이트
strbuf    equ   NUMBUF_AT+16            ; 글자 조립 영역 (80바이트)
DIRLIST   equ   strbuf+80               ; 파일 목록 (칸 16바이트 x 255 = 약 4KB)
DIR_MAX   equ   255                     ; 목록 번호는 1바이트: 0..254 (FFh 는 "모름" 표시로 쓴다)
DIR_REC   equ   16
DIRMARK   equ   DIRLIST+DIR_MAX*DIR_REC ; 목록 번호별 선택 표시 (0 / 1) x 255

INFOC     equ   DIRMARK+DIR_MAX         ; 목록 번호별 파일 정보 기억 (칸 44바이트 x 128) - 낮은 영역의 남는 자리 (8000h 아래)
INFO_REC  equ   44
INFO_MAX  equ   128
INFO_END  equ   INFOC+INFO_MAX*INFO_REC
INFOS     equ   INFO_END                ; 목록 번호별 바뀜 확인용 도장 8바이트 (시각 2 + 날짜 2 + 크기 4) x INFO_MAX
INFOIO    equ   INFOS+INFO_MAX*8        ; X86MIDI.DIR 읽기/쓰기 덩어리 (512바이트)
INFO_ALL_END equ INFOIO+512
        IF INFO_ALL_END > 7F00h
        defw    1/0                 ; 낮은 영역(8000h 아래)을 넘으면 일부러 0 으로 나눠 빌드를 멈춘다
        ENDIF

TRKS      equ   hi_end                  ; 트랙 칸 16개 x 40바이트
TMAP      equ   TRKS+640                ; 박자표 64개 x 7바이트 (틱 4 + 템포 3)
TMAP_MAX  equ   64
diag_log  equ   TMAP+TMAP_MAX*7         ; 진단용 MIDI 출력 기록
DIAG_LOG_SZ equ 1024
EVBUF_RAW equ   diag_log+DIAG_LOG_SZ
EVBUF     equ   (EVBUF_RAW+0FFh) AND 0FF00h ; 메타/SysEx 자료 한 건 (256바이트, 쪽 경계에 맞춤)
EVBUF_SZ  equ   256
TBUF      equ   EVBUF+EVBUF_SZ          ; 트랙 창 16개 x 512바이트 (곡 크기와 무관한 고정 8KB)
TBUF_SZ   equ   512
TBUF_END  equ   TBUF+16*TBUF_SZ
MQ        equ   (TBUF_END+0FFh) AND 0FF00h ; MIDI 출력 큐 (256바이트 고리, 쪽 경계에 맞춤)
MEM_END   equ   MQ+256                  ; 이 주소까지 쓴다 (check_track_memory 가 비교)
