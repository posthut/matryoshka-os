# MatryoshkaOS - Context for Next Session

**CRITICAL: Read this file first when resuming work!**

---

## 🎯 IMMEDIATE CONTEXT: WHERE WE STOPPED

### Current Problem: BOOT FAILURE 🔴

**Symptom:** Kernel compiles successfully but shows **BLACK SCREEN** after GRUB menu.

**What We Know:**
1. ✅ GRUB2 menu appears (3 options visible)
2. ✅ Kernel builds without errors (~50KB ELF binary)
3. ✅ All dependencies installed in WSL2
4. ❌ After selecting "MatryoshkaOS" → BLACK SCREEN
5. ❌ No VGA output visible
6. ❌ No serial output captured
7. ❌ Debug symbols in entry.asm (A,B,C) not displayed

**What This Means:**
- Problem occurs BEFORE or DURING kernel_main execution
- Either entry.asm fails, OR kernel_main crashes immediately

---

## 🧪 TEST IN PROGRESS (MUST CHECK FIRST!)

**There is a QEMU window that may still be open with TEST KERNEL!**

### What to Check:

1. **Look for QEMU window** (might be minimized or paused)
2. **Check what's displayed:**

   **Scenario A:** Screen shows **"MatryoshkaOS Test - Kernel Booted!"**
   - ✅ This means: entry.asm works, Multiboot2 works, long mode works
   - 🔴 Problem is in: PMM code or full kernel.c
   - **Next step:** Gradually re-enable PMM code to find which part fails

   **Scenario B:** Screen is **BLACK** (nothing visible)
   - 🔴 Problem is in: entry.asm boot sequence or Multiboot2 header
   - **Next step:** Fix entry.asm and Multiboot2 header

### How to Check:
```bash
# In Windows PowerShell
wsl bash -c "ps aux | grep qemu"  # See if QEMU is running

# If running, look for the window
# If not running or closed, re-run test:
wsl bash -c "cd /mnt/c/myDesc/devSpace/matryoshka-os && qemu-system-x86_64 -cdrom iso/test-kernel.iso -m 512M"
```

---

## 📊 PROJECT STATE

### Code Status:
- **Stage 1-2:** ✅ COMPLETE (initial kernel, VGA driver, build system)
- **Stage 3.1:** 🟡 PMM CODE COMPLETE, BUT NOT TESTED (boot fails)

### What Was Implemented in Last Session:

#### 1. Physical Memory Manager (PMM) - FULL IMPLEMENTATION
**Files Created:**
- `kernel/include/matryoshka/multiboot2.h` (130 lines) - Parse bootloader info
- `kernel/arch/x86_64/multiboot2.c` (70 lines) - Multiboot2 parser
- `kernel/include/matryoshka/pmm.h` (80 lines) - PMM API
- `kernel/mm/pmm.c` (280 lines) - Bitmap allocator implementation

**What PMM Does:**
- Gets memory map from GRUB2
- Creates bitmap (1 bit = 1 frame of 4KB)
- Allocates/frees physical memory frames
- Tracks statistics (total/used/free memory)

**Key Functions:**
```c
void pmm_init(multiboot_info_t *mbi);     // Initialize PMM
uint64_t pmm_alloc_frame(void);           // Allocate 4KB
void pmm_free_frame(uint64_t addr);       // Free 4KB
void pmm_get_stats(pmm_stats_t *stats);   // Get memory info
```

#### 2. Updated kernel_main
**File:** `kernel/kernel.c`
- Added PMM initialization
- Added memory statistics display
- Added PMM allocation tests
- **Modified signature:** `void kernel_main(unsigned long mbi_addr)`
  - Now receives Multiboot2 info pointer from entry.asm

#### 3. Updated entry.asm
**File:** `kernel/arch/x86_64/entry.asm`
- Modified to pass Multiboot2 info to kernel_main
- Added debug output (A,B,C,D characters directly to VGA)
- Should display 'ABC' if reaches kernel_main call

