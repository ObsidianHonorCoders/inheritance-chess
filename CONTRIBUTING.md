# Contributing Guidelines

## Purpose

Thank you for your interest in contributing to this C++ project.  
These guidelines ensure consistency, code quality, and long-term maintainability.

---

## Table of Contents

- Getting Started
- Issue Reporting
- Branching & Workflow
- Pull Request Guidelines
  - Commit Message Convention
  - Pull Request Title Convention
  - Pull Request Best Practices
- Code Review Process
- Coding Standards
- File Header Standardization
- Documentation Requirements
- Testing
- CI & Merge Requirements
- Licensing
- Questions


## Getting Started

1. Fork the repository.
2. Clone your fork:
   ```bash
   git clone https://github.com/<your-username>/<repository-name>.git
   ```
3. Create a feature branch:
   ```bash
   git checkout -b feature/short-description
   ```
4. Make your changes following the rules below.
5. Push your branch and open a Pull Request (PR).

---

## Issue Reporting

When opening an issue, include:

- **Title:** short descriptive title
- **Summary:** one-paragraph description of the problem
- **Steps to reproduce:** minimal steps and example input
- **Expected vs Actual:** what you expected to happen and what actually happened
- **Logs / output / screenshots:** any relevant output or images
- **Environment:** OS, compiler, build options (if applicable)

Use labels to indicate severity and area (e.g., `bug`, `enhancement`, `docs`).

---

## Branching & Workflow

| Branch        | Purpose                         |
|---------------|---------------------------------|
| `main`        | Stable, production-ready code   |
| `develop`     | Integration branch              |
| `feature/*`   | New features                    |
| `fix/*`       | Bug fixes                       |
| `docs/*`      | Documentation changes           |

---

PR branch flow and guidance:

- Target `develop` for routine feature branches (`feature/*`).
- Open hotfixes directly against `main` and merge into `develop` after release.
- Small bugfix branches (`fix/*`) may target `develop` unless urgent for `main`.
- Rebase feature branches onto `develop` to keep history clean; use merge commits for release merges.

---

## Pull Request Guidelines

### Commit Message Convention

Use clear, imperative, atomic commit messages.

**Format:**
```
<short description>
```

**Examples:**
```
add vector normalization utility
handle division by zero in matrix operations
add authorship header standard
simplify ownership semantics
```


### Pull Request Title Convention

Use the same structured format for PR titles to make reviews and changelogs consistent and discoverable. Keep the PR title short (≤72 characters) and put extended details in the PR description.

**Format:**
```
<type>(scope): short imperative summary
```

Allowed types:
- feat
- fix
- docs
- refactor
- test
- build
- chore

**PR title examples:**
```
fix(board): handle null-pointer in move parser
feat(pawns): add en-passant support
docs: update contributing guidelines
```


### Pull Request Best Practices

1. One PR = One Logical Change

A Pull Request should represent one coherent unit of work.

Good examples:

- "Add move validation for bishop"
- "Refactor Board class to reduce coupling"
- "Fix memory leak in GameManager"
- "Add unit tests for checkmate detection"

Bad example:

- "Added bishop validation + refactored engine + updated README + fixed random bug"

A PR should answer clearly: What problem does this PR solve? If the answer is multiple unrelated things → split it.

2. Keep PRs Small (Reviewable Size)

Ideal PR size: 100–400 lines changed (rule of thumb). Keep a focused scope so the change is easy to review in under 15 minutes.

3. Avoid Mixing Unrelated Topics in One PR

High-Level Rule:

A good PR should be:

- Small
- Focused
- Self-contained
- Easy to review
- Reversible

Follow these practices to make reviews faster and higher-quality: describe intent, include testing notes, link related issues, add a short checklist (CI, tests, docs, formatting), and keep commits atomic (squash/rebase when requested).

---

## Code Review Process

- Every PR requires at least **one approval**.
- Code must:
  - Compile without warnings
  - Pass all tests
  - Follow formatting and documentation rules

---

## Coding Standards

- Language standard: **C++17**
- Formatting: `clang-format` (project `.clang-format` file)
- Naming conventions:
| Entity          | Java / TS / JS | Python      | C / C++     | CMake                |
|-----------------|----------------|-------------|-------------|----------------------|
| Types (Classes) | PascalCase     | PascalCase  | PascalCase  | PascalCase (Targets) |
| Functions       | camelCase      | snake_case  | PascalCase  | snake_case           |
| Variables       | camelCase      | snake_case  | snake_case  | snake_case (Local)   |
| Constants       | UPPER_SNAKE    | UPPER_SNAKE | kPascalCase | UPPER_SNAKE (Global) |
| Macros          | N/A            | N/A         | UPPER_SNAKE | N/A                  |
- Headers:
  ```cpp
  #pragma once
  ```

---

## File Header Standardization

All `.cpp` and `.h` files **must include a standardized file header** at the top.

### Required Header Template for .cpp and .hpp Files

```cpp
/// @file         [Filename]
/// @brief        [Short description of the file's purpose, no more than one line sentence]
/// @project      Inheritance Chess 
/// @organization Obsidian Honor Coders
/// @repository   https://github.com/ObsidianHonorCoders/inheritance-chess (for .hpp files)
/// @author       [Original Author Name/GitHub Handler]
/// @details      [Multiple line description.
///               Of the file functionality.]
/// @date         [Creation date]
/// @copyright    MIT License - see LICENSE file for details
```

### Rules

- Do not include changelogs in file headers; Git history is the source of truth.
- Use Doxygen-compatible syntax.
- Keep descriptions concise and meaningful.

---

## Documentation Requirements

- Public classes, functions, and APIs must be documented using Doxygen.
- Example:
  ```cpp
  /// @brief      Pure virtual method to calculate valid moves for the piece.
  /// @param[out] p       Vector to be filled with valid move positions.
  /// @param[in]  other_p Vector of positions of all other pieces on the board for move validation.
  /// @param[in]  other_c Vector of colors corresponding to each piece in oth_p for determining valid captures.
  /// @note       Must be implemented by derived classes for their specific movement rules.
  /// @details    This overload provides piece positions and colors separately, allowing independent
  ///             validation of piece positions and ownership for move calculation.
  virtual void moves(PositionList& p, const PositionList& other_p, const ColorList& other_c) const = 0;
  ```

---

## Testing

- New features must include tests.
- Bug fixes must include regression tests.
- All tests must pass before merging.

Run tests locally using the project's build system. Example (CMake-based):

```bash
cmake --build build --target test
ctest --test-dir build --output-on-failure
```

---

## CI & Merge Requirements

- All PRs must pass the repository's CI pipeline before merge (unit tests, basic static analysis, and formatting checks where configured).
- Required status checks should be defined in branch protection rules (e.g., `build`, `tests`, `clang-format`).
- Do not merge until CI is green and required approvals are present. Prefer `Squash and merge` or `Rebase and merge` per the project's preference.

If CI is flaky, address or document the flakiness in the PR; do not bypass checks unless explicitly authorized by maintainers.

---

## Licensing

By contributing, you agree that your contributions are licensed under the **MIT License**, consistent with this project.

---

## Questions

For questions or suggestions:
- Open an issue
- Start a discussion
- Contact the maintainers

We value clean code, clear ownership, and professional collaboration.
