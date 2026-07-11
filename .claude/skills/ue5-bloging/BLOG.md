---
적용: 항상
---

---
name: write-velog-system-post
description: Draft and save Korean Velog-style Markdown posts about specific Unreal Engine, GAS, gameplay, AI, combat, camera, or engineering systems. Use when the user asks to write, create, or polish a blog post in snow2271's technical post style, especially when they provide a system name, implementation notes, code, screenshots, bug notes, or a reference post and want a .md file.
---

# Write Velog System Post

## Overview

Create a Korean technical blog post as a Markdown file, following the user's Velog article rhythm: result first, why it was needed, discarded design, final architecture, concrete flow, code snippets, bug notes, final summary, and reflection.

## Workflow

1. Gather the minimum concrete inputs:
    - System/topic name and optional English subtitle.
    - Project context and the problem that motivated the system.
    - Final behavior/result, preferably with screenshots or GIF paths.
    - Main classes, GameplayTags, Blackboard keys, Behavior Tree nodes, Ability names, data assets, or subsystems involved.
    - Failed or discarded approaches and why they were abandoned.
    - Bugs encountered, with situation, cause, and fix.
    - Code snippets or local source files to inspect.

2. If the user points to local code, inspect it before writing. Use `rg` to find class names, tags, functions, and assets. Do not invent class names, tag names, or function behavior.

3. If key details are missing, ask at most three short questions. If enough context exists, draft with explicit `TODO:` placeholders for missing screenshots, exact class names, or unverified code.

4. Save the output as a `.md` file in the current workspace unless the user specifies a path. Prefer `YYYY-MM-DD-{english-slug}.md` or `{system-slug}.md` when no date convention exists.

5. After writing, quickly review the Markdown for section completeness, code fence validity, image placeholders, and unsupported claims.

## Article Shape

Use this structure by default. Rename sections naturally when the system calls for it.

````markdown
# Unreal Engine 5 + GAS로 {Korean system name} 만들기 — {English subtitle}

> Unreal Engine 5 + {core technologies}로 {system behavior}를 구현한 과정과 설계하면서 겪었던 문제들을 정리한다.

## 결과부터

#### {짧은 결과 설명}

![결과 GIF](./images/{slug}-result.gif)

#### {보조 화면: BT, 디버그, 로그, 에디터 등}

![보조 이미지](./images/{slug}-bt.png)

* * *

## 배경 — 왜 만들었나

## 1차 설계 — {초기 접근} (폐기)

## 최종 설계 — {핵심 설계 키워드}

### 핵심 아이디어

### 전체 흐름

```text
{actor/event}
  ↓
{component/function}
  → {filter or state update}
  → {tag/event/blackboard update}
  ↓
{BT / Ability / subsystem}
```

### {중요 구현 포인트}

```cpp
// Only include code that came from the user, the local project, or a clearly labeled draft.
```

* * *

## 에러 모음

### 1. {증상}

상황: {what happened}

원인: {why it happened}

해결: {what changed}

* * *

## 최종 구조 요약

| 대상 | 역할 |
| --- | --- |
| `{ClassOrTag}` | {responsibility} |

* * *

## 되돌아보며

- {lesson}
- {tradeoff}
- {future checklist}
````

## Style Rules

- Write in Korean in a first-person engineering diary tone. Make it feel like the developer is explaining decisions after actually building the system.
- Prefer concrete responsibility boundaries over generic praise. Explain what each component knows, what it deliberately does not know, and why that split matters.
- Lead with the playable or observable result before deep implementation details.
- Include a rejected design when possible. The rejection reason should be architectural, such as duplicated responsibility, unclear lifecycle, stale state, hidden coupling, or impossible ownership.
- Preserve tradeoffs instead of pretending the final design is perfect.
- Use exact identifiers in backticks: `GameplayTag`, class names, function names, Blackboard keys, Ability names, montage names, or config values.
- Use `* * *` separators between major story beats.
- Use `상황:`, `원인:`, `해결:` in the error section.
- Keep explanations dense but readable. Avoid long beginner tutorials for Unreal, GAS, C++, or Behavior Tree basics unless the user asks for them.
- Do not copy large passages from an existing post. Reuse the structure and cadence, not the wording.

## Accuracy Rules

- Never invent implementation facts. If a detail is unknown, write `TODO:` or ask the user.
- If writing from local source, verify names and call flow from the files.
- If code is a proposed draft rather than existing code, label it clearly in the surrounding text.
- Do not add fake images. Use real paths supplied by the user or placeholders under `./images/`.
- Do not claim a bug existed unless the user provided it, it appears in logs/issues, or it is clearly marked as a draft example.

## Markdown Output Checklist

Before finishing, confirm the `.md` file has:

- A title matching `{technology}로 {system} 만들기 — {subtitle}` or a natural variant.
- An intro blockquote.
- `## 결과부터`.
- `## 배경 — 왜 만들었나`.
- At least one design/architecture section.
- A concrete flow diagram or text arrow flow.
- Code snippets or explicit `TODO:` placeholders.
- `## 에러 모음`, unless the user explicitly has no bug notes.
- `## 최종 구조 요약`.
- `## 되돌아보며`.
