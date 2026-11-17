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


## Stage 2 clear crash report (2026-11-16)
- User reports that immediately after clearing Stage 2, the display went black, audio stopped, and the application returned to the Vita home screen.
- This is consistent with an application abort/uncaught exception rather than a rendering-only hang; root cause is not confirmed.
- Existing runtime patch `tools/vita_patch_runtime.py::patch_float_tostring` documents a previously reproduced TeaVM `Float.toString` buffer overrun that surfaced as `IndexOutOfBounds` in `String.<init>` while Gson serialized a save float during a room transition. It replaces the formatter with `snprintf("%.9g")`. This is a relevant lead because Stage 2 completion may trigger save/transition serialization, but the current report does not prove this is the same fault.
- Native startup redirects stdout/stderr (including TeaVM uncaught-exception traces immediately before `abort()`) to `ux0:data/stsvita/stsvita.log`; the file is truncated at each launch. Critical stderr is unbuffered. The log is the highest-value diagnostic artifact to inspect after reproducing the crash.
- Next investigation: obtain the log from the affected run if still present (do not relaunch first, because startup truncates it); inspect the tail for Java exception/TeaVM abort, native memory/GC diagnostics, and the last game/save transition events. If the log is unavailable, reproduce once and retrieve it before another launch. Keep VSync/FPS and unrelated performance code unchanged until evidence identifies the fault.


## Uploaded stsvita.log analysis (2026-11-16)
- Retrieved user-uploaded `stsvita.log` from conversation Files. The inspected log has 2,093 lines and reaches Act 2 diagnostics after the Act 1 boss victory; the last section contains repeated `[act2-diag] draw-reserve ... invalid=1 ... overflow=1` entries.
- At the same point diagnostics report heap arena free about 1 MB (used 237 MB / arena 238 MB), VRAM 95/96 MB, RAM 70/78 MB, external/newlib use about 240 MB, and `[audio] sound pcm malloc FAILED (7862828 bytes)`. Texture decode also reports `bad png sig` and `[DIAG stb] decode FAILED`.
- This is strong evidence of severe memory/resource exhaustion and/or corrupted/overflowing draw-buffer reservation in Act 2, not merely a settings/save serialization issue. It is consistent with the reported black screen, audio stopping, and return to Vita home. The uploaded log's final entries are repeated invalid draw-reserve diagnostics; no Java exception is established in the available tail.
- Next: inspect the native draw-reserve/circular-buffer guard and texture decode/asset lifetime path around the Act 2 transition. Prioritize preventing invalid index/overflow and bounding retained texture/heap allocations. Do not blindly increase pool sizes: current diagnostics show several pools already near capacity. Keep FPS/VSync changes untouched. Verify with host-side tests and CI; real Vita test required to confirm the Act 2 clear path.


## Stage 2 draw-buffer investigation (2026-11-16)
- Corrected interpretation of the uploaded log: `overflow=1` is not by itself evidence of a corrupted pointer. In upstream vitaGL's `vgl_reserve_data_pool`, each scratch allocation advances the current frame's circular-pool pointer; when it passes the limit, vitaGL falls back to `gpu_alloc_mapped_for_cpu(size)`. The pointer is reset to the start of that display buffer at the end of the frame in `gxm.c`.
- The log's pool #2 base/limit is approximately 10.7 MiB per display buffer (consistent with vitaGL's default 32 MiB total / 3 display buffers). Its pointer had advanced about 14.3 MiB, i.e. roughly 3.6 MiB beyond the limit, so the Act 2 frame had already exceeded its per-frame scratch capacity and was relying on fallback allocations.
- The terminal `size=8388608` reserve is an 8 MiB fallback request. At that time the log shows only ~49 KiB VRAM, ~2.1 MiB RAM, ~7.4 MiB PHYCONT, and ~1.6 MiB newlib free; allocation failure is therefore consistent with overall memory exhaustion. This makes the fallback allocation failure the immediate failure path, while the exact GL buffer operation responsible for the 8 MiB request remains unknown.
- Upstream source references inspected: `Rinnegatamante/vitaGL/source/vgl.c` (`vgl_reserve_data_pool`), `source/gxm.c` (frame-end pool index rotation/reset), `source/buffers.c` (scratch buffer creation/update), and `source/utils/gpu_utils.c` (fallback allocation order). The external source was used to interpret behavior; the exact vitaGL revision bundled in the user's build still needs confirmation from the build workflow.
- Added narrow diagnostics around `glBufferData` and `glBufferSubData` so the next Vita log can attribute large scratch reservations to their originating buffer operation (size, target, usage/offset). Changes: `gl_common_glue.c` and linker wraps in `native/CMakeLists.txt`; commits `6fb81829bc8fded15f418be1cd89e383dcf5d048`, `47f4277cac08a99386186c68ca70f40624e17ed8`, `fc1cad7de79571b65bec04c2486ed55101a5a5b2`.
- Do not increase circular-pool size blindly: doing so reserves more of the same scarce Vita GPU memory that is already nearly exhausted and may displace texture allocations. First identify whether the 8 MiB request comes from dynamic or stream buffers and whether the City frame's unusually high scratch demand is avoidable. Keep VSync/FPS untouched.

