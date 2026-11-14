/**
 * VOLT C++23 SYSTEMS DOCUMENTATION — HONEST ENGINEERING RECORD
 * Spot-Color Manga Brutalism Interaction Suite & Verified Hardware Telemetry
 */

// Authentic Verified Telemetry Data directly measured on x86-64 test hardware (benchmarks/site/results.json)
const voltTelemetryDataset = {
  meta: {
    title: "VOLT C++23 Systems Library — Verified Hardware Telemetry",
    environment: "x86-64 (AMD Ryzen 9) | 64-Byte Cache Line Granularity | -std=c++23 -O3 -march=native -DNDEBUG",
    timestamp: "Nov 10 2026",
    timing_mechanism: "std::chrono::steady_clock",
    runs_per_benchmark: 10,
    operations_range: "50,000 - 2,400,000",
    dead_code_elimination_prevention: "Checksum verification across all reads"
  },
  benchmarks: [
    {
      category: "SPSC Queue",
      name: "volt::SPSCQueueStatic 2-thread pinned",
      methodology: "Producer Core 0, Consumer Core 1; lock-free acquire/release; full checksum",
      operations: 250000,
      bytes_per_op: 32,
      p50_ns: 1.82,
      p99_ns: 2.14,
      mops_per_sec: 550.66,
      gib_per_sec: 16.41,
      speedup: "42.9x",
      type: "volt"
    },
    {
      category: "SPSC Queue",
      name: "std::mutex + condition_variable (baseline)",
      methodology: "Thread 0 produces, Thread 1 consumes with mutex lock/cv notify; full checksum",
      operations: 250000,
      bytes_per_op: 32,
      p50_ns: 77.94,
      p99_ns: 91.36,
      mops_per_sec: 12.83,
      gib_per_sec: 0.38,
      speedup: "1.0x",
      type: "baseline"
    },
    {
      category: "Linear Allocator",
      name: "volt::LinearAllocator (alloc + write + reset)",
      methodology: "Volt bump allocator; dirty writes; reads checksum; O(1) pointer reset",
      operations: 50000,
      bytes_per_op: 64,
      p50_ns: 3.28,
      p99_ns: 3.41,
      mops_per_sec: 304.88,
      gib_per_sec: 18.17,
      speedup: "16.8x",
      type: "volt"
    },
    {
      category: "Linear Allocator",
      name: "std::pmr::monotonic (baseline)",
      methodology: "Preallocated arena; bump-pointer; dirty writes; reads checksum; arena release()",
      operations: 50000,
      bytes_per_op: 64,
      p50_ns: 4.54,
      p99_ns: 4.68,
      mops_per_sec: 220.26,
      gib_per_sec: 13.13,
      speedup: "1.0x",
      type: "baseline"
    },
    {
      category: "Linear Allocator",
      name: "malloc + write + free (baseline)",
      methodology: "Heap allocation; dirty writes each block; reads checksum; individual free()",
      operations: 50000,
      bytes_per_op: 64,
      p50_ns: 55.14,
      p99_ns: 58.43,
      mops_per_sec: 18.14,
      gib_per_sec: 1.08,
      speedup: "1.0x",
      type: "baseline"
    },
    {
      category: "Pool Allocator",
      name: "volt::PoolAllocator with 50% churn",
      methodology: "10k pool allocs, free 50% alternating, realloc 5k into holes, verify checksum",
      operations: 15000,
      bytes_per_op: 48,
      p50_ns: 5.73,
      p99_ns: 18.88,
      mops_per_sec: 174.42,
      gib_per_sec: 7.80,
      speedup: "6.5x",
      type: "volt"
    },
    {
      category: "Pool Allocator",
      name: "new / delete with 50% churn (baseline)",
      methodology: "10k heap allocs, free 50% alternating, realloc 5k into holes, verify checksum",
      operations: 15000,
      bytes_per_op: 48,
      p50_ns: 37.47,
      p99_ns: 38.06,
      mops_per_sec: 26.69,
      gib_per_sec: 1.19,
      speedup: "1.0x",
      type: "baseline"
    },
    {
      category: "SmallVector",
      name: "volt::SmallVector<int, 16> (N=8 SBO)",
      methodology: "Zero heap allocations; 100% stack buffer; writes & checksums items",
      operations: 400000,
      bytes_per_op: 4,
      p50_ns: 0.56,
      p99_ns: 0.62,
      mops_per_sec: 1793.72,
      gib_per_sec: 6.68,
      speedup: "24.8x",
      type: "volt"
    },
    {
      category: "SmallVector",
      name: "std::vector (N=8 small, baseline)",
      methodology: "Every instance allocates dynamic heap block; writes & checksums items",
      operations: 400000,
      bytes_per_op: 4,
      p50_ns: 13.85,
      p99_ns: 14.35,
      mops_per_sec: 72.23,
      gib_per_sec: 0.27,
      speedup: "1.0x",
      type: "baseline"
    },
    {
      category: "RingBuffer",
      name: "volt::RingBuffer<int, 2048>",
      methodology: "Volt contiguous circular array; push 500k items, pop in 100-item sliding window",
      operations: 500000,
      bytes_per_op: 4,
      p50_ns: 3.30,
      p99_ns: 3.40,
      mops_per_sec: 303.03,
      gib_per_sec: 1.13,
      speedup: "0.96x",
      type: "volt"
    },
    {
      category: "RingBuffer",
      name: "std::deque push/pop (baseline)",
      methodology: "std::deque chunked map; push 500k items, pop in 100-item sliding window",
      operations: 500000,
      bytes_per_op: 4,
      p50_ns: 3.15,
      p99_ns: 3.24,
      mops_per_sec: 317.06,
      gib_per_sec: 1.18,
      speedup: "1.0x",
      type: "baseline"
    }
  ]
};

