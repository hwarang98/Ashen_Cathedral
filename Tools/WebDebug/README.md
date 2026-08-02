# Ashen Cathedral — 웹 디버그 대시보드

전투 타임라인과 런 통계를 브라우저에서 본다. 외부 브라우저 + 듀얼 모니터가 목적이므로
게임 안에 임베드하지 않는다(CEF/WebBrowser 위젯 사용 안 함).

```
게임 (UE)                                브라우저
 UACWebDebugSubsystem ── HTTP  8091 ───▶  정적 파일 + /api/runs
                      └─ WS    8092 ───▶  15Hz 이벤트 스트림
 UACRunLogSubsystem   ── Saved/RunLogs/run-XXXXX.json
```

---

## 1. 웹만으로 개발하기 (언리얼 불필요)

```bash
cd Tools/WebDebug
npm install
npm run dev          # http://localhost:5173
```

WebSocket 연결을 2초 안에 못 열면 **자동으로 mock 모드**로 넘어가
`mock/sample-timeline.json` 을 실시간처럼 재생한다. 상단에 `MOCK MODE` 배지가 뜬다.
런 통계 화면도 `/api/runs` 가 실패하면 `mock/runs/*.json` 을 읽으므로,
언리얼을 설치하지 않아도 전체 화면을 그대로 개발할 수 있다.

mock 데이터를 다시 만들려면:

```bash
npm run gen:mock     # sample-timeline.json + runs/run-0001..0020.json
```

`mock/` 디렉터리가 Vite 의 정적 루트(`publicDir`)다. 디스크 경로 `mock/runs/run-0001.json` 이
URL `/runs/run-0001.json` 으로 서빙된다.

## 2. 게임에 붙이기

```bash
npm run build        # → ../../Saved/WebDebug/ 로 바로 출력
```

게임을 실행하고 콘솔에서:

```
ac.WebDebug 1              서버 on/off (기본 0)
ac.WebDebug.Port 8091      HTTP 포트. WebSocket 은 이 값 +1
ac.WebDebug.Rate 15        전송 Hz
```

브라우저에서 `http://127.0.0.1:8091` 을 연다. 바인딩은 **127.0.0.1 고정**이며
외부 인터페이스에는 바인딩하지 않는다.

---

## 3. 스키마 요약

계약은 `src/lib/schema.ts` 와 `Source/Ashen_Cathedral/Public/Debug/ACWebDebugTypes.h` 두 곳에
같은 모양으로 있다. **한쪽만 바꾸면 안 된다.**

### 3.1 세션 헤더 — 연결 직후 1회, 전투 시작 시 갱신

```json
{ "kind": "session", "schema": 1, "sessionId": "a3f2c891", "runSeed": 8291,
  "bossId": "MetaProgression.BossID.AshenKnight", "attempt": 12,
  "weapon": "Player.Weapon.Katana", "buildConfig": "Development",
  "startedAtUtc": "2026-08-01T13:00:00Z" }
```

### 3.2 이벤트 배치 — 15Hz 로 묶어서 전송

```json
{ "kind": "events", "events": [ { "t": 12.345, "type": "gauge", "src": "player",
  "key": "Health", "norm": 0.72, "raw": 864.0 } ] }
```

| 필드 | 필수 | 설명 |
|---|---|---|
| `t` | ✔ | 전투 시작 기준 경과 초 (소수 3자리) |
| `type` | ✔ | `gauge` / `ability` / `tag` / `hit` / `marker` |
| `src` | ✔ | `player` / `boss` |
| `key` | ✔ | 어트리뷰트명 / 어빌리티 클래스명 / 태그 전체 경로 / 마커명 |
| `norm` | gauge만 | **0.0~1.0 정규화 값**. 차트는 이 값만 쓴다 |
| `raw` | 선택 | 절대값 (툴팁용) |
| `phase` | ability/tag | `begin` / `end` / `instant` |
| `meta` | hit만 | `attackTags` `sourceAbility` `direction` `postureDamage` `guardDamage` `wasBlocked` `wasParried` |

