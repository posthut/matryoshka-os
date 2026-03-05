# Git Workflow Notes

## For AI Assistant: PowerShell Git Commands

**CRITICAL:** See `.cursor/GIT_POWERSHELL_REFERENCE.md` for complete reference!

**Quick command for commits via PowerShell:**
```powershell
git commit -m "type(scope): title`n`nDetails`n- Point 1`n- Point 2"
```

Use **backtick-n** (`` `n``) for newlines, NOT `\n` or heredoc!

---

## Making Commits (For Users)

### Quick Commit
```bash
git add -A
git commit -m "Short message"
git push origin develop
```

### Detailed Commit (Recommended)
Create a commit message file, then commit:

```bash
# Create message file
cat > /tmp/commit_msg.txt << 'EOF'
type(scope): short description

Longer description with:
- Bullet points
- Multiple lines
- Details

Closes #issue
EOF

# Commit using the file
git commit -F /tmp/commit_msg.txt
```

### Commit Types
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation
- `style`: Code style (formatting)
- `refactor`: Code refactoring
- `test`: Tests
- `chore`: Maintenance

## Git Identity (Already Configured in Cursor)
Git identity is configured globally through Cursor settings.
No need to set `user.name` and `user.email` manually in WSL.
