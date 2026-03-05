# Contributing to MatryoshkaOS

Thank you for your interest in contributing to MatryoshkaOS! This document provides guidelines for contributing to the project.

## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [Getting Started](#getting-started)
- [Development Workflow](#development-workflow)
- [Coding Standards](#coding-standards)
- [Commit Guidelines](#commit-guidelines)
- [Pull Request Process](#pull-request-process)
- [Testing Requirements](#testing-requirements)

---

## Code of Conduct

This project adheres to a code of conduct that all contributors are expected to follow. Please be respectful and constructive in all interactions.

## Getting Started

### Prerequisites

- WSL2 (Ubuntu) on Windows 11
- GCC cross-compiler for x86_64
- NASM assembler
- QEMU for testing
- Git

See [docs/BUILD.md](docs/BUILD.md) for detailed setup instructions.

### Fork and Clone

1. Fork the repository on GitHub
2. Clone your fork:
```bash
git clone https://github.com/YOUR_USERNAME/matryoshka-os.git
cd matryoshka-os
```

3. Add upstream remote:
```bash
git remote add upstream https://github.com/ORIGINAL_OWNER/matryoshka-os.git
```

## Development Workflow

### Branch Strategy

- `main` - Stable releases only
- `develop` - Active development branch
- `feature/*` - New features
- `bugfix/*` - Bug fixes
- `test/*` - Testing improvements

### Creating a Feature Branch

```bash
# Update develop branch
git checkout develop
git pull upstream develop

# Create feature branch
git checkout -b feature/your-feature-name
```

## Coding Standards

### General Principles

1. **Single Responsibility Principle** - Each module/function has one clear purpose
2. **Descriptive Naming** - Use full, clear names that explain purpose
3. **No Stubs** - Only commit fully working, tested code
4. **Comments** - Explain WHY, not WHAT (code should be self-documenting)

### C Code Style

```c
// File header
/**
 * MatryoshkaOS - Module Name
 * Brief description of module purpose
 */

#include <matryoshka/types.h>

// Constants
#define MAX_BUFFER_SIZE 1024

// Type definitions
typedef struct process {
    uint64_t pid;
    uint64_t state;
} process_t;

/**
 * Function documentation
 * @param param1 Description of parameter
 * @return Description of return value
 */
int function_name(int param1) {
    // Implementation
    return 0;
}
```

### Code Style Rules

- **Indentation**: 4 spaces (no tabs)
- **Line length**: Maximum 100 characters
- **Braces**: K&R style (opening brace on same line)
- **Naming**:
  - Functions: `snake_case`
  - Types: `snake_case_t`
  - Constants: `UPPER_CASE`
  - Macros: `UPPER_CASE`
  - Global variables: `prefix_snake_case`

### Assembly Style

```asm
; MatryoshkaOS - Module Name
; Brief description

section .text
global function_name

function_name:
    ; Clear comment explaining what this block does
    push rbp
    mov rbp, rsp
    
    ; Implementation
    
    pop rbp
    ret
```

### File Organization

- Header files (`.h`) in `kernel/include/matryoshka/`
- Implementation files (`.c`) in appropriate `kernel/*/` subdirectory
- Assembly files (`.asm`) in `kernel/arch/x86_64/`

## Commit Guidelines

### Commit Message Format

```
<type>(<scope>): <subject>

<body>

<footer>
```

### Types

- `feat` - New feature
- `fix` - Bug fix
- `docs` - Documentation only
- `style` - Code style changes (formatting, etc.)
- `refactor` - Code refactoring
- `test` - Adding or updating tests
- `chore` - Maintenance tasks

### Examples

```bash
feat(pmm): implement bitmap-based physical memory allocator

- Add frame allocation and deallocation
- Implement bitmap tracking
- Add unit tests for edge cases
- Test coverage: 85%

Closes #15
```

```bash
fix(vga): correct cursor position after scrolling

The cursor was not updating correctly when the screen scrolled.
Fixed by calling vga_update_cursor() after scroll operation.

Fixes #23
```

### Commit Best Practices

- ✅ One logical change per commit
- ✅ Write clear, descriptive messages
- ✅ Reference issue numbers
- ✅ Keep commits focused and atomic
- ❌ Don't commit commented-out code
- ❌ Don't commit debug print statements
- ❌ Don't mix refactoring with features

## Pull Request Process

### Before Submitting

1. **Build successfully**:
```bash
make clean
make all
```

2. **Run all tests**:
```bash
make test-all
```

3. **Check code style** (ensure consistency)

4. **Update documentation** if needed

5. **Rebase on latest develop**:
```bash
git fetch upstream
git rebase upstream/develop
```

### PR Description Template

```markdown
## Description
Brief description of changes

## Type of Change
- [ ] Bug fix
- [ ] New feature
- [ ] Breaking change
- [ ] Documentation update

## Testing
- [ ] Unit tests added/updated
- [ ] Integration tests added/updated
- [ ] Tested in QEMU
- [ ] Tested on VMware
- [ ] All tests pass

## Checklist
- [ ] Code follows project style guidelines
- [ ] Self-review completed
- [ ] Comments added for complex code
- [ ] Documentation updated
- [ ] No new warnings generated
- [ ] Commit messages follow guidelines

## Related Issues
Closes #issue_number
```

### Review Process

1. Automated CI checks must pass
2. At least one maintainer approval required
3. All review comments must be addressed
4. No merge conflicts with develop branch

## Testing Requirements

### Unit Tests Required

All new code must include unit tests:

```c
// tests/unit/test_module.c
#include "unity.h"
#include <matryoshka/module.h>

void test_feature_basic(void) {
    // Arrange
    int input = 5;
    
    // Act
    int result = feature_function(input);
    
    // Assert
    TEST_ASSERT_EQUAL(10, result);
}
```

### Integration Tests

For modules that interact with other components:

```c
void test_pmm_vmm_integration(void) {
    // Test that PMM and VMM work together correctly
}
```

### Coverage Requirements

- **Critical modules** (mm, proc, syscall): >= 80%
- **Drivers**: >= 70%
- **Utilities**: >= 60%

## Component-Specific Guidelines

### Memory Management

- Always check for NULL returns
- Document ownership and lifetime
- No memory leaks
- Handle OOM gracefully

### Drivers

- Use appropriate I/O functions (inb/outb)
- Include hardware documentation references
- Test on real hardware when possible

### Architecture-Specific Code

- Isolate in `arch/` directory
- Document CPU features required
- Include fallback implementations if possible

## Questions?

- Open an issue for discussion
- Check [docs/](docs/) directory for detailed documentation
- Review existing code for examples

---

## Recognition

Contributors will be acknowledged in:
- README.md contributors section
- Release notes
- Project documentation

Thank you for contributing to MatryoshkaOS! 🚀