`gauge` 의 `key` 는 `Health` `Stamina` `Posture` `GuardGauge` `BurnGauge` 5종만.
`marker` 의 `key` 는 `GuardBroken` `PostureBroken` `ParrySuccess` `ParryFail`
`BlockSuccess` `CriticalAttack` `Phase2Enter` `Death` `BossDeath` 만.

### 3.3 런 로그

`Saved/RunLogs/run-{5자리}.json`. 전체 형태는 `schema.ts` 의 `RunLog` 를 볼 것.
이 도구의 핵심은 `bossFights[].timingSamples` 세 배열이다.

| 필드 | 의미 | 부호 |
|---|---|---|
| `parryInputOffsetMs` | 패링 판정 윈도우 **시작** 대비 입력 시각 | 음수 = 일찍 누름 |
| `hitAfterIframeEndMs` | 무적 **종료** 대비 피격 시각 | 양수 = 무적 끝난 뒤 |
| `comboWindowInputOffsetMs` | 콤보 윈도우 **시작** 대비 입력 시각 | 음수 = 일찍 누름 |

---

## 4. 화면

### `/timeline` — 전투 타임라인

하나의 시간축에 4개 레인이 세로로 쌓인다. canvas 에 직접 그린다(SVG 차트 라이브러리 금지 —
60초 × 15Hz × 5계열 = 4500포인트에서 버벅인다). 화면에 보이는 구간만 렌더하고,
픽셀당 표본이 넘치면 min/max 로 접는다.

| 레인 | 내용 |
|---|---|
| 1 게이지 | PLAYER / BOSS 두 구획이 **같은 0~100% 축**을 쓴다. `norm` 만 사용 |
| 2 어빌리티 | player 행 / boss 행 분리, begin~end 간트 막대 |
| 3 태그 | Invincible, Parry, SuperArmor, ComboWindow, PostureBroken … |
| 4 마커 | 피격·가드브레이크·체간붕괴·패링·사망 (수직선 + 글리프) |

조작:

| 입력 | 동작 |
|---|---|
| 드래그 | 스크럽 |
| ← / → | 1프레임(1/15초) 이동 |
| Shift + ← / → | 10프레임 이동 |
| Space / ▶ 버튼 | 라이브 ↔ 정지 |
| 휠 | 시간축 줌 (커서 위치 기준) |
| 상단 버튼 | 레인 표시/숨김 |
| 스냅샷 JSON 드롭 | `Saved/WebDebug/snapshots/death-*.json` 을 끌어다 놓으면 재생 |

### `/stats` — 런 통계

게임이 꺼져 있어도 동작한다.

1. **학습 곡선** — x=시도 횟수, y=`bossHealthPctAtEnd` + 최소제곱 추세선.
   내려가면 학습 중, 평평하면 패턴을 못 읽는 것.
2. **패링 입력 타이밍 분포** ★ — `parryInputOffsetMs` 히스토그램. 0.25s 윈도우가 배경 밴드.
   분포 중심이 음수 쪽이면 윈도우를 앞으로 옮겨야 한다.
3. **무적 종료 후 피격 분포** — `hitAfterIframeEndMs` 히스토그램. 0~50ms 에 몰리면 무적이 짧다.
4. **사망 원인 분포** — `damageTakenByAttackTag` 전체 합산 상위 8개 + `deathCause` 빈도.
5. **카드 픽률** — 제시 횟수(`offeredWith` 포함) 대비 선택 횟수. 픽률 0%는 없는 카드와 같다.

필터(보스 / 무기 / 시도 범위 / 결과)와 런 목록 테이블이 함께 있다. 행을 누르면 상세가 펼쳐진다.

---

## 5. 차트 설계 규칙 (협상 대상 아님)

`src/lib/palette.ts` 에 전부 코드로 박혀 있다.

1. **축은 하나만.** 게이지 5종은 전부 `norm`(0~1). 이중 축 차트는 만들지 않는다.
2. **계열 색은 고정 순서, 절대 돌려쓰지 않는다.** Health 는 언제나 같은 색이다.
   레인을 꺼도 남은 계열의 색이 바뀌지 않는다.
