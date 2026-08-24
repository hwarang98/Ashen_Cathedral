# Unreal Error Doctor - Usage Examples

Complete examples and common scenarios for using the Error Doctor skill.

---

## Example 1: Auto-fix with Explanation

**User**: "빌드 에러 났는데 도와줘"

**Agent**:

```text
🏥 Unreal Error Doctor

Step 1: Diagnosis
Found: Missing Semicolon
File: MyPlayerController.h:42
Confidence: 95%

Step 2: Analysis
Why This Happened:
UCLASS declarations require semicolon after closing brace.

Epic Best Practices:
✅ GENERATED_BODY() present
❌ Missing semicolon (current error)

Step 3: Preview
-}
+};

Safety: Backup will be created

Step 4: Apply
✅ Error fixed! Rebuild your project.
```

---

## Example 2: Educational Mode

**User**: "Why does 'EditAnywhere' cause a warning?"

**Agent**:

```text
📚 Educational Explanation

Epic Best Practice:
EditAnywhere should be paired with BlueprintReadOnly.

For Components: MUST use BlueprintReadOnly

Example:
❌ Bad:
UPROPERTY(EditAnywhere)
UStaticMeshComponent* MeshComponent;

✅ Good:
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
UStaticMeshComponent* MeshComponent;

Want me to fix this for you? [Yes/No]
```

---

## Common Scenarios

### Scenario 1: Missing Semicolon on UCLASS

- **Error**: `expected ';' after class definition`
- **Fix Time**: <1 second
- **Confidence**: 95%

### Scenario 2: .generated.h Not Last Include

- **Error**: Various compilation errors
- **Fix**: Move `.generated.h` to last include
- **Confidence**: 90%

### Scenario 3: UPROPERTY Missing Category

- **Warning**: `EditAnywhere property without Category`
- **Fix**: Add `Category="Gameplay"`
- **Confidence**: 85%
