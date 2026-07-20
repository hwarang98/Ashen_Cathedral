# UE5의 TScriptInterface, 왜 필요하고 뭘 조심해야 하나

> 검증 환경: Unreal Engine 5.7
> 인용한 엔진 코드: `Engine/Source/Runtime/CoreUObject/Public/UObject/ScriptInterface.h`

상호작용(Interaction) 시스템을 만들다 보면 반드시 마주치는 타입이 있습니다.

```cpp
TScriptInterface<IInteractableInterface> CurrentInteractable;
```

처음 보면 "그냥 `IInteractableInterface*` 쓰면 안 되나?" 싶습니다.
안 됩니다. 왜 안 되는지, 그리고 이 타입이 숨기고 있는 함정 두 개를 정리합니다.

---

## 1. UE 인터페이스는 클래스가 두 개다

언리얼에서 인터페이스를 만들면 헤더에 클래스가 **두 개** 선언됩니다.

```cpp
// InteractableInterface.h

UINTERFACE(MinimalAPI)
class UInteractableInterface : public UInterface   // ① 리플렉션용 껍데기
{
    GENERATED_BODY()
};

class ASHEN_CATHEDRAL_API IInteractableInterface   // ② 실제 함수가 사는 곳
{
    GENERATED_BODY()

public:
    virtual void Interact(APawn* InstigatorPawn) = 0;
    virtual FText GetInteractionText() const { return FText::GetEmpty(); }
};
```

| | 역할 | UObject인가? |
|---|---|---|
| `UInteractableInterface` | UObject 시스템이 "이런 인터페이스가 존재한다"고 인식하기 위한 메타데이터. 함수는 없음 | **O** |
| `IInteractableInterface` | 가상 함수가 실제로 선언된 순수 C++ 클래스 | **X** |

우리가 상속하고 호출하는 건 전부 `I~` 쪽입니다. 그리고 이게 **UObject가 아니라는 점**이 모든 문제의 출발점입니다.

---

## 2. 같은 객체인데 주소가 두 개다

인터페이스를 구현하는 액터는 이렇게 생겼습니다.

```cpp
class AACStageExitPoint : public AActor, public IInteractableInterface
```

다중 상속입니다. 메모리 배치를 보면:

```
AACStageExitPoint 객체 하나
├─ [AActor 서브오브젝트]                ← AActor* 가 가리키는 주소
└─ [IInteractableInterface 서브오브젝트]  ← IInteractableInterface* 가 가리키는 주소
                                            (같은 객체지만 오프셋만큼 다름)
```

**같은 객체를 가리키는데 포인터 값이 서로 다릅니다.** 이걸 기억해두면 뒤에 나오는 API 설계가 전부 납득됩니다.

---

## 3. 그래서 생 포인터로는 못 든다

`IInteractableInterface*`를 그냥 멤버로 들면 세 가지가 깨집니다.

**① GC가 추적하지 못한다**
UObject 포인터가 아니니 가비지 컬렉터 입장에선 정체불명의 주소입니다. 대상 액터가 파괴돼도 널로 만들어주지 않아 댕글링 포인터가 됩니다.

**② `UPROPERTY()`를 못 붙인다**
리플렉션 시스템이 모르는 타입이라 프로퍼티로 등록 자체가 안 됩니다.

**③ 블루프린트에 노출할 수 없다**
직렬화 경로가 없습니다.

---

## 4. TScriptInterface의 실체

해결책은 단순합니다. **두 포인터를 한 쌍으로 같이 들고 다니면 됩니다.**

실제 엔진 코드입니다.

```cpp
// ScriptInterface.h:21
class FScriptInterface
{
private:
    /** A pointer to a UObject that implements an interface. */
    TObjectPtr<UObject> ObjectPointer = nullptr;

    /** For native interfaces, pointer to the location of the interface object
        within the UObject referenced by ObjectPointer. */
    void* InterfacePointer = nullptr;
```

`TScriptInterface<T>`는 이 `FScriptInterface`를 상속해서 타입만 입혀준 템플릿입니다.

```cpp
// ScriptInterface.h:137
template <typename InInterfaceType>
class TScriptInterface : public FScriptInterface
```

핵심은 `ObjectPointer`가 **`TObjectPtr<UObject>`** 라는 점입니다. UObject 포인터니까 `UPROPERTY()`만 붙으면 GC가 추적할 수 있고, 리플렉션에도 등록되고, BP에도 노출됩니다. 3장에서 깨졌던 세 가지가 전부 해결됩니다.

### GC와의 협력 방식

`GetInterface()` 구현에 재미있는 방어 코드가 있습니다.

