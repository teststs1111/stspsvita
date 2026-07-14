/* Process startup: memory + vitaGL init before the Java world boots. */

#include <vitaGL.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/power.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>

/* The newlib malloc heap holds TeaVM's Java heap (144MB, committed up front
 * by TEAVM_PSP virtualAlloc=malloc) + native malloc (stb decode + FreeType)
 * + the free space vitaGL falls back to for PER-DRAW vertex buffers.
 * 272MB keeps most of the proven-stable 280MB arena while giving vitaGL
 * about 8MB more dedicated texture space. This compensates for the
 * full-resolution UI/chest set without repeating the unsafe 260MB experiment
 * or forcing 9MB of persistent textures into the draw-buffer heap. */
int _newlib_heap_size_user = 272 * 1024 * 1024;

/* TeaVM's generated main, renamed via -Dmain=stsv_java_main (CMake). */
int stsv_java_main(int argc, char **argv);

/* Enables SceCtrl analog + SceTouch front-panel sampling (input_glue.c). */
void stsv_input_init(void);

/* Replaces vitaGL's spinner with the generated title loading frame. */
void stsv_show_loading_screen(void);

/* Opens the OpenAL device/context for VitaAudio (audio_glue.c). */
void stsv_audio_init(void);

int main(int argc, char **argv) {
    /* The port is developed/tested at max clocks (PSVshell equivalent). */
    scePowerSetArmClockFrequency(444);
    scePowerSetBusClockFrequency(222);
    scePowerSetGpuClockFrequency(222);
    scePowerSetGpuXbarClockFrequency(166);

    /* Game data (saves, preferences, info.displayconfig) lives on ux0:.
     * The game reads/writes some of it via cwd-relative paths, so move
     * cwd there; assets are always absolute (app0:/assets/). */
    mkdir("ux0:data", 0777);
    mkdir("ux0:data/stsvita", 0777);
    chdir("ux0:data/stsvita");

    /* Capture the game's stdout + the C runtime's stderr (TeaVM prints an
     * uncaught-exception trace there right before abort()) to a file on ux0:,
     * since the Vita has no console. Both streams open in append mode (Vita
     * newlib has no dup2) so their O_APPEND writes interleave at EOF without
     * clobbering; truncate once up front, unbuffered so it survives abort(). */
    {
        FILE *f = fopen("ux0:data/stsvita/stsvita.log", "w");
        if (f) fclose(f);
    }
    freopen("ux0:data/stsvita/stsvita.log", "a", stdout);
    freopen("ux0:data/stsvita/stsvita.log", "a", stderr);
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    /* Give vitaGL large DEDICATED pools (its own memblocks) so texture
     * overflow lands there instead of spilling into the malloc heap
     * (VGL_MEM_EXTERNAL) and colliding with stb decode buffers. Capacity:
     * CDRAM 100MB (separate from the 256MB RAM budget) + PHYCONT 20MB + a
     * 64MB RAM pool + 8MB legacy = ~192MB, which with DXT5-compressed atlases
     * holds StS's full texture set. */
    /* vglInitExtended auto-grabs all available CDRAM + PHYCONT (no over-request
     * failure) and carves a RAM pool from the USER_RW left after the newlib
     * heap, leaving `ram_threshold` bytes free for GXM/stacks. Textures overflow
     * into that RAM pool instead of the malloc heap. */
    GLboolean ok = vglInitExtended(8 * 1024 * 1024, 960, 544,
                                   16 * 1024 * 1024, SCE_GXM_MULTISAMPLE_NONE);
    fprintf(stderr,
        "[boot] vglInitExtended -> %d | pools: VRAM=%dMB RAM=%dMB PHYCONT=%dMB\n",
        (int) ok,
        (int) (vglMemTotal(VGL_MEM_VRAM) >> 20),
        (int) (vglMemTotal(VGL_MEM_RAM) >> 20),
        (int) (vglMemTotal(VGL_MEM_SLOW) >> 20));

    stsv_show_loading_screen();

    stsv_input_init();
    stsv_audio_init();

    int rc = stsv_java_main(argc, argv);

    /* This vitaGL build exposes no vglEnd(); process teardown on
     * sceKernelExitProcess reclaims the GL context and memory pools. */
    sceKernelExitProcess(rc);
    return rc;
}
