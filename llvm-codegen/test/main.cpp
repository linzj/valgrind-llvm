#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <libvex.h>
#include <libvex_trc_values.h>
#include <libvex_guest_amd64.h>
#include "IRContext.h"

int machine_x86_have_mxcsr;
VexArchInfo info_host;
#define LOGE(...) fprintf(stderr, __VA_ARGS__)

static void
add_cache(VexCacheInfo* ci, VexCache cache)
{
    static unsigned int num_allocated = 0;

    if (ci->num_caches == num_allocated) {
        num_allocated += 6;
        ci->caches = static_cast<VexCache*>(realloc(ci->caches,
            num_allocated * sizeof *ci->caches));
    }

    if (ci->num_levels < cache.level)
        ci->num_levels = cache.level;
    ci->caches[ci->num_caches++] = cache;
}

#define add_icache(level, size, assoc, linesize)                                 \
    do {                                                                         \
        add_cache(ci, VEX_CACHE_INIT(INSN_CACHE, level, size, linesize, assoc)); \
    } while (0)

#define add_dcache(level, size, assoc, linesize)                                 \
    do {                                                                         \
        add_cache(ci, VEX_CACHE_INIT(DATA_CACHE, level, size, linesize, assoc)); \
    } while (0)

#define add_ucache(level, size, assoc, linesize)                                    \
    do {                                                                            \
        add_cache(ci, VEX_CACHE_INIT(UNIFIED_CACHE, level, size, linesize, assoc)); \
    } while (0)

#define add_itcache(level, size, assoc)                                 \
    do {                                                                \
        VexCache c = VEX_CACHE_INIT(INSN_CACHE, level, size, 0, assoc); \
        c.is_trace_cache = True;                                        \
        add_cache(ci, c);                                               \
    } while (0)

#define add_I1(size, assoc, linesize) add_icache(1, size, assoc, linesize)
#define add_D1(size, assoc, linesize) add_dcache(1, size, assoc, linesize)
#define add_U1(size, assoc, linesize) add_ucache(1, size, assoc, linesize)
#define add_I2(size, assoc, linesize) add_icache(2, size, assoc, linesize)
#define add_D2(size, assoc, linesize) add_dcache(2, size, assoc, linesize)
#define add_U2(size, assoc, linesize) add_ucache(2, size, assoc, linesize)
#define add_I3(size, assoc, linesize) add_icache(3, size, assoc, linesize)
#define add_D3(size, assoc, linesize) add_dcache(3, size, assoc, linesize)
#define add_U3(size, assoc, linesize) add_ucache(3, size, assoc, linesize)

#define add_I1T(size, assoc) \
    add_itcache(1, size, assoc)