```cpp
// ScriptInterface.h:78
UE_FORCEINLINE_HINT void* GetInterface() const
{
    // Only access the InterfacePointer if we have a valid ObjectPointer. This is necessary
    // because garbage collection may only clear the ObjectPointer.
    return ObjectPointer ? InterfacePointer : nullptr;
}
```

GC는 자기가 아는 `ObjectPointer`만 널로 정리합니다. `InterfacePointer`는 `void*`라 손대지 못하고 그대로 남습니다. 그래서 **`ObjectPointer`가 살아있을 때만 `InterfacePointer`를 반환**하도록 막아둔 겁니다. 댕글링을 구조적으로 차단하는 영리한 설계입니다.

---

## 5. 사용법

프로젝트 실제 코드로 보겠습니다.

### 대입 — 그냥 액터 포인터를 넘기면 된다

```cpp
// ACStageExitPoint.cpp
PlayerCharacter->SetCurrentInteractable(this);   // this는 AACStageExitPoint*
```

암시적 변환 생성자가 두 포인터를 알아서 채웁니다.

```cpp
// ScriptInterface.h:161
template <typename U UE_REQUIRES(std::is_convertible_v<U, UObjectType*>)>
inline TScriptInterface(U&& Source)
{
    UObjectType* SourceObject = ImplicitConv<UObjectType*>(Source);
    SetObject(SourceObject);       // ← ObjectPointer 채움

    if constexpr (std::is_base_of<InInterfaceType, /*...*/>::value)
    {
        SetInterface(Source);      // ← InterfacePointer 채움 (컴파일 타임에 확정)
    }
    else
    {
        InInterfaceType* SourceInterface = Cast<InInterfaceType>(SourceObject);
        SetInterface(SourceInterface);
    }
}
```

### 호출과 검사 — `->` 와 `.GetObject()` 는 다른 걸 준다

```cpp
// ACPlayerCharacter.cpp
void AACPlayerCharacter::Input_Interact()
{
    if (CurrentInteractable.GetObject())        // ① UObject* 를 꺼내 유효성 검사
    {
        CurrentInteractable->Interact(this);    // ② I* 로 함수 호출
    }
}
```

| 표현식 | 반환 | 정의 위치 |
|---|---|---|
| `.GetObject()` | `UObject*` (액터 쪽) | `ScriptInterface.h:351` |
| `operator->` | `InterfaceType*` (인터페이스 쪽) | `ScriptInterface.h:317` |
| `operator*` | `InterfaceType&` | `ScriptInterface.h:327` |

**`.GetObject()`와 `->`가 서로 다른 주소를 반환한다**는 걸 놓치면 헷갈립니다. 2장의 메모리 배치를 떠올리면 됩니다.

### 비교

```cpp
// ACPlayerCharacter.cpp — 떠나는 액터가 현재 등록된 대상일 때만 해제
void AACPlayerCharacter::ClearCurrentInteractable(TScriptInterface<IInteractableInterface> InInteractable)
{
    if (CurrentInteractable == InInteractable)
    {
        CurrentInteractable = nullptr;
    }
}
```

`operator==`는 **두 포인터를 모두** 비교합니다.

```cpp
// ScriptInterface.h:108
bool operator==(const FScriptInterface& Other) const
{
    return GetInterface() == Other.GetInterface() && ObjectPointer == Other.GetObject();
}
```

---

## 6. 함정 ①: UPROPERTY를 빼먹으면 의미가 없다

`TScriptInterface`를 쓰는 **이유 자체가** `ObjectPointer`를 GC에 추적시키는 것인데, `UPROPERTY()`가 없으면 리플렉션에 등록이 안 돼서 GC가 이 참조를 여전히 모릅니다.

```cpp
// ✗ 타입만 맞고 GC 추적은 안 됨
TScriptInterface<IInteractableInterface> CurrentInteractable;

// ○
UPROPERTY()
TScriptInterface<IInteractableInterface> CurrentInteractable;
```

GC가 참조를 수집하는 경로는 이겁니다.

```cpp
// ScriptInterface.h:117
void AddReferencedObjects(FReferenceCollector& Collector)
{
    Collector.AddReferencedObject(ObjectPointer);
}
```

이 함수가 호출되려면 프로퍼티로 등록돼 있어야 합니다. `UPROPERTY()` 한 줄이 빠지면 "GC 추적 가능한 그릇에 담아뒀지만 GC에게 알려주진 않은" 상태가 됩니다. 타입만 보고 안심하기 쉬운 부분입니다.

---

## 7. 함정 ②: 블루프린트로 구현하면 `->`가 nullptr이다

