---
name: 🗺 프로젝트 TODO 트래커
about: 전체 작업 현황을 한 Issue에서 관리합니다
title: "[TRACKER] Ashen Cathedral 작업 현황"
labels: tracker
assignees: ''
---

## 🔲 TODO

### P1 — 전투 루프 뼈대
- [ ] `AttributeSet` 구현 (HP / 스태미나 / 그로기 게이지)
- [ ] 데미지 `GameplayEffect` — 무기 스탯 DataTable 연동
- [ ] 히트 리액션 어빌리티 구현
- [ ] 사망 처리 어빌리티 + 리스폰 흐름

### P2 — 게임 루프 완성
- [ ] 보스 AI — Behavior Tree 기반 Ashen Knight 패턴
- [ ] `Enemy` 캐릭터 클래스 + `EnemyCombatComponent`
- [ ] 런 기반 능력 선택 시스템 (보스 처치 후 3선택)
- [ ] 패링(Parry) 시스템
- [ ] 타겟 록온 시스템 완성

### P3 — 완성도
- [ ] 플레이어 HUD (HP바 / 스태미나바 / 스킬 쿨다운)
- [ ] 메타 진행 저장 시스템 (Ash Souls 재화)
- [ ] 히트스톱 + 피격 VFX
- [ ] 회피 무적 프레임 (i-frame) 시스템

---

## 🔄 In Progress
<!-- 현재 작업 중인 항목을 여기로 이동 -->

---

## ✅ Done
<!-- 완료된 항목을 여기로 이동 -->
- [x] GAS 인프라 (ASC / InputComponent / DataAsset 구조)
- [x] 무기 장착 / 해제 어빌리티
- [x] 무기 충돌 판정 (WeaponBase 히트박스)
- [x] 스프린트 / 점프 / 이동 입력
- [x] Niagara 파티클 프리로딩
