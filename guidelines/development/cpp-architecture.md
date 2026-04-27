# Guidelines for Complex C++ Architecture Work

**Related Documentation:**
- [unit-testing.md](unit-testing.md) - Unit testing patterns and best practices
- [voice-setter-system.md](voice-setter-system.md) - Example of comprehensive feature documentation

---

## Research & Understanding

### 1. Understand Deeply Before Acting
Before proposing solutions, thoroughly investigate existing code and documentation. This means:

- **Anticipate related mechanisms** - If discussing read locks, proactively address write locks. If discussing one code path, consider parallel paths (like parameter setters alongside process methods)
- **Focus on the architectural "why"** - Don't just understand what code does - understand why it exists and how it interacts with other systems
- **Question whether existing patterns are correct** - Or just legacy that needs revisiting

### 2. Delegate Research Effectively
For architectural questions spanning multiple files or requiring deep analysis, delegate to explore subagents. When delegating:

1. **Include key class/pattern names** - Name the specific abstractions the research must consider, even if they seem obvious from prior conversation
2. **State what's already established** - Summarize relevant findings from earlier in the session
3. **Point to relevant documentation** - Reference files the subagent should read first
4. **Define scope relative to known systems** - Frame the question in terms of how it relates to established architecture

**Example:** When asking a subagent to analyze parameter handling, don't just say "analyze setParameter". Instead: "Analyze how setParameter interacts with the PolyData voice iteration system. Key classes: `ScopedVoiceSetter` (sets voice index on audio thread), `ScopedAllVoiceSetter` (UI thread protection). Read `voice-setter-system.md` first."

### 3. Verify Understanding Before Proceeding
When corrected, re-read relevant documentation/code before continuing. Explicitly state the corrected understanding and verify it's accurate before proposing next steps.

### 4. Identify and Clarify Naming Confusions Early
Similar names, parallel naming patterns, or related-sounding concepts cause confusion and lead to incorrect analysis. When encountering potentially confusing names:

1. **Watch for similar names with different purposes** - `UIUpdater` vs `MessageThreadUpdater`, `ScopedVoiceSetter` vs `ScopedAllVoiceSetter` may sound related but serve different roles
2. **Watch for same names in different scopes** - `Outer::Foo` vs `Inner::Foo` may be completely different concepts
3. **Clarify deprecated aliases** - Old names kept for compatibility can mislead analysis
4. **Document distinctions explicitly** - When confusing similarities exist, create a comparison table showing the differences
5. **Consider renaming** - If naming causes persistent confusion, propose clearer names

**The test:** Can you clearly explain how two similar-sounding classes differ in one sentence each?

---

## Solution Design

### 5. Don't Over-Engineer Solutions
Start with minimal, targeted fixes. Avoid proposing elaborate patterns (like atomic flags, deferred updates) when simpler solutions using existing infrastructure suffice.

### 6. Extend Existing Patterns Rather Than Inventing New Ones
When solving a new problem, first look for existing patterns in the codebase that solve similar problems. Prefer extending those patterns (adding a parameter, reusing an enum) over creating new abstractions. This results in:
- Consistent API across related methods
- Less cognitive load for users of the API
- Fewer new concepts to document and maintain
- Code that "rhymes" - if you know how `get()` works, you know how `all()` works

### 7. Separate Logic Verification from Implementation
For complex changes, separate phases: (1) understand the problem, (2) verify the design, (3) implement. Don't rush to code before the logic is solid.

---

## API Design

### 8. Design APIs That Make Wrong Usage Visible
When a code path has performance (or correctness) implications that could silently degrade the system:

1. **Make the safe path the default** - require explicit opt-out for dangerous/slow paths
2. **Use enums, not bools** - `get(true)` is meaningless; named methods like `all()` document intent
3. **Add debug-mode assertions** - catch misuse during development, not in production
4. **Name things by their intent** - `AllowUncached` explains *why* you're opting out

**The test:** Can a developer reading the call site understand the implications without looking up documentation?

---

## Threading & Performance

### 9. Be Precise About Thread Contexts
For audio/real-time code, always be explicit about which thread calls each method and what the lock-free requirements are. Document this in tables and comments.

### 10. Trace Performance Implications Across Call Boundaries
Code that is correct in isolation may defeat optimizations established by callers. Consider the full call chain:

- Who calls this code and how often?
- What optimizations has the caller established?
- Does this code honor or bypass those optimizations?

**The test:** If a caller sets up caching/pooling/batching, does this code use it or work around it?

---

## Code Quality

### 11. Verify Changes End-to-End
After making changes, review them critically. Explain why each change was made. Watch for code smells that indicate a wrong approach:
- Unnecessary casts (especially `const_cast`)
- Working around the API rather than using it directly
- Changes that don't match the patterns used elsewhere in the codebase

**The test:** Does this change use the API as intended, or does it feel like a workaround?

### 12. Explain "Why" in Comments When Patterns Deviate
When code deviates from the expected pattern, add a comment explaining:
1. **Why** - the reason this deviation is necessary
2. **When** - the contexts where this applies
3. **Safety** - why this is safe despite being unusual

**The test:** Can someone reading this code in 6 months understand why it's different without asking the original author?

---

## Project Management

### 13. Document Deferred Work Thoroughly
When deferring work, create detailed TODO documentation capturing: problem statement, current state, proposed solution, files to modify, and dependencies. This enables clean handoff to future sessions.

### 14. Keep Documentation in Sync with Evolving Understanding
As understanding deepens during a session, previously written documentation may become outdated or incomplete. When significant new insights emerge:

1. **Identify affected documentation** - What docs were written before this insight?
2. **Update proactively** - Don't wait to be asked; propose documentation updates
3. **Capture the full picture** - New sections should reference and integrate with existing content

---

## Critical Thinking

### 15. Critically Evaluate Proposals Before Planning Implementation
Don't assume user suggestions are optimal. Before planning implementation:

1. **Ask "What problem does this actually solve?"** - Verify the diagnosis before accepting the prescription
2. **Look for contradictions with existing design** - Does this proposal conflict with patterns already in the codebase?
3. **Challenge scope** - Is this change necessary, or is there a simpler path?
4. **Voice concerns explicitly** - Say "I'm not sure this is right because..." rather than silently proceeding

**Anti-pattern:** Immediately responding "Yes, I'll implement that" to every suggestion. The goal is correct solutions, not agreement.