static bool
intel_cache_info(int level, VexCacheInfo* ci)
{
    unsigned int cpuid1_eax;
    unsigned int cpuid1_ignore;
    int family;
    int model;
    unsigned char info[16];
    int i, j, trials;

    if (level < 2) {
        LOGE("warning: CPUID level < 2 for intel "
             "processor (%d)\n",
            level);
        return false;
    }

    /* family/model needed to distinguish code reuse (currently 0x49) */
    cpuid(1, 0, &cpuid1_eax, &cpuid1_ignore,
        &cpuid1_ignore, &cpuid1_ignore);
    family = (((cpuid1_eax >> 20) & 0xff) << 4) + ((cpuid1_eax >> 8) & 0xf);
    model = (((cpuid1_eax >> 16) & 0xf) << 4) + ((cpuid1_eax >> 4) & 0xf);

    cpuid(2, 0, (unsigned int*)&info[0], (unsigned int*)&info[4],
        (unsigned int*)&info[8], (unsigned int*)&info[12]);
    trials = info[0] - 1; /* AL register - bits 0..7 of %eax */
    info[0] = 0x0; /* reset AL */

    if (0 != trials) {
        LOGE("warning: non-zero CPUID trials for intel "
             "processor (%d)\n",
            trials);
        return false;
    }

    ci->num_levels = 0;
    ci->num_caches = 0;
    ci->icaches_maintain_coherence = True;
    ci->caches = NULL;

    for (i = 0; i < 16; i++) {

        switch (info[i]) {

        case 0x0: /* ignore zeros */
            break;

        /* TLB info, ignore */
        case 0x01:
        case 0x02:
        case 0x03:
        case 0x04:
        case 0x05:
        case 0x0b:
        case 0x4f:
        case 0x50:
        case 0x51:
        case 0x52:
        case 0x55:
        case 0x56:
        case 0x57:
        case 0x59:
        case 0x5a:
        case 0x5b:
        case 0x5c:
        case 0x5d:
        case 0x76:
        case 0xb0:
        case 0xb1:
        case 0xb2:
        case 0xb3:
        case 0xb4:
        case 0xba:
        case 0xc0:
        case 0xca:
            break;

        case 0x06:
            add_I1(8, 4, 32);
            break;
        case 0x08:
            add_I1(16, 4, 32);
            break;
        case 0x09:
            add_I1(32, 4, 64);
            break;
        case 0x30:
            add_I1(32, 8, 64);
            break;

        case 0x0a:
            add_D1(8, 2, 32);
            break;
        case 0x0c:
            add_D1(16, 4, 32);
            break;
        case 0x0d:
            add_D1(16, 4, 64);
            break;
        case 0x0e:
            add_D1(24, 6, 64);
            break;
        case 0x2c:
            add_D1(32, 8, 64);
            break;

        /* IA-64 info -- panic! */
        case 0x10:
        case 0x15:
        case 0x1a:
        case 0x88:
        case 0x89:
        case 0x8a:
        case 0x8d:
        case 0x90:
        case 0x96:
        case 0x9b:
            LOGE("IA-64 cache detected?!");
            EMASSERT(false);

        /* L3 cache info. */
        case 0x22:
            add_U3(512, 4, 64);
            break;
        case 0x23:
            add_U3(1024, 8, 64);
            break;
        case 0x25:
            add_U3(2048, 8, 64);
            break;
        case 0x29:
            add_U3(4096, 8, 64);
            break;
        case 0x46:
            add_U3(4096, 4, 64);
            break;
        case 0x47:
            add_U3(8192, 8, 64);
            break;
        case 0x4a:
            add_U3(6144, 12, 64);
            break;
        case 0x4b:
            add_U3(8192, 16, 64);
            break;
        case 0x4c:
            add_U3(12288, 12, 64);
            break;
        case 0x4d:
            add_U3(16384, 16, 64);
            break;
        case 0xd0:
            add_U3(512, 4, 64);
            break;
        case 0xd1:
            add_U3(1024, 4, 64);
            break;
        case 0xd2:
            add_U3(2048, 4, 64);
            break;
        case 0xd6:
            add_U3(1024, 8, 64);
            break;
        case 0xd7:
            add_U3(2048, 8, 64);
            break;
        case 0xd8:
            add_U3(4096, 8, 64);
            break;
        case 0xdc:
            add_U3(1536, 12, 64);
            break;
        case 0xdd:
            add_U3(3072, 12, 64);
            break;
        case 0xde:
            add_U3(6144, 12, 64);
            break;
        case 0xe2:
            add_U3(2048, 16, 64);
            break;
        case 0xe3:
            add_U3(4096, 16, 64);
            break;
        case 0xe4:
            add_U3(8192, 16, 64);
            break;
        case 0xea:
            add_U3(12288, 24, 64);
            break;
        case 0xeb:
            add_U3(18432, 24, 64);
            break;
        case 0xec:
            add_U3(24576, 24, 64);
            break;

        /* Described as "MLC" in intel documentation */
        case 0x21:
            add_U2(256, 8, 64);
            break;

        /* These are sectored, whatever that means */
        // FIXME: I did not find these in the intel docs
        case 0x39:
            add_U2(128, 4, 64);
            break;
        case 0x3c:
            add_U2(256, 4, 64);
            break;

        /* If a P6 core, this means "no L2 cache".
         If a P4 core, this means "no L3 cache".
         We don't know what core it is, so don't issue a warning.  To detect
         a missing L2 cache, we use 'L2_found'. */
        case 0x40:
            break;

        case 0x41:
            add_U2(128, 4, 32);
            break;
        case 0x42:
            add_U2(256, 4, 32);
            break;
        case 0x43:
            add_U2(512, 4, 32);
            break;
        case 0x44:
            add_U2(1024, 4, 32);
            break;
        case 0x45:
            add_U2(2048, 4, 32);
            break;
        case 0x48:
            add_U2(3072, 12, 64);
            break;
        case 0x4e:
            add_U2(6144, 24, 64);
            break;
        case 0x49:
            if (family == 15 && model == 6) {
                /* On Xeon MP (family F, model 6), this is for L3 */
                add_U3(4096, 16, 64);
            }
            else {
                add_U2(4096, 16, 64);
            }
            break;

        /* These are sectored, whatever that means */
        case 0x60:
            add_D1(16, 8, 64);
            break; /* sectored */
        case 0x66:
            add_D1(8, 4, 64);
            break; /* sectored */
        case 0x67:
            add_D1(16, 4, 64);
            break; /* sectored */
        case 0x68:
            add_D1(32, 4, 64);
            break; /* sectored */

        /* HACK ALERT: Instruction trace cache -- capacity is micro-ops based.
       * conversion to byte size is a total guess;  treat the 12K and 16K
       * cases the same since the cache byte size must be a power of two for
       * everything to work!.  Also guessing 32 bytes for the line size...
       */
        case 0x70: /* 12K micro-ops, 8-way */
            add_I1T(12, 8);
            break;
        case 0x71: /* 16K micro-ops, 8-way */
            add_I1T(16, 8);
            break;
        case 0x72: /* 32K micro-ops, 8-way */
            add_I1T(32, 8);
            break;

        /* not sectored, whatever that might mean */
        case 0x78:
            add_U2(1024, 4, 64);
            break;

        /* These are sectored, whatever that means */
        case 0x79:
            add_U2(128, 8, 64);
            break;
        case 0x7a:
            add_U2(256, 8, 64);
            break;
        case 0x7b:
            add_U2(512, 8, 64);
            break;
        case 0x7c:
            add_U2(1024, 8, 64);
            break;
        case 0x7d:
            add_U2(2048, 8, 64);
            break;
        case 0x7e:
            add_U2(256, 8, 128);
            break;
        case 0x7f:
            add_U2(512, 2, 64);
            break;
        case 0x80:
            add_U2(512, 8, 64);
            break;
        case 0x81:
            add_U2(128, 8, 32);
            break;
        case 0x82:
            add_U2(256, 8, 32);
            break;
        case 0x83:
            add_U2(512, 8, 32);
            break;
        case 0x84:
            add_U2(1024, 8, 32);
            break;
        case 0x85:
            add_U2(2048, 8, 32);
            break;
        case 0x86:
            add_U2(512, 4, 64);
            break;
        case 0x87:
            add_U2(1024, 8, 64);
            break;

        /* Ignore prefetch information */
        case 0xf0:
        case 0xf1:
            break;

        case 0xff:
            j = 0;
            cpuid(4, j++, (unsigned int*)&info[0], (unsigned int*)&info[4],
                (unsigned int*)&info[8], (unsigned int*)&info[12]);

            while ((info[0] & 0x1f) != 0) {
                unsigned int assoc = ((*(unsigned int*)&info[4] >> 22) & 0x3ff) + 1;
                unsigned int parts = ((*(unsigned int*)&info[4] >> 12) & 0x3ff) + 1;
                unsigned int line_size = (*(unsigned int*)&info[4] & 0x7ff) + 1;
                unsigned int sets = *(unsigned int*)&info[8] + 1;

                unsigned int size = assoc * parts * line_size * sets / 1024;

                switch ((info[0] & 0xe0) >> 5) {
                case 1:
                    switch (info[0] & 0x1f) {
                    case 1:
                        add_D1(size, assoc, line_size);
                        break;
                    case 2:
                        add_I1(size, assoc, line_size);
                        break;
                    case 3:
                        add_U1(size, assoc, line_size);
                        break;
                    default:
                        LOGE("warning: L1 cache of unknown type ignored\n");
                        break;
                    }
                    break;
                case 2:
                    switch (info[0] & 0x1f) {
                    case 1:
                        add_D2(size, assoc, line_size);
                        break;
                    case 2:
                        add_I2(size, assoc, line_size);
                        break;
                    case 3:
                        add_U2(size, assoc, line_size);
                        break;
                    default:
                        LOGE("warning: L2 cache of unknown type ignored\n");
                        break;
                    }
                    break;
                case 3:
                    switch (info[0] & 0x1f) {
                    case 1:
                        add_D3(size, assoc, line_size);
                        break;
                    case 2:
                        add_I3(size, assoc, line_size);
                        break;
                    case 3:
                        add_U3(size, assoc, line_size);
                        break;
                    default:
                        LOGE("warning: L3 cache of unknown type ignored\n");
                        break;
                    }
                    break;
                default:
                    LOGE("warning: L%u cache ignored\n",
                        (info[0] & 0xe0) >> 5);
                    break;
                }

                cpuid(4, j++, (unsigned int*)&info[0], (unsigned int*)&info[4],
                    (unsigned int*)&info[8], (unsigned int*)&info[12]);
            }
            break;
        case 0x63:
            // vbox intel cache.
            break;

        default:
            LOGE("warning: Unknown intel cache config value (0x%x), "
                 "ignoring\n",
                info[i]);
            EMASSERT(false);
            break;
        }
    }

    return true;
}

