/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-runtime/Core.h>
#include <atomic>
#include <mutex>
#include <condition_variable>

#if PLY_TARGET_POSIX
#include <errno.h>
#if PLY_KERNEL_MACH
#include <mach/mach.h>
#else
#include <semaphore.h>
#endif
#endif

namespace ply {

//  ▄▄▄▄▄                    ▄▄
//  ██  ██  ▄▄▄▄  ▄▄▄▄▄   ▄▄▄██  ▄▄▄▄  ▄▄▄▄▄▄▄
//  ██▀▀█▄  ▄▄▄██ ██  ██ ██  ██ ██  ██ ██ ██ ██
//  ██  ██ ▀█▄▄██ ██  ██ ▀█▄▄██ ▀█▄▄█▀ ██ ██ ██
//

// xoroshiro128** 1.0 generator seeded using misc. information from the environment.
// Based on http://xorshift.di.unimi.it/
class Random {
private:
    u64 s[2];

public:
    PLY_DLL_ENTRY Random();
    PLY_DLL_ENTRY Random(u64 seed); // Explicit seed
    PLY_DLL_ENTRY u64 next64();
    PLY_INLINE u32 next32() {
        return (u32) next64();
    }
    PLY_INLINE u16 next16() {
        return (u16) next64();
    }
    PLY_INLINE u8 next8() {
        return (u8) next64();
    }
    PLY_INLINE float nextFloat() {
        return next32() / 4294967295.f;
    }
};

//   ▄▄▄▄  ▄▄▄▄▄  ▄▄  ▄▄ ▄▄▄▄▄▄ ▄▄
//  ██  ▀▀ ██  ██ ██  ██   ██   ▄▄ ▄▄▄▄▄▄▄   ▄▄▄▄  ▄▄▄▄▄
//  ██     ██▀▀▀  ██  ██   ██   ██ ██ ██ ██ ██▄▄██ ██  ▀▀
//  ▀█▄▄█▀ ██     ▀█▄▄█▀   ██   ██ ██ ██ ██ ▀█▄▄▄  ██
//

struct CPUTimer {
    struct Duration {
        using Ticks = std::chrono::high_resolution_clock::duration;
        Ticks ticks;
        PLY_INLINE operator s64() const {
            return s64(ticks.count());
        }
    };

    struct Point {
        using Tick = std::chrono::high_resolution_clock::time_point;
        Tick tick;
        PLY_INLINE Point(s64 v = 0) : tick{Duration::Ticks{v}} {
        }
        PLY_INLINE Point(const Tick& tick) : tick{tick} {
        }
        PLY_INLINE Point operator+(Duration d) const {
            return {tick + d.ticks};
        }
        PLY_INLINE Duration operator-(Point b) const {
            return {tick - b.tick};
        }
        PLY_INLINE bool operator<(Point b) const {
            return tick < b.tick;
        }
        PLY_INLINE bool operator<=(Point b) const {
            return tick <= b.tick;
        }
        PLY_INLINE bool operator>(Point b) const {
            return tick > b.tick;
        }
        PLY_INLINE bool operator>=(Point b) const {
            return tick >= b.tick;
        }
        PLY_INLINE bool operator==(Point b) const {
            return tick == b.tick;
        }
    };

    PLY_INLINE static Point get() {
        return {std::chrono::high_resolution_clock::now()};
    }

    struct Converter {
        PLY_INLINE Converter() {
        }
        static PLY_INLINE float toSeconds(Duration duration) {
            return std::chrono::duration_cast<std::chrono::duration<float>>(
                       duration.ticks)
                .count();
        }
        Duration toDuration(float seconds) const {
            return {std::chrono::duration_cast<Duration::Ticks>(
                std::chrono::duration<float>{seconds})};
        }
    };
};

//  ▄▄▄▄▄          ▄▄          ▄▄▄▄▄▄ ▄▄
//  ██  ██  ▄▄▄▄  ▄██▄▄  ▄▄▄▄    ██   ▄▄ ▄▄▄▄▄▄▄   ▄▄▄▄
//  ██  ██  ▄▄▄██  ██   ██▄▄██   ██   ██ ██ ██ ██ ██▄▄██
//  ██▄▄█▀ ▀█▄▄██  ▀█▄▄ ▀█▄▄▄    ██   ██ ██ ██ ██ ▀█▄▄▄
//

struct DateTime {
    s32 year = 0;
    u8 month = 0;
    u8 day = 0;
    u8 weekday = 0; // [0, 6] -> [Sun, Sat]
    u8 hour = 0;
    u8 minute = 0;
    u8 second = 0;
    s8 timeZoneHour = 0;
    u8 timeZoneMinute = 0;
    u32 microseconds = 0;

    // Number of microseconds since January 1, 1970 at 00:00:00 UTC.
    static PLY_DLL_ENTRY s64 getCurrentEpochMicroseconds();

    // Conversion
    static PLY_DLL_ENTRY DateTime fromEpochMicroseconds(s64 us);
    PLY_DLL_ENTRY s64 toEpochMicroseconds() const;
};

//  ▄▄▄▄▄▄ ▄▄▄▄ ▄▄▄▄▄
//    ██    ██  ██  ██
//    ██    ██  ██  ██
//    ██   ▄██▄ ██▄▄█▀
//

#if PLY_TARGET_WIN32
// ┏━━━━━━━━━┓
// ┃  Win32  ┃
// ┗━━━━━━━━━┛
using TID = u32;
using PID = u32;

static TID getCurrentThreadID() {
#if PLY_CPU_X64
    return ((DWORD*) __readgsqword(48))[18]; // Read directly from the TIB
#elif PLY_CPU_X86
    return ((DWORD*) __readfsdword(24))[9]; // Read directly from the TIB
#else
    return GetCurrentThreadID();
#endif
}

static PID getCurrentProcessID() {
#if PLY_CPU_X64
    return ((DWORD*) __readgsqword(48))[16]; // Read directly from the TIB
#elif PLY_CPU_X86
    return ((DWORD*) __readfsdword(24))[8]; // Read directly from the TIB
#else
    return GetCurrentProcessID();
#endif
}

#elif PLY_KERNEL_MACH
// ┏━━━━━━━━┓
// ┃  Mach  ┃
// ┗━━━━━━━━┛
class TID {
public:
    using TID = SizedInt<sizeof(thread_port_t)>::Unsigned;
    using PID = SizedInt<sizeof(pid_t)>::Unsigned;

    static TID getCurrentThreadID() {
        return pthread_mach_thread_np(pthread_self());
    }

    static PID getCurrentProcessID() {
        return getpid();
    }
};

#elif PLY_TARGET_POSIX
// ┏━━━━━━━━━┓
// ┃  POSIX  ┃
// ┗━━━━━━━━━┛
class TID {
public:
#ifdef PLY_TARGET_MINGW
    typedef uptr TID;
#else
    // This only works when pthread_t is an integer type, as it is in the GNU C Library
    // >= 2.3.3. If that's not true for your Pthreads library, we'll need to extend
    // Plywood to fetch TIDs from somewehere else in the environment.
    using TID = SizedInt<sizeof(pthread_t)>::Unsigned;
#endif
    using PID = SizedInt<sizeof(pid_t)>::Unsigned;

    static TID getCurrentThreadID() {
        // FIXME: On Linux, would the kernel task ID be more useful for debugging?
        // If so, detect NPTL at compile time and create TID_NPTL.h which uses gettid()
        // instead.
#ifdef PLY_KERNEL_FREEBSD
        return pthread_getthreadid_np();
#elif PLY_TARGET_MINGW
        return (TID) pthread_self().p;
#else
        return pthread_self();
#endif
    }

    static PID getCurrentProcessID() {
        return getpid();
    }
};

#endif // TID

//   ▄▄▄▄    ▄▄▄   ▄▄▄ ▄▄        ▄▄  ▄▄
//  ██  ██  ██    ██   ▄▄ ▄▄▄▄▄  ▄▄ ▄██▄▄ ▄▄  ▄▄
//  ██▀▀██ ▀██▀▀ ▀██▀▀ ██ ██  ██ ██  ██   ██  ██
//  ██  ██  ██    ██   ██ ██  ██ ██  ▀█▄▄ ▀█▄▄██
//                                         ▄▄▄█▀

#if PLY_TARGET_WIN32
// ┏━━━━━━━━━┓
// ┃  Win32  ┃
// ┗━━━━━━━━━┛
class Affinity {
private:
    typedef ULONG_PTR AffinityMask;
    static const ureg MaxHWThreads = sizeof(AffinityMask) * 8;

    bool m_isAccurate;
    ureg m_numPhysicalCores;
    ureg m_numHWThreads;
    AffinityMask m_physicalCoreMasks[MaxHWThreads];

public:
    PLY_DLL_ENTRY Affinity();

    bool isAccurate() const {
        return m_isAccurate;
    }

    u32 getNumPhysicalCores() const {
        return static_cast<u32>(m_numPhysicalCores);
    }

    u32 getNumHWThreads() const {
        return static_cast<u32>(m_numHWThreads);
    }

    u32 getNumHWThreadsForCore(ureg core) const {
        PLY_ASSERT(core < m_numPhysicalCores);
        return static_cast<u32>(countSetBits(m_physicalCoreMasks[core]));
    }

    PLY_DLL_ENTRY bool setAffinity(ureg core, ureg hwThread);
};

#elif PLY_KERNEL_LINUX
// ┏━━━━━━━━━┓
// ┃  Linux  ┃
// ┗━━━━━━━━━┛
class Affinity_Linux {
private:
    struct CoreInfo {
        std::vector<u32> hwThreadIndexToLogicalProcessor;
    };
    bool m_isAccurate;
    std::vector<CoreInfo> m_coreIndexToInfo;
    u32 m_numHWThreads;

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

