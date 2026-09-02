# Novadesk C++ Commenting & Documentation Guidelines

This document establishes the official commenting and documentation standards for all C and C++ source code (`.cpp`, `.h`, `.hpp`, `.c`) in the Novadesk project.

---

## 📋 Table of Contents
1. [Core Principles](#1-core-principles)
2. [File License Header](#2-file-license-header)
3. [Header File Documentation (`.h` / `.hpp`)](#3-header-file-documentation-h--hpp)
   - [Classes & Structs](#31-classes--structs)
   - [Methods & Functions](#32-methods--functions)
   - [Enums & Constants](#33-enums--constants)
4. [Implementation Comments (`.cpp`)](#4-implementation-comments-cpp)
   - [Single-Line Comments](#41-single-line-comments)
   - [Multi-Line Logic & Algorithms](#42-multi-line-logic--algorithms)
5. [Section Dividers & Code Organization](#5-section-dividers--code-organization)
6. [Standardized Action Tags (TODO, NOTE, SAFETY)](#6-standardized-action-tags)
7. [C++ Do's and Don'ts](#7-c-dos-and-donts)

---

## 1. Core Principles

- **Explain the "Why", not the obvious "What":** Focus on the *intent*, *design decisions*, *invariants*, and *edge cases*. Do not write comments that merely rephrase C++ syntax.
- **Header Files define the "Contract":** Headers describe *what* an interface does, its preconditions, thread-safety, ownership of pointers/handles, and return values.
- **Source Files explain the "Mechanics":** Implementation files explain *how* complex algorithms, Win32 API interactions, or memory optimizations work.
- **No Commented-Out Dead Code:** Delete old/unused code instead of commenting it out. Git tracks all history.
- **Formatting:** Always include a single space after the comment delimiter (`// `, `/// `).

---

## 2. File License Header

Every C and C++ header and source file must start with the standard Novadesk license header:

```cpp
/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */
```

---

## 3. Header File Documentation (`.h` / `.hpp`)

Use Doxygen-style documentation (`/** ... */` or triple-slash `///`) in header files so IDE tooltips, autocompletion, and docs extract clean information.

### 3.1 Classes & Structs

Describe the responsibility of the type, ownership model, and thread-safety:

```cpp
/**
 * @brief Manages Direct2D surface rendering and window message dispatching for a widget.
 * 
 * @note Instances are owned by DesktopManager and must only be created/destroyed
 *       on the main UI thread.
 */
class Widget
{
    // ...
};
```

### 3.2 Methods & Functions

Document parameters, return values, pointer ownership, and side effects:

```cpp
/**
 * @brief Compresses a source directory or single file into a .zip archive.
 * 
 * @param sourcePath Path to the file or directory to compress (absolute or relative).
 * @param destinationZip Output file path for the resulting archive.
 * @param options Compression configuration (level, overwrite flag).
 * 
 * @return True if compression completed successfully; false otherwise.
 * 
 * @note Recursively traverses subdirectories and preserves relative folder hierarchy.
 */
bool CompressToZip(const std::wstring &sourcePath,
                   const std::wstring &destinationZip,
                   const ZipCompressOptions &options = {});
```

#### Standard Doxygen Tags:
| Tag | Description |
|---|---|
| `@brief` | Concise one-line description of the function/class. |
| `@param` | Describes a parameter and any constraints (e.g. non-null, coordinate space). |
| `@return` | Describes the return value and error indicators (`nullptr`, `false`, negative codes). |
| `@note` | Important implementation context, caveats, or behavioral notes. |
| `@warning` | Critical caveats (e.g., resource leaks, thread safety constraints). |

### 3.3 Enums & Constants

Document each enum member when the name alone is not fully self-explanatory:

```cpp
/**
 * @brief Specifies window backdrop composition style.
 */
enum class WindowBackdropStyle
{
    None,      ///< Standard opaque or software-keyed alpha rendering.
    Acrylic,   ///< Windows 10/11 Acrylic blur material (requires DWM support).
    Mica,      ///< Windows 11 Mica surface material matching system theme.
    Tabbed     ///< Windows 11 Tabbed/Mica Alt material.
};
```

---

## 4. Implementation Comments (`.cpp`)

### 4.1 Single-Line Comments

Use `// ` with a leading space for quick explanations within function bodies:

```cpp
// Fallback to primary monitor coordinates if saved position is off-screen
if (!IsCoordinateVisible(x, y))
{
    x = primaryMonitor.left + 20;
    y = primaryMonitor.top + 20;
}
```

### 4.2 Multi-Line Logic & Algorithms

Use block comments or consecutive single-line comments for non-trivial logic, Win32 API quirks, or QuickJS engine bindings:

```cpp
// QuickJS JS_IsArray only takes 1 argument (JSValueConst val).
// Verify both that the argument is an object and that it is an array
// before attempting to iterate over numeric properties.
if (!JS_IsObject(argv[0]) || !JS_IsArray(argv[0]))
{
    return JS_ThrowTypeError(ctx, "Expected array of strings");
}
```

---

## 5. Section Dividers & Code Organization

For large translation units, organize related functions with uniform section banners:

```cpp
// ============================================================================
// Internal Helpers
// ============================================================================

namespace
{
    // ...
}

// ============================================================================
// Public API Implementation
// ============================================================================

void Widget::Initialize()
{
    // ...
}
```

---

## 6. Standardized Action Tags

Always format tags in **UPPERCASE** followed by a colon or optional author attribution:

| Tag | Purpose | Example |
|---|---|---|
| `// TODO:` | Planned feature or cleanup. | `// TODO: Support per-monitor DPI scaling during dynamic drag.` |
| `// FIXME:` | Known bug or edge case requiring a fix. | `// FIXME: Handle zero-byte files without throwing an error.` |
| `// NOTE:` | Rationale explaining non-obvious code. | `// NOTE: Must run on an STA thread for COM IFileOpenDialog.` |
| `// SAFETY:` | Explains why an unsafe operation/pointer cast is valid. | `// SAFETY: Buffer length is validated against header.payloadSize above.` |
| `// WARNING:` | Highlights resource ownership or thread constraints. | `// WARNING: Caller is responsible for calling DeleteObject on returned HBITMAP.` |
| `// PERF:` | Explains a performance optimization. | `// PERF: Reuse Direct2D path geometry to avoid rebuild on every tick.` |

---

## 7. C++ Do's and Don'ts

| ✅ DO | ❌ DON'T |
|---|---|
| `// Release mutex before launching child process to prevent deadlock` | `// release mutex` (restating code) |
| `/// @param rawPath Must be normalized using PathUtils::NormalizePath beforehand` | `/// @param rawPath path` (meaningless) |
| `// Workaround for Windows 11 DWM shadow flicker when resizing (MSDN #4081)` | `// hack for windows bug` (no context) |
| `/** ... */` in headers for public interfaces | Putting all documentation in `.cpp` where IDEs cannot find it |
| Keep comments updated when refactoring logic | Leaving outdated comments that contradict the code |
| Clean, well-spaced comment blocks | Cluttered comments attached to the end of every line |
