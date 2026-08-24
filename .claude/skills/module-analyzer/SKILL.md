---
name: module-analyzer
model: opus
description: "Analyze UE module structure (Build.cs, headers, deps) to generate OVERVIEW.md and CLASSES.md. Triggers on '모듈 분석', 'module analysis'."
compatibility: "Requires NarshaMCP MCP server (v0.7.0+) with Unreal Engine 5.x C++ project"
allowed-tools: [Bash, Read, Grep, Glob, Write]
metadata:
  version: "1.0.0"
  author: "Next-Stage-Inc"
  license: "MIT"
---

# Module Analyzer Skill

UE 모듈 구조 분석 전문가. Build.cs, 헤더 파일, 의존성을 분석하여 OVERVIEW.md와 CLASSES.md를 생성합니다.

## Triggers

다음 키워드에 자동 활성화:
- "모듈 분석", "module analysis"
- "OVERVIEW 생성", "generate overview"
- "아키텍처 파악", "architecture analysis"
- "클래스 목록", "class list"

## 경로 안내

출력 경로는 환경에 따라 다릅니다:
- **개발 환경** (저장소): `docs/engine/ue{version}/modules/`
- **릴리즈 패키지** (플러그인 설치 후): `MCP/docs/engine/ue{version}/modules/`

## Inputs

| Parameter | Type | Required | Description |
|-----------|------|----------|-------------|
| module_path | string | Yes | 모듈 소스 경로 (예: `Engine/Source/Runtime/Core`) |
| engine_version | string | Yes | 엔진 버전 (`5.6` 또는 `5.7`) |
| engine_root | string | Yes | 엔진 루트 경로 |
| output_dir | string | No | 출력 디렉토리 (기본: 개발 환경 `docs/engine/ue{version}/modules/`, 릴리즈 `MCP/docs/engine/ue{version}/modules/`) |

## Outputs

| File | Maturity | Description |
|------|----------|-------------|
| OVERVIEW.md | core | 모듈 개요, 목적, 아키텍처 다이어그램 |
| CLASSES.md | stable | 주요 클래스 목록, 소스 라인 참조 |
| dependency_graph.json | internal | 모듈 의존성 그래프 (시각화용) |

## Workflow

```text
1. scan_module.py 실행
   - Build.cs 파싱 (의존성 추출)
   - 디렉토리 구조 스캔
   - 모듈 메타데이터 수집

2. extract_classes.py 실행
   - 헤더 파일 분석
   - UCLASS/USTRUCT/UENUM 추출
   - 라인 번호 참조 생성

3. 템플릿 적용
   - OVERVIEW_TEMPLATE.md 렌더링
   - CLASSES_TEMPLATE.md 렌더링

4. Ground Truth 검증
   - 소스 파일 존재 확인
   - 라인 번호 검증
   - 신뢰도 점수 계산
```

## Scripts

### scan_module.py

모듈 메타데이터 추출:

```bash
python "${CLAUDE_SKILL_DIR}/scripts/scan_module.py" \
  --module-path "Engine/Source/Runtime/Core" \
  --engine-root "C:/UE_5.7" \
  --output module_metadata.json
```

### extract_classes.py

주요 클래스 추출:

```bash
python "${CLAUDE_SKILL_DIR}/scripts/extract_classes.py" \
  --module-path "Engine/Source/Runtime/Core" \
  --engine-root "C:/UE_5.7" \
  --output classes.json
```

## Example Usage

```python
# Claude Code에서 호출
/skill module-analyzer --module-path "Engine/Source/Runtime/Core" --engine-version "5.7"

# 결과
docs/engine/ue5_6/modules/Core/Core/
  ├── OVERVIEW.md     # 모듈 개요
  ├── CLASSES.md      # 클래스 목록
  └── metadata.json   # 분석 메타데이터
```

## Quality Standards

- **Ground Truth 참조**: 모든 클래스는 소스 파일:라인 번호 포함
- **신뢰도 점수**: 최소 85% (직접 소스 참조)
- **의존성 정확도**: Build.cs 기반 100% 정확
- **Mermaid 다이어그램**: 아키텍처 시각화 필수

## ⚠️ Common Failures

| 에러 유형 | 원인 | 해결 방법 |
|-----------|------|-----------|
| Build.cs 파싱 실패 | 비표준 Build.cs 구조 또는 C# 문법 오류 | Build.cs 파일 수동 확인, 표준 UBT 패턴인지 검증 |
| 헤더 파일 스캔 0건 | `module_path` 오류 또는 Public/Private 폴더 미존재 | 정확한 모듈 소스 경로 확인 (Engine/Source/Runtime/Core 형식) |
| UCLASS 추출 누락 | 전처리기 매크로로 감싸진 선언 | `#if` 블록 내 선언은 수동 확인 필요, `--include-conditional` 옵션 고려 |
| 라인 번호 검증 실패 | 엔진 업데이트 후 소스 변경 | `--engine-version` 정확히 지정, Ground Truth 재생성 |
| 의존성 그래프 순환 참조 | 모듈 간 상호 의존성 존재 | 순환 감지 로직 확인, 그래프에서 약한 의존성(Optional) 분리 |
| Mermaid 다이어그램 렌더링 오류 | 긴 모듈 이름 또는 특수문자 포함 | 모듈 이름 축약, 특수문자 이스케이프 처리 |

---

## Output Format

```
=== Module Analysis: {ModuleName} ===
Engine: UE {version}
Path:   {module_path}

OVERVIEW.md: Generated (L1-L{N} lines)
CLASSES.md:  Generated ({class_count} classes)
dependency_graph.json: Generated ({dep_count} dependencies)

Confidence: {score}% (ground truth verified)
Mermaid:    {diagram_status}

Summary: {class_count} classes, {dep_count} deps, {file_count} headers scanned
```

## Error Recovery

| Error | Cause | Recovery |
|-------|-------|----------|
| `Build.cs parse failed` | Non-standard C# syntax or missing file | Verify module path exists and contains a valid `Build.cs`; fall back to directory-only scan |
| `0 headers scanned` | Incorrect `module_path` or empty Public/Private dirs | Re-check path format (`Engine/Source/Runtime/{Module}`); use `--include-conditional` for guarded declarations |
| `Line number verification failed` | Engine source changed after last analysis | Re-run with correct `--engine-version`; regenerate ground truth data |

---

## Related Skills

- `api-diff-analyzer`: 버전 간 API 변경점 분석
- `example-extractor`: 사용 예제 추출
- `troubleshoot-generator`: 트러블슈팅 문서 생성
## MCP Tool Examples

```python
# Analyze module symbols and dependencies
ue_analyze_symbols(operation="smart", query="UMyModule")
```
