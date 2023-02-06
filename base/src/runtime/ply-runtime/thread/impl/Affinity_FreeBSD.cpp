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

static bool set_affinity_raw(u32 logical_processor) {
    cpuset_t cpu_set;
    CPU_ZERO(&cpu_set);
    CPU_SET(logical_processor, &cpu_set);
    int rc = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set), &cpu_set);
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

static bool get_topology(s32* _physical, s32* _core) {
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

        s32 logical_processor;
        CoreID core_id;
        std::map<CoreID, u32> core_idto_index;

        CoreInfoCollector() : logical_processor(-1) {
        }

        void flush(Affinity_FreeBSD& affinity) {
            if (logical_processor >= 0) {
                if (core_id.physical < 0 && core_id.core < 0) {
                    // On PowerPC Linux 3.2.0-4, /proc/cpuinfo outputs "processor", but
                    // not "physical id" or "core id". Emulate a single physical CPU
                    // with N cores:
                    core_id.physical = 0;
                    core_id.core = logical_processor;
                }
                std::map<CoreID, u32>::iterator iter = core_idto_index.find(core_id);
                u32 core_index;
                if (iter == core_idto_index.end()) {
                    core_index = (u32) affinity.m_coreIndexToInfo.size();
                    affinity.m_coreIndexToInfo.resize(core_index + 1);
                    core_idto_index[core_id] = core_index;
                } else {
                    core_index = iter->second;
                }
                affinity.m_coreIndexToInfo[core_index]
                    .hw_thread_index_to_logical_processor.push_back(logical_processor);
                affinity.m_numHWThreads++;
            }
            logical_processor = -1;
            core_id = CoreID();
        }
    };

    CoreInfoCollector collector;
    u32 logical_processor = 0;
    for (u32 logical_processor = 0; set_affinity_raw(logical_processor);
         logical_processor++) {
        collector.logical_processor = logical_processor;
        get_topology(&collector.core_id.physical, &collector.core_id.core);
        collector.flush(*this);
    }

    m_isAccurate = (m_numHWThreads > 0);
    if (!m_isAccurate) {
        m_coreIndexToInfo.resize(1);
        m_coreIndexToInfo[0].hw_thread_index_to_logical_processor.push_back(0);
        m_numHWThreads = 1;
    }
}

bool Affinity_FreeBSD::set_affinity(ureg core, ureg hw_thread) {
    u32 logical_processor =
        m_coreIndexToInfo[core].hw_thread_index_to_logical_processor[hw_thread];
    return set_affinity_raw(logical_processor);
}

} // namespace ply

#endif // PLY_KERNEL_FREEBSD
