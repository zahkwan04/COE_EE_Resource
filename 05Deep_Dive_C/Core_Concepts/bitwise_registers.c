#include <windows.h>
#include <stdio.h>
#include <stdint.h>

// ============================================================
// PART 1 — Win32 API functions that return bitfields
// ============================================================
void part1_getsysteminfo(void) {
    printf("========== PART 1: GetSystemInfo() bit flags ==========\n");

    SYSTEM_INFO si;
    GetSystemInfo(&si);

    printf("Processor architecture: ");
    switch (si.wProcessorArchitecture) {
        case PROCESSOR_ARCHITECTURE_AMD64: printf("x64\n"); break;
        case PROCESSOR_ARCHITECTURE_INTEL: printf("x86\n"); break;
        case PROCESSOR_ARCHITECTURE_ARM:   printf("ARM\n"); break;
        case PROCESSOR_ARCHITECTURE_ARM64: printf("ARM64\n"); break;
        default: printf("Unknown (%u)\n", si.wProcessorArchitecture); break;
    }

    printf("Number of processors:  %lu\n", si.dwNumberOfProcessors);
    printf("Page size:             %lu bytes\n", si.dwPageSize);
    printf("Allocation granularity: %lu bytes\n", si.dwAllocationGranularity);

    // dwActiveProcessorMask is a BITMASK — one bit per active processor
    printf("Active processor mask: 0b");
    for (int i = 31; i >= 0; --i) {
        printf("%d", (si.dwActiveProcessorMask >> i) & 1);
    }
    printf("  (0x%lX)\n", (unsigned long)si.dwActiveProcessorMask);

    // Count how many bits are set — that's how many CPUs are active
    int count = 0;
    uint32_t mask = (uint32_t)si.dwActiveProcessorMask;
    while (mask) {
        count += mask & 1;
        mask >>= 1;
    }
    printf("  -> %d bits set = %d active processors\n\n", count, count);
}

// ============================================================
// PART 2 — GetVersionEx-style flags (deprecated but illustrative)
// ============================================================
void part2_system_flags(void) {
    printf("========== PART 2: GetSystemMetrics() flags ==========\n");

    // These return flags packed into a single integer
    BOOL hasMouse    = GetSystemMetrics(SM_MOUSEPRESENT);
    BOOL hasKeyboard = GetSystemMetrics(SM_MOUSEHORIZONTALWHEELPRESENT);
    int  monitors    = GetSystemMetrics(SM_CMONITORS);

    printf("Mouse present:     %s\n", hasMouse ? "yes" : "no");
    printf("Horiz wheel:       %s\n", hasKeyboard ? "yes" : "no");
    printf("Monitor count:     %d\n\n", monitors);
}