- Diagnostic build verification: host-side workflow `43871099781` completed successfully. Vita cloud build `43871099887` is running on commit `d0964ac6d8758e154402506e07bdfbf40779dfdd`; VitaSDK install and private source-JAR retrieval completed, VPK build step is still in progress at handoff update time. The workflow removes prior exposed public VPK artifacts and uploads the generated full VPK to the private `stspsvita-build-private` release only after verification.


## Diagnostic build #14 result (2026-11-16)
- Host-side checks run `43871099781`: success.
- Vita cloud build run `43871099887` / commit `d0964ac6d8758e154402506e07bdfbf40779dfdd`: native/Vita build completed successfully; Japanese font manifest validation also passed (`eng,jpn`, `font/jpn` = 3 files / 49,929,880 bytes, staging assets matched). This confirms the added GL buffer diagnostic wrappers compile into the Vita build.
- Final upload to private release `teststs1111/stspsvita-build-private` failed with HTTP 403 at the step that replaces/uploads `STSVITA.vpk` and `build-manifest.json`. The release metadata GET succeeded, so the likely blocker is that secret `STS_BUILD_TOKEN` lacks Contents: Read and write on the private build repository (or otherwise lacks release-asset write permission). No VPK was delivered; build cleanup ran and removed the game input/generated files. Do not route proprietary build outputs through public releases/artifacts.
- Before rerunning, user needs to update the fine-grained PAT used by public repo secret `STS_BUILD_TOKEN`: repository access must include `teststs1111/stspsvita-build-private`, Repository permissions -> Contents -> Read and write. Then replace the existing secret value in `teststs1111/stspsvita` Settings -> Secrets and variables -> Actions -> `STS_BUILD_TOKEN`. Avoid changing unrelated permissions.
- The diagnostic source changes compile successfully; after the private upload permission is corrected, rerun the build, retrieve the VPK privately, and test Act 2. Do not claim the runtime issue is fixed until the Vita log identifies the originating GL buffer call.

- Follow-up instrumentation refinement: upstream vitaGL `buffers.c` shows `glNamedBufferSubData` can reserve a full `gpu_buf->size` scratch copy even when the requested sub-update is small. Therefore wrappers around `glBufferSubData` alone cannot reveal the true reservation size. Added a direct `__wrap_vgl_reserve_data_pool(uint32_t)` hook that logs the exact reserve size, caller return address, and returned pointer. This should identify the actual 8 MiB request and whether it originates in buffer data/subdata or another vitaGL caller. Commits: `e46b9a2b8edf5b6ad59af8ae6a445bb3c954c24e`, `ac9a510777c480794f757dbf681efe59f1ca428f`.


## VitaSDK build workflow recovery (2026-11-17)
- User asked to continue the private VPK build. The current cloud runner installs VitaSDK 2026.08 from upstream vdpm.
- Build run #17 (43984774866, commit bf318891...) failed during package installation because the upstream all-packages script requested obsolete standalone bzip2; host tests passed.
- A narrow follow-up skipped bzip2, but run #18 (43998384436, commit d1884a362543aaab61fcaa1358fb6a735112404d) then failed on libzip. This confirms that editing the giant upstream install-all list package-by-package is not robust.
- Inspected native/CMakeLists.txt: the project directly links VitaSDK components vitaGL, vitashark, SceShaccCgExt, mathneon, freetype, png16, z, bz2, openal, vorbisfile, vorbis, ogg, and taihen, plus SDK stubs/runtime libraries.
- Replaced upstream ./install-all.sh in .github/workflows/cloud-vita-build.yml with an explicit vdpm install loop for only the direct package targets: vitaGL, vitaShaRK, SceShaccCgExt, libmathneon, freetype, libpng, openal-soft, libvorbis, taihen. This lets vdpm resolve package dependencies while avoiding unrelated legacy package targets such as libzip.
- Commit: 38414c8808ca94b9cc331a2beab6fbcb7396cb77 (ci: install only required VitaSDK packages). Push triggers a new host test and private Vita build. Confirm each requested package target exists in the 2026.08 channel from the next job log; if a direct target name is wrong, correct it based on actual package metadata rather than deleting random packages.
- Run #18 host tests succeeded. Run #18 Vita build failed only at ./install-all.sh with error: target not found: libzip; game JAR download, VPK packaging, and private upload were skipped. Token write permission remains untested by a successful upload.
- Keep proprietary game JAR/assets/VPK private. Do not change main; work only on optimization/baseline-profiling.
