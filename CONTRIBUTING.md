# Contributing Guidelines

Thank you for your interest in contributing to this C++ project.  
These guidelines ensure consistency, code quality, and long-term maintainability.

---

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

## Branching & Workflow

| Branch        | Purpose                         |
|---------------|---------------------------------|
| `main`        | Stable, production-ready code   |
| `develop`     | Integration branch              |
| `feature/*`   | New features                    |
| `fix/*`       | Bug fixes                       |
| `docs/*`      | Documentation changes           |

---

## Commit Message Convention

Use clear, imperative, atomic commit messages.

**Format:**
```
<type>: <short description>
```

**Examples:**
```
feat: add vector normalization utility
fix: handle division by zero in matrix operations
docs: add authorship header standard
refactor: simplify ownership semantics
```

Allowed types:
- feat
- fix
- docs
- refactor
- test
- build
- chore

---

## C++ Coding Standards

- Language standard: **C++20**
- Formatting: `clang-format` (project `.clang-format` file)
- Naming conventions:
  - Classes: `PascalCase`
  - Functions: `camelCase`
  - Variables: `camelCase`
  - Constants: `UPPER_SNAKE_CASE`
- Headers:
  ```cpp
  #pragma once
  ```

---

## Authorship Header Standardization

All `.cpp` and `.hpp` files **must include a standardized authorship header** at the top of the file.

### Required Header Template for .hpp Files

```cpp
/// @file      [Filename]
/// @brief     [Short description of the file's purpose, no more than one line sentence]
/// @author    [Original Author Name/GitHub Handler]
/// @date      [Creation date YYYY-MM-DD]
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @see       https://github.com/ObsidianHonorCoders/inheritance-chess
/// @details   [Multiple line description.
///            Of the file functionality.]
```

### Required Header Template for .cpp Files

```cpp
/// @file      [Filename]
/// @brief     [Short description of the file's purpose, no more than one line sentence]
/// @author    [Original Author Name/GitHub Handler]
/// @date      [Creation date YYYY-MM-DD]
/// @copyright 2026 Obsidian Honor Coders. Licensed under Apache 2.0.
/// @details   [Multiple line description.
///            Of the file functionality.]
```

### Rules
- Do not include change logs in file headers.
- Git history is the source of truth for changes.
- Use Doxygen-compatible syntax.
- Keep descriptions concise and meaningful.

---

## Documentation Requirements

- Public classes, functions, and APIs must be documented using Doxygen.
- Example:
  ```cpp
  /**
   * @brief Normalizes a 3D vector.
   * @param v Input vector.
   * @return Normalized vector.
   */
  Vector normalize(const Vector& v);
  ```

---

## Testing

- New features must include tests.
- Bug fixes must include regression tests.
- All tests must pass before merging.

---

## Code Review Process

- Every PR requires at least **one approval**.
- Code must:
  - Compile without warnings
  - Pass all tests
  - Follow formatting and documentation rules

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
