Project: Operating Systems — Concurrency Phases
Author: Paola Di Cesare

Overview
--------
Phase 1: Race Conditions Demo
  - Multiple threads perform deposits/withdrawals without synchronization.
  - Purpose: Observe nondeterministic results and lost updates.

Phase 2: Mutex-Protected Accounts
  - Adds per-account pthread mutexes.
  - Purpose: Eliminate data races and verify correctness; measure timing.

Phase 3: Deadlock Creation & Detection
  - Two-lock transfer that intentionally deadlocks (0->1 and 1->0).
  - Includes timeout-based deadlock detection and Coffman conditions documentation.

Phase 4: Deadlock Resolution (Lock Ordering)
  - Implement ordered locking to prevent circular wait.
  - (Future steps) Optional: timeout/trylock strategies and comparison.

Build Instructions (Ubuntu)
---------------------------
# Compile each phase (use the same flags for fair comparisons)

# Phase 1
gcc -pthread -O0 -Wall -Wextra -o phase1 phase1.c

# Phase 2
gcc -pthread -O0 -Wall -Wextra -o phase2 phase2.c

# Phase 3 (uses GNU pthread_tryjoin_np for detection)
gcc -D_GNU_SOURCE -pthread -O0 -Wall -Wextra -o phase3 phase3.c

# Phase 4 (lock ordering; file to be added)
gcc -pthread -O0 -Wall -Wextra -o phase4 phase4.c

Run Instructions
----------------
./phase1
./phase2
./phase3
./phase4

Notes
-----
- Use -O0 while exploring timing of race conditions; later compare with -O2.
- Phase 2 should show no logical discrepancies once expected totals account for net deposits/withdrawals or after switching to a money-conserving pattern.
- Phase 3 will intentionally deadlock; the program reports suspected deadlock after ~5 seconds.
- Phase 4 will prevent the circular wait via consistent lock ordering.

Git Usage
---------
git init
git add phase1.c phase2.c phase3.c
git commit -m "Add Phase 1–3 sources"
git add .gitignore README.txt
git commit -m "Add .gitignore and README"