        void flush(Affinity_Linux& affinity) {
            if (logicalProcessor >= 0) {
                if (coreID.physical < 0 && coreID.core < 0) {
                    // On PowerPC Linux 3.2.0-4, /proc/cpuinfo outputs "processor", but
                    // not "physical id" or "core id". Emulate a single physical CPU
                    // with N cores:
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
                affinity.m_coreIndexToInfo[coreIndex]
                    .hwThreadIndexToLogicalProcessor.push_back(logicalProcessor);
                affinity.m_numHWThreads++;
            }
            logicalProcessor = -1;
            coreID = CoreID();
        }
    };

public:
    Affinity_Linux();

    bool isAccurate() const {
        return m_isAccurate;
    }

    u32 getNumPhysicalCores() const {
        return m_coreIndexToInfo.size();
    }

    u32 getNumHWThreads() const {
        return m_numHWThreads;
    }

    u32 getNumHWThreadsForCore(ureg core) const {
        return m_coreIndexToInfo[core].hwThreadIndexToLogicalProcessor.size();
    }

    bool setAffinity(ureg core, ureg hwThread);
};

#elif PLY_KERNEL_FREEBSD
// ┏━━━━━━━━━━━┓
// ┃  FreeBSD  ┃
// ┗━━━━━━━━━━━┛
class Affinity {
private:
    struct CoreInfo {
        std::vector<u32> hwThreadIndexToLogicalProcessor;
    };
    bool m_isAccurate;
    std::vector<CoreInfo> m_coreIndexToInfo;
    u32 m_numHWThreads;

public:
    Affinity();

    bool isAccurate() const {
        return m_isAccurate;
    }

    u32 getNumPhysicalCores() const {
        return m_coreIndexToInfo.size();
    }

    u32 getNumHWThreads() const {
        return m_numHWThreads;
    }

    u32 getNumHWThreadsForCore(ureg core) const {
        return m_coreIndexToInfo[core].hwThreadIndexToLogicalProcessor.size();
    }

    bool setAffinity(ureg core, ureg hwThread);
};

#elif PLY_KERNEL_MACH
// ┏━━━━━━━━┓
// ┃  Mach  ┃
// ┗━━━━━━━━┛
class Affinity {
private:
    bool m_isAccurate;
    u32 m_numHWThreads;
    u32 m_numPhysicalCores;
    u32 m_hwThreadsPerCore;

public:
    Affinity()
        : m_isAccurate(false), m_numHWThreads(1), m_numPhysicalCores(1),
          m_hwThreadsPerCore(1) {
        int count;
        // Get # of HW threads
        size_t countLen = sizeof(count);
        if (sysctlbyname("hw.logicalcpu", &count, &countLen, NULL, 0) == 0) {
            if (count > 0) {
                m_numHWThreads = (u32) count;
                // Get # of physical cores
                size_t countLen = sizeof(count);
                if (sysctlbyname("hw.physicalcpu", &count, &countLen, NULL, 0) == 0) {
                    if (count > 0) {
                        m_numPhysicalCores = count;
                        m_hwThreadsPerCore = u32(m_numHWThreads / count);
                        if (m_hwThreadsPerCore < 1)
                            m_hwThreadsPerCore = 1;
                        else
                            m_isAccurate = true;
                    }
                }
            }
        }
    }

    bool isAccurate() const {
        return m_isAccurate;
    }

    u32 getNumPhysicalCores() const {
        return m_numPhysicalCores;
    }

    u32 getNumHWThreads() const {
        return m_numHWThreads;
    }

    u32 getNumHWThreadsForCore(ureg core) const {
        PLY_ASSERT(core < m_numPhysicalCores);
        return m_hwThreadsPerCore;
    }

