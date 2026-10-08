# 4.0.1 — SKSE 2.3.x / Skyrim 1.7.99–1.7.104

- Rebuilt against CommonLibSSE-NG 11.0.0 (alandtse/CommonLibVR `ng` @ 94faaed):
  reads the new Address Library V5 database and declares the
  `AddressLibraryV5` flag that SKSE 2.3.x requires
  (fixes "must be recompiled for new address library").
- `SKSE::Init(skse, true)` + `SKSE::AllocTrampoline(28)` replaced by
  `SKSE::Init(skse, { .trampoline = true, .trampolineSize = 28 })`.
- MainUpdate hook fixed for Skyrim 1.7.x: the old AE offset (+0xC2B in
  Main::Update, ID 36564) lands in the middle of an instruction on 1.7.104
  (and +0xC26 is now a call to an allocator, not a nullsub). Verified on the
  1.7.104 executable: the per-frame nullsub call is at +0xC3D (ID 78641); +0xC38
  is kept as a fallback for 1.7.99 (offset used by OAR / Precision / TDM).
  The site is chosen at runtime by checking it is a CALL to an empty function.
- InteriorChange hooks verified on 1.7.104: +0x46D (ID 13316) and +0x1E
  (ID 13317) are still both calls to the same function (ID 105631), no change.
- Hooks (MainUpdate, InteriorChange) now check that the patch site is a CALL
  (E8) before writing. On a mismatch the hook is skipped and a critical line is
  written to `ReShadeEffectToggler.log` instead of crashing the game.
- Dependencies pinned: glaze v4.4.3, vcpkg 00c5775. The git submodules were
  replaced by the GitHub Actions workflow (`.github/workflows/build.yml`).