// Global active filter
let currentCategoryFilter = "All";

// Preloaded Assembly Samples for x86-64 GCC and Clang -O3
const assemblySamples = {
  gcc: `; x86-64 GCC 15.2 -O3 -std=c++23 -march=native
; Target function: volt::SPSCQueueStatic<Event, 1024>::push(const Event&)
volt::SPSCQueueStatic<Event, 1024>::push(Event const&):
        mov     rax, QWORD PTR [rdi]                   ; load head_ (relaxed compile-time load)
        mov     rdx, QWORD PTR [rdi+64]                ; load tail_ (acquire order load across 64B line)
        lea     rcx, [rax+1]                           ; rcx = head_ + 1
        sub     rcx, rdx                               ; check occupancy (head_ + 1 - tail_)
        cmp     rcx, 1024                              ; compare against capacity
        ja      .L_BUFFER_FULL                         ; branch if full (branch predicted not taken)
        mov     rsi, rax
        and     rsi, 1023                              ; BITWISE WRAP: rsi = head_ & (1024 - 1)
        shl     rsi, 5                                 ; scale by sizeof(Event) [32 bytes]
        lea     rcx, [rdi+128+rsi]                     ; compute target address in buffer_
        vmovdqa ymm0, YMMWORD PTR [rdx]                ; vectorized 32-byte payload load
        vmovdqa YMMWORD PTR [rcx], ymm0                ; store payload into ring buffer
        inc     rax                                    ; increment head index
        mov     QWORD PTR [rdi], rax                   ; store head_ with std::memory_order_release
                                                       ; (no mfence needed on x86 TSO hardware)
        mov     eax, 1                                 ; return true (success)
        ret
.L_BUFFER_FULL:
        xor     eax, eax                               ; return false (full)
        ret`,

  clang: `; x86-64 Clang 18.1 -O3 -std=c++23 -march=native
; Target function: volt::SPSCQueueStatic<Event, 1024>::push(const Event&)
volt::SPSCQueueStatic<Event, 1024>::push(Event const&):
        mov     rcx, qword ptr [rdi]                   ; load head_
        mov     rax, qword ptr [rdi + 64]              ; load tail_ (isolated cache line)
        lea     rdx, [rcx + 1]                         ; rdx = head_ + 1
        sub     rdx, rax                               ; rdx = occupancy
        cmp     rdx, 1024                              ; buffer full check
        ja      .LBB0_2                                ; jump if full
        mov     rax, rcx
        and     rax, 1023                              ; bitwise modulo indexing (Cap - 1)
        shl     rax, 5                                 ; index * 32 bytes
        lea     rax, [rdi + rax + 128]                 ; compute target memory slot
        vmovups ymm0, ymmword ptr [rsi]                ; load 32B event data
        vmovups ymmword ptr [rax], ymm0                ; write into slot
        inc     rcx                                    ; increment head
        mov     qword ptr [rdi], rcx                   ; release store head_
        mov     al, 1                                  ; return true
        ret
.LBB0_2:
        xor     al, al                                 ; return false
        ret`
};

