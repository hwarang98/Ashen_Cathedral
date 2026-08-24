---
name: unreal-error-doctor
description: "Purpose: UE compilation error resolution. 4-step medical workflow (Diagnosis → Analysis → Preview → Apply) with multi-tool escalation for complex errors (symbol + config analysis). Handles 95%+ common errors AND complex cross-domain issues. Triggers: '빌드 에러', 'compile error', 'linker error', 'symbol not found', 'config mismatch', 'UPROPERTY error'."
---

# Unreal Error Doctor

**Version**: 1.0.0 (Phase 1 MVP)
**Priority**: Highest
**Goal**: 95%+ error resolution accuracy with 75-80% time savings

---

## 🎯 Purpose

Diagnoses and fixes Unreal Engine C++ compilation errors using a **medical diagnosis workflow**:

1. **Diagnosis** - Identify error type and root cause
2. **Analysis** - Explain why it happened and impact
3. **Preview** - Show diff of proposed fix
4. **Apply** - Execute fix with .backup safety

---

## 🔍 Trigger Phrases

### Korean

- "컴파일 에러", "빌드 에러", "빌드 실패"
- "링커 에러", "헤더 에러"
- "UPROPERTY 에러", "UCLASS 에러"
- "세미콜론", "generated.h"

### English

- "compile error", "build error", "build failed"
- "linker error", "header error"
- "UPROPERTY error", "UCLASS error"
- "semicolon", "generated.h"

---

## 🏥 Workflow Overview

### 4-Step Medical Diagnosis Process

```text
Step 1: Diagnosis (진단)
↓ Identify error pattern from build log
↓ Tools: ue_fix_errors(mode="smart")  ⭐ NEW: Smart mode with AI-friendly guidance

Step 2: Analysis (분석)
↓ Explain root cause + Epic best practices
↓ Show prevention strategies
↓ Use next_actions field for AI guidance  ⭐ NEW: Issue #755 Phase 1

Step 3: Preview (미리보기)
↓ Generate unified diff
↓ Show safety info (.backup)
↓ Filter false positives (engine code, generated code)  ⭐ NEW: Issue #755 Phase 3

Step 4: Apply (적용)
↓ Execute fix with backup
↓ Suggest next steps (rebuild)
```

**Key Features**:

- 95%+ accuracy on Top 5 error patterns
- .backup files for safety
- Epic Games best practices integration
- Bilingual Korean/English support
- **NEW (v5.2.0 - Issue #755)**: Smart mode with auto-routing and false positive suppression

---

## 🚀 Quick Example

**User**: "빌드 에러 났는데 도와줘"

**Error Doctor**:

```text
🏥 Diagnosis: Missing Semicolon
File: MyPlayerController.h:42
Confidence: 95%

📊 Analysis:
UCLASS declarations require semicolon after closing brace.
Epic Best Practice: ✅ GENERATED_BODY() present

🔍 Preview:
-}
+};

✅ Fix Applied!
Backup: MyPlayerController.h.backup
Next: Rebuild project
```

---

## ⚠️ Common Failures

| 에러 유형 | 원인 | 해결 방법 |
|-----------|------|-----------|
| `ue_fix_errors` 호출 시 project_root 미설정 | `.env`에 `UECODEGEN_PROJECT_DIR` 누락 | `.env` 파일에 프로젝트 경로 설정 후 MCP 서버 재시작 |
| false positive: 엔진 코드에 대한 수정 제안 | Engine/Source 경로 파일을 사용자 코드로 오인 | Issue #755 Phase 3 필터링 확인, 엔진 코드 경로 제외 설정 |
| `.backup` 파일 복원 실패 | backup 파일이 다른 fix에 의해 덮어쓰기됨 | `git checkout -- <file>` 사용하여 git에서 복원 |
| 빌드 로그 파싱 시 에러 패턴 미인식 | Top 5 패턴 외의 비표준 UE 에러 메시지 | 전체 빌드 로그를 수동으로 확인, 패턴 DB 확장 요청 |
| smart mode 자동 라우팅 실패 (wrong mode 선택) | 에러 메시지가 모호하여 분류 불가 | `mode="auto"` 대신 `mode="preflight"` 등 명시적 모드 지정 |
| 복수 파일 에러에서 일부만 수정됨 | 연쇄 에러에서 첫 번째만 처리 후 중단 | 첫 수정 후 리빌드, 남은 에러에 대해 재실행 |

---

## 📚 Related Files

For detailed information, see:

- **EXAMPLES.md** - Complete usage examples and scenarios
- **ADVANCED.md** - Tool integration, success criteria, version history
- **REFERENCE.md** - Complete workflow details, error patterns, external links

---

## Output Format

```
=== Error Doctor Report ===

Diagnosis: Missing Semicolon after UCLASS declaration
File: Source/MyGame/MyPlayerController.h:42
Confidence: 95%

Analysis:
  UCLASS declarations require semicolon after closing brace.
  Epic Best Practice: GENERATED_BODY() present

Preview:
  - }
  + };

Fix Applied: YES
Backup: MyPlayerController.h.backup
Next Step: Rebuild project (Ctrl+Shift+B)
```

## Error Recovery

| Error | Cause | Recovery |
|-------|-------|----------|
| `ue_fix_errors` returns no suggestions | Error pattern not in Top 5 database or non-standard UE error message | Provide full build log manually; use `mode="preflight"` or `mode="scan_build_log"` for broader detection |
| False positive: fix suggested for Engine code | Engine/Source path file misidentified as user code | Verify the file path is under project `Source/`; engine code should never be modified |
| `.backup` file overwritten by subsequent fix | Multiple fix cycles ran on the same file | Use `git checkout -- <file>` to restore from version control instead of `.backup` |

**Status**: Phase 1 MVP Ready for Testing
**Last Updated**: 2025-10-20