static int
decode_AMD_cache_L2_L3_assoc(int bits_15_12)
{
    /* Decode a L2/L3 associativity indication.  It is encoded
      differently from the I1/D1 associativity.  Returns 1
      (direct-map) as a safe but suboptimal result for unknown
      encodings. */
    switch (bits_15_12 & 0xF) {
    case 1:
        return 1;
    case 2:
        return 2;
    case 4:
        return 4;
    case 6:
        return 8;
    case 8:
        return 16;
    case 0xA:
        return 32;
    case 0xB:
        return 48;
    case 0xC:
        return 64;
    case 0xD:
        return 96;
    case 0xE:
        return 128;
    case 0xF: /* fully associative */
    case 0: /* L2/L3 cache or TLB is disabled */
    default:
        return 1;
    }
}

static bool
AMD_cache_info(VexCacheInfo* ci)
{
    unsigned int ext_level;
    unsigned int dummy, model;
    unsigned int I1i, D1i, L2i, L3i;
    unsigned int size, line_size, assoc;

    cpuid(0x80000000, 0, &ext_level, &dummy, &dummy, &dummy);

    if (0 == (ext_level & 0x80000000) || ext_level < 0x80000006) {
        LOGE("warning: ext_level < 0x80000006 for AMD "
             "processor (0x%x)\n",
            ext_level);
        return false;
    }

    cpuid(0x80000005, 0, &dummy, &dummy, &D1i, &I1i);
    cpuid(0x80000006, 0, &dummy, &dummy, &L2i, &L3i);

    cpuid(0x1, 0, &model, &dummy, &dummy, &dummy);

    /* Check for Duron bug */
    if (model == 0x630) {
        LOGV("warning: Buggy Duron stepping A0. "
             "Assuming L2 size=65536 bytes\n");
        L2i = (64 << 16) | (L2i & 0xffff);
    }

    ci->num_levels = 2;
    ci->num_caches = 3;
    ci->icaches_maintain_coherence = True;

    /* Check for L3 cache */
    if (((L3i >> 18) & 0x3fff) > 0) {
        ci->num_levels = 3;
        ci->num_caches = 4;
    }

    ci->caches = static_cast<VexCache*>(malloc(ci->num_caches * sizeof *ci->caches));

    // D1
    size = (D1i >> 24) & 0xff;
    assoc = (D1i >> 16) & 0xff;
    line_size = (D1i >> 0) & 0xff;
    ci->caches[0] = VEX_CACHE_INIT(DATA_CACHE, 1, size, line_size, assoc);

    // I1
    size = (I1i >> 24) & 0xff;
    assoc = (I1i >> 16) & 0xff;
    line_size = (I1i >> 0) & 0xff;
    ci->caches[1] = VEX_CACHE_INIT(INSN_CACHE, 1, size, line_size, assoc);

    // L2    Nb: different bits used for L2
    size = (L2i >> 16) & 0xffff;
    assoc = decode_AMD_cache_L2_L3_assoc((L2i >> 12) & 0xf);
    line_size = (L2i >> 0) & 0xff;
    ci->caches[2] = VEX_CACHE_INIT(UNIFIED_CACHE, 2, size, line_size, assoc);

    // L3, if any
    if (((L3i >> 18) & 0x3fff) > 0) {
        /* There's an L3 cache. */
        /* NB: the test in the if is "if L3 size > 0 ".  I don't know if
         this is the right way to test presence-vs-absence of L3.  I
         can't see any guidance on this in the AMD documentation. */
        size = ((L3i >> 18) & 0x3fff) * 512;
        assoc = decode_AMD_cache_L2_L3_assoc((L3i >> 12) & 0xf);
        line_size = (L3i >> 0) & 0xff;
        ci->caches[3] = VEX_CACHE_INIT(UNIFIED_CACHE, 3, size, line_size, assoc);
    }

    return true;
}