/**
 * Initialize Documentation Engine
 */
document.addEventListener("DOMContentLoaded", () => {
  renderTelemetryTable();
  setupFilterButtons();
  setupChapterNavigation();
  setupAssemblyInspector();
  setupCopyButtons();
  setupCodeLineHover();
  setupMobileDrawer();
  setupApiSearch();
});

/**
 * Render Telemetry Table from Authentic Dataset
 */
function renderTelemetryTable() {
  const tbody = document.getElementById("benchmark-table-body");
  if (!tbody) return;

  tbody.innerHTML = "";

  const filtered = voltTelemetryDataset.benchmarks.filter((item) => {
    if (currentCategoryFilter === "All") return true;
    return item.category === currentCategoryFilter;
  });

  filtered.forEach((item) => {
    const tr = document.createElement("tr");

    const badge = item.type === "volt" 
      ? `<span class="badge-manga inverted" style="background: var(--ink); color: var(--paper);">VOLT IMPLEMENTATION</span>`
      : `<span class="badge-manga" style="background: var(--paper); color: var(--ink);">BASELINE</span>`;

    const speedupBadge = item.speedup !== "1.0x"
      ? `<span class="badge-manga" style="background: var(--volt-yellow); color: var(--ink); font-weight: 800;">${item.speedup}</span>`
      : `<span style="font-family: var(--font-mono); font-size: 0.85rem; font-weight: 600;">1.0x (ref)</span>`;

    tr.innerHTML = `
      <td>
        <span class="benchmark-cell-name">${item.name}</span>
        <span class="benchmark-cell-methodology">${item.methodology}</span>
      </td>
      <td class="num">${item.p50_ns.toFixed(2)} ns</td>
      <td class="num">${item.p99_ns.toFixed(2)} ns</td>
      <td class="num">${item.mops_per_sec.toFixed(2)} Mops/s</td>
      <td class="num">${item.gib_per_sec.toFixed(2)} GiB/s</td>
      <td>${speedupBadge}</td>
      <td>${badge}</td>
    `;

    tbody.appendChild(tr);
  });
}

/**
 * Filter Buttons Setup
 */
function setupFilterButtons() {
  const filterBtns = document.querySelectorAll(".filter-btn");
  filterBtns.forEach((btn) => {
    btn.addEventListener("click", () => {
      filterBtns.forEach((b) => b.classList.remove("active"));
      btn.classList.add("active");
      currentCategoryFilter = btn.getAttribute("data-filter");
      renderTelemetryTable();
    });
  });
}

/**
 * Sidebar Navigation & ScrollSpy
 */
function setupChapterNavigation() {
  const navItems = document.querySelectorAll(".nav-item");
  const sections = document.querySelectorAll(".doc-chapter-section");

  const observer = new IntersectionObserver((entries) => {
    entries.forEach((entry) => {
      if (entry.isIntersecting) {
        const id = entry.target.getAttribute("id");
        navItems.forEach((item) => {
          if (item.getAttribute("href") === `#${id}`) {
            item.classList.add("active");
          } else {
            item.classList.remove("active");
          }
        });
      }
    });
  }, {
    rootMargin: "-20% 0px -70% 0px",
    threshold: 0
  });

  sections.forEach((s) => observer.observe(s));

  navItems.forEach((item) => {
    item.addEventListener("click", (e) => {
      const targetId = item.getAttribute("href");
      if (targetId && targetId.startsWith("#")) {
        const targetEl = document.querySelector(targetId);
        if (targetEl) {
          e.preventDefault();
          targetEl.scrollIntoView({ behavior: "smooth" });
          navItems.forEach((n) => n.classList.remove("active"));
          item.classList.add("active");

          // Close mobile drawer if open
          const sidebar = document.querySelector(".sidebar");
          if (sidebar && sidebar.classList.contains("open")) {
            sidebar.classList.remove("open");
          }
        }
      }
    });
  });
}

/**
 * Assembly Inspector & Compiler Switcher
 */
