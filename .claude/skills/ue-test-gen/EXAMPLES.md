# UE Test Generator — Examples

## Example 1: BDD Spec for Character (Default)

**Request**: "AMyCharacter 테스트 생성해줘"

**Step 1**: Analyze target
```python
ue_analyze_symbols(operation="get_methods", class_name="AMyCharacter")
# Returns: TakeDamage(float), GetHealth() -> float, Die()
```

**Step 2**: Generate test
```python
ue_generate_code(operation="generate_test", params={
    "test_target": "AMyCharacter",
    "module_name": "MyGame",
    "functions": ["TakeDamage", "GetHealth", "Die"],
    "output_dir": "Source/MyGame/Tests"
})
```

**Output**: `Source/MyGame/Tests/MyCharacterSpec.cpp`
```cpp
// Auto-generated UE BDD Spec Test (Issue #5294)
#include "Misc/AutomationTest.h"
#include "CoreMinimal.h"
#include "MyCharacter.h"

BEGIN_DEFINE_SPEC(FMyCharacterSpec, TEXT("MyGame.MyCharacter"),
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter |
    EAutomationTestFlags::SmokeFilter | EAutomationTestFlags::CriticalPriority)
END_DEFINE_SPEC(FMyCharacterSpec);

void FMyCharacterSpec::Define()
{
    Describe(TEXT("AMyCharacter::TakeDamage"), [this]()
    {
        It(TEXT("Should execute successfully"), [this]()
        {
            // Arrange, Act, Assert
        });
        It(TEXT("Should handle edge cases"), [this]()
        {
            // Edge case tests
        });
    });

    Describe(TEXT("AMyCharacter::GetHealth"), [this]()
    {
        // ...
    });

    Describe(TEXT("AMyCharacter::Die"), [this]()
    {
        // ...
    });
}
```

---

## Example 2: Simple Test for Single Function

**Request**: "Generate test for AMyCharacter::TakeDamage"

```python
ue_generate_code(operation="generate_test", params={
    "test_target": "AMyCharacter::TakeDamage",
    "test_type": "simple"
})
```

**Output**: `Source/Tests/MyCharacterTakeDamageTest.cpp`
```cpp
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMyCharacterTakeDamageTest,
    TEXT("Game.MyCharacter.TakeDamage"),
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter |
    EAutomationTestFlags::SmokeFilter | EAutomationTestFlags::CriticalPriority)

bool FMyCharacterTakeDamageTest::RunTest(const FString& Parameters)
{
    // Arrange
    // TODO: Create instance of AMyCharacter

    // Act
    // TODO: Call AMyCharacter::TakeDamage

    // Assert
    TestTrue(TEXT("AMyCharacter::TakeDamage behaves correctly"), true);

    return true;
}
```

---

## Example 3: Complex Latent Test for Async Systems

**Request**: "Create async test for AMyCharacter with TakeDamage and Heal"

```python
ue_generate_code(operation="generate_test", params={
    "test_target": "AMyCharacter",
    "test_type": "complex_latent",
    "functions": ["TakeDamage", "Heal"]
})
```

**Output**: Includes `DEFINE_LATENT_AUTOMATION_COMMAND`, `IMPLEMENT_COMPLEX_AUTOMATION_TEST`, and `ADD_LATENT_AUTOMATION_COMMAND`.

---

## Example 4: Smart Mode (Auto-Routing)

**Request**: Just provide `test_target` — smart mode auto-routes:

```python
ue_generate_code(operation="smart", test_target="AMyCharacter::TakeDamage")
# Auto-routes to generate_test, produces BDD spec by default
```

---

## Example 5: Full Workflow with Build Verification

**Request**: "TakeDamage 테스트 만들고 빌드해줘"

```python
# Step 1: Generate
result = ue_generate_code(operation="generate_test", params={
    "test_target": "AMyCharacter::TakeDamage",
    "test_type": "bdd_spec",
    "module_name": "LyraGame",
    "output_dir": "Source/LyraGame/Tests"
})

# Step 2: Write file (Claude writes cpp_code to cpp_path)

# Step 3: Build to verify compilation
ue_fix_errors(operation="build_and_fix", project_root="I:/Lyra/LyraStarterGame")

# Step 4: Run the test (requires Editor)
ue_editor_automation(operation="run_automation_tests",
    test_name="LyraGame.MyCharacter.TakeDamage")
```
