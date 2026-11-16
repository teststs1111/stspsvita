# STS PS Vita handoff

## Current focus (2026-11-16)
User reports that the current Slay the Spire Vita build feels good in brief play. Preserve existing performance and gameplay behavior; current requested fixes are:
1. Disable VSync by default.
2. Fix Japanese language text disappearing due to missing CJK font assets.

## Changes committed on optimization/baseline-profiling
- `VitaApplication.Config.vsync` now defaults to false.
- `VitaGraphics.init()` applies the config before the first frame through `VitaNatives.setVsync(config.vsync)`.
- `native/src/glue/gl_glue.c` now calls VitaGL's documented `vglWaitVblankStart(on ? GL_TRUE : GL_FALSE)`; the old local flag was not connected to swap behavior. Keep `vglSwapBuffers(has_commondialog)` argument dedicated to Common Dialog composition; it is not the VSync control.
- Build language defaults are now `eng,jpn` in `build.sh`, `Makefile`, and `pipeline/stsvita_common.py`.
- `pipeline/assets.py` fails clearly if a requested CJK font directory (e.g. `font/jpn`) is absent from the user's source jar instead of silently producing a package without those glyphs.
- README updated to explain Japanese + English fonts are retained by default; other CJK sets can be requested explicitly.
- Cloud Vita build workflow explicitly passes `--langs eng,jpn`.

## Verification
- Host-side checks succeeded on commit `796668e7e0b301dd36013267868c5fd4c3fc5892` (workflow run `43801579103`).
- An earlier host run `43801575064` also succeeded.
- VitaSDK/native VPK build and real-device behavior are not yet verified for these changes.
- The Cloud Vita build workflow is configured to run from the next workflow-file commit; it downloads the user's private source jar from the existing temporary private release, builds locally in CI, and uploads short-retention artifacts. Do not publish or commit game-derived assets or VPKs.

## Next steps
1. Confirm the cloud Vita build succeeds with Japanese fonts retained and the VSync native call linked.
2. Inspect build manifest and asset tree to confirm `languages` contains `eng,jpn` and `font/jpn` files are packaged.
3. If build succeeds, provide the VPK artifact and ask for one focused real-device check: Japanese UI text appears and VSync starts disabled; verify the game still feels smooth.
4. If Japanese text still disappears despite font assets being present, inspect runtime language-to-font selection, FreeType glyph generation, glyph atlas uploads, and existing `[DIAG font]` logs. Do not assume missing assets are the only possible cause.
5. Do not change unrelated performance paths unless measurements show a regression or bottleneck.

## Project policy
- Work exclusively on `optimization/baseline-profiling` in `teststs1111/stspsvita`.
- Never modify main or unrelated repositories.
- Never request, upload, commit, or distribute the user's proprietary `desktop-1.0.jar`, extracted game assets, generated asset trees, or game-derived VPKs outside the private, short-lived build artifact workflow.
- Keep progress and next actions in this file.
- Distinguish host tests, VitaSDK builds, emulator tests, and physical Vita tests; never claim one proves another.
