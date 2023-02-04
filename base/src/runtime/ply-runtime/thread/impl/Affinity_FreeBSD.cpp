/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-runtime/Precomp.h>

#if PLY_KERNEL_FREEBSD

#include <ply-runtime/thread/impl/Affinity_FreeBSD.h>
#include <pthread_np.h>
#include <sys/param.h>
#include <sys/cpuset.h>
#include <map>
#include <string>
#include <fstream>
#include <stdio.h>

#include <assert.h>

namespace ply {

static bool setAffinityRaw(u32 logicalProcessor) {
    cpuset_t cpuSet;
    CPU_ZERO(&cpuSet);
    CPU_SET(logicalProcessor, &cpuSet);
    int rc = pthread_setaffinity_np(pthread_self(), sizeof(cpuSet), &cpuSet);
    return (rc == 0);
}

typedef struct _cpu_regs_t {
    u32 eax;
    u32 ebx;
    u32 ecx;
    u32 edx;
} cpu_regs_t, *pcpu_regs_t;

typedef struct _x2apic_level_t {
    u32 mask;
    bool reported;
} x2apic_level_t, *px2apic_level_t;

static u32 ctz(u32 v) {
    u32 c;
    if (v) {
        v = (v ^ (v - 1)) >> 1; /* Set v's trailing 0s to 1s and zero rest */
        for (c = 0; v; c++) {
            v >>= 1;
        }
    } else {
        c = CHAR_BIT * sizeof(v);
    }
    return c;
}

static bool cpuid(pcpu_regs_t regs) {
#ifdef PLY_CPU_X86
    static bool cpuid_support = false;
    if (!cpuid_support) {
        u32 pre_change, post_change;
        const u32 id_flag = 0x200000;
        asm("pushfl\n\t"     // Save %eflags to restore later.
            "pushfl\n\t"     // Push second copy, for manipulation.
            "popl %1\n\t"    // Pop it into post_change.
            "movl %1,%0\n\t" // Save copy in pre_change.
            "xorl %2,%1\n\t" // Tweak bit in post_change.
            "pushl %1\n\t"   // Push tweaked copy...
            "popfl\n\t"      // ... and pop it into %eflags.
            "pushfl\n\t"     // Did it change?  Push new %eflags...
            "popl %1\n\t"    // ... and pop it into post_change.
            "popfl"          // Restore original value.
            : "=&r"(pre_change), "=&r"(post_change)
            : "ir"(id_flag));
        if (((pre_change ^ post_change) & id_flag) == 0)
            return false;
        else
            cpuid_support = true;
    }
#endif
    asm volatile("cpuid"
                 : "=a"(regs->eax), "=b"(regs->ebx), "=c"(regs->ecx), "=d"(regs->edx)
                 : "0"(regs->eax), "2"(regs->ecx));
    return true;
}

static bool getTopology(s32* _physical, s32* _core) {
    static x2apic_level_t socket = {0}, core = {0}, thread = {0};
    cpu_regs_t regs = {0};
    u32 id;

    if (!cpuid(&regs))
        return false;

    // We need to have the x2APIC leaf in order to identify socket/core/thread.
    if (regs.eax < 0xB)
        return false;

    // Capture this local APIC ID first.
    regs = {0xB, 0, 0, 0};
    cpuid(&regs);
    id = regs.edx;

    // Now run through the x2APIC leaf and figure out the bit shfits we need to
    // get the physical/core/thread indices.
    if (!socket.reported) {
        for (u32 i = 0;; i++) {
            u32 level;

            regs = {0xB, 0, i, 0};

            if (!cpuid(&regs))
                return false;

            if (!(regs.eax || regs.ebx))
                break;

            level = (regs.ecx >> 8) & 0xff;

            if (level == 0)
                break;

            switch (level) {
                case 1: // Thread
                    thread.mask = ~((-1) << (regs.eax & 0x1f));
                    thread.reported = true;
                    break;
                case 2: // Core
                    core.mask = ~((-1) << (regs.eax & 0x1f));
                    core.reported = true;

                    socket.mask = (-1) ^ core.mask;
                    break;
            }
        }

        if (thread.reported && core.reported) {
            core.mask = core.mask ^ thread.mask;
        } else if (!core.reported && thread.reported) {
            core.mask = 0;
            socket.mask = (-1) ^ thread.mask;
        } else {
            return false;
        }

        socket.reported = true;
    }

    *_physical = (id & socket.mask) >> ctz(socket.mask);
    *_core = (id & core.mask) >> ctz(core.mask);

    return true;
}

Affinity_FreeBSD::Affinity_FreeBSD() : m_isAccurate(false), m_numHWThreads(0) {
    struct CoreInfoCollector {
        struct CoreID {
            s32 physical;
            s32 core;
            CoreID() : physical(-1), core(-1) {
            }
            bool operator<(const CoreID& other) const {
                if (physical != other.physical)
                    return physical < other.physical;
                return core < other.core;
            }
        };

        s32 logicalProcessor;
        CoreID coreID;
        std::map<CoreID, u32> coreIDToIndex;

        CoreInfoCollector() : logicalProcessor(-1) {
        }

        void flush(Affinity_FreeBSD& affinity) {
            if (logicalProcessor >= 0) {
                if (coreID.physical < 0 && coreID.core < 0) {
                    // On PowerPC Linux 3.2.0-4, /proc/cpuinfo outputs "processor", but not
                    // "physical id" or "core id". Emulate a single physical CPU with N cores:
                    coreID.physical = 0;
                    coreID.core = logicalProcessor;
                }
                std::map<CoreID, u32>::iterator iter = coreIDToIndex.find(coreID);
                u32 coreIndex;
                if (iter == coreIDToIndex.end()) {
                    coreIndex = (u32) affinity.m_coreIndexToInfo.size();
                    affinity.m_coreIndexToInfo.resize(coreIndex + 1);
                    coreIDToIndex[coreID] = coreIndex;
                } else {
                    coreIndex = iter->second;
                }
                affinity.m_coreIndexToInfo[coreIndex].hwThreadIndexToLogicalProcessor.push_back(
                    logicalProcessor);
                affinity.m_numHWThreads++;
            }
            logicalProcessor = -1;
            coreID = CoreID();
        }
    };

    CoreInfoCollector collector;
    u32 logicalProcessor = 0;
    for (u32 logicalProcessor = 0; setAffinityRaw(logicalProcessor); logicalProcessor++) {
        collector.logicalProcessor = logicalProcessor;
        getTopology(&collector.coreID.physical, &collector.coreID.core);
        collector.flush(*this);
    }

    m_isAccurate = (m_numHWThreads > 0);
    if (!m_isAccurate) {
        m_coreIndexToInfo.resize(1);
        m_coreIndexToInfo[0].hwThreadIndexToLogicalProcessor.push_back(0);
        m_numHWThreads = 1;
    }
}

bool Affinity_FreeBSD::setAffinity(ureg core, ureg hwThread) {
    u32 logicalProcessor = m_coreIndexToInfo[core].hwThreadIndexToLogicalProcessor[hwThread];
    return setAffinityRaw(logicalProcessor);
}

} // namespace ply

#endif // PLY_KERNEL_FREEBSD
