; ============================================================
; 만든이: KimTaeWoo
; 도와준이: Lee.J.H
; data_player.inc - 플레이어 화면의 고정 자료 (자동 생성: 아이콘 점그림, 건반 위치표, 건반 글자)
; ============================================================
; 버튼 안쪽 14 x 10 점그림 (한 줄 = 2바이트, 왼쪽 맞춤, 1 = 아이콘)
icon_prev:
        defw    00000h          ; ..............
        defw    03020h          ; ..##......#...
        defw    03060h          ; ..##.....##...
        defw    031E0h          ; ..##...####...
        defw    033E0h          ; ..##..#####...
        defw    033E0h          ; ..##..#####...
        defw    031E0h          ; ..##...####...
        defw    03060h          ; ..##.....##...
        defw    03020h          ; ..##......#...
        defw    00000h          ; ..............
icon_rew:
        defw    00000h          ; ..............
        defw    00410h          ; .....#.....#..
        defw    00C30h          ; ....##....##..
        defw    01C70h          ; ...###...###..
        defw    03CF0h          ; ..####..####..
        defw    03CF0h          ; ..####..####..
        defw    01C70h          ; ...###...###..
        defw    00C30h          ; ....##....##..
        defw    00410h          ; .....#.....#..
        defw    00000h          ; ..............
icon_play:
        defw    00000h          ; ..............
        defw    00800h          ; ....#.........
        defw    00C00h          ; ....##........
        defw    00F00h          ; ....####......
        defw    00F80h          ; ....#####.....
        defw    00F80h          ; ....#####.....
        defw    00F00h          ; ....####......
        defw    00C00h          ; ....##........
        defw    00800h          ; ....#.........
        defw    00000h          ; ..............
icon_pause:
        defw    00000h          ; ..............
        defw    00CC0h          ; ....##..##....
        defw    00CC0h          ; ....##..##....
        defw    00CC0h          ; ....##..##....
        defw    00CC0h          ; ....##..##....
        defw    00CC0h          ; ....##..##....
        defw    00CC0h          ; ....##..##....
        defw    00CC0h          ; ....##..##....
        defw    00CC0h          ; ....##..##....
        defw    00000h          ; ..............
icon_stop:
        defw    00000h          ; ..............
        defw    00000h          ; ..............
        defw    00FC0h          ; ....######....
        defw    00FC0h          ; ....######....
        defw    00FC0h          ; ....######....
        defw    00FC0h          ; ....######....
        defw    00FC0h          ; ....######....
        defw    00FC0h          ; ....######....
        defw    00000h          ; ..............
        defw    00000h          ; ..............
icon_ff:
        defw    00000h          ; ..............
        defw    02080h          ; ..#.....#.....
        defw    030C0h          ; ..##....##....
        defw    038E0h          ; ..###...###...
        defw    03CF0h          ; ..####..####..
        defw    03CF0h          ; ..####..####..
        defw    038E0h          ; ..###...###...
        defw    030C0h          ; ..##....##....
        defw    02080h          ; ..#.....#.....
        defw    00000h          ; ..............
icon_next:
        defw    00000h          ; ..............
        defw    02060h          ; ..#......##...
        defw    03060h          ; ..##.....##...
        defw    03C60h          ; ..####...##...
        defw    03E60h          ; ..#####..##...
        defw    03E60h          ; ..#####..##...
        defw    03C60h          ; ..####...##...
        defw    03060h          ; ..##.....##...
        defw    02060h          ; ..#......##...
        defw    00000h          ; ..............
; 건반 C 글자 "C1".."C8" : 7 x 5 점그림
keylab_c1:
        defw    0E400h          ; ###..#.
        defw    08C00h          ; #...##.
        defw    08400h          ; #....#.
        defw    08400h          ; #....#.
        defw    0EE00h          ; ###.###
keylab_c2:
        defw    0EE00h          ; ###.###
        defw    08200h          ; #.....#
        defw    08E00h          ; #...###
        defw    08800h          ; #...#..
        defw    0EE00h          ; ###.###
keylab_c3:
        defw    0EE00h          ; ###.###
        defw    08200h          ; #.....#
        defw    08E00h          ; #...###
        defw    08200h          ; #.....#
        defw    0EE00h          ; ###.###
keylab_c4:
        defw    0EA00h          ; ###.#.#
        defw    08A00h          ; #...#.#
        defw    08E00h          ; #...###
        defw    08200h          ; #.....#
        defw    0E200h          ; ###...#
keylab_c5:
        defw    0EE00h          ; ###.###
        defw    08800h          ; #...#..
        defw    08E00h          ; #...###
        defw    08200h          ; #.....#
        defw    0EE00h          ; ###.###
keylab_c6:
        defw    0EE00h          ; ###.###
        defw    08800h          ; #...#..
        defw    08E00h          ; #...###
        defw    08A00h          ; #...#.#
        defw    0EE00h          ; ###.###
