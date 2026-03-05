# Git Setup and Initial Commit Guide

This guide will help you initialize the repository and push to GitHub.

## Prerequisites

1. GitHub account created
2. Repository created on GitHub: `matryoshka-os`
3. Git installed in WSL2

## Step 1: Install Git (if not installed)

```bash
sudo apt update
sudo apt install git -y
```

## Step 2: Configure Git

```bash
# Set your name and email
git config --global user.name "Your Name"
git config --global user.email "your.email@example.com"

# Optional: Set default branch name
git config --global init.defaultBranch main

# Optional: Set editor
git config --global core.editor "vim"
```

## Step 3: Initialize Repository

```bash
# Navigate to project directory
cd /mnt/c/myDesc/devSpace/matryoshka-os

# Initialize git
git init

# Check status
git status
```

## Step 4: Stage All Files

```bash
# Add all files
git add .

# Verify what will be committed
git status
```

Expected files to be added:
- All kernel source files (.c, .asm, .h)
- Build system (Makefile, linker.ld, grub.cfg)
- Documentation (.md files)
- GitHub templates (.github/)
- CLI scripts (scripts/)
- Configuration files (.gitignore, LICENSE)

## Step 5: Create Initial Commit

```bash
git commit -m "feat: initial commit - minimal working kernel

Stage 1-2 Complete:
- Project structure with all directories
- Full VGA text mode driver (180 lines)
- x86_64 entry point with long mode transition (185 lines)
- Multiboot2 bootloader support
- Build system (Makefile, linker script)
- CLI tools (matryoshka/mshka commands)
- Comprehensive documentation
- GitHub workflows and templates
- Testing infrastructure scaffolding

Features:
✓ Boots successfully in QEMU
✓ VGA color output working
✓ Long mode (64-bit) enabled
✓ GDT and paging configured
✓ GRUB2 multiboot2 compatible
✓ Professional GitHub repository setup

Next: Stage 3 - Memory Management (PMM, VMM, heap)

Lines of code: ~600
Files: 30+
Test coverage: 0% (tests infrastructure ready)"
```

## Step 6: Add GitHub Remote

Replace `YOUR_USERNAME` with your actual GitHub username:

```bash
git remote add origin https://github.com/YOUR_USERNAME/matryoshka-os.git

# Verify remote
git remote -v
```

## Step 7: Push to GitHub

```bash
# Push to main branch
git push -u origin main
```

If you get authentication errors, you may need to use a Personal Access Token:

1. Go to GitHub → Settings → Developer settings → Personal access tokens
2. Generate new token with `repo` scope
3. Use token as password when pushing

Alternative: Use SSH keys (recommended for frequent pushes)

## Step 8: Create Develop Branch

```bash
# Create and switch to develop branch
git checkout -b develop

# Push develop branch
git push -u origin develop
```

## Step 9: Verify on GitHub

Visit your repository on GitHub and verify:

- [x] All files are present
- [x] README displays correctly
- [x] CI workflow appears in Actions tab
- [x] Issue templates available
- [x] License is detected

## Branch Strategy

- `main` - Stable, working code only
- `develop` - Active development
- `feature/*` - New features (branch from develop)
- `bugfix/*` - Bug fixes (branch from develop)

## Future Workflow

For all future work:

```bash
# Start new feature
git checkout develop
git pull origin develop
git checkout -b feature/memory-management

# Make changes
# ... work on code ...

# Commit
git add .
git commit -m "feat(mm): implement physical memory manager"

# Push
git push -u origin feature/memory-management

# Create PR on GitHub: feature/memory-management → develop
```

## Useful Git Commands

```bash
# Check status
git status

# View commit history
git log --oneline --graph

# View changes
git diff

# Undo changes (not committed)
git checkout -- filename

# Undo last commit (keep changes)
git reset --soft HEAD~1

# View branches
git branch -a

# Switch branches
git checkout branch-name

# Pull latest changes
git pull origin main
```

## GitHub Actions

After pushing, GitHub Actions will automatically:

1. Build the kernel
2. Create ISO image
3. Test boot in QEMU
4. Check code quality
5. Verify documentation

Check the Actions tab on GitHub to see results.

## Troubleshooting

### Authentication Failed

Use Personal Access Token:
```bash
git remote set-url origin https://YOUR_USERNAME:YOUR_TOKEN@github.com/YOUR_USERNAME/matryoshka-os.git
```

### Large Files Error

If you accidentally added large files:
```bash
git rm --cached large_file
echo "large_file" >> .gitignore
git commit --amend
```

### Wrong Email in Commit

```bash
git commit --amend --author="Your Name <correct@email.com>"
```

## Next Steps

After successful push:

1. ✅ Verify CI passes on GitHub
2. ✅ Add project description and topics on GitHub
3. ✅ Add project to your portfolio
4. 🚀 Start Stage 3: Memory Management

---

**Congratulations!** Your MatryoshkaOS repository is now live on GitHub! 🎉

