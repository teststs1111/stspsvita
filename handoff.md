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


## VitaSDK 2026.08 package-name inventory (2026-11-17)
- Run #19 (44001304186, commit 38414c8808ca94b9cc331a2beab6fbcb7396cb77) failed at the first direct package install: `error: target not found: vitaGL`. VitaSDK bootstrap itself succeeded; host tests 44001304177 passed. Proprietary JAR retrieval and VPK generation were skipped. This is a package-name mismatch, not a token/upload failure.
- Do not keep guessing package names or remove more dependencies. The installed vdpm supports `vdpm search TERM`; run #20 is an inventory-only diagnostic that searches the signed 2026.08 channel for `vitagl`, `vitashark`, `shacc`, `mathneon`, `freetype`, `png`, `openal`, `vorbis`, and `taihen`, then intentionally stops before private JAR access. Commit 6f513a82e584d81c09bae73cc3390484405c6081.
- Run #20 Vita build 44001708079 / job 133767127115 was still in host dependency installation at last check; host-side checks 44001708107 passed. Next: poll job steps/logs, extract exact package names from the inventory, then restore the real package install loop and run a full private build. Keep work on optimization/baseline-profiling; no VPK has yet been produced or uploaded.


## VitaSDK package database inspection follow-up (2026-11-17)
- Run #20 (44001708079 / job 133767127115) completed and confirmed VitaSDK 2026.08 bootstrap/install succeeds. The `vdpm search TERM` command produced no visible package results for all test terms; the step was intentionally stopped with exit 42 before proprietary JAR retrieval. Host checks 44001708107 passed.
- Inspected upstream vdpm source/docs: supported syntax is indeed `vdpm search TERM`, implemented as pacman `--sync --search`. The package list in upstream `install-packages.sh` includes legacy names, but the current 2026.08 channel may differ. Need inspect the actual sync database using pacman's `--sync --list`, not infer from silent search calls.
- Run #21 started on commit ac5345d4c9701facf8e2cdd482ec5bc54ab2b0fc (Vita run 44002097588 / job 133768425360; host run 44002097454). It prints installed packages and raw pacman sync database listing before an intentional stop. At last check, job was still in host dependency installation; no VPK was generated. Note: the current diagnostic command uses `./vdpm search vita; search_status=$? || true` under `set -e`; if it exits nonzero, it may terminate before the intended stop. Correct this if the job fails at that line.



## VitaSDK package database follow-up (2026-11-17, latest)
- Run #21 (44002097588 / job 133768425360) completed and failed only at the intentional inventory stop; host tests 44002097454 passed.
- The raw package database query returned a noticeably incomplete-looking set of packages: it did not show expected direct dependencies such as `vitaGL`, `vitaShaRK`, `SceShaccCgExt`, `freetype`, `libpng`, `openal-soft`, or `libvorbis`. A case-sensitive `vdpm search vita` is not conclusive because package names such as `vitaGL` contain uppercase letters, but the earlier exact install attempt `vdpm install vitaGL` did fail with `target not found`.
- Official VitaSDK 2026.08 release/catalogue documentation confirms that the channel should contain 134 packages and lists `vitaGL`, `vitaShaRK`, `SceShaccCgExt`, `freetype`, `libpng`, `openal-soft`, `libvorbis`, and `libmathneon`. The migration guide confirms the new client syntax is `vdpm install PACKAGE`; dependency resolution is automatic. Sources checked: https://vitasdk.org/vitasdk-web/channel/2026.08/packages.html and https://vitasdk.org/migration/.
- This changes the diagnosis: do not treat this as simply a renamed package list. There may be a mismatch between the signed channel catalogue and the actual sync database/configuration used by the runner, or a package DB/architecture selection issue. Do not proceed to proprietary JAR retrieval until the exact metadata issue is understood.
- Workflow commit `f1de603fa2a5669813ec68f03cd74d10dcfb8f03` adds a bounded inventory diagnostic: print pacman.conf, filter exact package entries, search with case-sensitive terms, and request exact package metadata. It intentionally stops before proprietary input access. Poll the resulting run, compare the package configuration/database against the official catalogue, then restore the real package installation and build only after direct package resolution is confirmed.
- No VPK has been generated or uploaded. Keep the JAR/assets/VPK private, keep changes on `optimization/baseline-profiling`, and do not touch `main`.