// ============================================================
// PART 3 — File attributes: real Windows bitflags
// ============================================================
void part3_file_attributes(void) {
    printf("========== PART 3: File Attributes (real bitflags) ==========\n");

    // Get file attributes of this running executable
    char exePath[MAX_PATH];
    GetModuleFileNameA(NULL, exePath, MAX_PATH);

    DWORD attrs = GetFileAttributesA(exePath);
    if (attrs == INVALID_FILE_ATTRIBUTES) {
        printf("Failed to get attributes\n\n");
        return;
    }

    printf("File: %s\n", exePath);
    printf("Raw attributes: 0x%08lX\n\n", (unsigned long)attrs);

    // Each attribute is a separate bit. This is exactly the pattern
    // you use to read hardware registers — but for files.
    printf("Flags:\n");
    printf("  [%c] READONLY         (0x%08X)\n",
           (attrs & FILE_ATTRIBUTE_READONLY)   ? 'X' : ' ',
           FILE_ATTRIBUTE_READONLY);
    printf("  [%c] HIDDEN           (0x%08X)\n",
           (attrs & FILE_ATTRIBUTE_HIDDEN)     ? 'X' : ' ',
           FILE_ATTRIBUTE_HIDDEN);
    printf("  [%c] SYSTEM           (0x%08X)\n",
           (attrs & FILE_ATTRIBUTE_SYSTEM)     ? 'X' : ' ',
           FILE_ATTRIBUTE_SYSTEM);
    printf("  [%c] DIRECTORY        (0x%08X)\n",
           (attrs & FILE_ATTRIBUTE_DIRECTORY)  ? 'X' : ' ',
           FILE_ATTRIBUTE_DIRECTORY);
    printf("  [%c] ARCHIVE          (0x%08X)\n",
           (attrs & FILE_ATTRIBUTE_ARCHIVE)    ? 'X' : ' ',
           FILE_ATTRIBUTE_ARCHIVE);
    printf("  [%c] NORMAL           (0x%08X)\n",
           (attrs & FILE_ATTRIBUTE_NORMAL)     ? 'X' : ' ',
           FILE_ATTRIBUTE_NORMAL);
    printf("  [%c] TEMPORARY        (0x%08X)\n",
           (attrs & FILE_ATTRIBUTE_TEMPORARY)  ? 'X' : ' ',
           FILE_ATTRIBUTE_TEMPORARY);
    printf("  [%c] COMPRESSED       (0x%08X)\n",
           (attrs & FILE_ATTRIBUTE_COMPRESSED) ? 'X' : ' ',
           FILE_ATTRIBUTE_COMPRESSED);

    // Compute the composite mask: all flags we support
    DWORD supportMask = FILE_ATTRIBUTE_READONLY
                      | FILE_ATTRIBUTE_HIDDEN
                      | FILE_ATTRIBUTE_SYSTEM
                      | FILE_ATTRIBUTE_DIRECTORY
                      | FILE_ATTRIBUTE_ARCHIVE
                      | FILE_ATTRIBUTE_NORMAL
                      | FILE_ATTRIBUTE_TEMPORARY
                      | FILE_ATTRIBUTE_COMPRESSED;

    DWORD unknown = attrs & ~supportMask;
    if (unknown) {
        printf("\nUnknown/unhandled flags: 0x%08lX\n", (unsigned long)unknown);
    }

    printf("\n");
}

// ============================================================
// PART 4 — Console mode flags: real Windows terminal control
// ============================================================
void part4_console_mode(void) {
    printf("========== PART 4: Console Mode Bitflags ==========\n");

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;

    if (!GetConsoleMode(hOut, &mode)) {
        printf("Not running in a real console — skipping.\n\n");
        return;
    }

    printf("Raw console mode: 0x%08lX\n\n", (unsigned long)mode);

    printf("Flags:\n");
    printf("  [%c] ENABLE_PROCESSED_OUTPUT           (0x%04X)\n",
           (mode & ENABLE_PROCESSED_OUTPUT)            ? 'X' : ' ',
           ENABLE_PROCESSED_OUTPUT);
    printf("  [%c] ENABLE_WRAP_AT_EOL_OUTPUT         (0x%04X)\n",
           (mode & ENABLE_WRAP_AT_EOL_OUTPUT)          ? 'X' : ' ',
           ENABLE_WRAP_AT_EOL_OUTPUT);
    printf("  [%c] ENABLE_VIRTUAL_TERMINAL_PROCESSING (0x%04X)\n",
           (mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) ? 'X' : ' ',
           ENABLE_VIRTUAL_TERMINAL_PROCESSING);

    // SET a flag — same idiom as a hardware register
    DWORD newMode = mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    if (newMode != mode) {
        if (SetConsoleMode(hOut, newMode)) {
            printf("\n  -> Enabled ENABLE_VIRTUAL_TERMINAL_PROCESSING\n");

            // Now ANSI escape codes work — print in red
            printf("\033[31m  -> This text is RED using ANSI escape codes!\033[0m\n");

            // Restore original mode
            SetConsoleMode(hOut, mode);
            printf("  -> Restored original console mode\n");
        }
    } else {
        printf("\n  -> ANSI mode already enabled\n");
    }

    // CLEAR a flag
    DWORD clearedMode = mode & ~ENABLE_WRAP_AT_EOL_OUTPUT;
    printf("After clearing WRAP_AT_EOL: 0x%08lX (bit cleared)\n\n",
           (unsigned long)clearedMode);
}