    bool setAffinity(ureg core, ureg hwThread) {
        PLY_ASSERT(core < m_numPhysicalCores);
        PLY_ASSERT(hwThread < m_hwThreadsPerCore);
        u32 index = core * m_hwThreadsPerCore + hwThread;
        thread_t thread = mach_thread_self();
        thread_affinity_policy_data_t policyInfo = {(integer_t) index};
        // Note: The following returns KERN_NOT_SUPPORTED on iOS. (Tested on iOS
        // 9.2.)
        kern_return_t result = thread_policy_set(thread, THREAD_AFFINITY_POLICY,
                                                 (thread_policy_t) &policyInfo,
                                                 THREAD_AFFINITY_POLICY_COUNT);
        return (result == KERN_SUCCESS);
    }
};

#endif // Affinity

//  ▄▄▄▄▄▄                        ▄▄▄          ▄▄
//    ██    ▄▄▄▄  ▄▄▄▄▄▄▄  ▄▄▄▄▄   ██   ▄▄▄▄  ▄██▄▄  ▄▄▄▄   ▄▄▄▄
//    ██   ██▄▄██ ██ ██ ██ ██  ██  ██   ▄▄▄██  ██   ██▄▄██ ▀█▄▄▄
//    ██   ▀█▄▄▄  ██ ██ ██ ██▄▄█▀ ▄██▄ ▀█▄▄██  ▀█▄▄ ▀█▄▄▄   ▄▄▄█▀
//                         ██

// By default, View<T> is an alias for T itself. Specializations for String and Array
// are defined later.
namespace traits {
template <typename T>
struct View {
    using Type = T;
};
} // namespace traits

template <typename T>
using View = typename traits::View<T>::Type;

namespace subst {

// ┏━━━━━━━━━━━━━━━━━┓
// ┃  createDefault  ┃
// ┗━━━━━━━━━━━━━━━━━┛
template <typename T, std::enable_if_t<std::is_arithmetic<T>::value, int> = 0>
T createDefault() {
    return 0;
}

template <typename T,
          std::enable_if_t<
              !std::is_arithmetic<T>::value && !std::is_same<T, void>::value, int> = 0>
T createDefault() {
    return {};
}

template <typename T, std::enable_if_t<std::is_same<T, void>::value, int> = 0>
T createDefault() {
}

// ┏━━━━━━━━━━━━━━━━━━━┓
// ┃  unsafeConstruct  ┃
// ┗━━━━━━━━━━━━━━━━━━━┛
template <typename T,
          std::enable_if_t<std::is_default_constructible<T>::value, int> = 0>
void unsafeConstruct(T* obj) {
    new (obj) T;
}

template <typename T,
          std::enable_if_t<!std::is_default_constructible<T>::value, int> = 0>
void unsafeConstruct(T* obj) {
    // Not constructible
    PLY_FORCE_CRASH();
}

// ┏━━━━━━━━━━━━━━━━━━┓
// ┃  createByMember  ┃
// ┗━━━━━━━━━━━━━━━━━━┛
PLY_MAKE_WELL_FORMEDNESS_CHECK_1(HasCreate, T0::create())

template <typename T, std::enable_if_t<HasCreate<T>, int> = 0>
T* createByMember() {
    return T::create();
}

template <
    typename T,
    std::enable_if_t<!HasCreate<T> && std::is_default_constructible<T>::value, int> = 0>
T* createByMember() {
    return new T;
}

template <typename T,
          std::enable_if_t<!HasCreate<T> && !std::is_default_constructible<T>::value,
                           int> = 0>
T* createByMember() {
    // Not constructible
    PLY_FORCE_CRASH();
    return nullptr;
}

// ┏━━━━━━━━━━━━━━━━━━━┓
// ┃  destroyByMember  ┃
// ┗━━━━━━━━━━━━━━━━━━━┛
PLY_MAKE_WELL_FORMEDNESS_CHECK_1(HasDestroyMember, std::declval<T0>().destroy())
PLY_MAKE_WELL_FORMEDNESS_CHECK_1(HasNamespaceDestroy, destroy((T0*) nullptr))

template <typename T, std::enable_if_t<HasDestroyMember<T>, int> = 0>
void destroyByMember(T* obj) {
    if (obj) {
        obj->destroy();
    }
}

template <typename T,
          std::enable_if_t<!HasDestroyMember<T> && HasNamespaceDestroy<T>, int> = 0>
void destroyByMember(T* obj) {
    if (obj) {
        destroy(obj);
    }
}

template <typename T,
          std::enable_if_t<!HasDestroyMember<T> && !HasNamespaceDestroy<T>, int> = 0>
void destroyByMember(T* obj) {
    delete obj; // Passing nullptr to delete is allowed
}

// ┏━━━━━━━━━━━━━━━━━━━━┓
// ┃  destructByMember  ┃
// ┗━━━━━━━━━━━━━━━━━━━━┛
PLY_MAKE_WELL_FORMEDNESS_CHECK_1(HasDestruct, std::declval<T0>().destruct())

template <typename T, std::enable_if_t<HasDestruct<T>, int> = 0>
void destructByMember(T* obj) {
    obj->destruct();
}

template <typename T, std::enable_if_t<!HasDestruct<T>, int> = 0>
void destructByMember(T* obj) {
    obj->~T();
}

// ┏━━━━━━━━━━━━━━━━━━━━━━━┓
// ┃  unsafeMoveConstruct  ┃
// ┗━━━━━━━━━━━━━━━━━━━━━━━┛
template <typename T, std::enable_if_t<std::is_move_constructible<T>::value, int> = 0>
void unsafeMoveConstruct(T* dst, T* src) {
    new (dst) T{std::move(*src)};
}

template <typename T, std::enable_if_t<!std::is_move_constructible<T>::value, int> = 0>
void unsafeMoveConstruct(T* dst, T* src) {
    // Not move constructible
    PLY_FORCE_CRASH();
}

// ┏━━━━━━━━━━━━━━┓
// ┃  unsafeCopy  ┃
// ┗━━━━━━━━━━━━━━┛
template <typename T, std::enable_if_t<std::is_copy_assignable<T>::value, int> = 0>
void unsafeCopy(T* dst, const T* src) {
    *dst = *src;
}

template <typename T, std::enable_if_t<!std::is_copy_assignable<T>::value, int> = 0>
void unsafeCopy(T* dst, const T* src) {
    // Not copy assignable
    PLY_FORCE_CRASH();
}

// ┏━━━━━━━━━━━━━━┓
// ┃  unsafeMove  ┃
// ┗━━━━━━━━━━━━━━┛
template <typename T, std::enable_if_t<std::is_move_assignable<T>::value, int> = 0>
void unsafeMove(T* dst, T* src) {
    *dst = std::move(*src);
}

template <typename T, std::enable_if_t<!std::is_move_assignable<T>::value, int> = 0>
void unsafeMove(T* dst, T* src) {
    // Not move assignable
    PLY_FORCE_CRASH();
}

// ┏━━━━━━━━━━━━━━━━━━┓
// ┃  constructArray  ┃
// ┗━━━━━━━━━━━━━━━━━━┛
template <typename T,
          std::enable_if_t<std::is_trivially_default_constructible<T>::value, int> = 0>
void constructArray(T* items, s32 size) {
    // Trivially constructible
}

template <typename T,
          std::enable_if_t<!std::is_trivially_default_constructible<T>::value, int> = 0>
void constructArray(T* items, s32 size) {
    // Explicitly constructble
    while (size-- > 0) {
        new (items++) T;
    }
}

// ┏━━━━━━━━━━━━━━━━━┓
// ┃  destructArray  ┃
// ┗━━━━━━━━━━━━━━━━━┛
template <typename T,
          std::enable_if_t<std::is_trivially_destructible<T>::value, int> = 0>
void destructArray(T* items, s32 size) {
    // Trivially destructible
}

template <typename T,
          std::enable_if_t<!std::is_trivially_destructible<T>::value, int> = 0>
void destructArray(T* items, s32 size) {
    // Explicitly destructble
    while (size-- > 0) {
        (items++)->~T();
    }
}

// ┏━━━━━━━━━━━━━━━━━━━━━━┓
// ┃  constructArrayFrom  ┃
// ┗━━━━━━━━━━━━━━━━━━━━━━┛
template <typename T,
          std::enable_if_t<std::is_trivially_copy_constructible<T>::value, int> = 0>
void constructArrayFrom(T* dst, const T* src, s32 size) {
    // Trivially copy constructible
    memcpy(dst, src, sizeof(T) * size);
}

template <typename T, typename U,
          std::enable_if_t<std::is_constructible<T, const U&>::value, int> = 0>
void constructArrayFrom(T* dst, const U* src, s32 size) {
    // Invoke constructor explicitly on each item
    while (size-- > 0) {
        // Use parentheses instead of curly braces to avoid narrowing conversion errors.
        new (dst++) T(*src++);
    }
}

// ┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
// ┃  unsafeConstructArrayFrom  ┃
// ┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
template <typename T, typename U,
          std::enable_if_t<std::is_constructible<T, const U&>::value, int> = 0>
void unsafeConstructArrayFrom(T* dst, const U* src, s32 size) {
    constructArrayFrom(dst, src, size);
}

template <typename T, typename U,
          std::enable_if_t<!std::is_constructible<T, const U&>::value, int> = 0>
void unsafeConstructArrayFrom(T* dst, const U* src, s32 size) {
    PLY_FORCE_CRASH();
}

// ┏━━━━━━━━━━━━━━━━━━━━━━┓
// ┃  moveConstructArray  ┃
// ┗━━━━━━━━━━━━━━━━━━━━━━┛
template <typename T,
          std::enable_if_t<std::is_trivially_move_constructible<T>::value, int> = 0>
void moveConstructArray(T* dst, const T* src, s32 size) {
    // Trivially move constructible
    memcpy(dst, src, sizeof(T) * size);
}

template <typename T, typename U,
          std::enable_if_t<std::is_constructible<T, U&&>::value, int> = 0>
void moveConstructArray(T* dst, U* src, s32 size) {
    // Explicitly move constructible
    while (size-- > 0) {
        new (dst++) T{std::move(*src++)};
    }
}

// ┏━━━━━━━━━━━━━┓
// ┃  copyArray  ┃
// ┗━━━━━━━━━━━━━┛
template <typename T,
          std::enable_if_t<std::is_trivially_copy_assignable<T>::value, int> = 0>
void copyArray(T* dst, const T* src, s32 size) {
    // Trivially copy assignable
    memcpy(dst, src, sizeof(T) * size);
}

template <typename T,
          std::enable_if_t<!std::is_trivially_copy_assignable<T>::value, int> = 0>
void copyArray(T* dst, const T* src, s32 size) {
    // Explicitly copy assignable
    while (size-- > 0) {
        *dst++ = *src++;
    }
}

// ┏━━━━━━━━━━━━━┓
// ┃  moveArray  ┃
// ┗━━━━━━━━━━━━━┛
template <typename T,
          std::enable_if_t<std::is_trivially_move_assignable<T>::value, int> = 0>
void moveArray(T* dst, const T* src, s32 size) {
    // Trivially move assignable
    memcpy(dst, src, sizeof(T) * size);
}

template <typename T,
          std::enable_if_t<!std::is_trivially_move_assignable<T>::value, int> = 0>
void moveArray(T* dst, T* src, s32 size) {
    // Explicitly move assignable
    while (size-- > 0) {
        *dst++ = std::move(*src++);
    }
}

} // namespace subst

// ┏━━━━━━━━━━━━━┓
// ┃  InitItems  ┃
// ┗━━━━━━━━━━━━━┛
namespace impl {
template <typename T>
struct InitItems {
    static void init(T*) {
    }
    template <typename Arg, typename... RemainingArgs>
    static void init(T* items, Arg&& arg, RemainingArgs&&... remainingArgs) {
        *items = T{std::forward<Arg>(arg)};
        init(items + 1, std::forward<RemainingArgs>(remainingArgs)...);
    }
};
} // namespace impl

//   ▄▄▄▄   ▄▄                   ▄▄
//  ██  ██ ▄██▄▄  ▄▄▄▄  ▄▄▄▄▄▄▄  ▄▄  ▄▄▄▄
//  ██▀▀██  ██   ██  ██ ██ ██ ██ ██ ██
//  ██  ██  ▀█▄▄ ▀█▄▄█▀ ██ ██ ██ ██ ▀█▄▄▄
//

// clang-format off
inline void signalFenceConsume() { std::atomic_signal_fence(std::memory_order_acquire); }
inline void signalFenceAcquire() { std::atomic_signal_fence(std::memory_order_acquire); }
inline void signalFenceRelease() { std::atomic_signal_fence(std::memory_order_release); }
inline void signalFenceSeqCst() { std::atomic_signal_fence(std::memory_order_seq_cst); }

inline void threadFenceConsume() { std::atomic_thread_fence(std::memory_order_acquire); }
inline void threadFenceAcquire() { std::atomic_thread_fence(std::memory_order_acquire); }
inline void threadFenceRelease() { std::atomic_thread_fence(std::memory_order_release); }
inline void threadFenceSeqCst() { std::atomic_thread_fence(std::memory_order_seq_cst); }
// clang-format on

enum MemoryOrder {
    Relaxed = std::memory_order_relaxed,
    Consume = std::memory_order_consume,
    Acquire = std::memory_order_acquire,
    Release = std::memory_order_release,
    ConsumeRelease = std::memory_order_acq_rel,
    AcquireRelease = std::memory_order_acq_rel,
};

template <typename T>
class Atomic : protected std::atomic<T> {
public:
    Atomic() {
    }
    Atomic(T value) : std::atomic<T>{value} {
    }
    Atomic(const Atomic& other) : std::atomic<T>{other.loadNonatomic()} {
    }
    // Hide operator=
    void operator=(T value) = delete;
    void operator=(const Atomic& other) {
        storeNonatomic(other.loadNonatomic());
    }
    T loadNonatomic() const {
        return std::atomic<T>::load(std::memory_order_relaxed);
    }
    T load(MemoryOrder memoryOrder) const {
        return std::atomic<T>::load((std::memory_order) memoryOrder);
    }
    void storeNonatomic(T value) {
        return std::atomic<T>::store(value, std::memory_order_relaxed);
    }
    void store(T value, MemoryOrder memoryOrder) {
        return std::atomic<T>::store(value, (std::memory_order) memoryOrder);
    }
    T compareExchange(T expected, T desired, MemoryOrder memoryOrder) {
        std::atomic<T>::compare_exchange_strong(expected, desired,
                                                (std::memory_order) memoryOrder);
        return expected; // modified by reference by compare_exchange_strong
    }
    bool compareExchangeStrong(T& expected, T desired, MemoryOrder memoryOrder) {
        return std::atomic<T>::compare_exchange_strong(expected, desired,
                                                       (std::memory_order) memoryOrder);
    }
    bool compareExchangeWeak(T& expected, T desired, MemoryOrder success,
                             MemoryOrder failure) {
        return std::atomic<T>::compare_exchange_weak(expected, desired,
                                                     (std::memory_order) success,
                                                     (std::memory_order) failure);
    }
    T exchange(T desired, MemoryOrder memoryOrder) {
        return std::atomic<T>::exchange(desired, (std::memory_order) memoryOrder);
    }
    T fetchAdd(T operand, MemoryOrder memoryOrder) {
        return std::atomic<T>::fetch_add(operand, (std::memory_order) memoryOrder);
    }
    T fetchSub(T operand, MemoryOrder memoryOrder) {
        return std::atomic<T>::fetch_sub(operand, (std::memory_order) memoryOrder);
    }
    T fetchAnd(T operand, MemoryOrder memoryOrder) {
        return std::atomic<T>::fetch_and(operand, (std::memory_order) memoryOrder);
    }
    T fetchOr(T operand, MemoryOrder memoryOrder) {
        return std::atomic<T>::fetch_or(operand, (std::memory_order) memoryOrder);
    }
};

//  ▄▄   ▄▄         ▄▄
//  ███▄███ ▄▄  ▄▄ ▄██▄▄  ▄▄▄▄  ▄▄  ▄▄
//  ██▀█▀██ ██  ██  ██   ██▄▄██  ▀██▀
//  ██   ██ ▀█▄▄██  ▀█▄▄ ▀█▄▄▄  ▄█▀▀█▄
//

template <typename LockType>
class LockGuard {
private:
    LockType& m_lock;

public:
    LockGuard(LockType& lock) : m_lock(lock) {
        m_lock.lock();
    }
    ~LockGuard() {
        m_lock.unlock();
    }
    LockType& getMutex() {
        return m_lock;
    }
};

class Mutex : protected std::recursive_mutex {
private:
    friend class LockGuard<Mutex>;

public:
    Mutex() : std::recursive_mutex() {
    }

