# X86MIDI

**한국어** | [English](README.en.md)

MSX-DOS2 / Nextor에서 동작하는 **MIDI(.mid) 파일 재생 프로그램**입니다.
X86MSX(PC용 MSX 에뮬레이터)와 MSX2 이상 기종(turbo R 포함)에서 실행하도록 Z80 어셈블리로 작성했습니다.

> 상태: 개발 중입니다. 현재 개발판은 시험 단계(V1.4.26T)이며, 실기 확인이 끝난 뒤 정식판으로 정리합니다.

## 특징

- **정확한 박자**: 재생 타이밍 정확도를 최우선으로 설계했습니다.
- **SCREEN 7 (512x212)** 화면의 5개 화면 구성

  | 키 | 화면 | 내용 |
  |----|------|------|
  | F1 | Player | 재생 상태, 진행, 조작 |
  | F2 | Roll | 피아노 롤 |
  | F3 | Lyrics | 가사 |
  | F4 | Files | 파일 / 폴더 목록 (곡 제목 표시) |
  | F5 | Setup | 설정 |

- 플레이어 화면에서 **H** 키로 단축키 도움말 팝업
- **파일 목록 제목 기억**: 폴더마다 `X86MIDI.DIR`에 곡 정보를 저장해 다음 실행 때 다시 읽지 않고 바로 표시 (폴더당 128곡까지)
- 코드는 기능별 `.inc` 파일로 나누어 공용화했습니다 (엔진, 화면, 파일, VDP, 키 처리 등).

## 요구 사양

- MSX2 이상 (MSX2+, turbo R 포함), 또는 X86MSX
- MSX-DOS2 또는 Nextor
- MIDI 출력 장치(또는 X86MSX의 MIDI 출력)

## 실행

```
X86MIDI.COM
```

MSX-DOS2 프롬프트에서 실행하고, F4 화면에서 `.mid` 파일을 골라 재생합니다.

## 빌드

[pasmo](https://pasmo.speccy.org/)가 필요합니다.

```
pasmo -I . --bin X86MIDI.ASM x86midi.com x86midi.sym
```

`x86midi.com`이 실행 파일, `x86midi.sym`이 심볼 파일입니다.

## 폴더 구성

| 파일 | 역할 |
|------|------|
| `X86MIDI.ASM` | 메인, 모든 모듈을 포함 |
| `engine.inc`, `tick.inc`, `isr.inc` | 재생 엔진, 타이밍 |
| `smf.inc`, `midi_out.inc` | MIDI 파일 해석, MIDI 출력 |
| `scr_*.inc`, `screens.inc` | 화면별 그리기 (Player, Roll, Lyrics, Files, Setup 등) |
| `vdp7.inc`, `text7.inc`, `font.inc`, `pal.inc` | VDP, 문자 출력, 글꼴, 팔레트 |
| `info_file.inc` | `X86MIDI.DIR` 곡 정보 저장 / 복원 |
| `dos.inc`, `bss.inc` | DOS 호출 정의, 메모리 배치 |

## 시험 기종

DPC-200, CPC-400, FS-A1WX, MSX turbo R GT 에서 확인합니다.

## 알려진 제한

- 폴더당 128곡까지만 곡 정보를 기억합니다.
- 연주 중에는 `X86MIDI.DIR`을 저장하지 않습니다.
- 쓰기 금지 디스크에서는 정보 저장이 조용히 건너뛰어집니다.

## 만든이

- 만든이: KimTaeWoo
- 도와준이: Lee.J.H

모든 소스 파일(`X86MIDI.ASM`, `*.inc`) 맨 위에도 같은 내용을 적어 두었습니다.

## 라이선스

(라이선스를 정해서 이곳에 적어 주세요.)
