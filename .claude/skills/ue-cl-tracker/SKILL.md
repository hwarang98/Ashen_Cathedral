---
name: ue-cl-tracker
description: "Epic Games UnrealEngine GitHub에서 엔진 소스 코드의 변경 이력을 조회한다. 4가지 모드: 파일 이력, CL 상세, 키워드 검색, 크래시 원인 추적. Triggers: 'CL 조회', 'CL 번호', '엔진 코드 변경 이력', '이 코드 누가 바꿨어', 'when was X added', 'which CL', 'is this fixed', 'version history', 'regression', 'backed out', '어느 버전에서 추가', '수정 이력', '언제 고쳐졌나', 'fixed in', 'backport', 'Perforce history', '엔진 변경사항'."
argument-hint: "[파일명/CL번호/키워드/크래시함수명]"
user_invocable: true
---

# /ue-cl-tracker — Epic UnrealEngine 소스 변경 이력 조회 (GitHub API)

Epic Games `EpicGames/UnrealEngine` GitHub 리포에서 소스 코드 변경 이력,
CL 상세, 키워드 검색, 크래시 원인 추적을 수행한다.

> **GitHub API 기반** — P4 Licensee 계정 불필요. `gh` CLI만 있으면 동작.
> **READ-ONLY**: GitHub API 읽기만 사용. push/PR/issue 생성 없음.

---

## Phase 0: 모드 판별

사용자 요청에서 실행 모드를 자동 판별:

| 모드 | 트리거 예시 | 실행 Phase |
|------|------------|-----------|
| **CL 상세** | "CL 41293144 보여줘", "이 CL 뭐 바꿨어" | → 2A |
| **파일 이력** | "SlateRHIRenderer.cpp 변경 이력", "이 파일 언제 바뀌었어" | → 2B |
| **키워드 검색** | "FlushPendingDeletes 관련 변경사항", "ErrorLevel BuildGraph" | → 2C |
| **크래시 추적** | "FSlateRHIRenderer::FlushPendingDeletes 크래시 원인", "regression in 5.5" | → 2D |

---

## Phase 1: GitHub 접속 확인

```bash
gh auth status 2>&1 | head -3
```