    void lock() {
        std::recursive_mutex::lock();
    }

    bool tryLock() {
        return std::recursive_mutex::try_lock();
    }

    void unlock() {
        std::recursive_mutex::unlock();
    }
};

// Specialize LockGuard<Mutex> so that ConditionVariable_CPP11 can use it:
template <>
class LockGuard<Mutex> : public std::unique_lock<std::recursive_mutex> {
public:
    LockGuard(Mutex& mutex) : std::unique_lock<std::recursive_mutex>(mutex) {
    }
};

#define PLY_LOCK_GUARD

// ┏━━━━━━━━━━━━━━━━━━┓
// ┃  Mutex_LazyInit  ┃
// ┗━━━━━━━━━━━━━━━━━━┛
// A mutex with no constructor that works when zero-init at global scope.
class Mutex_LazyInit {
private:
    Atomic<bool> m_initFlag;
    Atomic<bool> m_spinLock;
    PLY_DECL_ALIGNED(u8 m_buffer[sizeof(Mutex)], alignof(Mutex));

    Mutex& getMutex() {
        return *(Mutex*) m_buffer;
    }

    void lazyInit() {
        // We use the thread-safe DCLI pattern via spinlock in case threads are spawned
        // during static initialization of global C++ objects. In that case, any of them
        // could call lazyInit().
        while (m_spinLock.compareExchange(false, true, Acquire)) {
            // FIXME: Implement reusable AdaptiveBackoff class and apply it here
        }
        if (!m_initFlag.loadNonatomic()) {
            new (&getMutex()) Mutex;
            m_initFlag.store(true, Release);
        }
        m_spinLock.store(false, Release);
    }

public:
    // Manual initialization is needed if not created at global scope:
    void zeroInit() {
        memset(static_cast<void*>(this), 0, sizeof(*this));
    }

    // There should be no threads racing to lock when the destructor is called.
    // It's valid to attempt to lock after the destructor, though.
    // This permits Mutex_LazyInit to be used at global scope where destructors are
    // called in an arbitrary order.
    ~Mutex_LazyInit() {
        if (m_initFlag.loadNonatomic()) {
            getMutex().Mutex::~Mutex();
            zeroInit();
        }
    }

    void lock() {
        if (!m_initFlag.load(Acquire))
            lazyInit();
        getMutex().lock();
    }

    void unlock() {
        PLY_ASSERT(m_initFlag.loadNonatomic());
        getMutex().unlock();
    }
};

//  ▄▄▄▄▄                    ▄▄ ▄▄    ▄▄        ▄▄                 ▄▄
//  ██  ██  ▄▄▄▄   ▄▄▄▄   ▄▄▄██ ██ ▄▄ ██ ▄▄▄▄▄  ██     ▄▄▄▄   ▄▄▄▄ ██  ▄▄
//  ██▀▀█▄ ██▄▄██  ▄▄▄██ ██  ██ ▀█▄██▄█▀ ██  ▀▀ ██    ██  ██ ██    ██▄█▀
//  ██  ██ ▀█▄▄▄  ▀█▄▄██ ▀█▄▄██  ██▀▀██  ██     ██▄▄▄ ▀█▄▄█▀ ▀█▄▄▄ ██ ▀█▄
//

#if PLY_TARGET_WIN32
// ┏━━━━━━━━━┓
// ┃  Win32  ┃
// ┗━━━━━━━━━┛
class ReadWriteLock {
private:
    SRWLOCK m_rwLock;

public:
    ReadWriteLock() {
        InitializeSRWLock(&m_rwLock);
    }

    ~ReadWriteLock() {
        // SRW locks do not need to be destroyed.
    }

    void lockExclusive() {
        AcquireSRWLockExclusive(&m_rwLock);
    }

    void unlockExclusive() {
        ReleaseSRWLockExclusive(&m_rwLock);
    }

    void lockShared() {
        AcquireSRWLockShared(&m_rwLock);
    }

    void unlockShared() {
        ReleaseSRWLockShared(&m_rwLock);
    }
};

#elif PLY_TARGET_POSIX
// ┏━━━━━━━━━┓
// ┃  POSIX  ┃
// ┗━━━━━━━━━┛
class ReadWriteLock {
private:
    pthread_rwlock_t m_rwLock;

public:
    ReadWriteLock() {
        pthread_rwlock_init(&m_rwLock, NULL);
    }

    ~ReadWriteLock() {
        pthread_rwlock_destroy(&m_rwLock);
    }

    void lockExclusive() {
        pthread_rwlock_wrlock(&m_rwLock);
    }

    void unlockExclusive() {
        pthread_rwlock_unlock(&m_rwLock);
    }

    void lockShared() {
        pthread_rwlock_rdlock(&m_rwLock);
    }

    void unlockShared() {
        pthread_rwlock_unlock(&m_rwLock);
    }
};

#endif // ReadWriteLock

// ┏━━━━━━━━━━━━━━━━━━━┓
// ┃  SharedLockGuard  ┃
// ┗━━━━━━━━━━━━━━━━━━━┛
template <typename LockType>
class SharedLockGuard {
private:
    LockType& m_lock;

public:
    SharedLockGuard(LockType& lock) : m_lock(lock) {
        m_lock.lockShared();
    }

    ~SharedLockGuard() {
        m_lock.unlockShared();
    }
};

// ┏━━━━━━━━━━━━━━━━━━━━━━┓
// ┃  ExclusiveLockGuard  ┃
// ┗━━━━━━━━━━━━━━━━━━━━━━┛
template <typename LockType>
class ExclusiveLockGuard {
private:
    LockType& m_lock;

public:
    ExclusiveLockGuard(LockType& lock) : m_lock(lock) {
        m_lock.lockExclusive();
    }

    ~ExclusiveLockGuard() {
        m_lock.unlockExclusive();
    }
};

//   ▄▄▄▄                                ▄▄
//  ██  ▀▀  ▄▄▄▄  ▄▄▄▄▄▄▄   ▄▄▄▄  ▄▄▄▄▄  ██▄▄▄   ▄▄▄▄  ▄▄▄▄▄   ▄▄▄▄
//   ▀▀▀█▄ ██▄▄██ ██ ██ ██  ▄▄▄██ ██  ██ ██  ██ ██  ██ ██  ▀▀ ██▄▄██
//  ▀█▄▄█▀ ▀█▄▄▄  ██ ██ ██ ▀█▄▄██ ██▄▄█▀ ██  ██ ▀█▄▄█▀ ██     ▀█▄▄▄
//                                ██

#if PLY_TARGET_WIN32
// ┏━━━━━━━━━┓
// ┃  Win32  ┃
// ┗━━━━━━━━━┛
class Semaphore {
private:
    HANDLE m_sem;

public:
    Semaphore() {
        m_sem = CreateSemaphore(NULL, 0, INT32_MAX, NULL);
    }

    ~Semaphore() {
        CloseHandle(m_sem);
    }

    void wait() {
        WaitForSingleObject(m_sem, INFINITE);
    }

    void signal(ureg count = 1) {
        ReleaseSemaphore(m_sem, (DWORD) count, NULL);
    }
};

#elif PLY_KERNEL_MACH
// ┏━━━━━━━━┓
// ┃  Mach  ┃
// ┗━━━━━━━━┛
// Mach can't use POSIX semaphores due to
// http://lists.apple.com/archives/darwin-kernel/2009/Apr/msg00010.html
class Semaphore {
private:
    semaphore_t m_semaphore;

public:
    Semaphore() {
        semaphore_create(mach_task_self(), &m_semaphore, SYNC_POLICY_FIFO, 0);
    }

    ~Semaphore() {
        semaphore_destroy(mach_task_self(), m_semaphore);
    }

    void wait() {
        semaphore_wait(m_semaphore);
    }

    void signal(ureg count = 1) {
        while (count-- > 0)
            semaphore_signal(m_semaphore);
    }
};

#elif PLY_TARGET_POSIX
// ┏━━━━━━━━━┓
// ┃  POSIX  ┃
// ┗━━━━━━━━━┛
class Semaphore {
private:
    sem_t m_sem;

public:
    Semaphore() {
        sem_init(&m_sem, 0, 0);
    }

    ~Semaphore() {
        sem_destroy(&m_sem);
    }

    void wait() {
        // http://stackoverflow.com/questions/2013181/gdb-causes-sem-wait-to-fail-with-eintr-error
        int rc;
        do {
            rc = sem_wait(&m_sem);
        } while (rc == -1 && errno == EINTR);
    }

    void signal(ureg count = 1) {
        while (count-- > 0)
            sem_post(&m_sem);
    }
};

#endif // Semaphore

//   ▄▄▄▄                    ▄▄ ▄▄  ▄▄   ▄▄               ▄▄   ▄▄
//  ██  ▀▀  ▄▄▄▄  ▄▄▄▄▄   ▄▄▄██ ▄▄ ▄██▄▄ ▄▄  ▄▄▄▄  ▄▄▄▄▄  ██   ██  ▄▄▄▄  ▄▄▄▄▄
//  ██     ██  ██ ██  ██ ██  ██ ██  ██   ██ ██  ██ ██  ██  ██ ██   ▄▄▄██ ██  ▀▀
//  ▀█▄▄█▀ ▀█▄▄█▀ ██  ██ ▀█▄▄██ ██  ▀█▄▄ ██ ▀█▄▄█▀ ██  ██   ▀█▀   ▀█▄▄██ ██
//

class ConditionVariable {
private:
    std::condition_variable_any m_condVar;

public:
    void wait(LockGuard<Mutex>& guard) {
        m_condVar.wait(guard);
    }

