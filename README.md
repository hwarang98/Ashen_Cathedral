<p align="center">
  <img src="./Ashen_Cathedral.png" alt="Ashen Cathedral Icon" width="150" />
</p>

<h1 align="center">ASHEN CATHEDRAL</h1>

<p align="center">
  몰락한 대성당에서 적의 공세를 꺾고 자신만의 전투 빌드를 완성하는<br />
  <strong>다크 판타지 액션 로그라이크</strong>
</p>

<p align="center">
  Unreal Engine 5 · C++ · Gameplay Ability System · StateTree
</p>

![Ashen Cathedral](./AshenCathedral.png)

## 프로젝트 소개

**ASHEN CATHEDRAL**은 어둠에 잠식된 대성당을 무대로 진행되는 3인칭 액션 로그라이크 게임입니다.

플레이어는 공격을 무작정 이어 가는 대신 적의 움직임을 읽고, 회피·방어·패링·카운터를 활용해 전투의 주도권을 가져와야 합니다. 전투와 보스 처치를 통해 새로운 능력 카드를 획득하고, 매 런마다 서로 다른 조합의 빌드를 구성할 수 있습니다.

현재 프로젝트는 핵심 전투 시스템, 보스 AI, 보상 카드, 메타 성장 구조를 중심으로 개발 중입니다.

## 핵심 게임 루프

1. 대성당의 스테이지에 진입합니다.
2. 일반 적과 전투하며 자원과 진행 기회를 확보합니다.
3. 보스의 패턴과 페이즈 변화에 대응해 전투를 완료합니다.
4. 보상 카드 중 하나를 선택해 현재 런의 빌드를 강화합니다.
5. 획득한 메타 재화와 보스 클리어 기록을 저장합니다.
6. 다음 스테이지 또는 새로운 런에서 더 강한 전투에 도전합니다.

## 주요 시스템

### 전투 시스템

- 약공격·강공격과 콤보 연계
- 스태미나를 사용하는 질주 및 전투 행동
- 회피, 방어, 패링, 카운터 공격
- 타겟 락과 전투 대상 전환
- 가드 게이지, 가드 브레이크, 자세 붕괴
- 방향별 피격 반응과 그로기 상태
- 조건을 충족했을 때 실행되는 치명 공격
- 공격 무게, 방어 판정, 상태 태그를 반영한 데미지 계산

전투 로직은 Unreal Engine의 **Gameplay Ability System(GAS)**을 기반으로 구성되어 있습니다. 어빌리티, Gameplay Effect, Attribute, Gameplay Tag를 분리해 플레이어와 적이 공통 전투 규칙을 공유할 수 있도록 설계했습니다.

### 보스 AI

- Behavior Tree와 StateTree를 활용한 전투 상태 제어
- 거리와 상황에 따른 공격 패턴 선택
- 블록·패링·회피·카운터 대응
- 공격 예고와 패턴 실행 상태 동기화
- 체력 및 전투 상태에 따른 페이즈 전환
- Ashen Knight와 Ordan 보스 전투 로직

보스 AI는 Gameplay Tag와 Gameplay Event를 통해 GAS 전투 시스템과 연결됩니다. StateTree의 상태 전환과 어빌리티 실행을 분리하여 새로운 패턴과 페이즈를 확장할 수 있도록 구성했습니다.

### 보상 카드와 런 빌드

- 보스 처치 후 후보 카드 생성 및 선택 UI 표시
- 일반·고급·희귀·전설 등급별 가중치 적용
- 카드별 최대 중첩 수와 현재 스택 관리
- 공격, 방어, 기동, 패링, 자원·회복 계열 능력
- Gameplay Effect 및 Gameplay Ability를 통한 카드 효과 적용
- DataTable 기반 카드 데이터 관리

카드 선택은 현재 런에 직접 영향을 주며, 플레이어가 선택한 조합에 따라 전투 방식과 성장 방향이 달라집니다.

### 메타 성장과 저장

- 보스 최초 및 반복 클리어 보상 분리
- 메타 성장 재화 관리
- 보스 클리어 기록 저장
- 여러 저장 슬롯을 고려한 SaveGame 구조
- `GameInstanceSubsystem` 기반의 전역 진행 데이터 관리

### 연출과 사용자 경험

- Motion Warping을 활용한 근접 전투 보정
- Niagara 기반 전투 VFX 및 Gameplay Cue
- 보스 클리어, 보상 카드, 상호작용 UI
- 동적 하늘과 날씨 환경
- 키보드·마우스 및 게임패드 입력 지원