## VitaSDK package snapshot mismatch resolved in workflow (2026-11-17)
- Run #22 (44002558430 / job 133769941821) failed at the intentional inventory stop; host tests run #58 (44002558437) passed.
- Exact log evidence: bootstrap's pacman.conf pointed at `packages-2026.08-snapshot-20261116.70.1`. GitHub Release API confirms this snapshot has only 57 assets and is missing every required package queried (including vitaGL, vitaShaRK, SceShaccCgExt, libmathneon, freetype, libpng, openal-soft, libvorbis, taihen, and libzip). The database search returned only glm among the terms queried.
- The last complete published 2026.08 snapshot `packages-2026.08-snapshot-20261024.61.1` has 139 release assets and includes all queried native packages. This is confirmed from the official GitHub Release API; the official catalogue also lists the same package set.
- Workflow commit `70b07c29fef73c5460787adfb6ba5f9c34b7fbb6` pins only the Vita repo URL in generated pacman.conf to that immutable complete snapshot, refreshes the DB, verifies exact package metadata, then installs the native dependency set. It does not change the toolchain/core ABI and does not access proprietary game input until dependency installation succeeds.
- Next: poll build run triggered by this commit; ensure refresh and package metadata succeed, then inspect dependency install and native compile. Host tests must pass. Private JAR/VPK upload remains gated behind successful install/build/manifest checks.
- No VPK has been generated or uploaded. Keep all work on `optimization/baseline-profiling`; do not touch `main`.


## VitaSDK package DB visibility diagnostic (2026-11-17)
- Run #23 (44003078750 / job 133771686837) failed in the package verification step: after successfully rewriting pacman.conf to the immutable 2026.08 snapshot and refreshing the `x86_64-linux-gnu` and `vita` sync databases, `./vdpm info vitaGL` returned `package 'vitaGL' was not found`. The build never retrieved the private game JAR or created a VPK. Host checks run 61 (44003093206) passed.
- The workflow now adds a bounded diagnostic before package verification: explicitly exports `VDPM_PACMAN_CONF`, prints the exact config, lists sync DB files and filters the pacman repository inventory for all required package names. This is commit 0a7c5b53d59e68850f404d192061a9e46c8e86a8.
- New diagnostic build run #24 is 44004032289 (job 133774850925), currently in progress at last check. Poll logs and inspect whether pacman sync DB actually contains the expected 134-package 2026.08 catalogue. Do not make assumptions about the root cause until that inventory is available.
- Official 2026.08 catalogue/release docs show the immutable snapshot `packages-2026.08-snapshot-20261024.61.1` and list the required packages; see https://vitasdk.org/vitasdk-web/channel/2026.08/packages.html and https://vitasdk.org/vitasdk-web/releases.html.
- Keep the proprietary JAR/assets/VPK private; no JAR retrieval or packaging until dependency resolution is proven. All changes remain on `optimization/baseline-profiling`; do not touch `main`.


## Vita package database content inspection (2026-11-17)
- Run #24 (44004032289 / job 133774850925) failed at VitaSDK package verification. Exact log evidence: config printed the pinned immutable repo URL packages-2026.08-snapshot-20261024.61.1, pacman --sync --refresh reported both databases downloaded, but the resulting local vita.db was only 6,964 bytes and pacman --sync --list returned no package entries. Required package metadata lookup then failed for vitaGL. This is a repository DB content/selection problem, not simply package naming. The official release API reports the pinned snapshot's vita.db asset is 18,185 bytes and includes the expected 134-package catalogue; the asset vitaGL-0.0.0.r1488.g2bdbe89-1-vita.pkg.tar.xz is present (278,360 bytes).
- The workflow was extended to inspect each downloaded DB with file and tar -tf, dump pacman's complete repo list, and request exact vitaGL metadata without failing early. Commit: e1f3b452cf5b5ce3e8acc6bfb4d8aadf2dc22644.
- Next run is triggered by this commit. Poll its Vita build job and inspect archive contents / metadata. The private game JAR is still gated behind successful SDK dependency installation; no VPK exists yet.
- Keep all changes on optimization/baseline-profiling, do not touch main, and keep proprietary game input and outputs private.