#### 4. Test Kernel Created
**File:** `kernel/kernel_test.c`
- Minimal kernel without PMM
- Just displays "MatryoshkaOS Test - Kernel Booted!"
- Used to isolate the problem
- **ISO:** `iso/test-kernel.iso` (already built)

#### 5. Helper Scripts
- `scripts/check-deps.sh` - Verify all tools installed
- `scripts/build.sh` - Automated build
- `scripts/run-qemu.sh` - Launch QEMU
- `scripts/test-boot.sh` - Test boot with serial capture

---

## 🔍 DEBUGGING STRATEGY

### Step 1: Determine Problem Location

**Run test kernel and observe:**

```bash
cd C:\myDesc\devSpace\matryoshka-os

# Kill any running QEMU
wsl bash -c "pkill -9 qemu"

# Run test kernel
wsl bash -c "qemu-system-x86_64 -cdrom iso/test-kernel.iso -m 512M"
```

**Result interpretation:**

| What You See | What It Means | Next Action |
|-------------|---------------|-------------|
| "MatryoshkaOS Test..." | entry.asm works fine | Debug PMM/kernel.c |
| Black screen | entry.asm fails | Debug boot sequence |
| 'A' only | Crash after segment init | Debug GDT/paging |
| 'AB' only | Crash during screen clear | Debug VGA access |
| 'ABC' only | Crash calling kernel_main | Debug calling convention |

### Step 2A: If Test Kernel Works (Problem in PMM)

**Strategy:** Binary search through PMM code

1. **Disable PMM init in kernel_main:**
   ```c
   // Comment out this line:
   // pmm_init(mbi);
   ```
   - If boots → PMM init crashes
   - If still fails → Problem before PMM

2. **If PMM init crashes, check:**
   - Is mbi_addr valid (not NULL)?
   - Does Multiboot2 memory map exist?
   - Is bitmap allocation correct?

3. **Add debug output:**
   ```c
   vga_puts("Before PMM init\n");
   pmm_init(mbi);
   vga_puts("After PMM init\n");
   ```

### Step 2B: If Test Kernel Fails (Problem in Boot)

**Check these files:**

1. **entry.asm Multiboot2 header:**
   ```asm
   multiboot_header_start:
       dd 0xE85250D6                   ; Magic (MUST be exact)
       dd 0                            ; Architecture
       dd multiboot_header_end - multiboot_header_start
       dd -(0xE85250D6 + 0 + (...))   ; Checksum
   ```
   - Verify magic number is correct
   - Verify checksum calculation
   - Verify header is in first 32KB of file

2. **Long mode transition:**
   - Check page table setup
   - Verify GDT is correct
   - Check if CR3, CR4, CR0 set properly

3. **Try simpler approach:**
   - Remove screen clearing code
   - Use direct VGA write (0xB8000)
   - Write single character first

---

## 🛠️ QUICK COMMANDS

### Build and Run:
```bash
cd C:\myDesc\devSpace\matryoshka-os

# Full build
wsl bash -c "cd /mnt/c/myDesc/devSpace/matryoshka-os && make clean && make all && make iso"

# Run main kernel
wsl bash -c "qemu-system-x86_64 -cdrom iso/matryoshka-os.iso -m 512M"

# Run test kernel
wsl bash -c "qemu-system-x86_64 -cdrom iso/test-kernel.iso -m 512M"
```

### Check Git Status:
```bash
git status
git log --oneline -5
git diff
```

### View Recent Changes:
```bash
git show HEAD
git diff HEAD~1
```

---

## 📁 KEY FILES TO EXAMINE

### If Debugging Boot:
1. `kernel/arch/x86_64/entry.asm` (lines 1-100) - Boot sequence
2. `linker.ld` - Verify load address (1M)
3. `grub.cfg` - GRUB configuration

