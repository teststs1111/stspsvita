# STS PS Vita handoff

## Current focus (2026-11-16)
User reports that the current Slay the Spire Vita build feels good in brief play. Preserve existing performance and gameplay behavior; current requested fixes are:
1. Disable VSync by default.
2. Fix Japanese language text disappearing due to missing CJK font assets.

## Changes committed on optimization/baseline-profiling
- Commit: `312a9ac9455edf962d21d2257c97a71fa181fe5c`.
- `VitaApplication.Config.vsync` now defaults to false.
- `VitaGraphics.init()` applies the config before the first frame through `VitaNatives.setVsync(config.vsync)`.
- `native/src/glue/gl_glue.c` now calls VitaGL's documented `vglWaitVblankStart(on ? GL_TRUE : GL_FALSE)`; the old local flag was not connected to swap behavior. Keep `vglSwapBuffers(has_commondialog)` argument dedicated to Common Dialog composition; it is not the VSync control.
- Build language defaults are now `eng,jpn` in `build.sh`, `Makefile`, and `pipeline/stsvita_common.py`.
- `pipeline/assets.py` fails clearly if a requested CJK font directory (e.g. `font/jpn`) is absent from the user's source jar instead of silently producing a package without those glyphs.
- README updated to explain Japanese + English fonts are retained by default; other CJK sets can be requested explicitly.
- Cloud Vita build workflow explicitly passes `--langs eng,jpn`.

## Verification
- Host-side checks succeeded on commit `312a9ac9455edf962d21d2257c97a71fa181fe5c`, workflow run `43801654118`.
- VitaSDK/native cloud build succeeded on the same commit, workflow run `43801654160`.
- All cloud build jobs succeeded, including VitaSDK installation, private source-jar retrieval, VPK packaging, and temporary artifact upload.
- Validation reported: `validate: OK (full + slim, 3405 staged assets)`, with build options `--langs eng,jpn` and `--audio-kbps 0`.
- Outputs: full `STSVITA.vpk` (187 MB) and code-only `STSVITA-slim.vpk` (7.5 MB).
- Temporary Actions artifact: `STSVITA-temporary-build`, artifact ID `14197276777`, run `43801654160`; scheduled to expire 2026-11-17 04:14 UTC (2026-11-17 13:14 JST).
- SECURITY: this source repository is public, so its Actions artifacts must be treated as potentially downloadable by anyone with repository read access. Do not share or use this artifact as a private distribution channel. Arrange deletion/expiry and move future VPK delivery to a private, access-controlled destination before handing builds to the user.
- The artifact contains only the two VPKs, not the separate `work/assets_vita/` directory. For device testing, the full `STSVITA.vpk` is self-contained; the slim VPK requires the matching external assets from the exact same build.
- This verifies build/package creation, not Japanese glyph rendering or actual VSync behavior on a physical Vita. Real-device test remains outstanding.

## Next steps
1. Replace the public Actions artifact delivery with a private, access-controlled short-lived delivery channel; do not expose game-derived VPKs through the public repository.
2. After secure delivery, user tests Japanese language selection, VSync-off behavior, and whether the game still feels smooth.
3. If Japanese text still disappears, inspect runtime language-to-font selection, FreeType glyph generation, glyph atlas uploads, and existing `[DIAG font]` logs. Do not assume missing assets are the only possible cause.
4. After this focused test, streamline deployment so asset-only updates can be transferred separately without a full VPK reinstall, while ensuring the asset pack and code package always come from the same clean build.
4. Do not change unrelated performance paths unless measurements show a regression or bottleneck.

## Project policy
- Work exclusively on `optimization/baseline-profiling` in `teststs1111/stspsvita`.
- Never modify main or unrelated repositories.
- Never request, upload, commit, or distribute the user's proprietary `desktop-1.0.jar`, extracted game assets, generated asset trees, or game-derived VPKs outside the private, short-lived build artifact workflow.
- Keep progress and next actions in this file.
- Distinguish host tests, VitaSDK builds, emulator tests, and physical Vita tests; never claim one proves another.

## VSync follow-up (2026-11-16)
- User reports the VSync checkbox remains checked after restart; the on-screen check does not prove the native swap-wait state.
- Root cause candidate: the game's persisted desktop-oriented VSYNC preference can call `Graphics.setVSync(true)` after Vita backend initialization, overriding the backend's false startup config.
- `VitaGraphics.setVSync(boolean)` now deliberately ignores the game's requested value and always calls `VitaNatives.setVsync(false)`. This enforces native VSync OFF even if the game's own checkbox remains checked.
- Commit: `aca14324bd96c3f1defebadc803c094ae7ab75b8`.
- Next: run host CI and a fresh Vita build; physical-device validation is still required to confirm actual frame pacing. UI checkbox state may still reflect the game's persisted preference and is not yet synchronized with the forced native OFF state.