    void timedWait(LockGuard<Mutex>& guard, ureg waitMillis) {
        if (waitMillis > 0)
            m_condVar.wait_for(guard, std::chrono::milliseconds(waitMillis));
    }

    void wakeOne() {
        m_condVar.notify_one();
    }

    void wakeAll() {
        m_condVar.notify_all();
    }
};

//  ▄▄▄▄▄▄ ▄▄                              ▄▄
//    ██   ██▄▄▄  ▄▄▄▄▄   ▄▄▄▄   ▄▄▄▄   ▄▄▄██
//    ██   ██  ██ ██  ▀▀ ██▄▄██  ▄▄▄██ ██  ██
//    ██   ██  ██ ██     ▀█▄▄▄  ▀█▄▄██ ▀█▄▄██
//

#define PLY_THREAD_STARTCALL

class Thread {
protected:
    std::thread thread;

public:
    typedef void* ReturnType;
    typedef void* (*StartRoutine)(void*);

    PLY_INLINE Thread() = default;

    template <typename Callable>
    PLY_INLINE Thread(Callable&& callable) : thread{std::forward<Callable>(callable)} {
    }

    PLY_INLINE ~Thread() {
        if (this->thread.joinable())
            this->thread.detach();
    }

    PLY_INLINE bool isValid() const {
        return this->thread.joinable();
    }

    PLY_INLINE void join() {
        this->thread.join();
    }

    template <typename Callable>
    PLY_INLINE void run(Callable&& callable) {
        if (this->thread.joinable())
            this->thread.detach();
        this->thread = std::thread(std::forward<Callable>(callable));
    }

    static PLY_INLINE void sleepMillis(ureg millis) {
        std::this_thread::sleep_for(std::chrono::milliseconds(millis));
    }
};

//  ▄▄▄▄▄▄ ▄▄                              ▄▄ ▄▄                        ▄▄▄
//    ██   ██▄▄▄  ▄▄▄▄▄   ▄▄▄▄   ▄▄▄▄   ▄▄▄██ ██     ▄▄▄▄   ▄▄▄▄  ▄▄▄▄   ██
//    ██   ██  ██ ██  ▀▀ ██▄▄██  ▄▄▄██ ██  ██ ██    ██  ██ ██     ▄▄▄██  ██
//    ██   ██  ██ ██     ▀█▄▄▄  ▀█▄▄██ ▀█▄▄██ ██▄▄▄ ▀█▄▄█▀ ▀█▄▄▄ ▀█▄▄██ ▄██▄
//

// Used as the return value of ThreadLocal::setInScope()
template <template <typename> class TL, typename T>
class ThreadLocalScope {
private:
    TL<T>* var;
    T oldValue;

public:
    PLY_INLINE ThreadLocalScope(TL<T>* var, T newValue) : var{var} {
        this->oldValue = var->load();
        var->store(newValue);
    }

    ThreadLocalScope(const ThreadLocalScope&) = delete;
    PLY_INLINE ThreadLocalScope(ThreadLocalScope&& other) {
        this->var = other->var;
        this->oldValue = std::move(other.oldValue);
        other->var = nullptr;
    }

    ~ThreadLocalScope() {
        if (this->var) {
            this->var->store(this->oldValue);
        }
    }
};

#if PLY_TARGET_WIN32
// ┏━━━━━━━━━┓
// ┃  Win32  ┃
// ┗━━━━━━━━━┛
template <typename T>
class ThreadLocal {
private:
    PLY_STATIC_ASSERT(sizeof(T) <= PLY_PTR_SIZE);
    DWORD m_tlsIndex;

public:
    PLY_INLINE ThreadLocal() {
        m_tlsIndex = TlsAlloc();
        PLY_ASSERT(m_tlsIndex != TLS_OUT_OF_INDEXES);
    }

    ThreadLocal(const ThreadLocal&) = delete;

    PLY_INLINE ~ThreadLocal() {
        BOOL rc = TlsFree(m_tlsIndex);
        PLY_ASSERT(rc != 0);
        PLY_UNUSED(rc);
    }

    template <typename U = T, std::enable_if_t<std::is_pointer<U>::value, int> = 0>
    PLY_INLINE U load() const {
        LPVOID value = TlsGetValue(m_tlsIndex);
        PLY_ASSERT(value != 0 || GetLastError() == ERROR_SUCCESS);
        return (T) value;
    }

    template <
        typename U = T,
        std::enable_if_t<std::is_enum<U>::value || std::is_integral<U>::value, int> = 0>
    PLY_INLINE U load() const {
        LPVOID value = TlsGetValue(m_tlsIndex);
        PLY_ASSERT(value != 0 || GetLastError() == ERROR_SUCCESS);
        return (T) (uptr) value;
    }

    PLY_INLINE void store(T value) {
        BOOL rc = TlsSetValue(m_tlsIndex, (LPVOID) value);
        PLY_ASSERT(rc != 0);
        PLY_UNUSED(rc);
    }

    // In C++11, you can write auto scope = myTLvar.setInScope(value);
    using Scope = ThreadLocalScope<ThreadLocal, T>;
    PLY_INLINE Scope setInScope(T value) {
        return {this, value};
    }
};

#elif PLY_TARGET_POSIX
// ┏━━━━━━━━━┓
// ┃  POSIX  ┃
// ┗━━━━━━━━━┛
template <typename T>
class ThreadLocal {
private:
    PLY_STATIC_ASSERT(sizeof(T) <= PLY_PTR_SIZE);
    pthread_key_t m_tlsKey;

public:
    PLY_INLINE ThreadLocal() {
        int rc = pthread_key_create(&m_tlsKey, NULL);
        PLY_ASSERT(rc == 0);
        PLY_UNUSED(rc);
    }

    ThreadLocal(const ThreadLocal&) = delete;

    PLY_INLINE ~ThreadLocal() {
        int rc = pthread_key_delete(m_tlsKey);
        PLY_ASSERT(rc == 0);
        PLY_UNUSED(rc);
    }

    template <typename U = T, std::enable_if_t<std::is_pointer<U>::value, int> = 0>
    PLY_INLINE U load() const {
        void* value = pthread_getspecific(m_tlsKey);
        return (T) value;
    }

    template <
        typename U = T,
        std::enable_if_t<std::is_enum<U>::value || std::is_integral<U>::value, int> = 0>
    PLY_INLINE U load() const {
        void* value = pthread_getspecific(m_tlsKey);
        return (T) (uptr) value;
    }

    template <
        typename U = T,
        std::enable_if_t<std::is_enum<U>::value || std::is_integral<U>::value, int> = 0>
    PLY_INLINE void store(U value) {
        int rc = pthread_setspecific(m_tlsKey, (void*) (uptr) value);
        PLY_ASSERT(rc == 0);
        PLY_UNUSED(rc);
    }

    // In C++11, you can write auto scope = myTLvar.setInScope(value);
    using Scope = ThreadLocalScope<ThreadLocal, T>;
    PLY_INLINE Scope setInScope(T value) {
        return {this, value};
    }
};

#endif // ThreadLocal

//  ▄▄▄▄▄                      ▄▄▄▄▄          ▄▄                 ▄▄
//  ██  ██  ▄▄▄▄   ▄▄▄▄  ▄▄▄▄  ██  ██  ▄▄▄▄  ▄██▄▄  ▄▄▄▄   ▄▄▄▄ ▄██▄▄  ▄▄▄▄  ▄▄▄▄▄
//  ██▀▀█▄  ▄▄▄██ ██    ██▄▄██ ██  ██ ██▄▄██  ██   ██▄▄██ ██     ██   ██  ██ ██  ▀▀
//  ██  ██ ▀█▄▄██ ▀█▄▄▄ ▀█▄▄▄  ██▄▄█▀ ▀█▄▄▄   ▀█▄▄ ▀█▄▄▄  ▀█▄▄▄  ▀█▄▄ ▀█▄▄█▀ ██
//

#if PLY_WITH_ASSERTS

class RaceDetector {
private:
    ply::Atomic<u8> m_entered = 0;

public:
    void enter() {
        if (m_entered.exchange(1, ply::Acquire) != 0) {
            PLY_FORCE_CRASH();
        }
    }
    void exit() {
        if (m_entered.exchange(0, ply::Acquire) != 1) {
            PLY_FORCE_CRASH();
        }
    }
};

class RaceDetectGuard {
private:
    RaceDetector& m_guard;

public:
    RaceDetectGuard(RaceDetector& guard) : m_guard(guard) {
        m_guard.enter();
    }
    ~RaceDetectGuard() {
        m_guard.exit();
    }
};

#define PLY_DEFINE_RACE_DETECTOR(name) mutable ply::RaceDetector name;
#define PLY_RACE_DETECT_GUARD(name) \
    ply::RaceDetectGuard PLY_UNIQUE_VARIABLE(raceDetectGuard)(name)
#define PLY_RACE_DETECT_ENTER(name) name.enter()
#define PLY_RACE_DETECT_EXIT(name) name.exit()

#else

// clang-format off
#define PLY_DEFINE_RACE_DETECTOR(name)
#define PLY_RACE_DETECT_GUARD(name) do {} while (0)
#define PLY_RACE_DETECT_ENTER(name) do {} while (0)
#define PLY_RACE_DETECT_EXIT(name) do {} while (0)
// clang-format on

#endif // RaceDetector

//  ▄▄   ▄▄                 ▄▄▄▄▄
//  ███▄███  ▄▄▄▄  ▄▄▄▄▄▄▄  ██  ██  ▄▄▄▄   ▄▄▄▄▄  ▄▄▄▄
//  ██▀█▀██ ██▄▄██ ██ ██ ██ ██▀▀▀   ▄▄▄██ ██  ██ ██▄▄██
//  ██   ██ ▀█▄▄▄  ██ ██ ██ ██     ▀█▄▄██ ▀█▄▄██ ▀█▄▄▄
//                                         ▄▄▄█▀

class MemPage {
public:
    struct Info {
        uptr allocationGranularity;
        uptr pageSize;
    };

