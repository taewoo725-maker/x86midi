; ============================================================
; 만든이: KimTaeWoo
; 도와준이: Lee.J.H
;  ui_macros.inc - 화면 그리기 매크로 (rect / frame / txt 를 한 줄로 쓰게 해 준다)
;  (매개변수 이름이 레지스터 이름이면 안 되므로 p 로 시작)
; ============================================================
; 채운 사각형
BOX     macro   px,py,pw,ph,pc
        call    rect
        defw    px,py,pw,ph
        defb    pc
        endm

; 테두리만
LINEBOX macro   px,py,pw,ph,pc
        call    frame
        defw    px,py,pw,ph
        defb    pc
        endm

; 채움 + 테두리 (시안의 box(x,y,w,h,fill,line) 와 같다)
PANEL   macro   px,py,pw,ph,pfill,pline
        call    rect
        defw    px,py,pw,ph
        defb    pfill
        call    frame
        defw    px,py,pw,ph
        defb    pline
        endm

; 글자 : x, y(글자 맨 윗줄), 칸너비(0=글자너비), 글자색, 바탕색, 정렬(0왼 1가운데 2오른), 글
TXT     macro   px,py,pw,pfg,pbg,pal,ptext
        call    txt
        defw    px
        defb    py
        defw    pw
        defb    pfg,pbg,pal
        defb    ptext,0
        endm

; 2배 크기 글자 (큰 숫자)
TXT2    macro   px,py,pw,pfg,pbg,pal,ptext
        call    txt2
        defw    px
        defb    py
        defw    pw
        defb    pfg,pbg,pal
        defb    ptext,0
        endm

; 변수 글자 : 글은 HL 에 담아 부른다 (글 자리를 뺀 TXT)
TXTD    macro   px,py,pw,pfg,pbg,pal
        call    txtd
        defw    px
        defb    py
        defw    pw
        defb    pfg,pbg,pal
        endm

TXTD2   macro   px,py,pw,pfg,pbg,pal
        call    txtd2
        defw    px
        defb    py
        defw    pw
        defb    pfg,pbg,pal
        endm