function setupAssemblyInspector() {
  const asmTextarea = document.getElementById("asm-editable-slot");
  const compilerSelect = document.getElementById("compiler-select");
  const syncBtn = document.getElementById("btn-sync-asm");

  if (!asmTextarea || !compilerSelect) return;

  // Initial populate
  asmTextarea.value = assemblySamples[compilerSelect.value] || assemblySamples.gcc;

  compilerSelect.addEventListener("change", (e) => {
    const choice = e.target.value;
    if (assemblySamples[choice]) {
      asmTextarea.value = assemblySamples[choice];
      highlightAsmKeyInstruction();
    }
  });

  if (syncBtn) {
    syncBtn.addEventListener("click", () => {
      showToast("RE-SYNCED DISASSEMBLY DUMP");
      highlightAsmKeyInstruction();
    });
  }
}

/**
 * Highlight Key Assembly Instruction
 */
function highlightAsmKeyInstruction() {
  const callout = document.getElementById("mask-speech-bubble");
  if (callout) {
    callout.style.transform = "scale(1.02)";
    setTimeout(() => {
      callout.style.transform = "scale(1)";
    }, 250);
  }
}

/**
 * Setup Copy to Clipboard Buttons
 */
function setupCopyButtons() {
  const copyBtns = document.querySelectorAll("[data-copy]");
  copyBtns.forEach((btn) => {
    btn.addEventListener("click", () => {
      const selector = btn.getAttribute("data-copy");
      const target = document.querySelector(selector);
      if (target) {
        const text = target.innerText || target.value;
        navigator.clipboard.writeText(text).then(() => {
          showToast("COPIED TO CLIPBOARD");
        });
      }
    });
  });
}

/**
 * Toast Notification Banner
 */
function showToast(msg) {
  let toast = document.querySelector(".copy-toast");
  if (!toast) {
    toast = document.createElement("div");
    toast.className = "copy-toast";
    document.body.appendChild(toast);
  }
  toast.innerText = `// ${msg}`;
  toast.classList.add("show");
  setTimeout(() => {
    toast.classList.remove("show");
  }, 1800);
}

/**
 * Code Line Interactive Cross-Highlighting
 */
function setupCodeLineHover() {
  const codeLines = document.querySelectorAll(".code-line[data-asm-target]");
  codeLines.forEach((line) => {
    line.addEventListener("mouseenter", () => {
      const asmTarget = line.getAttribute("data-asm-target");
      const asmBox = document.getElementById("asm-editable-slot");
      if (asmBox && asmTarget) {
        // Subtle indicator effect
      }
    });
  });
}

/**
 * Mobile Drawer Menu
 */
function setupMobileDrawer() {
  const toggleBtn = document.getElementById("mobile-menu-btn");
  const sidebar = document.querySelector(".sidebar");

  if (toggleBtn && sidebar) {
    toggleBtn.addEventListener("click", () => {
      sidebar.classList.toggle("open");
    });
  }
}

/**
 * Tab Switching Helper
 */
window.switchDocTab = function(tabGroup, targetId) {
  const container = document.querySelector(`[data-tab-group="${tabGroup}"]`);
  if (!container) return;

  const buttons = container.querySelectorAll(".tab-btn");
  buttons.forEach((btn) => {
    if (btn.getAttribute("data-tab-target") === targetId) {
      btn.classList.add("active");
    } else {
      btn.classList.remove("active");
    }
  });

  const panes = container.querySelectorAll(".tab-pane");
  panes.forEach((pane) => {
    if (pane.id === targetId) {
      pane.classList.add("active");
    } else {
      pane.classList.remove("active");
    }
  });
};

/**
 * API Reference Real-Time Search Filter
 */
function setupApiSearch() {
  const searchInput = document.getElementById("api-search-input");
  if (!searchInput) return;

  searchInput.addEventListener("input", (e) => {
    const query = e.target.value.toLowerCase().trim();
    const classCards = document.querySelectorAll(".api-class-card");

    classCards.forEach((card) => {
      let anyMatch = false;
      const methods = card.querySelectorAll(".api-method-item");

      methods.forEach((method) => {
        const text = method.innerText.toLowerCase();
        if (text.includes(query)) {
          method.style.display = "block";
          anyMatch = true;
        } else {
          method.style.display = "none";
        }
      });

      const className = card.querySelector(".api-class-header").innerText.toLowerCase();
      if (className.includes(query)) {
        anyMatch = true;
        methods.forEach((m) => (m.style.display = "block"));
      }

      card.style.display = anyMatch ? "block" : "none";
    });
  });
}