    static const Info& getInfo();

    static bool alloc(char*& outAddr, uptr numBytes);
    static bool reserve(char*& outAddr, uptr numBytes);
    static void commit(char* addr, uptr numBytes);
    static void decommit(char* addr, uptr numBytes);
    // On Windows, free() is only able to free a single entire region returned by
    // alloc() or reserve(). For this reason, dlmalloc doesn't use MemPage_Win32.
    static void free(char* addr, uptr numBytes);
};

//  ▄▄  ▄▄
//  ██  ██  ▄▄▄▄   ▄▄▄▄  ▄▄▄▄▄
//  ██▀▀██ ██▄▄██  ▄▄▄██ ██  ██
//  ██  ██ ▀█▄▄▄  ▀█▄▄██ ██▄▄█▀
//                       ██

struct HeapStats {
    ureg peakSystemBytes;
    ureg systemBytes;
    ureg inUseBytes;
};

namespace memory_dl {

// Adapted from Doug Lea's malloc: ftp://g.oswego.edu/pub/misc/malloc-2.8.6.c
//
// Note: You can create new heaps by instantiating new
// Heap_t objects, but as long as it exists, you cannot destroy the heap
// in which you instantiated the Heap_t object itself.
//
// This limitation doesn't really matter as long as all your Heap_t objects
// exist at global scope or are allocated in the default heap.
//
// (Doug Lea's original malloc does not have this limitation because it
// represents separate heaps as "mspaces", where the malloc_state is embedded
// the header of a memory region owned by the mspace and passed around by
// pointer.)
//
// The approach chosen here simplifies the implementation of Heap_t, avoids
// any issues with static initialization order, and minimizes runtime overhead.
#define PLY_DLMALLOC_FAST_STATS 0

static const unsigned int NSMALLBINS = (32U);
static const unsigned int NTREEBINS = (32U);

struct malloc_chunk;
typedef struct malloc_chunk mchunk;
typedef struct malloc_chunk* mchunkptr;
typedef struct malloc_chunk* sbinptr; /* The type of bins of chunks */
typedef unsigned int bindex_t;        /* Described below */
typedef unsigned int binmap_t;        /* Described below */
typedef unsigned int flag_t;          /* The type of various bit flag sets */

struct malloc_tree_chunk;
typedef struct malloc_tree_chunk* tbinptr; /* The type of bins of trees */

struct malloc_segment {
    char* base;                  /* base address */
    size_t size;                 /* allocated size */
    struct malloc_segment* next; /* ptr to next segment */
    flag_t sflags;               /* mmap and extern flag */
};
typedef struct malloc_segment msegment;

struct malloc_state {
    binmap_t smallmap;
    binmap_t treemap;
    size_t dvsize;
    size_t topsize;
    char* least_addr;
    mchunkptr dv;
    mchunkptr top;
    size_t trim_check;
    size_t release_checks;
    size_t magic;
    mchunkptr smallbins[(NSMALLBINS + 1) * 2];
    tbinptr treebins[NTREEBINS];
    size_t footprint;
    size_t max_footprint;
    size_t footprint_limit; /* zero means no limit */
    flag_t mflags;
    msegment seg;
    void* extp; /* Unused but available for extensions */
    size_t exts;
#if PLY_DLMALLOC_FAST_STATS
    size_t inUseBytes;
#endif
};
typedef struct malloc_state* mstate;

} // namespace memory_dl

class Heap_t {
private:
    memory_dl::malloc_state mstate;
    Mutex_LazyInit mutex;

public:
    // If you create a Heap_t at global scope, it will be automatically zero-init.
    // Otherwise, you should call this function before using it:
    void zeroInit();
    void* alloc(ureg size);
    void* realloc(void* ptr, ureg newSize);
    void free(void* ptr);
    void* allocAligned(ureg size, ureg alignment);
    void freeAligned(void* ptr);
    HeapStats getStats();
    ureg getInUseBytes() const;
    ureg getSize(void* ptr);
};

extern Heap_t Heap;

//   ▄▄▄▄                              ▄▄   ▄▄ ▄▄
//  ██  ██ ▄▄▄▄▄  ▄▄▄▄▄   ▄▄▄▄  ▄▄  ▄▄ ██   ██ ▄▄  ▄▄▄▄  ▄▄    ▄▄
//  ██▀▀██ ██  ▀▀ ██  ▀▀  ▄▄▄██ ██  ██  ██ ██  ██ ██▄▄██ ██ ██ ██
//  ██  ██ ██     ██     ▀█▄▄██ ▀█▄▄██   ▀█▀   ██ ▀█▄▄▄   ██▀▀██
//                               ▄▄▄█▀

struct StringView;
struct MutStringView;

template <typename T_>
struct ArrayView {
    using T = T_;

    T* items = nullptr;
    u32 numItems = 0;

    // Constructors
    ArrayView() = default;
    ArrayView(T* items, u32 numItems) : items{items}, numItems{numItems} {
    }
    template <typename U = T, std::enable_if_t<std::is_const<U>::value, int> = 0>
    ArrayView(std::initializer_list<T> init)
        : items{init.begin()}, numItems{safeDemote<u32>(init.size())} {
        PLY_ASSERT((uptr) init.end() - (uptr) init.begin() == sizeof(T) * init.size());
    }
    template <u32 N>
    ArrayView(T (&s)[N]) : items{s}, numItems{N} {
    }

    // Conversion
    operator ArrayView<const T>() const {
        return {this->items, this->numItems};
    }
    static ArrayView<const T> from(StringView view);
    static ArrayView<T> from(MutStringView view);
    StringView stringView() const;
    MutStringView mutableStringView();

    // Indexing
    T& operator[](u32 index) {
        PLY_ASSERT(index < numItems);
        return items[index];
    }
    const T& operator[](u32 index) const {
        PLY_ASSERT(index < numItems);
        return items[index];
    }
    T& back(s32 offset = -1) {
        PLY_ASSERT(u32(numItems + offset) < numItems);
        return items[numItems + offset];
    }
    const T& back(s32 offset = -1) const {
        PLY_ASSERT(u32(numItems + offset) < numItems);
        return items[numItems + offset];
    }

    void offsetHead(u32 ofs) {
        PLY_ASSERT(ofs <= numItems);
        items += ofs;
        numItems -= ofs;
    }
    void offsetBack(s32 ofs) {
        PLY_ASSERT((u32) -ofs <= numItems);
        numItems += ofs;
    }

    explicit operator bool() const {
        return this->numItems > 0;
    }
    bool isEmpty() const {
        return numItems == 0;
    }
    u32 sizeBytes() const {
        return numItems * u32(sizeof(T));
    }
    ArrayView subView(u32 start) const {
        PLY_ASSERT(start <= numItems);
        return {items + start, numItems - start};
    }
    ArrayView subView(u32 start, u32 numItems) const {
        PLY_ASSERT(start <= this->numItems); // FIXME: Support different end parameters
        PLY_ASSERT(start + numItems <= this->numItems);
        return {items + start, numItems};
    }
    ArrayView shortenedBy(u32 numItems) const {
        PLY_ASSERT(numItems <= this->numItems);
        return {this->items, this->numItems - numItems};
    }

    // Range-for support
    T* begin() const {
        return items;
    }
    T* end() const {
        return items + numItems;
    }
};

namespace impl {

// There's an ArrayTraits specialization for each array-like class template in this
// library. If ArrayTraits<Arr>::ItemType is well-formed, Arr is convertible to
// ArrayView of that type. If ArrayTraits<Arr>::IsOwner is true, Arr is considered the
// owner of its elements.
template <typename>
struct ArrayTraits {
    static constexpr bool IsOwner = false;
};
template <typename T>
struct ArrayTraits<ArrayView<T>> {
    using ItemType = T;
    static constexpr bool IsOwner = false;
};
// ArrayViewType<Arr> is a convenience template that returns the item type of an
// array-like class Arr, even if Arr is const or a reference. If Arr is const (or
// const&) and owns its elements, the resulting item type will be const. For example:
// ArrayViewType<const Array<u32>&> evaluates to const u32.
template <typename Arr>
using ArrayViewType =
    std::conditional_t<ArrayTraits<std::decay_t<Arr>>::IsOwner &&
                           std::is_const<std::remove_reference_t<Arr>>::value,
                       const typename ArrayTraits<std::decay_t<Arr>>::ItemType,
                       typename ArrayTraits<std::decay_t<Arr>>::ItemType>;

// If the second argument is an rvalue reference, Arr will be deduced as non-reference.
// Otherwise, Arr will be deduced as a reference, and ArrayTraits<Arr>::IsOwner will be
// false. Therefore, the "moving" version of this function will only be called if the
// second argument is an rvalue reference to an array-like class for which
// ArrayTraits<Arr>::IsOwner is true, which is what we want.
template <typename T, typename Arr,
          std::enable_if_t<ArrayTraits<Arr>::IsOwner, int> = 0>
void moveOrCopyConstruct(T* dst, Arr&& src) {
    ArrayView<ArrayViewType<Arr>> srcView{src};
    subst::moveConstructArray(dst, srcView.items, srcView.numItems);
}
template <typename T, typename Arr,
          std::enable_if_t<!ArrayTraits<Arr>::IsOwner, int> = 0>
void moveOrCopyConstruct(T* dst, Arr&& src) {
    ArrayView<ArrayViewType<Arr>> srcView{src};
    subst::constructArrayFrom(dst, srcView.items, srcView.numItems);
}

} // namespace impl

template <typename T0, typename T1>
bool operator==(ArrayView<T0> a, ArrayView<T1> b) {
    if (a.numItems != b.numItems)
        return false;
    for (u32 i = 0; i < a.numItems; i++) {
        if (!(a[i] == b[i]))
            return false;
    }
    return true;
}

template <typename Arr0, typename Arr1, typename T0 = impl::ArrayViewType<Arr0>,
          typename T1 = impl::ArrayViewType<Arr1>>
bool operator==(Arr0&& a, Arr1&& b) {
    return ArrayView<T0>{a} == ArrayView<T1>{b};
}

#define PLY_ALLOC_STACK_ARRAY(T, count) \
    ArrayView<T> { (T*) alloca(sizeof(T) * (count)), (count) }

//   ▄▄▄▄   ▄▄          ▄▄               ▄▄   ▄▄ ▄▄
//  ██  ▀▀ ▄██▄▄ ▄▄▄▄▄  ▄▄ ▄▄▄▄▄   ▄▄▄▄▄ ██   ██ ▄▄  ▄▄▄▄  ▄▄    ▄▄
//   ▀▀▀█▄  ██   ██  ▀▀ ██ ██  ██ ██  ██  ██ ██  ██ ██▄▄██ ██ ██ ██
//  ▀█▄▄█▀  ▀█▄▄ ██     ██ ██  ██ ▀█▄▄██   ▀█▀   ██ ▀█▄▄▄   ██▀▀██
//                                 ▄▄▄█▀

struct String;
struct HybridString;
template <typename>
class Array;
enum UnicodeType;

namespace fmt {
template <typename>
struct TypeParser;
template <typename>
struct FormatParser;
} // namespace fmt

inline bool isWhite(char cp) {
    return (cp == ' ') || (cp == '\t') || (cp == '\r') || (cp == '\n');
}

inline bool isAsciiLetter(char cp) {
    return (cp >= 'A' && cp <= 'Z') || (cp >= 'a' && cp <= 'z');
}

inline bool isDecimalDigit(char cp) {
    return (cp >= '0' && cp <= '9');
}

struct StringView {
    const char* bytes = nullptr;
    u32 numBytes = 0;