### If Debugging PMM:
1. `kernel/mm/pmm.c` - PMM implementation
2. `kernel/kernel.c` (lines 80-150) - PMM initialization
3. `kernel/arch/x86_64/multiboot2.c` - Memory map parser

### Reference:
1. `SESSION_STATUS.md` - Detailed session log
2. `TECHNICAL_SPECIFICATION.md` - Original design
3. `PROJECT_STATUS.md` - Current progress

---

## 🎯 EXPECTED WORKING OUTPUT

When kernel boots correctly, you should see:

```
MatryoshkaOS v1.0
====================================

Kernel booted successfully!

System Information:
  - Architecture: x86_64
  - Bootloader: GRUB2 Multiboot2
  - VGA Mode: 80x25 text mode

Initializing Memory Management...
  Total Memory: 512 MB
  Free Memory:  510 MB
  Used Memory:  2 MB

Testing PMM...
  [OK] Allocated frame 1
  [OK] Allocated frame 2
  [OK] Freed frame 1
  [OK] Re-allocated frame

Status:
  [OK] VGA driver initialized
  [OK] Kernel running in long mode
  [OK] Physical Memory Manager initialized
  [OK] PMM allocation/deallocation working

System halted. Next steps:
  1. Implement virtual memory management (VMM)
  2. Add heap allocator (kmalloc/kfree)
  3. Setup interrupt handling
```

---

## 💡 HINTS FOR DEBUGGING

### Common Issues:

1. **NULL Pointer in PMM:**
   - Check if mbi_addr is 0
   - Verify entry.asm passes RDI correctly
   - System V AMD64 ABI: first param in RDI

2. **Bitmap Overflow:**
   - Check bitmap size calculation
   - Verify `_kernel_end` symbol in linker
   - Ensure bitmap doesn't overlap with kernel

3. **Wrong Multiboot Magic:**
   - Should be 0x36d76289 in EAX (GRUB sets this)
   - Don't confuse with header magic 0xE85250D6

4. **VGA Not Working:**
   - Check if address 0xB8000 is mapped
   - In long mode, need identity mapping
   - Verify paging setup in entry.asm

---

## 📞 WHAT TO ASK AI IN NEXT SESSION

**Copy this to new chat:**

```
Я работаю над проектом MatryoshkaOS - операционная система с нуля.

Текущая ситуация:
- Реализован Physical Memory Manager (PMM)
- Код компилируется успешно
- НО: при загрузке в QEMU показывает черный экран

Прочитай пожалуйста файл SESSION_STATUS.md чтобы понять контекст.

ПЕРВОЕ ЧТО НУЖНО: проверить результат тестового ядра.
Запусти: wsl bash -c "qemu-system-x86_64 -cdrom iso/test-kernel.iso -m 512M"

Если видишь текст - проблема в PMM
Если черный экран - проблема в entry.asm

Помоги найти и исправить проблему загрузки.
```

---

## 🔗 USEFUL LINKS

- **GitHub:** https://github.com/posthut/matryoshka-os
- **Branch:** develop
- **Last Commit:** 706913c
- **OSDev Wiki:** https://wiki.osdev.org
- **Multiboot2 Spec:** https://www.gnu.org/software/grub/manual/multiboot2/

---

## ⚠️ IMPORTANT NOTES

1. **Don't commit broken code to main** - keep debugging in develop
2. **All builds happen in WSL2** - not Windows PowerShell
3. **Test kernel is separate** - iso/test-kernel.iso vs iso/matryoshka-os.iso
4. **QEMU may be paused** - look for [Paused] in window title
5. **Session file preserved** - all context in SESSION_STATUS.md

---

**Last Updated:** March 6, 2026, 02:00 AM  
**Status:** PMM code complete, boot debugging needed  
**Next Milestone:** Get kernel booting with PMM working  
**Estimated Time to Fix:** 30-60 minutes once problem identified  

---

*Good luck! The code is good, just need to find the boot issue.* 🚀