## VitaSDK database direct-asset verification (2026-11-17)
- Latest build #25 (44005233304 / job 133778845306) completed failure at VitaSDK install; host checks passed. The downloaded vita.db is a valid gzip/tar archive but only 6,964 bytes and contains a small, mismatched package set (e.g. newer box2d/expat versions), while the official pinned 2026.08 catalogue has 134 packages and includes vitaGL. pacman --sync --list exposed only a small subset; vitaGL lookup failed.
- GitHub Release API for vitasdk/vitasdk-autobuild tag packages-2026.08-snapshot-20261024.61.1 reports the exact vita.db asset size 18,185 bytes and digest sha256:fdc3383892de595ccc2ca55bf93ce54df84af1fa8f3f25c12595ad11d65c0202; release contains 139 assets and the vitaGL package asset is present.
- Likely fault is pacman's repository DB retrieval/cache path returning a different file than the release asset, not absent package publication. Workflow now downloads the immutable vita.db asset directly with curl, verifies exact size and SHA-256, then installs it into pacman's sync DB path before listing/verifying dependencies. This is a guarded diagnostic/fix; if direct download differs, job stops before private JAR retrieval.
- Workflow commit: 1f21cb5231e7d1da2ae9332ab2c6bc6d9f2449a9. Poll the resulting run and inspect direct DB verification plus pacman --sync --list / vdpm info vitaGL. No private JAR retrieval or VPK packaging has occurred yet. Keep all work on optimization/baseline-profiling; no main changes.


## VitaSDK environment propagation and native compile diagnosis (2026-11-17)
- Build #26 (44010087234) passed VitaSDK package installation but failed in the native compile before VPK packaging. The private game JAR was successfully downloaded, extracted (3,412 asset files), and processed; cleanup ran after failure. No VPK was produced and no proprietary files were published.
- Exact compiler failure in `native/src/glue/gl_common_glue.c`: duplicate definitions of `__real_vgl_reserve_data_pool` and `__wrap_vgl_reserve_data_pool` with conflicting return types (`void *` vs `uint8_t *`). The existing first wrapper already logs the reservation size, circular pool state, invalid fallback result, and draw context. The second wrapper had been added later to log caller addresses, causing the conflicting declarations.
- Fixed by removing the duplicate second wrapper and folding `__builtin_return_address(0)` caller attribution plus >=1 MiB reservation logging into the original wrapper. Commit `168187fbdb5a816fdf634b8fe639e66c8fe14223`.
- Removed the duplicate `--wrap=vgl_reserve_data_pool` linker option from `native/CMakeLists.txt`. Commit `c51bf4fcd59e431c512c487c283e31facb70518a`.
- Earlier in this same build iteration, workflow commit `a28c5e4230f1d38982269871d7c557fc47882dc4` exports `VITASDK` and adds its bin directory to `GITHUB_PATH` for subsequent steps. Host-side tests for that commit passed (run 44010706765).
- Latest commits trigger a fresh private Vita build and host checks. Poll the latest run on `optimization/baseline-profiling`; confirm native compilation, VPK generation, Japanese font manifest validation, and private Release upload. If compilation advances, inspect any further linker errors before changing source. No VPK is confirmed yet.
- Keep proprietary JAR/assets/VPK private, do not touch `main`, and do not claim hardware testing.


## Memory investigation continuation (2026-11-19)
- Re-enabled the single existing Slay the Spire Vita automation (daily flexible schedule, target 20:30 Asia/Tokyo) and narrowed its mission to memory ownership, draw-pool attribution, safe reuse/release, and regression-tested fixes. No competing automation was created.
- Diagnostic Vita build run `44410431622` and host test run `44410431775` both completed successfully. Latest `native-callsite-disassembly` artifact: `14478524832`; latest symbol report artifact: `14478779662`.
- Parsed every direct `bl __wrap_vgl_reserve_data_pool` callsite against the current build's symbol table. Calls originate from multiple categories: client-array pointer uploads (`vglVertexPointer`, `vglColorPointer`, `vglTexCoordPointer`, `vglIndexPointer`); custom-shader and fixed-function draw paths (`glDrawArrays`, `glDrawElements`, multidraw variants); matrix uniform uploads; texture allocators (`gpu_alloc_compressed_texture`, compressed/cube variants); plus `glReadPixels`, framebuffer blit, `glEnd`, and `scene_reset`.
- Therefore the 8 MiB reserve failure cannot yet be attributed solely to draw calls: the same circular pool serves both draw/uniform scratch and texture allocation paths. The old logged caller `0x8158dda3` does not match any current-build reserve callsite and still resolves inside a non-call instruction in `cff_parse_vsindex`; likely binary mismatch or pre-entry return-address corruption. The wrapper preserves LR immediately on entry.
- Source review: `VitaApplication` keeps one GL context for the process; `Buffers.clearPins()` clears per-frame client-array pins; audio SFX decode frees its temporary PCM after `alBufferData`, and music disposal frees its OpenAL buffers/OGG handle. These paths are not currently proven leaks.
- A bounded-spill bookkeeping weakness was found for follow-up: `STSV_SPILL_TRACK=128`; `stsv_spill_note()` silently fails when the table is full (or texture id is zero), while `stsv_tex_can_alloc()` has already incremented `stsv_spill_used`. This can strand spill *budget accounting* after texture deletion and prevent later safe spill admissions; it does not itself prove the native allocation leak or explain the 8 MiB draw-reserve failure. Before changing it, add a small isolated test for slot/refund behavior and verify texture-id lifetime assumptions.
- Next decisive evidence is a log from the current diagnostic binary: map the exact caller address and size of the failed 8 MiB reservation. Do not make blind pool-size changes. Continue source-level ownership review and a regression-tested spill-accounting correction if the isolated test confirms it.