    StringView() = default;
    StringView(const char* s)
        : bytes{s}, numBytes{(u32) std::char_traits<char>::length(s)} {
        PLY_ASSERT(s[numBytes] ==
                   0); // Sanity check; numBytes must fit within 32-bit field
    }
    template <typename U, u32 N,
              std::enable_if_t<std::is_same<U, std::decay_t<decltype(*u8"")>>::value,
                               bool> = false>
    StringView(const U (&s)[N]) : bytes{s}, numBytes{N} {
    }
    u32 num_codepoints(UnicodeType decoder_type) const;
    template <typename U, typename = std::enable_if_t<std::is_same<U, char>::value>>
    StringView(const U& c) : bytes{&c}, numBytes{1} {
    }
    StringView(const char* bytes, u32 numBytes) : bytes{bytes}, numBytes{numBytes} {
    }

    static StringView fromRange(const char* startByte, const char* endByte) {
        return {startByte, safeDemote<u32>(endByte - startByte)};
    }
    const char& operator[](u32 index) const {
        PLY_ASSERT(index < this->numBytes);
        return this->bytes[index];
    }
    const char& back(s32 ofs = -1) const {
        PLY_ASSERT(u32(-ofs - 1) < this->numBytes);
        return this->bytes[this->numBytes + ofs];
    }
    void offsetHead(u32 numBytes) {
        PLY_ASSERT(numBytes <= this->numBytes);
        this->bytes += numBytes;
        this->numBytes -= numBytes;
    }

    void offsetBack(s32 ofs) {
        PLY_ASSERT((u32) -ofs <= this->numBytes);
        this->numBytes += ofs;
    }
    template <typename T>
    T to(const T& defaultValue = subst::createDefault<T>()) const;
    explicit operator bool() const {
        return this->numBytes != 0;
    }
    bool isEmpty() const {
        return this->numBytes == 0;
    }
    StringView subStr(u32 start) const {
        PLY_ASSERT(start <= numBytes);
        return {this->bytes + start, this->numBytes - start};
    }
    StringView subStr(u32 start, u32 numBytes) const {
        PLY_ASSERT(start <= this->numBytes);
        PLY_ASSERT(start + numBytes <= this->numBytes);
        return {this->bytes + start, numBytes};
    }
    bool contains(const char* curByte) const {
        return uptr(curByte - this->bytes) <= this->numBytes;
    }
    StringView left(u32 numBytes) const {
        PLY_ASSERT(numBytes <= this->numBytes);
        return {this->bytes, numBytes};
    }
    StringView shortenedBy(u32 numBytes) const {
        PLY_ASSERT(numBytes <= this->numBytes);
        return {this->bytes, this->numBytes - numBytes};
    }
    StringView right(u32 numBytes) const {
        PLY_ASSERT(numBytes <= this->numBytes);
        return {this->bytes + this->numBytes - numBytes, numBytes};
    }
    s32 findByte(char matchByte, u32 startPos = 0) const {
        for (u32 i = startPos; i < this->numBytes; i++) {
            if (this->bytes[i] == matchByte)
                return i;
        }
        return -1;
    }
    template <typename MatchFunc>
    s32 findByte(const MatchFunc& matchFunc, u32 startPos = 0) const {
        for (u32 i = startPos; i < this->numBytes; i++) {
            if (matchFunc(this->bytes[i]))
                return i;
        }
        return -1;
    }

    s32 rfindByte(char matchByte, u32 startPos) const {
        s32 i = startPos;
        for (; i >= 0; i--) {
            if (this->bytes[i] == matchByte)
                break;
        }
        return i;
    }
    template <typename MatchFunc>
    s32 rfindByte(const MatchFunc& matchFunc, u32 startPos) const {
        s32 i = startPos;
        for (; i >= 0; i--) {
            if (matchFunc(this->bytes[i]))
                break;
        }
        return i;
    }
    template <typename MatchFuncOrChar>
    s32 rfindByte(const MatchFuncOrChar& matchFuncOrByte) const {
        return this->rfindByte(matchFuncOrByte, this->numBytes - 1);
    }

    bool startsWith(StringView arg) const;
    bool endsWith(StringView arg) const;

    StringView trim(bool (*matchFunc)(char) = isWhite, bool left = true,
                    bool right = true) const;
    StringView ltrim(bool (*matchFunc)(char) = isWhite) const {
        return this->trim(matchFunc, true, false);
    }
    StringView rtrim(bool (*matchFunc)(char) = isWhite) const {
        return this->trim(matchFunc, false, true);
    }
    String join(ArrayView<const StringView> comps) const;
    Array<StringView> splitByte(char sep) const;
    String upperAsc() const;
    String lowerAsc() const;
    String reversedBytes() const;
    String filterBytes(char (*filterFunc)(char)) const;

    bool includesNullTerminator() const {
        return (this->numBytes > 0) ? (this->bytes[this->numBytes - 1] == 0) : false;
    }
    HybridString withNullTerminator() const;
    StringView withoutNullTerminator() const;

    const char* begin() const {
        return this->bytes;
    }
    const char* end() const {
        return this->bytes + this->numBytes;
    }
};

s32 compare(StringView a, StringView b);

inline bool operator==(StringView a, StringView b) {
    return compare(a, b) == 0;
}
inline bool operator!=(StringView a, StringView b) {
    return compare(a, b) != 0;
}
inline bool operator<(StringView a, StringView b) {
    return compare(a, b) < 0;
}
inline bool operator<=(StringView a, StringView b) {
    return compare(a, b) <= 0;
}
inline bool operator>(StringView a, StringView b) {
    return compare(a, b) > 0;
}
inline bool operator>=(StringView a, StringView b) {
    return compare(a, b) >= 0;
}
String operator+(StringView a, StringView b);
String operator*(StringView str, u32 count);

struct MutStringView {
    char* bytes = nullptr;
    u32 numBytes = 0;

    MutStringView() = default;
    MutStringView(char* bytes, u32 numBytes) : bytes{bytes}, numBytes{numBytes} {
    }

    char* end() {
        return this->bytes + this->numBytes;
    }
    static MutStringView fromRange(char* startByte, char* endByte) {
        return {startByte, safeDemote<u32>(endByte - startByte)};
    }
    operator const StringView&() const {
        return reinterpret_cast<const StringView&>(*this);
    }
    void offsetHead(u32 numBytes) {
        PLY_ASSERT(numBytes <= this->numBytes);
        this->bytes += numBytes;
        this->numBytes -= numBytes;
    }
    void offsetBack(s32 ofs) {
        PLY_ASSERT((u32) -ofs <= this->numBytes);
        this->numBytes += ofs;
    }
};

template <typename T>
ArrayView<const T> ArrayView<T>::from(StringView view) {
    u32 numItems = view.numBytes / sizeof(T); // Divide by constant is fast
    return {(const T*) view.bytes, numItems};
}

template <typename T>
ArrayView<T> ArrayView<T>::from(MutStringView view) {
    u32 numItems = view.numBytes / sizeof(T); // Divide by constant is fast
    return {(T*) view.bytes, numItems};
}

template <typename T>
StringView ArrayView<T>::stringView() const {
    return {(const char*) items, safeDemote<u32>(numItems * sizeof(T))};
}

template <typename T>
MutStringView ArrayView<T>::mutableStringView() {
    return {(char*) items, safeDemote<u32>(numItems * sizeof(T))};
}

namespace subst {
template <typename T>
void destructViewAs(StringView view) {
    subst::destructArray<T>((T*) view.bytes, view.numBytes / (u32) sizeof(T));
}
} // namespace subst

//  ▄▄▄▄▄                                ▄▄    ▄▄▄▄
//  ██     ▄▄▄▄  ▄▄▄▄▄  ▄▄▄▄▄▄▄   ▄▄▄▄  ▄██▄▄ ██  ██ ▄▄▄▄▄   ▄▄▄▄▄
//  ██▀▀  ██  ██ ██  ▀▀ ██ ██ ██  ▄▄▄██  ██   ██▀▀██ ██  ▀▀ ██  ██
//  ██    ▀█▄▄█▀ ██     ██ ██ ██ ▀█▄▄██  ▀█▄▄ ██  ██ ██     ▀█▄▄██
//                                                           ▄▄▄█▀

struct OutPipe;
struct OutStream;

#define PLY_ENABLE_IF_SIGNED(T) \
    typename std::enable_if_t<std::is_integral<T>::value && std::is_signed<T>::value, \
                              int> = 0
#define PLY_ENABLE_IF_UNSIGNED(T) \
    typename std::enable_if_t< \
        std::is_unsigned<T>::value && !std::is_same<T, bool>::value, int> = 0

struct FormatArg {
    enum Type {
        View,
        Bool,
        S64,
        U64,
        Double,
    };