3. **다크는 별도 설계.** 라이트 팔레트를 자동 반전시키지 않았다.
4. **색상 접근성은 계산으로 검증한다.**

   ```bash
   npm run validate:palette
   ```

   색맹(적록/청황) 시뮬레이션에서 인접 계열 ΔE 와 일반시야 ΔE 를 계산한다.
   현재 값 — dark(surface `#17171b`) CVD 8.4 / 일반 19.8, light(`#fbfbf9`) CVD 9.1 / 일반 22.9. 둘 다 통과.
   **색이나 계열 순서를 바꾸면 반드시 다시 돌릴 것.** 순서가 곧 CVD 안전장치다.
5. **2계열 이상이면 범례를 항상 표시.** 마커는 색 외에 글리프를 함께 쓴다.
6. **마크는 얇게(선 2px), 격자·축은 배경으로.** 모든 점에 숫자를 찍지 않는다.
7. **텍스트는 텍스트 색을 쓴다.** 값·라벨을 계열 색으로 칠하지 않는다.

계열 색 (고정 순서):

| 슬롯 | 계열 | dark | light | 2차 인코딩(dash) |
|---|---|---|---|---|
| 1 | Health | `#e66767` | `#e34948` | 실선 |
| 2 | Guard | `#3987e5` | `#2a78d6` | 7-4 |
| 3 | Posture | `#c98500` | `#eda100` | 2-3 |
| 4 | Stamina | `#199e70` | `#1baf7a` | 11-4-2-4 |
| 5 | Burn | `#d95926` | `#eb6834` | 4-3-1-3 |

### React Bits 사용 제약

정적이거나 가끔 바뀌는 영역(네비게이션, 런 요약 카드, 통계 패널 진입 애니메이션)에만 쓴다.
**타임라인 canvas 내부·실시간 게이지·스크럽 중 갱신되는 요소에는 금지** —
15Hz 스트림에 spring 애니메이션이 겹치면 잔상과 프레임 드랍이 생긴다.

---

## 6. 확장 방법

### 새 이벤트 종류 추가

1. `src/lib/schema.ts` 의 `EventType` 에 추가
2. `Public/Debug/ACWebDebugTypes.h` 의 `EACWebDebugEventType` 에 같은 값 추가
3. `Private/Debug/ACWebDebugSubsystem.cpp` 의 `TypeToString` / `WriteEventJson` 에 직렬화 추가
4. `routes/Timeline.tsx` 의 `buildModel` 과 `draw` 에 렌더 추가

### 새 게이지 추가

`schema.ts` 의 `GAUGE_KEYS`, `palette.ts` 의 `GAUGE_SERIES`,
`ACWebDebugSubsystem.cpp` 의 `TrackedGauges`, `ACAttributeSet::PostAttributeChange` 의 분기 —
네 곳을 함께 고친다. **계열을 추가하면 `npm run validate:palette` 를 반드시 다시 돌린다.**

### 새 태그 구간 추가

`ACWebDebugSubsystem.cpp` 의 `GetTrackedTags()` 에 태그를 넣으면 끝이다.
구독·begin/end 짝맞춤·태그 레인 렌더는 전부 자동으로 따라온다.

### 새 통계 위젯 추가

`src/lib/stats.ts` 에 집계 함수를 만들고 `routes/RunStats.tsx` 에서 `<Panel>` 로 감싼다.
차트는 Recharts 를 쓰되 §5 규칙을 지킬 것 — 특히 단일 계열이면 `SERIES_1` 하나만 쓴다.

---

## 7. Shipping 빌드

`Ashen_Cathedral.Build.cs` 가 Shipping 에서 `AC_WEB_DEBUG=0` 을 정의하고
관련 모듈 의존성을 아예 추가하지 않는다. 모든 디버그 코드는 `#if AC_WEB_DEBUG` 로 감싸여 있으므로
Shipping 빌드에는 한 바이트도 들어가지 않는다.