// ============================================================
// PART 5 — Memory protection constants (real bitflags)
// ============================================================
void part5_memory_flags(void) {
    printf("========== PART 5: Memory Protection Flags ==========\n");

    // NOTE: PAGE_* are already #defined by windows.h, so we can't
    // declare local variables with those names. Use the actual
    // Windows constants directly — they're exactly what VirtualAlloc
    // takes as its flProtect argument.

    DWORD prot = PAGE_EXECUTE_READWRITE;

    printf("Protection value: 0x%02lX\n", (unsigned long)prot);
    printf("  Execute?  %s\n", (prot & PAGE_EXECUTE)   ? "yes" : "no");
    printf("  Read?     %s\n", (prot & PAGE_READONLY)  ? "yes" : "no");
    printf("  Write?    %s\n", (prot & PAGE_READWRITE) ? "yes" : "no");

    // Combine flags with OR — same as any hardware register
    DWORD combined = PAGE_READONLY | PAGE_EXECUTE;
    printf("\nCombined flags: 0x%02lX (READONLY | EXECUTE)\n",
           (unsigned long)combined);

    printf("\n");
}

// ============================================================
// PART 6 — The real trick: reading actual Windows registry DWORDs
// ============================================================
void part6_registry(void) {
    printf("========== PART 6: Windows Registry DWORD (bitfields) ==========\n");

    HKEY hKey;
    // Read a real registry value that's a bitfield
    LONG result = RegOpenKeyExA(
        HKEY_CURRENT_USER,
        "Control Panel\\Desktop",
        0,
        KEY_READ,
        &hKey
    );

    if (result != ERROR_SUCCESS) {
        printf("Could not open registry key (error %ld)\n\n", result);
        return;
    }

    DWORD value = 0;
    DWORD size = sizeof(value);
    DWORD type = 0;

    result = RegQueryValueExA(
        hKey,
        "UserPreferencesMask",     // this is a binary bitfield
        NULL,
        &type,
        (LPBYTE)&value,
        &size
    );

    if (result == ERROR_SUCCESS && type == REG_BINARY) {
        printf("UserPreferencesMask = 0x%08lX\n\n", (unsigned long)value);

        // This registry value is a bitfield. Each bit controls a UI feature.
        // Here are a few known bits (from Windows internals):
        struct { uint32_t bit; const char *name; } bits[] = {
            { 0x00000001, "ActiveWindowTracking" },
            { 0x00000002, "MenuAnimation" },
            { 0x00000004, "ComboBoxAnimation" },
            { 0x00000008, "ListBoxSmoothScrolling" },
            { 0x00000010, "GradientCaptions" },
            { 0x00000020, "KeyboardCues" },
            { 0x00000040, "ActiveWndTrkZorder" },
            { 0x00000080, "HotTracking" },
            { 0x00000100, "MenuFade" },
            { 0x00000200, "SelectionFade" },
            { 0x00000400, "TooltipAnimation" },
            { 0x00000800, "TooltipFade" },
            { 0x00001000, "CursorShadow" },
        };

        printf("UI feature flags:\n");
        for (size_t i = 0; i < sizeof(bits)/sizeof(bits[0]); ++i) {
            printf("  [%c] %s\n",
                   (value & bits[i].bit) ? 'X' : ' ',
                   bits[i].name);
        }
    } else {
        printf("UserPreferencesMask not found or wrong type\n");
    }

    RegCloseKey(hKey);
    printf("\n");
}

// ============================================================
// Main
// ============================================================
int main(void) {
    // Enable ANSI escape codes on Windows (needed for colored output)
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(hOut, &mode)) {
        SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }

    part1_getsysteminfo();
    part2_system_flags();
    part3_file_attributes();
    part4_console_mode();
    part5_memory_flags();
    part6_registry();

    printf("========== DONE ==========\n");
    return 0;
}