    static void default_print(OutStream& out, const FormatArg& arg);
    void (*print_func)(OutStream& out, const FormatArg& arg) = default_print;
    union {
        StringView view;
        bool bool_;
        s64 s64_;
        u64 u64_;
        double double_;
    };
    Type type = View;
    u32 radix = 10;

    FormatArg(StringView view = {}) : view{view} {
    }
    template <typename T,
              typename std::enable_if_t<std::is_same<T, bool>::value, int> = 0>
    FormatArg(T v) : bool_{v}, type{Bool} {
    }
    template <typename T, PLY_ENABLE_IF_SIGNED(T)>
    FormatArg(T v) : s64_{v}, type{S64} {
    }
    template <typename T, PLY_ENABLE_IF_UNSIGNED(T)>
    FormatArg(T v) : u64_{v}, type{U64} {
    }
    FormatArg(double v) : double_{v}, type{Double} {
    }
    void print(OutStream& out) const {
        this->print_func(out, *this);
    }
};

struct with_radix : FormatArg {
    template <typename T, PLY_ENABLE_IF_SIGNED(T)>
    with_radix(T v, u32 radix) : FormatArg{v} {
        this->radix = radix;
    }
    template <typename T, PLY_ENABLE_IF_UNSIGNED(T)>
    with_radix(T v, u32 radix) : FormatArg{v} {
        this->radix = radix;
    }
    with_radix(double v, u32 radix) : FormatArg{v} {
        this->radix = radix;
    }
};

struct hex : FormatArg {
    template <typename T, PLY_ENABLE_IF_SIGNED(T)>
    hex(T v) : FormatArg{v} {
        this->radix = 16;
    }
    template <typename T, PLY_ENABLE_IF_UNSIGNED(T)>
    hex(T v) : FormatArg{v} {
        this->radix = 16;
    }
};

struct escape : FormatArg {
    static void do_print(OutStream& out, const FormatArg& arg);
    escape(StringView view) {
        this->print_func = do_print;
        this->view = view;
    }
};

struct xml_escape : FormatArg {
    static void do_print(OutStream& out, const FormatArg& arg);
    xml_escape(StringView view) {
        this->print_func = do_print;
        this->view = view;
    }
};

struct CmdLineArg_WinCrt : FormatArg {
    static void do_print(OutStream& out, const FormatArg& arg);
    CmdLineArg_WinCrt(StringView view) {
        this->print_func = do_print;
        this->view = view;
    }
};

//  ▄▄▄▄▄
//  ██    ▄▄▄▄▄  ▄▄▄▄▄   ▄▄▄▄  ▄▄▄▄▄
//  ██▀▀  ██  ▀▀ ██  ▀▀ ██  ██ ██  ▀▀
//  ██▄▄▄ ██     ██     ▀█▄▄█▀ ██
//

struct Error_t {
    void log_internal(StringView fmt, ArrayView<const FormatArg> args);
    template <typename... Args>
    void log(StringView fmt, Args&&... args) {
        FixedArray<FormatArg, sizeof...(Args)> arg_list;
        impl::InitItems<FormatArg>::init(arg_list.view().items,
                                         std::forward<Args>(args)...);
        this->log_internal(fmt, arg_list);
    }
};

extern Error_t Error;

//         ▄▄▄                       ▄▄  ▄▄   ▄▄
//   ▄▄▄▄   ██   ▄▄▄▄▄  ▄▄▄▄  ▄▄▄▄▄  ▄▄ ▄██▄▄ ██▄▄▄  ▄▄▄▄▄▄▄   ▄▄▄▄
//   ▄▄▄██  ██  ██  ██ ██  ██ ██  ▀▀ ██  ██   ██  ██ ██ ██ ██ ▀█▄▄▄
//  ▀█▄▄██ ▄██▄ ▀█▄▄██ ▀█▄▄█▀ ██     ██  ▀█▄▄ ██  ██ ██ ██ ██  ▄▄▄█▀
//               ▄▄▄█▀

// ┏━━━━━━━━┓
// ┃  find  ┃
// ┗━━━━━━━━┛
PLY_MAKE_WELL_FORMEDNESS_CHECK_2(IsComparable,
                                 std::declval<T0>() == std::declval<T1>());
PLY_MAKE_WELL_FORMEDNESS_CHECK_2(IsCallable, std::declval<T0>()(std::declval<T1>()));

template <typename T, typename U, std::enable_if_t<IsComparable<T, U>, int> = 0>
PLY_INLINE s32 find(ArrayView<const T> arr, const U& item) {
    for (u32 i = 0; i < arr.numItems; i++) {
        if (arr[i] == item)
            return i;
    }
    return -1;
}

template <typename T, typename Callback,
          std::enable_if_t<IsCallable<Callback, T>, int> = 0>
PLY_INLINE s32 find(ArrayView<const T> arr, const Callback& callback) {
    for (u32 i = 0; i < arr.numItems; i++) {
        if (callback(arr[i]))
            return i;
    }
    return -1;
}

template <typename Arr, typename Arg, typename T = impl::ArrayViewType<Arr>>
PLY_INLINE s32 find(const Arr& arr, const Arg& arg) {
    return find(ArrayView<const T>{arr}, arg);
}

template <typename T, typename U, std::enable_if_t<IsComparable<T, U>, int> = 0>
PLY_INLINE s32 rfind(ArrayView<const T> arr, const U& item) {
    for (s32 i = safeDemote<s32>(arr.numItems - 1); i >= 0; i--) {
        if (arr[i] == item)
            return i;
    }
    return -1;
}

template <typename T, typename Callback,
          std::enable_if_t<IsCallable<Callback, T>, int> = 0>
PLY_INLINE s32 rfind(ArrayView<const T> arr, const Callback& callback) {
    for (s32 i = safeDemote<s32>(arr.numItems - 1); i >= 0; i--) {
        if (callback(arr[i]))
            return i;
    }
    return -1;
}

template <typename Arr, typename Arg, typename T = impl::ArrayViewType<Arr>>
PLY_INLINE s32 rfind(const Arr& arr, const Arg& arg) {
    return rfind(ArrayView<const T>{arr}, arg);
}

// ┏━━━━━━━┓
// ┃  map  ┃
// ┗━━━━━━━┛
template <typename Iterable, typename MapFunc,
          typename MappedItemType = std::decay_t<decltype(std::declval<MapFunc>()(
              std::declval<impl::ItemType<Iterable>>()))>>
Array<MappedItemType> map(Iterable&& iterable, MapFunc&& mapFunc) {
    Array<MappedItemType> result;
    // FIXME: Reserve memory for result when possible. Otherwise, use a typed
    // ChunkBuffer.
    for (auto&& item : iterable) {
        result.append(mapFunc(item));
    }
    return result;
}

// ┏━━━━━━━━┓
// ┃  sort  ┃
// ┗━━━━━━━━┛
namespace impl {
template <typename T>
PLY_INLINE bool defaultLess(const T& a, const T& b) {
    return a < b;
}
} // namespace impl

template <typename T, typename IsLess = decltype(impl::defaultLess<T>)>
PLY_NO_INLINE void sort(ArrayView<T> view,
                        const IsLess& isLess = impl::defaultLess<T>) {
    if (view.numItems <= 1)
        return;
    u32 lo = 0;
    u32 hi = view.numItems - 1;
    u32 pivot = view.numItems / 2;
    for (;;) {
        while (lo < hi && isLess(view[lo], view[pivot])) {
            lo++;
        }
        while (lo < hi && isLess(view[pivot], view[hi])) {
            hi--;
        }
        if (lo >= hi)
            break;
        // view[lo] is >= pivot
        // All slots to left of lo are < pivot
        // view[hi] <= pivot
        // All slots to the right of hi are > pivot
        PLY_ASSERT(!isLess(view[lo], view[pivot]));
        PLY_ASSERT(!isLess(view[pivot], view[hi]));
        PLY_ASSERT(lo < hi);
        std::swap(view[lo], view[hi]);
        if (lo == pivot) {
            pivot = hi;
        } else if (hi == pivot) {
            pivot = lo;
        }
        lo++;
    }
    PLY_ASSERT((s32) hi >= 0);
    // Now, everything to left of lo is <= pivot, and everything from hi onwards is >=
    // pivot.
    PLY_ASSERT(hi <= lo);
    while (lo > 1) {
        if (!isLess(view[lo - 1], view[pivot])) {
            lo--;
        } else {
            sort(view.subView(0, lo), isLess);
            break;
        }
    }
    while (hi + 1 < view.numItems) {
        if (!isLess(view[pivot], view[hi])) {
            hi++;
        } else {
            sort(view.subView(hi), isLess);
            break;
        }
    }
}

template <typename Arr,
          typename IsLess = decltype(impl::defaultLess<impl::ArrayViewType<Arr>>)>
PLY_INLINE void
sort(Arr& arr, const IsLess& isLess = impl::defaultLess<impl::ArrayViewType<Arr>>) {
    using T = impl::ArrayViewType<Arr>;
    sort(ArrayView<T>{arr}, isLess);
}

} // namespace ply

#include <ply-runtime/container.h>
#include <ply-runtime/io.h>
#include <ply-runtime/network.h>
