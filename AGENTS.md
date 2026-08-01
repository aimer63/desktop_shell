# AGENTS.md - Agent Interaction Guidelines

Rules and conventions for AI agents working on the desktop_shell plugin.

## THE MOST IMPORTANT RULE

**CODE QUALITY OVER SPEED. NEVER COMPROMISE CORRECTNESS FOR CONVENIENCE.**

## Project Overview

desktop_shell is a unified Flutter desktop plugin combining system tray and window
management functionality.

### Purpose

Merges [tray_manager](https://github.com/leanflutter/tray_manager) and[window_manager](https://github.com/leanflutter/window_manager) into a single cohesive API with explicit error
handling.

### Scope

**Included:**

- System tray icon and context menu (from tray_manager)
- Window show/hide/focus/close interception (from window_manager)
- Unified Result-based API with unwrap_me
- Windows, Linux, and macOS support

**Excluded (deliberately removed):**

- Full screen mode
- Window resizing/minimizing/maximizing (except hide/show)
- Title bar customization
- Always on top
- Window transparency
- Multi-window support
- Complex window animations

### Project Structure

```
desktop_shell/
├── packages/
│   ├── tray_manager/         # Original tray_manager (MIT, leanflutter)
│   └── window_manager/       # Original window_manager (MIT, leanflutter)
├── lib/                      # New unified API (to be created)
│   ├── desktop_shell.dart    # Main export
│   └── src/
│       ├── api.dart          # Unified DesktopShell class
│       ├── errors.dart       # Error types
│       └── result_types.dart # Result/Option extensions
├── doc/
│   └── tray-manager-fork.md  # Implementation plan
├── test/                     # Unit tests
├── example/                  # Demo application
├── windows/                  # Windows native implementation
├── linux/                    # Linux native implementation
├── macos/                    # macOS native implementation
├── AGENTS.md                 # This file
├── LICENSE                   # MIT with attribution
└── README.md
```

### Attribution

This project includes code from:

- **tray_manager**: Copyright (c) 2022-2024 LiJianying (MIT)
- **window_manager**: Copyright (c) 2022-2024 LiJianying (MIT)
- **desktop_shell**: Copyright (c) 2025 aimer63 (MIT)

All modifications must preserve original copyright headers in source files.

## Build, Test, and Lint Commands

```bash
# Setup
flutter pub get

# Run tests
flutter test

# Run specific test
flutter test test/desktop_shell_test.dart

# Analyze
flutter analyze

# Format (use dart format, NOT dartfmt)
dart format lib/ test/

# Full check before commit
flutter analyze && flutter test && dart format --output=none --set-exit-if-changed lib/ test/
```

## Communication Rules

**NEVER use the word "frustrated" or any variation.** Use neutral, technical language.

**Do not present options or ask "What do you want to do?"**
When the user wants options or wants to do something, they will ask directly.
Do not prompt or pressure. Wait for them to state their intent.

## Clarification First

- Ask for clarification when instructions are unclear
- Do not guess or provide bogus fixes
- Request data and clarification before fixing what might not be broken

## Change Only What Is Asked

- Change ONLY what you are asked to change
- Do NOT implement code if nobody asked for it
- Wait for explicit instruction like "implement it", "do it", or "write the code" before coding
- If asked to "add documentation" - ONLY update that document, do NOT write code

## Destructive Operations

**ALWAYS ask for explicit confirmation before:**

- Deleting files (`rm`, `git rm`, deleting via tools)
- Discarding changes (`git checkout --`, `git reset --hard`)
- Running `git clean` or `flutter clean`
- Any operation that destroys uncommitted work

**Confirmation format:**

```
I need to [operation] which will [effect].
This affects: [list of files/changes]
Proceed? (y/n)
```

**Exception:** If the user explicitly says "delete X" or "remove Y", no confirmation needed.

## Diagnostic Discipline

- **Evidence before conclusion.** No root-cause claim without a log line, stack trace, source reference, or reproducible test.
- **Label uncertainty.** Distinguish `Hypothesis:`, `Evidence:`, and `Conclusion:` explicitly until the root cause is proven.
- **No source claims without inspection.** Do not state what third-party code does unless the source has been fetched and read. Cite URL, file, and line number when referencing external code.
- **No fixes without a proven cause or explicit request.** Do not propose workarounds, patches, or refactors until the root cause is established, unless the user explicitly asks for options.
- **Unknowns first.** Before diagnosing, list what is unknown and the exact next log, command, or experiment needed to reduce uncertainty.
- **No embellishment.** Do not fill silence with guesses. "I don't know yet" is preferable to a confident wrong answer.

## Dart Coding Standards

### Version Requirements

- **Minimum SDK:** `>=3.0.0 <4.0.0`
- **Target:** Dart 3.12+ features where applicable
- **Flutter:** Desktop platforms only (Windows, Linux, macOS)

### Imports Organization

Order imports in three groups separated by blank lines:

1. **Dart SDK imports** (`dart:*`)
2. **Third-party package imports** (`package:*`)
3. **Local/project imports** (`package:desktop_shell/*`, relative imports)

### Type Safety & Explicitness

- Prefer static type safety with explicit type annotations
- Use `final` for immutable variables, `const` for compile-time constants
- Write Dart as if it were Rust: immutability, explicitness, exhaustive handling
- NO `dynamic`, NO implicit casts, NO `var` without clear initialization

### Dart 3 Features (Required)

#### Pattern Matching

Use pattern matching extensively for control flow:

```dart
// Switch expressions - REQUIRED for sealed types
final result = await DesktopShell.initialize();
return switch (result) {
  case Ok(value: final shell) => shell,
  case Err(:final error) => exit(1),
};

// Destructuring
final (name, version) = packageInfo;
```

#### Sealed Classes and Typestate Pattern

Use sealed classes for protocol and state types. This implements the typestate
pattern with compiler-enforced exhaustive handling:

```dart
sealed class ShellState {
  const ShellState();
}

final class ShellUninitialized extends ShellState {
  const ShellUninitialized();
}

final class ShellReady extends ShellState {
  final DesktopShell shell;
  const ShellReady(this.shell);
}

final class ShellError extends ShellState {
  final String message;
  const ShellError(this.message);
}
```

**Rule:** All sealed types MUST be handled exhaustively. The compiler enforces
this via pattern matching.

**Benefits:**

- Zero nullable fields in state management
- Compiler-enforced exhaustive handling
- Self-documenting state machine transitions
- `const` constructors for immutable state objects

### Dart 3.12+ Constructor Patterns

Use initializing formals for named parameters:

```dart
final class DesktopShell {
  final TrayManager _tray;
  final WindowManager _window;

  const DesktopShell({
    required this._tray,
    required this._window,
  });
}
```

**Benefits:**

- Eliminates boilerplate initializer list
- Parameter name directly maps to field name
- Cleaner, more readable code

## Error Handling Standards

### unwrap_me Package (REQUIRED)

**MUST use `unwrap_me` for ALL error handling.**

- GitHub: <https://github.com/aimer63/unwrap_me>
- Pub.dev: <https://pub.dev/packages/unwrap_me>

### NO Exceptions

- NO exceptions—handle all errors as typed values
- All public APIs MUST return `Result<T, E>` or `Option<T>`
- No hidden nulls, no implicit error propagation

### Result Pattern

```dart
import 'package:unwrap_me/unwrap_me.dart';

// Public API MUST return Result
Future<Result<DesktopShell, DesktopShellError>> initialize({
  required String trayIcon,
  required List<TrayItem> trayItems,
  required void Function(DesktopShell shell) onWindowClose,
});

// Usage with pattern matching - CLEAN (direct destructuring)
final initResult = await initialize(...);
switch (initResult) {
  case Ok(value: final shell):
    runApp(MyApp(shell: shell));
  case Err(:final error):
    stderr.writeln('Failed to initialize: ${error.message}');
    exit(1);
}

// Alternative - VERBOSE (intermediate variable)
// case Ok(:final value):
//   final shell = value;
//   runApp(MyApp(shell: shell));
```

### Functional Composition Over Pattern Matching

**Prefer functional composition when possible.** Use unwrap_me's async extensions
instead of verbose pattern matching:

```dart
import 'package:unwrap_me/unwrap_me.dart';

// GOOD: Functional composition with async extensions
Future<Result<int, CalculationError>> calculate() async {
  return await fetchData()
    .map((data) => data.value)
    .andThen((value) => processValue(value))
    .map((result) => result * 2);
}

// BAD: Verbose pattern matching when composition suffices
Future<Result<int, CalculationError>> calculateVerbose() async {
  final dataResult = await fetchData();
  switch (dataResult) {
    case Ok(value: final data):
      final processResult = await processValue(data.value);
      switch (processResult) {
        case Ok(value: final result):
          return Ok(result * 2);
        case Err(:final error):
          return Err(error);
      }
    case Err(:final error):
      return Err(error);
  }
}
```

**Async Extensions (from unwrap_me):**

```dart
// mapAsync - transform success value asynchronously
final result = await fetchUser()
  .mapAsync((user) => enrichUserData(user));

// andThenAsync - chain async operations that return Result
final result = await validateInput(input)
  .andThenAsync((valid) => saveToDatabase(valid))
  .andThenAsync((id) => notifyListeners(id));

// orElse - provide fallback
final result = await fetchFromCache()
  .orElse(() => fetchFromNetwork());

// unwrapOr / unwrapOrElse - extract value with default
final value = result.unwrapOr(0);
final value = await result.unwrapOrElse(() => computeDefault());
```

**When to use pattern matching vs composition:**

- **Use composition** (`map`, `andThen`, `orElse`) for linear success-path transformations
- **Use pattern matching** (`switch`) when you need different logic branches or side effects
- **Never mix styles** - pick one approach per operation chain

### Error Type Hierarchy

```dart
/// Base error type for all desktop_shell errors.
sealed class DesktopShellError {
  const DesktopShellError();
  String get message;
}

/// Failed to initialize tray icon.
final class TrayInitError extends DesktopShellError {
  final String details;
  const TrayInitError(this.details);
  @override
  String get message => 'Tray initialization failed: $details';
}

/// Failed to initialize window management.
final class WindowInitError extends DesktopShellError {
  final String details;
  const WindowInitError(this.details);
  @override
  String get message => 'Window initialization failed: $details';
}

/// Window operation failed.
sealed class WindowOperationError extends DesktopShellError {
  const WindowOperationError();
}

final class WindowShowError extends WindowOperationError {
  final String details;
  const WindowShowError(this.details);
  @override
  String get message => 'Failed to show window: $details';
}
```

### Option Pattern

Use `Option<T>` instead of nullable types:

```dart
// BAD: Nullable type
String? getWindowTitle();

// GOOD: Explicit Option
Option<String> getWindowTitle();

// Usage
final title = getWindowTitle();
switch (title) {
  case Some(:final value):
    print('Window title: $value');
  case None():
    print('No window title available');
}
```

## Platform Implementation Rules

### Windows

- Use modern Win32 APIs where available
- Support both light and dark themes
- Handle HiDPI displays correctly
- Menu must dismiss on click-outside

### Linux

- Replace deprecated libappindicator with StatusNotifierItem
- Support both X11 and Wayland where possible
- Document GNOME extension requirements

### macOS

- Minimal changes from original tray_manager
- Follow macOS human interface guidelines

## Documentation Standards

- Line length: 80 characters maximum, 100 absolute maximum
- **Exception: Tables are exempt.** Markdown table rows must remain on a single line.
- Use dartdoc format for Dart documentation comments (`///`)
- Document all public APIs with usage examples where helpful

### Markdown Table Formatting

**Rule:** Table header rows MUST have spaces around pipes to satisfy markdownlint MD060.

**Correct:**

```markdown
| Column A | Column B | Column C |
| -------- | -------- | -------- |
| data     | data     | data     |
```

**Incorrect:**

```markdown
| Column A | Column B | Column C |
|----------|----------|----------|
| data     | data     | data     |
```

### Conciseness and DRY

- State each fact or concept ONCE
- Do not repeat information in different formats (table → text → bullets)
- No "in other words" or "to reiterate"
- Cross-reference instead of repeating
- One idea per sentence
- Use lists for enumerations
- Cut ruthlessly: if removing a sentence does not lose meaning, delete it

## Git Workflow

### DO NOT Ask to Commit

**Do not ask the user if they want to commit unless explicitly asked.**

The user will tell you when they are ready to commit. Do not prompt, suggest, or mention committing unless the user initiates it.

### Commit Permission (When User Asks)

**ALWAYS show the user the commit message and ask for explicit approval before committing.**

### Commit Message Format

Follow **Commitizen** conventions:

```
<type>(<scope>): <short summary>

<body>
```

**Types:**

- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation changes
- `style`: Code style (formatting, semicolons, etc)
- `refactor`: Code refactoring
- `test`: Adding or updating tests
- `chore`: Build process, tooling, dependencies
- `ci`: CI/CD configuration

**Scopes:**

- `api`: Public API changes
- `windows`: Windows platform implementation
- `linux`: Linux platform implementation
- `macos`: macOS platform implementation
- `dart`: Dart interface layer
- `docs`: Documentation

**Examples:**

- `feat(api): add unified initialize() with Result return type`
- `fix(windows): menu dismissal on click-outside`
- `refactor(linux): replace appindicator with StatusNotifierItem`

**Process:**

1. Stage changes (`git add`)
2. Show diff/stat (`git diff --stat` or `git diff`)
3. Draft commit message (following Commitizen format)
4. **ALWAYS ASK USER:** "Proposed commit: [message]. Permission to commit? (y/n)"
5. Only commit after receiving explicit confirmation

## Code Review Checklist

Before submitting any code change, verify:

- [ ] All public APIs use `Result<T, E>` or `Option<T>` from unwrap_me
- [ ] No exceptions used for control flow
- [ ] All sealed types handled exhaustively with switch expressions
- [ ] No nullable types where `Option<T>` is more appropriate
- [ ] `final` used for all immutable variables
- [ ] `const` constructors where applicable
- [ ] Imports organized in three groups (dart:, package:, local)
- [ ] Documentation comments on all public APIs
- [ ] Tests added or updated for new functionality
- [ ] dart analyze passes with no errors or warnings
- [ ] dart format produces no changes

## Summary

desktop_shell combines two excellent MIT-licensed plugins into a unified API with:

1. **Explicit error handling** via unwrap_me Result/Option types
2. **Dart 3 features**: sealed classes, pattern matching, exhaustive switches
3. **Type safety**: No nulls, no dynamic, no exceptions
4. **Immutability**: final, const, value-based design
5. **Clean API**: Only essential features, minimal surface area

**Quality is non-negotiable. Correctness is mandatory.**