이게 훨씬 무섭습니다. 엔진 주석이 아예 대놓고 경고합니다.

```cpp
// ScriptInterface.h:15
/**
 * For objects natively implementing an interface, ObjectPointer and InterfacePointer
 * point to different locations in the same UObject.
 * For objects that only implement an interface in blueprint, only ObjectPointer will be set
 * because there is no native representation.
 * UClass::ImplementsInterface can be used along with Execute_ event wrappers to properly
 * handle BP-implemented interfaces.
 */
```

**BP에서 인터페이스를 구현하면 네이티브 표현이 존재하지 않으므로 `InterfacePointer`가 널입니다.** 변환 생성자에도 같은 내용이 주석으로 붙어 있습니다.

```cpp
// ScriptInterface.h:175
// Tries to set the native interface instance, this will set it to null for BP-implemented interfaces
InInterfaceType* SourceInterface = Cast<InInterfaceType>(SourceObject);
```

결과적으로 이런 코드가 크래시합니다.

```cpp
// BP로만 인터페이스를 구현한 액터가 들어오면
if (CurrentInteractable.GetObject())      // ← 통과한다 (ObjectPointer는 유효)
{
    CurrentInteractable->Interact(this);  // ← 💥 operator-> 가 nullptr
}
```

`TScriptInterface` 클래스 주석도 못을 박습니다.

> This type is only useful with **native interfaces**, `UClass::ImplementsInterface` should be used to check for blueprint interfaces. — `ScriptInterface.h:135`

### 대응

**C++ 전용 인터페이스라면** 지금 구조 그대로 괜찮습니다. 순수 가상 함수(`= 0`)로 선언해두면 BP에서 구현하는 것 자체가 불가능하므로 이 함정에 빠질 일이 없습니다.

**BP에서도 구현하게 하려면** 인터페이스를 `UFUNCTION(BlueprintNativeEvent)`로 바꾸고 호출부를 `Execute_` 래퍼로 교체해야 합니다.

```cpp
// 인터페이스 선언
UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
void Interact(APawn* InstigatorPawn);

// 호출부 — I* 를 거치지 않고 UObject* 로 디스패치하므로 BP 구현도 안전하게 잡힌다
if (UObject* Target = CurrentInteractable.GetObject())
{
    if (Target->GetClass()->ImplementsInterface(UInteractableInterface::StaticClass()))
    {
        IInteractableInterface::Execute_Interact(Target, this);
    }
}
```

`Execute_` 래퍼는 `UObject*`를 받아 리플렉션으로 함수를 찾아 호출하기 때문에, C++ 구현이든 BP 구현이든 동일하게 동작합니다.

---

## 8. 함정 ③: `operator bool`은 "네이티브 구현 여부"다

7장의 연장선입니다. `if (MyScriptInterface)`가 직관과 다르게 동작합니다.

```cpp
// ScriptInterface.h:372
/**
 * Boolean operator, returns true if this object natively implements InterfaceType.
 * This will return false for objects that only implement the interface in blueprint classes.
 */
explicit operator bool() const
{
    return GetInterface() != nullptr;
}
```

즉 `if (MyScriptInterface)`는 **"대상이 있는가"가 아니라 "대상이 C++로 인터페이스를 구현했는가"** 를 묻습니다. BP로만 구현했다면 대상이 멀쩡히 살아있어도 `false`입니다.

| 검사 방법 | 의미 |
|---|---|
| `if (Interface)` | 네이티브 구현인가 (BP 구현이면 false) |
| `if (Interface.GetObject())` | 대상 객체가 존재하는가 |
| `Obj->GetClass()->ImplementsInterface(...)` | 구현 방식과 무관하게 인터페이스를 구현했는가 |

---

## 정리

- 언리얼 인터페이스는 리플렉션용 `U~`와 실제 함수가 있는 `I~` **두 클래스**로 나뉜다.
- `I~`는 UObject가 아니라서 생 포인터로 들면 GC 추적·`UPROPERTY`·BP 노출이 전부 막힌다.
- `TScriptInterface<T>`는 `TObjectPtr<UObject>`와 `void*`를 **쌍으로** 들고 다니며 이 문제를 해결한다.
- `.GetObject()`는 액터를, `operator->`는 인터페이스를 반환한다. **서로 다른 주소다.**
- **`UPROPERTY()`를 반드시 붙일 것.** 없으면 GC가 여전히 이 참조를 모른다.
- **BP로 구현한 인터페이스는 `operator->`가 nullptr이다.** BP 구현을 허용할 거라면 `BlueprintNativeEvent` + `Execute_` 래퍼로 가야 한다.
