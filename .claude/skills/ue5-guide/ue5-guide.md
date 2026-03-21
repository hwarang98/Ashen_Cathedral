---
name: ue5-guide
description: >
  UE5 시스템 구현 가이드 문서를 작성할 때 적용.
  "가이드 작성", "문서 만들어줘", "설정 정리해줘" 요청 시 자동 활성화.
---

## 문서 작성 워크플로우

### 1. 작성 전 파악
- 구현된 관련 코드(.h / .cpp)를 먼저 읽고 실제 구조를 파악한다
- 프로젝트 지식(Project Knowledge)에 관련 내용이 있으면 반드시 참고한다
- 코드와 문서가 불일치하면 코드 기준으로 작성한다

### 2. 문서 구조 (아래 순서를 반드시 따른다)

```
# 시스템 이름

## 전체 구조
(Mermaid 흐름도 — 전체 동작을 한눈에)

---

## 구성 요소
(클래스 / GE / DataAsset 등 항목별 상세 설명)

---

## 관련 코드 위치
(파일명 → 함수명 테이블)
```

### 3. Mermaid 활용 기준

**flowchart TD** — 데이터 흐름, 처리 순서, 조건 분기가 있을 때
다음은 예시이다
~~~
flowchart TD
    A[행동 발생] --> B{스태미나 충분?}
    B -->|Yes| C[GE 적용]
    B -->|No| D[어빌리티 차단]
~~~

**flowchart LR** — 컴포넌트 간 의존 관계, 데이터 전달 방향
다음은 예시이다
~~~
flowchart LR
    A[DataAsset] --> B[ASC]
    B --> C[AttributeSet]
~~~

**sequenceDiagram** — 함수 호출 순서, 이벤트 타이밍이 중요할 때
다음은 예시이다
~~~
sequenceDiagram
    Ability->>ASC: ApplyGameplayEffect
    ASC->>AttributeSet: PostGameplayEffectExecute
    AttributeSet->>ASC: ApplyGE (RegenDelay)
~~~

Mermaid를 남용하지 않는다. 텍스트로 충분히 설명되는 내용은 표나 목록으로 작성한다.

### 4. 표 작성 기준
설정값, 수치, 옵션 목록은 반드시 표로 작성한다.
다음은 예시이다

```markdown
| 항목 | 값 |
|---|---|
| Duration Policy | `Infinite` |
| Period | `0.1` |
```

- 코드/태그/클래스명은 인라인 코드(`` ` ``)로 감싼다
- 권장값은 **굵게** 표시한다

### 5. 주의사항 표기
헷갈리기 쉬운 설정, 흔한 실수는 반드시 아래 형식으로 강조한다.
다음은 예시이다

```markdown
> ⚠️ `Application Tag Requirements`가 아닌 **`Ongoing Tag Requirements`** 를 사용한다.
```

### 6. 코드 예시
사용 방법이 직관적이지 않은 경우에만 최소한의 코드 스니펫을 추가한다.
언어는 반드시 명시한다 (` ```cpp `).

### 7. 작성 금지 사항
- 구현되지 않은 기능을 추측해서 작성하지 않는다
- "향후 추가 예정", "TODO" 같은 미완성 내용을 문서에 포함하지 않는다
- 코드에 없는 클래스명/함수명을 임의로 만들어 쓰지 않는다