실패 시: `gh auth login` 안내. EpicGames/UnrealEngine 접근에는 GitHub 계정이
UnrealEngine 접근 권한이 있어야 함 (https://www.unrealengine.com/ue-on-github).

---

## Phase 2A: CL 상세 조회

사용자가 CL 번호를 제공한 경우, 커밋 메시지에서 해당 CL을 검색:

```bash
# CL 번호로 직접 검색 (커밋 메시지에 [CL XXXXXXXX by ...] 패턴)
gh api search/commits --method GET \
  -f "q=XXXXXXXX repo:EpicGames/UnrealEngine" \
  --jq '.items[:3] | .[] | {sha: .sha[:12], date: .commit.author.date, message: .commit.message[:400]}'
```

찾으면 상세 조회:

```bash
# 영향받은 파일 + 전체 커밋 메시지
gh api repos/EpicGames/UnrealEngine/commits/<SHA> \
  --jq '{
    message: .commit.message,
    date: .commit.author.date,
    author: .commit.author.name,
    files: [.files[] | {filename, status, additions, deletions}]
  }'
```

**출력 형식:**

```
=== CL XXXXXXXX 상세 ===

📋 제목: Backed out "Always defer slate widget rendering..."
👤 작성자: zach bethel
📅 날짜: 2025-04-02
🔀 SHA: 44d845a50061

📝 설명:
  - The widget renderer lifetimes are not tracked well
  - use-after-free
  - Certain assets deadlock due to draw buffer reference

📁 영향받은 파일 (N개):
  M  Engine/Source/Runtime/SlateRHIRenderer/Private/SlateRHIRenderer.cpp (+32/-39)
  M  Engine/Source/Runtime/UMG/Private/Slate/SRetainerWidget.cpp (+10/-2)

📌 UE 버전 추정: ~5.7 (2025-04 커밋)
```

**ROBOMERGE 추적**: 커밋 메시지에서 `#ROBOMERGE-SOURCE: CL {N}` 패턴을 파싱.
원본 CL이 있으면 추가 검색하여 표시.

---

## Phase 2B: 파일 변경 이력

특정 파일의 최근 커밋 이력 조회:

```bash
# 파일 경로로 커밋 이력 (path 필터)
gh api "repos/EpicGames/UnrealEngine/commits?path=Engine/Source/Runtime/SlateRHIRenderer/Private/SlateRHIRenderer.cpp&per_page=10" \
  --jq '.[] | {sha: .sha[:12], date: .commit.author.date, message: .commit.message[:200]}'
```

파일 경로를 모르면 먼저 검색:

```bash
# 파일명으로 검색 (GitHub code search — 제한적)
gh api search/commits --method GET \
  -f "q=filename:SlateRHIRenderer.cpp repo:EpicGames/UnrealEngine" \
  --jq '.items[:5] | .[] | {sha: .sha[:12], date: .commit.author.date, message: .commit.message[:200]}'
```

**출력 형식:**

```
=== 파일 변경 이력 ===
📄 Engine/Source/Runtime/SlateRHIRenderer/Private/SlateRHIRenderer.cpp

| # | SHA | 날짜 | 작성자 | 설명 |
|---|-----|------|--------|------|
| 1 | 44d845a50061 | 2025-04-02 | zach bethel | Backed out "Always defer..." |
| 2 | fcf816f62769 | 2025-06-03 | yohan | Fix random crash in SlateRHIRenderer... |

💡 "CL 41293144 상세 보여줘" — 특정 CL 상세 조회
```

---

## Phase 2C: 키워드/함수 변경 검색

함수명, 클래스명, CVar명 등 키워드로 관련 커밋 검색:

**반드시 복수 쿼리 전략 사용** — 하나의 쿼리로는 찾을 수 없는 경우가 많음:

```bash
# Query 1: 키워드 직접 검색
gh api search/commits --method GET \
  -f "q=<keyword> repo:EpicGames/UnrealEngine" \
  --jq '.items[:5] | .[] | {sha: .sha[:12], date: .commit.author.date, message: .commit.message[:300]}'

# Query 2: "backed out" + 관련 영역 (크래시 fix는 revert인 경우가 많음!)
gh api search/commits --method GET \
  -f "q=backed out <feature area> repo:EpicGames/UnrealEngine" \
  --jq '.items[:5] | .[] | {sha: .sha[:12], date: .commit.author.date, message: .commit.message[:300]}'

# Query 3: "fix" + 키워드
gh api search/commits --method GET \
  -f "q=fix <keyword> repo:EpicGames/UnrealEngine" \
  --jq '.items[:5] | .[] | {sha: .sha[:12], date: .commit.author.date, message: .commit.message[:300]}'

# Query 4: Jira 이슈 번호 (UE-XXXXXX)
gh api search/commits --method GET \
  -f "q=UE-<number> repo:EpicGames/UnrealEngine" \
  --jq '.items[:5] | .[] | {sha: .sha[:12], date: .commit.author.date, message: .commit.message[:300]}'
```

> **CRITICAL**: GitHub commit search는 커밋 **메시지** 만 검색함.
> 코드 내용은 검색 안 됨. 코드를 찾으려면 Phase 2E (파일 직접 읽기) 사용.

---

## Phase 2D: 크래시 원인 추적

크래시/regression 질문일 때의 체계적 추적 절차:

### Step 1: 크래시 함수/클래스 파싱

질문에서 추출:
- 크래시 함수명: `FSlateRHIRenderer::FlushPendingDeletes`
- 관련 컴포넌트: `UWidgetComponent`
- 영향 버전: `5.5.4`
- 증상: `world teardown`, `regression`

### Step 2: ★ 파일 이력으로 CL 찾기 (가장 확실한 방법)

> **파일 이력 > 키워드 검색**. 키워드 검색은 커밋 메시지 wording에 의존하여 불안정.
> 파일 이력은 해당 파일을 수정한 모든 커밋을 반환하므로 확실.

```bash
# ★ PRIMARY METHOD: 파일 이력 조회
# Step 2a: ue_grep 결과에서 크래시 파일 경로를 얻음 (예: SlateRHIRenderer.cpp)
# Step 2b: 해당 파일의 커밋 이력 가져오기
gh api "repos/EpicGames/UnrealEngine/commits?path=Engine/Source/Runtime/<module>/Private/<file>.cpp&per_page=20" \
  --jq '.[] | {sha: .sha[:12], date: .commit.author.date, message: .commit.message[:200]}'

# Step 2c: 결과에서 "Backed out" 또는 "fix" 커밋 찾기
# → "Backed out" 커밋 = regression fix (revert)
# → 바로 다음 커밋 = 원인 커밋 (regression 도입)
```

**예시**: `SlateRHIRenderer.cpp` 이력 20개 조회 →
15번째: "Backed out Always defer slate widget rendering" (CL 41293144, fix)
16번째: "Always defer slate widget rendering" (원인 커밋)

### Step 2-ALT: 키워드 검색 (파일 경로를 모를 때)

```bash
# Fallback 1: 크래시 함수명
gh api search/commits --method GET \
  -f "q=<CrashFunction> repo:EpicGames/UnrealEngine" \
  --jq '.items[:5] | .[] | {sha: .sha[:12], date: .commit.author.date, message: .commit.message[:300]}'

# Fallback 2: "backed out" + 짧은 키워드 (2단어)
gh api search/commits --method GET \
  -f "q=backed out <SHORT 2-word keyword> repo:EpicGames/UnrealEngine" \
  --jq '.items[:5] | .[] | {sha: .sha[:12], date: .commit.author.date, message: .commit.message[:300]}'
```

### Step 3: 유망한 커밋 상세 조회

```bash
gh api repos/EpicGames/UnrealEngine/commits/<SHA> \
  --jq '{
    message: .commit.message[:600],
    date: .commit.author.date,
    files: [.files[:10][] | {filename, additions, deletions}]
  }'
```

### Step 4: 관련 소스 파일 현재 상태 확인

```bash
gh api repos/EpicGames/UnrealEngine/contents/<file_path> \
  --jq '.content' | base64 -d | head -100
```

**출력 형식:**

```
=== 크래시 원인 추적 ===
🔍 크래시: FSlateRHIRenderer::FlushPendingDeletes (UE 5.5.4)
🔍 트리거: world-space UWidgetComponent, level unload

--- 관련 커밋 ---
1. [CL 41293144] sha 44d845a50061 (2025-04-02)
   "Backed out Always defer slate widget rendering..."
   → use-after-free, widget renderer lifetime 문제
   → Files: SlateRHIRenderer.cpp, SRetainerWidget.cpp

2. [CL 43181194] sha fcf816f62769 (2025-06-03)
   "Fix random crash in SlateRHIRenderer..."
   → Jira UE-289558, main/render thread race condition

--- 분석 ---
CL 41293144가 문제의 원인이 된 변경을 revert한 것으로,
이 CL이 적용된 버전(5.7+)에서는 해당 크래시가 해소됨.
5.5.4에는 이 revert가 백포트되지 않았을 가능성 높음.
```

---

## Phase 2E: 소스 파일 직접 읽기

BuildGraph XML, C# 파일 등 특정 파라미터를 확인할 때:

```bash
# 파일 내용 읽기 (main 브랜치)
gh api repos/EpicGames/UnrealEngine/contents/<file_path> \
  --jq '.content' | base64 -d | head -100
```

**자주 조회하는 파일:**
- BuildGraph 태스크: `Engine/Source/Programs/AutomationTool/BuildGraph/Tasks/*.cs`
- Horde 로그 파서: `Engine/Source/Programs/Horde/HordeAgent/Parser/*.cs`
- 빌드 설정: `Engine/Source/Programs/UnrealBuildTool/*.cs`

---

## UE 버전 추정

커밋 날짜로 UE 버전을 추정:

| 커밋 날짜 | 추정 UE 버전 |
|-----------|-------------|
| ~2024-01 ~ 2024-06 | UE 5.4 |
| ~2024-07 ~ 2024-12 | UE 5.5 |
| ~2025-01 ~ 2025-06 | UE 5.6 / 5.7 |
| ~2025-07 ~ 2025-12 | UE 5.7 / 5.8 |
| ~2026-01+ | UE 5.8+ (main) |

> ⚠️ 근사치. 정확한 버전은 릴리즈 노트 교차 확인 필요.

---

## 검색 전략 가이드 (★ 핵심)

GitHub commit search는 **커밋 메시지만** 검색하므로, 쿼리 전략이 결정적:

| 찾고 싶은 것 | 좋은 쿼리 | 나쁜 쿼리 (결과 없음) |
|-------------|----------|---------------------|
| 크래시 fix (revert) | `backed out widget rendering` | `backed out FlushPendingDeletes` ❌ |
| 크래시 fix (revert) | `backed out <���능 이름>` | `backed out <크래시 함수명>` ❌ |
| 특정 CL | `41293144` (숫자만) | `CL#41293144` ❌ |
| Jira 이슈 | `UE-289558` | `jira 289558` ❌ |
| 파라미터 추가 | `ErrorLevel` | `added parameter` ❌ |
| 특정 파일 변경 | `filename:SlateRHIRenderer.cpp` | 경로 전체 ❌ |

**★ 가장 중요한 3가지 규칙**:

1. **"backed out" 검색은 기능 이름 사용, ��래시 함수명 사용 금지!**
   - 크래시: `FlushPendingDeletes` → backed out 메시지: "widget rendering" (O)
   - `backed out FlushPendingDeletes` → 0건 (X)
   - `backed out widget rendering` → **CL#41293144 찾음** (O)
   - 이유: revert 커밋은 "원래 기능의 설명"을 포함, 크래시 함수명은 안 포함

2. CL 번호는 **숫자만** 으로 검색 (커밋 메시지에 `[CL 41293144 by ...]` 형태로 포함)

3. 하나의 쿼리로 안 찾아지면 **반드시 3-4개 변형 쿼리** 시도
   - 쿼리 변형: 크래시 함수 → 기능 이름 → 모듈 이름 → 클래스 이��

---

## 스킬 체인 연동

### From /ue-debug (Step 5):
```
엔진 버그/regression 의심 시:
→ Skill('ue-cl-tracker') + 크래시 함수명
→ CL 번호, fix 날짜, UE 버전 정보 획득
→ Debug Report에 포함
```

### From /ue-explain (Step 5):
```
최근 추가/변경된 기능 설명 시:
→ Skill('ue-cl-tracker') + 기능/클래스명
→ "Added in UE 5.x (CL#...)" 정보 포함
```

### From domain sub-agents:
```
서브에이전트가 "이건 regression/crash 문제"로 판단 시:
→ Bash로 gh api 직접 호출 (이 스킬의 Phase 2D 전략 사용)
→ CL 정보를 출력 최상단에 배치
```

---

## 에러 처리

| 상황 | 대응 |
|------|------|
| `gh auth` 실패 | `gh auth login` 안내 |
| 검색 결과 0건 | 쿼리 변형 시도 (prefix 제거, "backed out" 추가, 키워드 축소) |
| rate limit | 30초 대기 후 재시도, 쿼리 수 줄이기 |
| 파일 contents 404 | 경로 확인 — main 브랜치에 없을 수 있음 (삭제/이동) |
| base64 디코딩 실패 | 파일이 너무 크면 `--jq '.size'` 로 크기 확인 후 부분 읽기 |

---

## Evaluation Criteria

**Positive (should activate)**:
1. "CL 41293144 보여줘" → Phase 2A
2. "SlateRHIRenderer.cpp 변경 이력" → Phase 2B
3. "FlushPendingDeletes 관련 변경사항 찾아줘" → Phase 2C
4. "FSlateRHIRenderer::FlushPendingDeletes 크래시 원인, UE 5.5.4 regression" → Phase 2D
5. "ErrorLevel 파라미터 어디서 정의돼?" → Phase 2E
6. "이 버그가 5.7에서 고쳐졌나?" → Phase 2D

**Negative (should NOT activate)**:
1. "ACharacter 어떻게 동작해?" → /ue-explain
2. "빌드 에러 해결해줘" → /ue-debug
3. "Epic AI 도구 변경 추적" → /epic-toolset-tracker