keylab_c7:
        defw    0EE00h          ; ###.###
        defw    08200h          ; #.....#
        defw    08400h          ; #....#.
        defw    08400h          ; #....#.
        defw    0E400h          ; ###..#.
keylab_c8:
        defw    0EE00h          ; ###.###
        defw    08A00h          ; #...#.#
        defw    08E00h          ; #...###
        defw    08A00h          ; #...#.#
        defw    0EE00h          ; ###.###
keylab_tbl:
        defw    keylab_c1,keylab_c2,keylab_c3,keylab_c4,keylab_c5,keylab_c6,keylab_c7,keylab_c8
; 건반 위치표: 음번호 21(A0)..108(C8) 순서대로 3바이트 = 종류(0 흰건반/1 검은건반), x 좌표(2바이트)
KEY_COUNT equ 88
key_geom:
        defb    0
        defw    6       ; 음 21
        defb    1
        defw    13       ; 음 22
        defb    0
        defw    16       ; 음 23
        defb    0
        defw    25       ; 음 24
        defb    1
        defw    32       ; 음 25
        defb    0
        defw    35       ; 음 26
        defb    1
        defw    41       ; 음 27
        defb    0
        defw    44       ; 음 28
        defb    0
        defw    54       ; 음 29
        defb    1
        defw    61       ; 음 30
        defb    0
        defw    64       ; 음 31
        defb    1
        defw    70       ; 음 32
        defb    0
        defw    73       ; 음 33
        defb    1
        defw    80       ; 음 34
        defb    0
        defw    83       ; 음 35
        defb    0
        defw    92       ; 음 36
        defb    1
        defw    99       ; 음 37
        defb    0
        defw    102       ; 음 38
        defb    1
        defw    109       ; 음 39
        defb    0
        defw    112       ; 음 40
        defb    0
        defw    121       ; 음 41
        defb    1
        defw    128       ; 음 42
        defb    0
        defw    131       ; 음 43
        defb    1
        defw    137       ; 음 44
        defb    0
        defw    140       ; 음 45
        defb    1
        defw    147       ; 음 46
        defb    0
        defw    150       ; 음 47
        defb    0
        defw    160       ; 음 48
        defb    1
        defw    166       ; 음 49
        defb    0
        defw    169       ; 음 50
        defb    1
        defw    176       ; 음 51
        defb    0
        defw    179       ; 음 52
        defb    0
        defw    188       ; 음 53
        defb    1
        defw    195       ; 음 54
        defb    0
        defw    198       ; 음 55
        defb    1
        defw    205       ; 음 56
        defb    0
        defw    208       ; 음 57
        defb    1
        defw    214       ; 음 58
        defb    0
        defw    217       ; 음 59
        defb    0
        defw    227       ; 음 60
        defb    1
        defw    233       ; 음 61
        defb    0
        defw    236       ; 음 62
        defb    1
        defw    243       ; 음 63
        defb    0
        defw    246       ; 음 64
        defb    0
        defw    256       ; 음 65
        defb    1
        defw    262       ; 음 66
        defb    0
        defw    265       ; 음 67
        defb    1
        defw    272       ; 음 68
        defb    0
        defw    275       ; 음 69
        defb    1
        defw    281       ; 음 70
        defb    0
        defw    284       ; 음 71
        defb    0
        defw    294       ; 음 72
        defb    1
        defw    301       ; 음 73
        defb    0
        defw    304       ; 음 74
        defb    1
        defw    310       ; 음 75
        defb    0
        defw    313       ; 음 76
        defb    0
        defw    323       ; 음 77
        defb    1
        defw    329       ; 음 78
        defb    0
        defw    332       ; 음 79
        defb    1
        defw    339       ; 음 80
        defb    0
        defw    342       ; 음 81
        defb    1
        defw    349       ; 음 82
        defb    0
        defw    352       ; 음 83
        defb    0
        defw    361       ; 음 84
        defb    1
        defw    368       ; 음 85
        defb    0
        defw    371       ; 음 86
        defb    1
        defw    377       ; 음 87
        defb    0
        defw    380       ; 음 88
        defb    0
        defw    390       ; 음 89
        defb    1
        defw    397       ; 음 90
        defb    0
        defw    400       ; 음 91
        defb    1
        defw    406       ; 음 92
        defb    0
        defw    409       ; 음 93
        defb    1
        defw    416       ; 음 94
        defb    0
        defw    419       ; 음 95
        defb    0
        defw    428       ; 음 96
        defb    1
        defw    435       ; 음 97
        defb    0
        defw    438       ; 음 98
        defb    1
        defw    445       ; 음 99
        defb    0
        defw    448       ; 음 100
        defb    0
        defw    457       ; 음 101
        defb    1
        defw    464       ; 음 102
        defb    0
        defw    467       ; 음 103
        defb    1
        defw    473       ; 음 104
        defb    0
        defw    476       ; 음 105
        defb    1
        defw    483       ; 음 106
        defb    0
        defw    486       ; 음 107
        defb    0
        defw    496       ; 음 108
