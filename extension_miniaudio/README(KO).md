# Defold Miniaudio Extension

[English Version](./README(EN).md)

**Defold Miniaudio Extension**은 Defold 엔진을 위한 고성능 오디오 솔루션입니다.
[**miniaudio**](https://miniaud.io) 라이브러리를 기반으로 하며, `sys.load_resource`를 활용한 **메모리 기반 재생 방식**을 사용하여 번들링 후 발생하는 **파일 경로 문제를 완벽하게 해결**했습니다.

***

## 주요 기능 (Key Features)

- **완벽한 번들링 지원**:
파일 시스템 경로에 의존하지 않고 **메모리에서 직접 재생**되므로 Android, iOS, HTML5 등 **모든 플랫폼에서 안정적으로 작동**합니다.
- **고음질 오디오 지원**:
16-bit, 24-bit, 32-bit Float 포맷의 **고해상도 오디오 파일** 지원.
- **고급 제어 기능**:
    - **BGM**: 루핑(Loop), 일시정지/재개(Pause/Resume), 페이드 인/아웃(Fade)
    - **SFX**: 최대 8채널 풀링(Pooling), 패닝(Panning), 중첩 재생 지원
    - **Volume**: 마스터 볼륨 및 트랙별 실시간 볼륨 조절
- **최적화**:
C++ 힙(Heap) 메모리 관리로 Lua GC 부하 최소화.
- **간편한 사용**:
내장된 Lua 래퍼 모듈을 통해 바로 `require` 하여 사용 가능.

***

## 설치 방법 (Installation)

### 1. 폴더 배치

이 저장소를 다운로드하여 프로젝트 루트에 `extension_miniaudio` 폴더를 통째로 복사합니다.

```
Project Root
├── extension_miniaudio/
│   ├── include/
│   ├── src/
│   ├── ext.manifest
│   └── miniaudio.lua       <-- 핵심 Lua 모듈
├── assets/
│   └── sounds/             <-- 사용자의 사운드 파일 폴더
└── game.project
```


### 2. game.project 설정 (필수)

Defold가 사운드 파일을 번들링에 포함하도록 설정해야 합니다.

1. `game.project` 파일을 엽니다.
2. `[project]` 섹션의 `custom_resources` 항목에 사운드 폴더 경로를 추가합니다.

예시:

```
[project]
custom_resources = /assets/sounds
```

쉼표로 여러 경로를 구분할 수 있습니다.

***

## 퀵 스타트 (Quick Start)

가장 기본적인 사용 예시입니다.

```lua
-- 1. 모듈 불러오기
local ma = require "extension_miniaudio.miniaudio"

function init(self)
    -- 2. 엔진 초기화 (필수)
    ma.init()

    -- 3. 배경음악 재생
    ma.play_bgm("/assets/sounds/bgm_main.ogg")
end

function on_input(self, action_id, action)
    if action_id == hash("touch") and action.pressed then
        -- 4. 효과음 재생
        ma.play_sfx("/assets/sounds/click.wav")
    end
end
```


***

## API 매뉴얼 (API Reference)

### 1. 초기화

```lua
ma.init()
```

**설명:**
오디오 엔진을 초기화합니다.
게임 시작 시(`main.script` 등) **한 번만 호출**하면 됩니다.

***

### 2. 배경음악 (BGM)

**단일 트랙**이 자동 반복되며 동시에 하나만 재생됩니다.

```lua
ma.play_bgm(path)
```

- **path**: string — 오디오 파일 경로 (예: `"/assets/music.ogg"`)
- **설명**: 기존 BGM을 정지하고 메모리를 해제한 후 새 음악을 재생합니다.

```lua
ma.stop_bgm()
```

- **설명**: BGM을 완전히 정지하고 재생 위치를 처음으로 돌립니다.

```lua
ma.pause_bgm()
```

- **설명**: 현재 위치에서 일시 정지합니다.

```lua
ma.resume_bgm()
```

- **설명**: 정지된 위치에서 다시 재생합니다.

```lua
ma.set_bgm_volume(volume)
```

- **volume**: number — `0.0` (음소거) ~ `1.0` (최대)
- **설명**: 배경음악의 볼륨을 즉시 변경합니다.

```lua
ma.fade_bgm(target_volume, duration)
```

- **target_volume**: number (0.0 ~ 1.0)
- **duration**: number (초 단위)
- **설명**: 지정된 시간 동안 부드럽게 BGM 볼륨을 변경 (페이드 인/아웃 구현 가능).

***

### 3. 효과음 (SFX)

효과음은 **풀링 시스템**을 사용해 여러 소리를 겹쳐 재생할 수 있습니다.

```lua
ma.play_sfx(path, [pan])
```

- **path**: string — 효과음 파일 경로
- **pan** (옵션): number — 스테레오 패닝 값
    - `-1.0`: 왼쪽 100%
    - `0.0`: 중앙 (기본값)
    - `1.0`: 오른쪽 100%

**설명:** 효과음을 재생합니다. `pan`을 생략하면 중앙에서 재생됩니다.

```lua
ma.set_sfx_volume(volume)
```

- **volume**: number (0.0 ~ 1.0)
- **설명:** 모든 효과음 채널의 마스터 볼륨 조절.

***

## 고급 사용자 가이드 (Advanced Guide)

### 자연스러운 BGM 전환 (Crossfade)

음악이 뚝 끊기지 않고 부드럽게 전환되도록 하는 예시입니다.

```lua
-- 1. 현재 음악 페이드 아웃 (2초간)
ma.fade_bgm(0.0, 2.0)

-- 2. 2초 후 새 음악 재생 (타이머 사용)
timer.delay(2.0, false, function()
    ma.play_bgm("/assets/sounds/boss_theme.ogg")
    ma.set_bgm_volume(0.0) -- 시작 볼륨 0

    -- 3. 페이드 인 (3초간)
    ma.fade_bgm(1.0, 3.0)
end)
```


***

### 공간감 있는 사운드 (Panning)

화면 내 오브젝트 위치에 따라 소리의 방향을 바꿀 수 있습니다.

```lua
-- 화면 왼쪽(-1.0) ~ 오른쪽(1.0)
local screen_width = sys.get_config("display.width")
local pan_value = (action.x / screen_width) * 2.0 - 1.0

ma.play_sfx("/assets/sounds/explosion.wav", pan_value)
```


***

## 기술적 상세 (Memory Management)

이 확장은 `sys.load_resource()`로 받은 바이너리 데이터를
**C++ 힙(Heap) 메모리**에 복사(`malloc`)하여 보관합니다.

- **안정성**:
재생 중 Lua GC가 데이터를 해제해 소리가 끊기는 현상을 원천적으로 차단.
- **효율성**:
BGM 변경 시 이전 트랙의 메모리를 즉시 해제(`free`)해 누수(Leak) 발생을 방지.

***

## 라이선스 (License)

이 프로젝트는 **MIT License**를 따릅니다.
자세한 내용은 `LICENSE` 파일을 참조하세요.

***