static bool
get_caches_from_CPUID(VexCacheInfo* ci)
{
    int ret;
    unsigned int i;
    unsigned int level;
    char vendor_id[13];

    EMASSERT(has_cpuid());

    cpuid(0, 0, &level, (unsigned int*)&vendor_id[0],
        (unsigned int*)&vendor_id[8], (unsigned int*)&vendor_id[4]);
    vendor_id[12] = '\0';

    if (0 == level) { // CPUID level is 0, early Pentium?
        return false;
    }

    /* Only handling intel and AMD chips... no Cyrix, Transmeta, etc */
    if (0 == strcmp(vendor_id, "GenuineIntel")) {
        ret = intel_cache_info(level, ci);
    }
    else if (0 == strcmp(vendor_id, "AuthenticAMD")) {
        ret = AMD_cache_info(ci);
    }
    else {
        LOGE("CPU vendor ID not recognised (%s)\n",
            vendor_id);
        return false;
    }

    /* Successful!  Convert sizes from KB to bytes */
    for (i = 0; i < ci->num_caches; ++i) {
        ci->caches[i].sizeB *= 1024;
    }

    return ret;
}

static bool
get_cache_info(VexArchInfo* vai)
{
    bool ret = get_caches_from_CPUID(&vai->hwcache_info);
    return ret;
}

