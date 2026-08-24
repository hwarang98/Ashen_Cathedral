# Unreal Error Doctor - Complete Reference

Complete workflow details, error patterns, and external links.

---

## Complete Workflow Details

### Step 1: Diagnosis

- Read build log
- Parse errors with error_parser.py
- Classify into Top 10 patterns
- Identify affected files

### Step 2: Analysis

- Show code context
- Explain Epic Games best practices
- Suggest prevention strategies

### Step 3: Preview

- Generate unified diff
- Show safety info (.backup)
- Display confidence score

### Step 4: Apply

- Create .backup file
- Apply fix using Edit tool
- Suggest next steps (rebuild)

---

## Top 5 Error Patterns

1. **Missing Semicolon** (48%) - 95% confidence
2. **.generated.h Position** (25%) - 90% confidence
3. **Missing Category** (15%) - 85% confidence
4. **EditAnywhere without Blueprint** (8%) - 85% confidence
5. **Blueprint Private Mismatch** (4%) - 80% confidence

---

## External Links

- [Epic C++ Coding Standard](https://dev.epicgames.com/documentation/en-us/unreal-engine/epic-cplusplus-coding-standard-for-unreal-engine)
- [UPROPERTY Documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/uproperty-specifiers)
- [Claude Agent Skills](https://docs.claude.com/en/docs/agents-and-tools/agent-skills/best-practices)