## 기술 스택

| 구분 | 사용 기술 |
| --- | --- |
| Engine | Unreal Engine 5.7 |
| Language | C++ / Blueprint |
| Ability | Gameplay Ability System, Gameplay Tags, Gameplay Tasks |
| AI | StateTree, Gameplay StateTree, Behavior Tree, EQS |
| Input | Enhanced Input, Raw Input |
| Animation | Motion Warping, Animation Warping, Pose Search |
| VFX | Niagara, Gameplay Cue |
| UI | UMG |
| Platform | Windows |

## 프로젝트 구조

```text
Ashen_Cathedral/
├── Config/                         # 엔진, 입력, Gameplay Tag 설정
├── Content/                        # Blueprint, 맵, 애니메이션, UI, 게임 에셋
├── Docs/                           # 전투 시스템과 AI 설계 문서
├── Source/Ashen_Cathedral/
│   ├── Public/
│   │   ├── AI/                     # Behavior Tree 및 StateTree 노드
│   │   ├── Character/              # 플레이어와 적 캐릭터
│   │   ├── Components/             # 전투, 입력, UI, 보상 카드 컴포넌트
│   │   ├── GameplayAbilitySystem/  # Ability, Effect, Attribute, Task
│   │   ├── GameplayTags/           # 전투 및 상태 Gameplay Tag
│   │   ├── SaveGame/               # 영속 진행 데이터
│   │   └── Subsystems/             # 메타 성장 서브시스템
│   └── Private/                    # 각 시스템의 C++ 구현
└── Ashen_Cathedral.uproject
```

## 실행 방법

### 요구 사항

- Windows 10/11
- Unreal Engine 5.7
- Visual Studio 2022
- Visual Studio의 **Game development with C++** 워크로드

프로젝트에서 사용하는 Marketplace 또는 외부 플러그인이 로컬에 설치되어 있어야 합니다. 특히 `IconCreator`, `ProjectCleaner` 플러그인이 없으면 프로젝트 생성 또는 에디터 실행 중 경고가 발생할 수 있습니다.

### 에디터 실행

1. 저장소를 내려받습니다.
2. `Ashen_Cathedral.uproject`를 우클릭합니다.
3. **Generate Visual Studio project files**를 실행합니다.
4. 생성된 `Ashen_Cathedral.sln`을 Visual Studio 2022에서 엽니다.
5. 빌드 구성을 `Development Editor`와 `Win64`로 설정합니다.
6. `Ashen_Cathedral` 타깃을 빌드합니다.
7. `Ashen_Cathedral.uproject`를 Unreal Editor로 실행합니다.

## 개발 문서

| 문서 | 내용 |
| --- | --- |
| [GAS 공격 흐름](./Docs/GAS_AttackFlow_Guide.md) | 공격 이벤트부터 데미지와 피격 처리까지의 전체 흐름 |
| [GAS 무기 데미지](./Docs/GAS_WeaponDamage_Guide.md) | 무기 데이터와 공격 배율을 이용한 데미지 계산 구조 |
| [GAS 스태미나](./Docs/GAS_Stamina_Setup_Guide.md) | 스태미나 소모, 회복, 회복 유예 설정 |
| [GAS 가드 브레이크](./Docs/GAS_GuardBreak_Guide.md) | 가드 게이지와 가드 붕괴 처리 |
| [GAS 적 방어·패링](./Docs/GAS_EnemyBlockParry_Guide.md) | 보스의 방어, 패링, 카운터 공격 연동 |
| [GAS 플레이어 콤보](./Docs/GAS_ComboReset_Guide.md) | 공유 콤보 단계, 피니셔 연계, 스페셜 전환, 리셋 경로 |
| [StateTree 런타임](./Docs/StateTree_Runtime_Guide.md) | UE 5.7 StateTree 실행 구조와 프로젝트 연동 |
| [StateTree 보스 설계](./Docs/StateTree_Boss_Design.md) | Ordan 보스의 상태 및 전투 패턴 설계 |

## 개발 현황

프로젝트는 현재 개발 중입니다. 핵심 전투의 완성도 향상, 보스 패턴 확장, 능력 카드 밸런싱, 메타 성장 콘텐츠 및 시각 효과 개선을 진행하고 있습니다.

> 게임 사양과 구현 내용은 개발 과정에서 변경될 수 있습니다.