int machine_get_hwcaps_host(VexArchInfo* vai)
{
    unsigned int eax, ebx, ecx, edx, max_extended;
    bool have_sse1, have_sse2, have_cx8, have_mmxext, have_lzcnt;
    char vstr[13];
    memset(vai, 0, sizeof(*vai));
    if (!has_cpuid()) {
        return false;
    }
    cpuid(0, 0, &eax, &ebx, &ecx, &edx);
    if (eax < 1)
        /* we can't ask for cpuid(x) for x > 0.  Give up. */
        return False;

    /* Get processor ID string, and max basic/extended index
       values. */
    memcpy(&vstr[0], &ebx, 4);
    memcpy(&vstr[4], &edx, 4);
    memcpy(&vstr[8], &ecx, 4);
    vstr[12] = 0;

    if (0 == strcmp(vstr, "GenuineIntel")) {
    }
    else if (0 == strcmp(vstr, "AuthenticAMD")) {
    }
    else {
        return false;
    }
    cpuid(0x80000000, 0, &eax, &ebx, &ecx, &edx);
    max_extended = eax;

    /* get capabilities bits into edx */
    cpuid(1, 0, &eax, &ebx, &ecx, &edx);

    have_sse1 = (edx & (1 << 25)) != 0; /* True => have sse insns */
    have_sse2 = (edx & (1 << 26)) != 0; /* True => have sse2 insns */

    /* cmpxchg8b is a minimum requirement now; if we don't have it we
       must simply give up.  But all CPUs since Pentium-I have it, so
       that doesn't seem like much of a restriction. */
    have_cx8 = (edx & (1 << 8)) != 0; /* True => have cmpxchg8b */
    if (!have_cx8)
        return false;

    /* Figure out if this is an AMD that can do MMXEXT. */
    have_mmxext = false;
    if (0 == strcmp(vstr, "AuthenticAMD")
        && max_extended >= 0x80000001) {
        cpuid(0x80000001, 0, &eax, &ebx, &ecx, &edx);
        /* Some older AMD processors support a sse1 subset (Integer SSE). */
        have_mmxext = !have_sse1 && ((edx & (1 << 22)) != 0);
    }

    /* Figure out if this is an AMD or Intel that can do LZCNT. */
    have_lzcnt = false;
    if ((0 == strcmp(vstr, "AuthenticAMD")
            || 0 == strcmp(vstr, "GenuineIntel"))
        && max_extended >= 0x80000001) {
        cpuid(0x80000001, 0, &eax, &ebx, &ecx, &edx);
        have_lzcnt = (ecx & (1 << 5)) != 0; /* True => have LZCNT */
    }

    /* Intel processors don't define the mmxext extension, but since it
       is just a sse1 subset always define it when we have sse1. */
    if (have_sse1)
        have_mmxext = True;

    vai->endness = VexEndnessLE;
    if (have_sse2 && have_sse1 && have_mmxext) {
        vai->hwcaps = VEX_HWCAPS_X86_MMXEXT;
        vai->hwcaps |= VEX_HWCAPS_X86_SSE1;
        vai->hwcaps |= VEX_HWCAPS_X86_SSE2;
        if (have_lzcnt)
            vai->hwcaps |= VEX_HWCAPS_X86_LZCNT;
        machine_x86_have_mxcsr = 1;
    }
    else if (have_sse1 && have_mmxext) {
        vai->hwcaps = VEX_HWCAPS_X86_MMXEXT;
        vai->hwcaps |= VEX_HWCAPS_X86_SSE1;
        machine_x86_have_mxcsr = 1;
    }
    else if (have_mmxext) {
        vai->hwcaps = VEX_HWCAPS_X86_MMXEXT; /*integer only sse1 subset*/
        machine_x86_have_mxcsr = 0;
    }
    else {
        vai->hwcaps = 0; /*baseline - no sse at all*/
        machine_x86_have_mxcsr = 0;
    }

    return get_cache_info(vai);
}

extern "C" {
void yyparse(IRContext*);
typedef void* yyscan_t;
int yylex_init(yyscan_t* scanner);

int yylex_init_extra(struct IRContext* user_defined, yyscan_t* scanner);

int yylex_destroy(yyscan_t yyscanner);
}

int main()
{
    VexControl clo_vex_control;
    LibVEX_default_VexControl(&clo_vex_control);
    clo_vex_control.iropt_unroll_thresh = 4000;
    clo_vex_control.guest_max_insns = 1000;
    clo_vex_control.guest_chase_thresh = 999;
    clo_vex_control.iropt_register_updates = VexRegUpdSpAtMemAccess;

    // use define control to init vex.
    LibVEX_Init(&failure_exit, &log_bytes,
        1, /* debug_paranoia */
        False,
        &clo_vex_control);
    if (!machine_get_hwcaps_host(&info_host)) {
        LOGE("%s: get hwcap fails.\n", __FUNCTION__);
    }
    IRContext context;
    yylex_init_extra(&context, &context.m_scanner);
    yyparse(&context);
    yylex_destroy(context.m_scanner);
    return 0;
}