## Spill-budget accounting fix (2026-11-19, manual continuation)
- Confirmed a concrete accounting defect in `native/src/glue/gl_common_glue.c`: `stsv_tex_can_alloc()` incremented `stsv_spill_used` before `stsv_spill_note()`, but `stsv_spill_note()` silently did nothing when the bound texture ID was zero or all 128 tracking slots were occupied. Such admitted bytes could never be refunded by `stsv_gl_delete_texture()`, so the bounded spill budget could become permanently consumed even after textures were deleted.
- The previous re-upload path also checked `stsv_spill_used + need` before subtracting the existing entry for the same texture ID. A texture replacement could therefore be refused even when replacing its old spill allocation would fit the budget.
- Added allocation-free helper `native/src/glue/stsv_spill_tracker.h`. Admission now requires a nonzero texture ID and an existing/free tracking slot; replacing an existing ID subtracts its old recorded bytes before budget validation; rejected admissions leave both the table and usage unchanged. Refund clears the matching record and restores its bytes. If all tracking slots are occupied, new untrackable spill requests fail closed rather than silently leaking accounting.
- Wired the helper into `stsv_tex_can_alloc()` and `stsv_spill_refund()`. The existing pool-first behavior, 6 MiB spill budget, 48 MiB malloc reserve, texture formats, and visual/audio quality settings are unchanged. The 128-entry table was not enlarged.
- Added `native/tests/spill_tracker_test.c` and integrated it into `make test`. Cases cover re-upload replacement, projected-budget rejection, ID zero, full-table rejection, updating an existing ID with a full table, refund/slot reuse, and refund of an unknown ID.
- Commits on `optimization/baseline-profiling`: `138c2b07` (helper), `30019dd1` (C regression tests), `d27a73eb` (native integration), `472362ca` (test target), `a99a071f` (formatting), `cc806ab2` (use Make's configured compiler).
- Host CI: workflow run `44447331922` completed successfully on `cc806ab2b2b3f2ebdd9442575062d9caa4a3769d`. The run passed shell syntax/help checks, all 53 Python tests, the Java frame-pacing test, and the added C test (the C test is silent on success and is part of the successful `make test` recipe).
- Earlier host run `44447112041` failed only because the first test recipe used shell parameter expansion for the compiler; changed it to Make's built-in `$(CC)`, then reran the full workflow successfully.
- NOT YET VERIFIED: VitaSDK compile/link and physical Vita behavior. No VPK was built or installed in this continuation. This accounting fix prevents one identified source of artificial spill-budget exhaustion; it does NOT yet prove the original 8 MiB vitaGL draw-reserve failure or Act-transition crash is fixed.
- Next: inspect the latest vitaGL source/revision and memory ownership around compressed texture upload/delete, then attribute the 8 MiB reservation to an exact callsite. Before any real-device test, produce a VitaSDK build of this branch and only ask the user to test when that is the sole remaining blocker.


## vitaGL reserve-path source attribution (2026-11-19, manual continuation)
- Inspected the exact VitaSDK package from successful cloud build `44410431622`: `vitaGL 0.0.0.r1488.g2bdbe89-1`. Upstream source inspected at commit `2bdbe89`, matching the installed package revision.
- Key distinction: `gpu_alloc_mapped_temp(size)` in `source/utils/gpu_utils.h` calls `vgl_reserve_data_pool(size)` whenever circular pools are enabled. It is used for short-lived CPU-mapped staging buffers, not only vertex/index draw copies.
- In `source/vgl.c`, `vgl_reserve_data_pool` advances the current display buffer's circular scratch pointer. Once the per-frame region is exceeded, it falls back to `gpu_alloc_mapped_for_cpu(size)` and marks that fallback allocation dirty for deferred garbage collection. Thus an 8 MiB reserve can be a temporary texture-upload staging buffer; the name “draw reserve” is broader than the actual use.
- `source/utils/gpu_utils.c::gpu_alloc_compressed_texture` calls `gpu_alloc_mapped_temp(image_size)` in its UBC/PVRTC/default swizzle paths before `sceGxmTransferCopy`. The exact build disassembly has direct reserve callsites inside `gpu_alloc_compressed_texture` at call instructions around `0x81543aba`, `0x81543b06`, and `0x81543d3a`, and inside `gpu_alloc_compressed_cube_texture` around `0x81542b86`, `0x81542bd2`, and `0x81542c32`. Those are staging copies, not the permanent texture allocation itself.
- The same reserve pool is also called from client-array uploads, fixed/custom-shader draw paths, matrix-uniform uploads, `glEnd`, and `scene_reset`. Therefore the old 8 MiB failure cannot be assigned to drawing or texture upload without a valid caller address from the same binary.
- A relevant pressure interaction remains to verify: the project's `stsv_tex_can_alloc()` guard checks whether the permanent texture allocation appears admissible, but vitaGL may then allocate an additional `image_size` temporary staging buffer from the per-frame circular pool. The guard does not reserve that staging capacity. This is a plausible way for a texture upload to pass admission and then fail on the temporary reserve, but is not yet proven to be the reported failing call.
- The vitaGL source also contains a full-buffer scratch copy in `glNamedBufferSubData` when a recently used scratch VBO is updated. However, that function is not among the direct reserve callsites in the current `reserve-callers.txt` artifact, so do not prioritize it for this binary without new evidence.
- Artifact `14478524832` was re-downloaded before expiry and inspected in this session. The reserve callsite listing confirms the above function groups; the old logged `0x8158dda3` still does not match a valid call-return site in the current build.
- Next: use a same-build Vita log to map the failing 8 MiB request to one of the exact callsites and record the asset path/texture dimensions or draw count. Until that caller is identified, do not enlarge the circular pool or change texture formats/quality. The spill-tracker bookkeeping fix above is independent and does not itself resolve this reserve failure.


## vitaGL pool admission and temporary upload memory audit (2026-11-19, manual continuation)
- Inspected upstream vitaGL source at the exact installed package revision `2bdbe89` (`vitaGL 0.0.0.r1488.g2bdbe89-1`), including `source/utils/gpu_utils.c`, `source/utils/gpu_utils.h`, `source/utils/mem_utils.c`, and `source/vgl.c`.
- `gpu_alloc_compressed_texture()` allocates the permanent texture backing store with `gpu_alloc_mapped_for_gpu(tex_size)` first. For UBC2/UBC3/UBC5 and some PVRTC/default paths with aligned dimensions within the vitaGL limits, it then allocates a second, temporary CPU-mapped staging buffer of `image_size` via `gpu_alloc_mapped_temp(image_size)` and copies the source into it before `sceGxmTransferCopy`. Both allocations coexist during upload.
- `gpu_alloc_mapped_for_gpu` tries pools in order VRAM → RAM → PHYCONT → BUDGET → external/newlib. `gpu_alloc_mapped_for_cpu` (used by `vgl_reserve_data_pool` overflow) tries RAM → PHYCONT → BUDGET → VRAM → external/newlib. Thus permanent texture data and temporary staging can compete for the same remaining pools; an earlier successful texture-admission check does not reserve either allocation.
- `vglMemFree(type)` is not the largest contiguous free block: `mem_utils.c` reports `tm_free[type]` for the custom heap or `capacity - current_in_use` from mspace statistics. Both are aggregate free-byte counts. The project's `stsv_pool_maxfree()` takes the maximum of VRAM/RAM/PHYCONT and omits BUDGET, while the actual vitaGL allocators can fall through multiple pools and may fail on fragmentation even when aggregate free bytes exceed the request. Treat this guard as a conservative heuristic, not proof that the next allocation will succeed.
- In the UBC staging path, `gpu_alloc_mapped_temp()` returns the circular-pool region or a fallback from `gpu_alloc_mapped_for_cpu()`. The upstream function returns that pointer without a failure-safe caller contract; the UBC path immediately copies into it. A null fallback can therefore become a null-destination copy. This matches the observed failure mode but is not yet tied to the old 8 MiB request: the exact caller address from the old log is still inconsistent with the current build, and some large DXT paths use CPU swizzling instead of the temporary transfer path.
- No runtime code changed in this audit. Do not patch vitaGL allocation order or add a speculative direct-source upload macro until the exact failed callsite, texture format/dimensions, and source-buffer alignment/lifetime are established. A bypass of the staging copy may be unsafe if the source is not suitably aligned or remains live through the transfer.
- Latest branch head at this audit: `c2a2b085a97d617748750141ff756fac24ed1381`. VitaSDK workflow run `44448464630` is in progress on that exact commit; host regression run `44447331922` passed on the spill-tracker fix parent. Poll the VitaSDK run before requesting any device work.
- Next: inspect the exact `vglInitExtended` pool sizes and VitaSDK build result; then, if static source review cannot resolve the failure, use the current-build diagnostic VPK to obtain a same-binary caller address plus the asset/format/dimensions at the first failed large reservation. Only ask for physical-device testing once the build is ready and that log is the sole blocker.


## vitaGL circular scratch pool and deferred GC lifetime (2026-11-19, manual continuation)
- Corrected a potential interpretation trap in the `vglInitExtended(8 * 1024 * 1024, ...)` call: its first `pool_size` parameter is assigned to vitaGL's legacy immediate-mode vertex pool, NOT the circular scratch pool. The circular pool remains `CIRCULAR_POOL_SIZE_DEF = 32 MiB` unless explicitly changed.
- On normal app mode, vitaGL defaults to three display buffers (`gxm_display_buffer_count = 3`). It allocates `circular_data_pool_size / gxm_display_buffer_count` per buffer, so the default scratch capacity is about 10.67 MiB per frame-buffer slot, not 32 MiB per individual frame. A single 8 MiB request can therefore fit only if earlier scratch reservations on that slot have consumed less than about 2.67 MiB.
- At `vglSwapBuffers()` frame end, vitaGL increments the frame counter, rotates `vgl_circular_idx` to `vgl_framecount % gxm_display_buffer_count`, and resets that slot's pointer to its base. The GC then advances its purge ring. `FRAME_PURGE_FREQ` is 4; fallback allocations marked dirty by `vgl_reserve_data_pool()` are freed only when their purge slot is collected, several frame cycles later.
- Consequence: if one frame's uploads/draw scratch exceed the ~10.67 MiB slot, later reservations in that frame fall back to CPU-mapped allocations. Those allocations remain live through the deferred GPU-safe purge window; a burst of large uploads can therefore create a transient multi-frame native-memory peak without a permanent leak. This is a source-backed mechanism consistent with the old exhaustion evidence, but the actual 8 MiB caller still needs a valid same-build address before attributing the reported failure to texture upload versus draw scratch.
- Do not change display-buffer count, circular pool size, or purge frequency without measured evidence: each trades memory capacity against GPU in-flight safety, frame pacing, and total reserved memory.


## Diagnostic VPK build completed (2026-11-19 manual continuation)
- The VitaSDK workflow `44448464630` on commit `c2a2b085a97d617748750141ff756fac24ed1381` completed with conclusion `success`. All build steps succeeded, including VitaSDK install, VPK packaging, symbol/callsite reports, Japanese font payload/manifest verification, private-release upload, and cleanup of temporary game-derived build inputs.
- Host-side checks workflow `44450005277` on documentation-only head `964075a54b95a5c3458a8d7bdb9545d9ab2ff1ff` also completed with conclusion `success`.
- Private release `teststs1111/stspsvita-build-private` tag `temp-build-1` was updated at 2026-11-19 03:41 UTC. Confirmed assets include `STSVITA.vpk` (195,852,744 bytes; asset ID `766092603`), `build-manifest.json` (2,504 bytes; asset ID `766092975`), and `STSVITA.map` (7,858,504 bytes; asset ID `766093047`). Exact VPK release URL: https://github.com/teststs1111/stspsvita-build-private/releases/download/temp-build-1/STSVITA.vpk
- Workflow artifacts (all non-expired at check time): `native-callsite-disassembly` ID `14496760901`, `native-address-symbol-report` ID `14495728749`, and `native-linker-map` ID `14497020826`. Re-inspected the latest callsite disassembly. `__wrap_vgl_reserve_data_pool` saves the incoming LR immediately at function entry (`mov fp, lr`) before diagnostic logging, so this build should provide a reliable caller address for the wrapper's logged large/fallback reservations.
- The VPK was built from `c2a2...` before subsequent documentation-only commits; no runtime source changes were made by those later commits. Host checks pass on the current branch head. No physical-device result is known.
- Static analysis is now at the point where the remaining useful discriminator is a log from this exact diagnostic binary under the failing Act transition / long-session scenario. Do not ask for hardware testing unless this same diagnostic build is actually needed on-device; if it is, clearly label it as diagnostic-only, explain that it is not a fix, and ask the user to return the fresh `stsvita.log` (especially the first large `draw-reserve`/fallback failure and preceding texture/draw context). Avoid claiming a fix or requesting routine reinstallation.


## SFX decode peak-memory reduction (2026-11-19, manual continuation)
- Reviewed the native OGG/WAV loader and Java VitaAudio.newSound() boundary. stsv_sound_load() consumes the supplied byte pointer synchronously: OGG decoding completes and ov_clear() runs before return; WAV upload copies the samples into a temporary native buffer and frees it before return. Neither path stores the caller's encoded-data pointer.
- VitaAudio.newSound() previously copied each FileHandle.readBytes() result into an unsafe buffer, then called Buffers.addr() on that non-array-backed buffer. Buffers.addr() drains it into another Java byte[] and pins that array until end-of-frame. The decode therefore held two redundant copies of the encoded SFX payload (plus the original FileHandle byte[]) at once, with avoidable full-buffer copies and delayed release of the pinned copy.
- Changed the call to nSoundLoad(Address.ofData(ogg), ogg.length) and removed the unsafe-buffer allocation/copy/free imports. The local ogg array remains strongly reachable for the complete synchronous native call. This uses the same TeaVM backing-array address mechanism already used by newMusic() for its synchronous path-open call and documented in Buffers.java. The optimization removes both redundant copies and the frame-pinned duplicate.
- Runtime behavior, decoded PCM allocation, OpenAL buffer ownership, and audio quality are unchanged. This reduces transient memory by the encoded OGG file size, but it does not eliminate the separate decoded PCM allocation (the historical ~7.5 MiB malloc failure remains a symptom of broader memory pressure, not proven to originate in this copy).
- Commit: 8567457a7975c6ea554a6069932e92df27fa9087 (perf(audio): avoid copying encoded SFX before decode). Host CI and VitaSDK build verification are pending; no physical-device validation has been performed.
- Next: confirm host tests and VitaSDK compilation on this commit. Continue tracing long-session native memory ownership; do not claim the Act-transition failure is fixed and do not request a VPK reinstall for this unverified optimization.


## Audio object disposal audit (2026-11-19)
- Followed the Java disposal calls through to native OpenAL ownership. VitaSound.dispose() calls nSoundFree(buffer); stsv_sound_free() stops any source still bound to that buffer, unbinds it, and calls alDeleteBuffers(). VitaMusic.dispose() removes the object from ACTIVE, calls nMusicFree(handle), and clears its handle; stsv_music_free() stops/deletes its source, deletes its four stream buffers, closes the Vorbis decoder, and frees the native stream object.
- No missing native-free path was found in these explicit dispose() implementations. This does not prove the game always invokes dispose() for every asset, so it is not evidence that long-session AL resources cannot accumulate. Do not add duplicate cleanup or force-dispose objects without tracing the game's asset ownership and current playback references.
- The newSound() input-copy reduction is separate from decoded PCM and OpenAL buffer lifetime. The historical PCM allocation failure can still occur under severe heap pressure; the encoded OGG copy removal is a peak-memory reduction, not a full fix.


## Game-side sound ownership boundary and current CI verification (2026-11-19)
- Rechecked the complete Git tree at branch head `5a7ebbd12b5ebb5ef78f7414c24649b863c22e5a`. This repository contains the Vita backend and native glue only; it does not contain the upstream Slay the Spire game source, its AssetManager/SoundCache implementation, or the game JAR. The only Java audio implementation here is `backend/src/main/java/com/stsvita/backend/VitaAudio.java`.
- Therefore the game-side question "does the game dispose every Sound on scene/run reset, or retain it in an asset cache?" cannot be answered from this repository. The native disposal audit proves only what happens after `Sound.dispose()` is called; it does not establish who calls it or when. Do not infer a leak or add global cleanup based on this missing evidence.
- Verified GitHub Actions directly through the repository workflow-runs collection: latest head `5a7ebbd...` has Host-side checks run `44490693740` (run #98), event `push`, conclusion `success`. This corrects earlier handoff wording that CI was still unconfirmed.
- The latest head has no corresponding VitaSDK build run in the recent workflow list. The most recent successful private Vita build is run `44455664046` on commit `d4f7d7ba8dc59a8f78c68cac892dd119f84bd009`; that is not the exact current head. Do not describe the latest SFX change as freshly VitaSDK-built unless a run for `5a7ebbd...` is found.
- Next useful path: if game-side Sound lifecycle must be proven, inspect the exact matching game source/JAR in the private build workflow environment without publishing or copying proprietary contents into this public repository. Otherwise keep the current backend changes and focus on measured native allocation/texture diagnostics. No physical Vita validation has been reported.


## SFX pointer-lifetime clarification and current validation (2026-11-19)
- Added a callsite comment in `VitaAudio.newSound()` documenting the key lifetime contract: `stsv_sound_load()` is synchronous through decode and OpenAL upload, so the Java byte array may be released after the native call returns. This is documentation only; no runtime behavior changed.
- Commit: `9b3d82fddaf71ba1ee1b72b80f9701659a60e152`.
- Host-side checks for this exact commit completed successfully: workflow run `44493342516`; build script syntax, build help, and `make test` all succeeded.
- Private cloud Vita build for this exact commit is running: workflow run `44493342429`. Do not report VitaSDK build success until it completes.
- The game-side Sound/AssetManager lifecycle remains uninspectable in this backend-only repository. Do not add speculative global Sound cleanup or alter asset lifetime.


## SFX optimization VitaSDK build verified (2026-11-19)
- Private cloud Vita build workflow run `44493342429` completed with conclusion `success` for code commit `9b3d82fddaf71ba1ee1b72b80f9701659a60e152` on `optimization/baseline-profiling`.
- All workflow steps succeeded: VitaSDK installation, private JAR retrieval, full/slim VPK build, native address/callsite reports, Japanese font payload and manifest verification, private release upload, and cleanup of temporary game-derived inputs.
- Full VPK: `STSVITA.vpk`, 195,855,501 bytes; SHA-256 `9c8cd84ded4c24a8f272d7de3653a66837364d709d8804e7b6462d77ae3386ed`; private asset ID `767128192`.
- Private release `teststs1111/stspsvita-build-private`, tag `temp-build-1`, was updated 2026-11-19 09:54 UTC. VPK URL: https://github.com/teststs1111/stspsvita-build-private/releases/download/temp-build-1/STSVITA.vpk
- Manifest: asset ID `767128506`, 2,504 bytes, SHA-256 `13a6337615be7a145508c3e9b6aa824e1a21ce8b1b0c348bfed332c351bc6d47`. Build ID `9b3d82fddaf7-bb3245af30c5`; requested languages `eng,jpn`; Japanese font payload 3 files / 49,929,880 bytes; `full_assets_match_staging=True`.
- Native linker map asset ID `767128637`, 7,858,582 bytes. Diagnostic workflow artifacts: `native-callsite-disassembly` ID `14516358380`, `native-address-symbol-report` ID `14516348244`, `native-linker-map` ID `14517700491`. These expire around 2026-11-20 09:54 UTC.
- This confirms VitaSDK compilation/packaging and private artifact upload, not real Vita runtime behavior. No device test has been reported; do not claim the Act-transition memory failure is fixed. No reinstall requested yet.
- Host checks on the same code commit also passed in run `44493342516`. The only code change in `9b3d82f` relative to the earlier audio optimization is a lifetime-contract comment; the earlier SFX encoded-data copy reduction is included in this build.
- Next: continue source-backed native memory diagnosis and preserve the diagnostic artifacts before expiry if useful. Only request the user to install/test when a specific device-side measurement is necessary and the build is the exact required diagnostic/fix build.
