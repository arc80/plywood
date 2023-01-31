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

#endif

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
        : m_isAccurate(false), m_numHWThreads(1), m_numPhysicalCores(1), m_hwThreadsPerCore(1) {
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
        kern_return_t result =
            thread_policy_set(thread, THREAD_AFFINITY_POLICY, (thread_policy_t) &policyInfo,
                              THREAD_AFFINITY_POLICY_COUNT);
        return (result == KERN_SUCCESS);
    }
};

#endif

//   ▄▄                          ▄▄▄          ▄▄
//  ▄██▄▄  ▄▄▄▄  ▄▄▄▄▄▄▄  ▄▄▄▄▄   ██   ▄▄▄▄  ▄██▄▄  ▄▄▄▄   ▄▄▄▄
//   ██   ██▄▄██ ██ ██ ██ ██  ██  ██   ▄▄▄██  ██   ██▄▄██ ▀█▄▄▄
//   ▀█▄▄ ▀█▄▄▄  ██ ██ ██ ██▄▄█▀ ▄██▄ ▀█▄▄██  ▀█▄▄ ▀█▄▄▄   ▄▄▄█▀
//                        ██

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
    const char* end() const {
        return this->bytes + this->numBytes;
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

//   ▄▄▄▄
//  ██  ██ ▄▄▄▄▄  ▄▄  ▄▄
//  ██▀▀██ ██  ██ ██  ██
//  ██  ██ ██  ██ ▀█▄▄██
//                 ▄▄▄█▀

struct Any {
    uptr storage = 0;

    Any() = default;

    template <typename T, typename... Args>
    Any(T*, Args&&... args) {
        T* target = (T*) &storage;
        if (sizeof(T) > sizeof(storage)) {
            storage = (uptr) Heap.alloc(sizeof(T));
            target = (T*) storage;
        }
        new (target) T{std::forward<Args>(args)...};
    }

    template <typename T>
    Any(T&& other) {
        if (sizeof(T) > sizeof(storage)) {
            storage = other.storage;
            other.storage = nullptr;
        } else {
            (T&) storage = std::move((T&) other.storage);
        }
    }

    template <typename T>
    T* get() {
        return (sizeof(T) <= sizeof(storage)) ? (T*) &storage : (T*) storage;
    }

    template <typename T>
    void destruct() {
        subst::destructByMember(get<T>());
        if (sizeof(T) > sizeof(storage)) {
            Heap.free((void*) storage);
        }
    }
};

//   ▄▄▄▄   ▄▄          ▄▄               ▄▄   ▄▄ ▄▄        ▄▄
//  ██  ▀▀ ▄██▄▄ ▄▄▄▄▄  ▄▄ ▄▄▄▄▄   ▄▄▄▄▄ ███▄███ ▄▄ ▄▄  ▄▄ ▄▄ ▄▄▄▄▄
//   ▀▀▀█▄  ██   ██  ▀▀ ██ ██  ██ ██  ██ ██▀█▀██ ██  ▀██▀  ██ ██  ██
//  ▀█▄▄█▀  ▀█▄▄ ██     ██ ██  ██ ▀█▄▄██ ██   ██ ██ ▄█▀▀█▄ ██ ██  ██
//                                 ▄▄▄█▀

template <typename Derived>
struct StringMixin {
    template <typename T>
    T to(const T& defaultValue = subst::createDefault<T>()) const {
        return static_cast<const Derived*>(this)->view().to(defaultValue);
    }

    explicit operator bool() const {
        return (bool) static_cast<const Derived*>(this)->view();
    }

    bool isEmpty() const {
        return static_cast<const Derived*>(this)->view().isEmpty();
    }

    StringView subStr(u32 start) const {
        return static_cast<const Derived*>(this)->view().subStr(start);
    }
    StringView subStr(u32 start, u32 numBytes) const {
        return static_cast<const Derived*>(this)->view().subStr(start, numBytes);
    }

    StringView left(u32 numBytes) const {
        return static_cast<const Derived*>(this)->view().left(numBytes);
    }
    StringView shortenedBy(u32 numBytes) const {
        return static_cast<const Derived*>(this)->view().shortenedBy(numBytes);
    }
    StringView right(u32 numBytes) const {
        return static_cast<const Derived*>(this)->view().right(numBytes);
    }
    String operator+(StringView other) const;
    String operator*(u32 count) const;
    s32 findByte(char matchByte, u32 startPos = 0) const {
        return static_cast<const Derived*>(this)->view().findByte(matchByte, startPos);
    }
    template <typename MatchFuncOrChar>
    s32 findByte(const MatchFuncOrChar& matchFuncOrByte, u32 startPos = 0) const {
        return static_cast<const Derived*>(this)->view().findByte(matchFuncOrByte,
                                                                  startPos);
    }

    template <typename MatchFuncOrChar>
    s32 rfindByte(const MatchFuncOrChar& matchFuncOrByte, u32 startPos) const {
        return static_cast<const Derived*>(this)->view().rfindByte(matchFuncOrByte,
                                                                   startPos);
    }
    template <typename MatchFuncOrChar>
    s32 rfindByte(const MatchFuncOrChar& matchFuncOrByte) const {
        return static_cast<const Derived*>(this)->view().rfindByte(
            matchFuncOrByte, static_cast<const Derived*>(this)->numBytes - 1);
    }

    bool startsWith(StringView arg) const {
        return static_cast<const Derived*>(this)->view().startsWith(arg);
    }
    bool endsWith(StringView arg) const {
        return static_cast<const Derived*>(this)->view().endsWith(arg);
    }

    StringView trim(bool (*matchFunc)(char) = isWhite, bool left = true,
                    bool right = true) const {
        return static_cast<const Derived*>(this)->view().trim(matchFunc, left, right);
    }
    StringView ltrim(bool (*matchFunc)(char) = isWhite) const {
        return static_cast<const Derived*>(this)->view().ltrim(matchFunc);
    }
    StringView rtrim(bool (*matchFunc)(char) = isWhite) const {
        return static_cast<const Derived*>(this)->view().rtrim(matchFunc);
    }
    String join(ArrayView<const StringView> comps) const;
    auto splitByte(char sep) const {
        return static_cast<const Derived*>(this)->view().splitByte(sep);
    }
    String upperAsc() const;
    String lowerAsc() const;
    String reversedBytes() const;
    String filterBytes(char (*filterFunc)(char)) const;
    bool includesNullTerminator() const {
        return static_cast<const Derived*>(this)->view().includesNullTerminator();
    }
    HybridString withNullTerminator() const;
    StringView withoutNullTerminator() const {
        return static_cast<const Derived*>(this)->view().withoutNullTerminator();
    }
};

//   ▄▄▄▄   ▄▄          ▄▄
//  ██  ▀▀ ▄██▄▄ ▄▄▄▄▄  ▄▄ ▄▄▄▄▄   ▄▄▄▄▄
//   ▀▀▀█▄  ██   ██  ▀▀ ██ ██  ██ ██  ██
//  ▀█▄▄█▀  ▀█▄▄ ██     ██ ██  ██ ▀█▄▄██
//                                 ▄▄▄█▀
struct HybridString;

struct String : StringMixin<String> {
    using View = StringView;

    char* bytes = nullptr;
    u32 numBytes = 0;

    String() = default;
    String(StringView other);
    String(const String& other) : String{other.view()} {
    }
    String(const char* s) : String{StringView{s}} {
    }
    template <typename U, typename = std::enable_if_t<std::is_same<U, char>::value>>
    String(const U& u) : String{StringView{&u, 1}} {
    }
    String(String&& other) : bytes{other.bytes}, numBytes{other.numBytes} {
        other.bytes = nullptr;
        other.numBytes = 0;
    }
    String(HybridString&& other);

    ~String() {
        if (this->bytes) {
            Heap.free(this->bytes);
        }
    }

    template <typename = void>
    void operator=(StringView other) {
        char* bytesToFree = this->bytes;
        new (this) String{other};
        if (bytesToFree) {
            Heap.free(bytesToFree);
        }
    }

    void operator=(const String& other) {
        this->~String();
        new (this) String{other.view()};
    }
    void operator=(String&& other) {
        this->~String();
        new (this) String{std::move(other)};
    }

    operator const StringView&() const {
        return reinterpret_cast<const StringView&>(*this);
    }
    const StringView& view() const {
        return reinterpret_cast<const StringView&>(*this);
    }

    void clear() {
        if (this->bytes) {
            Heap.free(this->bytes);
        }
        this->bytes = nullptr;
        this->numBytes = 0;
    }

    void operator+=(StringView other) {
        *this = this->view() + other;
    }
    static String allocate(u32 numBytes);
    void resize(u32 numBytes);
    static String adopt(char* bytes, u32 numBytes) {
        String str;
        str.bytes = bytes;
        str.numBytes = numBytes;
        return str;
    }

    char* release() {
        char* r = this->bytes;
        this->bytes = nullptr;
        this->numBytes = 0;
        return r;
    }

    template <typename... Args>
    static String format(StringView fmt, const Args&... args);

    const char& operator[](u32 index) const {
        PLY_ASSERT(index < this->numBytes);
        return this->bytes[index];
    }
    char& operator[](u32 index) {
        PLY_ASSERT(index < this->numBytes);
        return this->bytes[index];
    }

    const char& back(s32 ofs = -1) const {
        PLY_ASSERT(u32(-ofs - 1) < this->numBytes);
        return this->bytes[this->numBytes + ofs];
    }
    char& back(s32 ofs = -1) {
        PLY_ASSERT(u32(-ofs - 1) < this->numBytes);
        return this->bytes[this->numBytes + ofs];
    }
};

namespace impl {
template <>
struct InitListType<String> {
    using Type = StringView;
};
} // namespace impl

template <typename Derived>
String StringMixin<Derived>::operator+(StringView other) const {
    return static_cast<const Derived*>(this)->view() + other;
}

template <typename Derived>
String StringMixin<Derived>::operator*(u32 count) const {
    return static_cast<const Derived*>(this)->view() * count;
}

template <typename Derived>
String StringMixin<Derived>::join(ArrayView<const StringView> comps) const {
    return static_cast<const Derived*>(this)->view().join(comps);
}

template <typename Derived>
String StringMixin<Derived>::upperAsc() const {
    return static_cast<const Derived*>(this)->view().upperAsc();
}

template <typename Derived>
String StringMixin<Derived>::lowerAsc() const {
    return static_cast<const Derived*>(this)->view().lowerAsc();
}

template <typename Derived>
String StringMixin<Derived>::reversedBytes() const {
    return static_cast<const Derived*>(this)->view().reversedBytes();
}

template <typename Derived>
String StringMixin<Derived>::filterBytes(char (*filterFunc)(char)) const {
    return static_cast<const Derived*>(this)->view().filterBytes(filterFunc);
}

//  ▄▄  ▄▄        ▄▄            ▄▄     ▄▄  ▄▄▄▄   ▄▄          ▄▄
//  ██  ██ ▄▄  ▄▄ ██▄▄▄  ▄▄▄▄▄  ▄▄  ▄▄▄██ ██  ▀▀ ▄██▄▄ ▄▄▄▄▄  ▄▄ ▄▄▄▄▄   ▄▄▄▄▄
//  ██▀▀██ ██  ██ ██  ██ ██  ▀▀ ██ ██  ██  ▀▀▀█▄  ██   ██  ▀▀ ██ ██  ██ ██  ██
//  ██  ██ ▀█▄▄██ ██▄▄█▀ ██     ██ ▀█▄▄██ ▀█▄▄█▀  ▀█▄▄ ██     ██ ██  ██ ▀█▄▄██
//          ▄▄▄█▀                                                        ▄▄▄█▀

struct HybridString : StringMixin<HybridString> {
    char* bytes;
    u32 isOwner : 1;
    u32 numBytes : 31;

    HybridString() : bytes{nullptr}, isOwner{0}, numBytes{0} {
    }
    HybridString(StringView view)
        : bytes{const_cast<char*>(view.bytes)}, isOwner{0}, numBytes{view.numBytes} {
        PLY_ASSERT(view.numBytes < (1u << 30));
    }

    HybridString(const String& str) : HybridString{str.view()} {
    }
    HybridString(String&& str) {
        this->isOwner = 1;
        PLY_ASSERT(str.numBytes < (1u << 30));
        this->numBytes = str.numBytes;
        this->bytes = str.release();
    }

    HybridString(HybridString&& other)
        : bytes{other.bytes}, isOwner{other.isOwner}, numBytes{other.numBytes} {
        other.bytes = nullptr;
        other.isOwner = 0;
        other.numBytes = 0;
    }

    HybridString(const HybridString& other);

    HybridString(const char* s)
        : bytes{const_cast<char*>(s)}, isOwner{0},
          numBytes{(u32) std::char_traits<char>::length(s)} {
        PLY_ASSERT(s[this->numBytes] ==
                   0); // Sanity check; numBytes must fit in 31-bit field
    }

    ~HybridString() {
        if (this->isOwner) {
            Heap.free(this->bytes);
        }
    }

    void operator=(HybridString&& other) {
        this->~HybridString();
        new (this) HybridString(std::move(other));
    }

    void operator=(const HybridString& other) {
        this->~HybridString();
        new (this) HybridString(other);
    }

    operator StringView() const {
        return {this->bytes, this->numBytes};
    }

    StringView view() const {
        return {this->bytes, this->numBytes};
    }
};

template <typename Derived>
HybridString StringMixin<Derived>::withNullTerminator() const {
    return static_cast<const Derived*>(this)->view().withNullTerminator();
}

//   ▄▄▄▄
//  ██  ██ ▄▄▄▄▄  ▄▄▄▄▄   ▄▄▄▄  ▄▄  ▄▄
//  ██▀▀██ ██  ▀▀ ██  ▀▀  ▄▄▄██ ██  ██
//  ██  ██ ██     ██     ▀█▄▄██ ▀█▄▄██
//                               ▄▄▄█▀

namespace impl {
struct BaseArray {
    void* m_items = nullptr;
    u32 m_numItems = 0;
    u32 m_allocated = 0;

    BaseArray() = default;

    void alloc(u32 numItems, u32 itemSize);
    void realloc(u32 numItems, u32 itemSize);
    void free();
    void reserve(u32 numItems, u32 itemSize); // m_numItems is unaffected
    void reserveIncrement(u32 itemSize);      // m_numItems is unaffected
    void truncate(u32 itemSize);
};
} // namespace impl

template <typename T>
class Array {
private:
    T* items;
    u32 numItems_;
    u32 allocated;

    // T cannot be const
    PLY_STATIC_ASSERT(!std::is_const<T>::value);

    // Arrays of C-style arrays such as Array<int[2]> are not allowed.
    // One reason is that placement new doesn't work (to copy/move/construct new items).
    // Another reason is that it confuses the .natvis debug visualizer in Visual Studio.
    // Make an Array of FixedArray<> instead.
    PLY_STATIC_ASSERT(!std::is_array<T>::value);

    template <typename>
    friend class Array;

    Array(T* items, u32 numItems, u32 allocator)
        : items{items}, numItems_{numItems}, allocated{allocated} {
    }

public:
    // Constructors
    Array() : items{nullptr}, numItems_{0}, allocated{0} {
    }
    Array(const Array& other) {
        ((impl::BaseArray&) *this).alloc(other.numItems_, (u32) sizeof(T));
        subst::unsafeConstructArrayFrom(this->items, other.items, other.numItems_);
    }
    Array(Array&& other)
        : items{other.items}, numItems_{other.numItems_}, allocated{other.allocated} {
        other.items = nullptr;
        other.numItems_ = 0;
        other.allocated = 0;
    }
    Array(InitList<T> init) {
        u32 initSize = safeDemote<u32>(init.size());
        ((impl::BaseArray&) *this).alloc(initSize, (u32) sizeof(T));
        subst::constructArrayFrom(this->items, init.begin(), initSize);
    }
    template <typename Other, typename U = impl::ArrayViewType<Other>>
    Array(Other&& other) {
        ((impl::BaseArray&) *this).alloc(ArrayView<U>{other}.numItems, (u32) sizeof(T));
        impl::moveOrCopyConstruct(this->items, std::forward<Other>(other));
    }
    ~Array() {
        PLY_STATIC_ASSERT(sizeof(Array) ==
                          sizeof(impl::BaseArray)); // Sanity check binary compatibility
        subst::destructArray(this->items, this->numItems_);
        Heap.free(this->items);
    }

    void operator=(const Array& other) {
        ArrayView<T> arrayToFree = {this->items, this->numItems_};
        new (this) Array{other};
        subst::destructArray(arrayToFree.items, arrayToFree.numItems);
        Heap.free(arrayToFree.items);
    }

    void operator=(Array&& other) {
        ArrayView<T> arrayToFree = {this->items, this->numItems_};
        new (this) Array{std::move(other)};
        subst::destructArray(arrayToFree.items, arrayToFree.numItems);
        Heap.free(arrayToFree.items);
    }
    void operator=(std::initializer_list<T> init) {
        subst::destructArray(this->items, this->numItems_);
        u32 initSize = safeDemote<u32>(init.size());
        ((impl::BaseArray&) *this).realloc(initSize, (u32) sizeof(T));
        subst::unsafeConstructArrayFrom(this->items, init.begin(), initSize);
    }
    template <typename Other, typename = impl::ArrayViewType<Other>>
    void operator=(Other&& other) {
        ArrayView<T> arrayToFree = {this->items, this->numItems_};
        new (this) Array{std::forward<Other>(other)};
        subst::destructArray(arrayToFree.items, arrayToFree.numItems);
        Heap.free(arrayToFree.items);
    }

    T& operator[](u32 index) {
        PLY_ASSERT(index < this->numItems_);
        return this->items[index];
    }
    const T& operator[](u32 index) const {
        PLY_ASSERT(index < this->numItems_);
        return this->items[index];
    }

    T* get(u32 index = 0) {
        PLY_ASSERT(index < this->numItems_);
        return this->items + index;
    }
    const T* get(u32 index = 0) const {
        PLY_ASSERT(index < this->numItems_);
        return this->items + index;
    }

    T& back(s32 offset = -1) {
        PLY_ASSERT(offset < 0 && u32(-offset) <= this->numItems_);
        return this->items[this->numItems_ + offset];
    }
    const T& back(s32 offset = -1) const {
        PLY_ASSERT(offset < 0 && u32(-offset) <= this->numItems_);
        return this->items[this->numItems_ + offset];
    }

    T* begin() const {
        return this->items;
    }
    T* end() const {
        return this->items + this->numItems_;
    }

    explicit operator bool() const {
        return this->numItems_ > 0;
    }
    bool isEmpty() const {
        return this->numItems_ == 0;
    }
    u32 numItems() const {
        return this->numItems_;
    }
    u32 sizeBytes() const {
        return this->numItems_ * (u32) sizeof(T);
    }
    void clear() {
        subst::destructArray(this->items, this->numItems_);
        Heap.free(this->items);
        this->items = nullptr;
        this->numItems_ = 0;
        this->allocated = 0;
    }

    void reserve(u32 numItems) {
        ((impl::BaseArray&) *this).reserve(numItems, (u32) sizeof(T));
    }
    void resize(u32 numItems) {
        if (numItems < this->numItems_) {
            subst::destructArray(this->items + numItems, this->numItems_ - numItems);
        }
        ((impl::BaseArray&) *this).reserve(numItems, (u32) sizeof(T));
        if (numItems > this->numItems_) {
            subst::constructArray(this->items + this->numItems_,
                                  numItems - this->numItems_);
        }
        this->numItems_ = numItems;
    }

    void truncate() {
        ((impl::BaseArray&) *this).truncate((u32) sizeof(T));
    }

    T& append(T&& item) {
        // The argument must not be a reference to an existing item in the array:
        PLY_ASSERT((&item < this->items) || (&item >= this->items + this->numItems_));
        if (this->numItems_ >= this->allocated) {
            ((impl::BaseArray&) *this).reserveIncrement((u32) sizeof(T));
        }
        T* result = new (this->items + this->numItems_) T{std::move(item)};
        this->numItems_++;
        return *result;
    }
    T& append(const T& item) {
        // The argument must not be a reference to an existing item in the array:
        PLY_ASSERT((&item < this->items) || (&item >= this->items + this->numItems_));
        if (this->numItems_ >= this->allocated) {
            ((impl::BaseArray&) *this).reserveIncrement((u32) sizeof(T));
        }
        T* result = new (this->items + this->numItems_) T{item};
        this->numItems_++;
        return *result;
    }
    template <typename... Args>
    T& append(Args&&... args) {
        if (this->numItems_ >= this->allocated) {
            ((impl::BaseArray&) *this).reserveIncrement((u32) sizeof(T));
        }
        T* result = new (this->items + this->numItems_) T{std::forward<Args>(args)...};
        this->numItems_++;
        return *result;
    }

    void extend(InitList<T> init) {
        u32 initSize = safeDemote<u32>(init.size());
        ((impl::BaseArray&) *this).reserve(this->numItems_ + initSize, (u32) sizeof(T));
        subst::constructArrayFrom(this->items + this->numItems_, init.begin(),
                                  initSize);
        this->numItems_ += initSize;
    }
    template <typename Other, typename U = impl::ArrayViewType<Other>>
    void extend(Other&& other) {
        u32 numOtherItems = ArrayView<U>{other}.numItems;
        ((impl::BaseArray&) *this)
            .reserve(this->numItems_ + numOtherItems, (u32) sizeof(T));
        impl::moveOrCopyConstruct(this->items + this->numItems_,
                                  std::forward<Other>(other));
        this->numItems_ += numOtherItems;
    }
    void moveExtend(ArrayView<T> other) {
        // The argument must not be a subview into the array itself:
        PLY_ASSERT((other.end() <= this->items) || (other.items >= this->end()));
        ((impl::BaseArray&) *this)
            .reserve(this->numItems_ + other.numItems, (u32) sizeof(T));
        subst::moveConstructArray(this->items + this->numItems_, other.items,
                                  other.numItems);
        this->numItems_ += other.numItems;
    }

    void pop(u32 count = 1) {
        PLY_ASSERT(count <= this->numItems_);
        resize(this->numItems_ - count);
    }
    T& insert(u32 pos, u32 count = 1) {
        PLY_ASSERT(pos <= this->numItems_);
        ((impl::BaseArray&) *this).reserve(this->numItems_ + count, (u32) sizeof(T));
        memmove(static_cast<void*>(this->items + pos + count),
                static_cast<const void*>(this->items + pos),
                (this->numItems_ - pos) * sizeof(T)); // Underlying type is relocatable
        subst::constructArray(this->items + pos, count);
        this->numItems_ += count;
        return this->items[pos];
    }

    void erase(u32 pos, u32 count = 1) {
        PLY_ASSERT(pos + count <= this->numItems_);
        subst::destructArray(this->items + pos, count);
        memmove(static_cast<void*>(this->items + pos),
                static_cast<const void*>(this->items + pos + count),
                (this->numItems_ - (pos + count)) *
                    sizeof(T)); // Underlying type is relocatable
        this->numItems_ -= count;
    }
    void eraseQuick(u32 pos) {
        PLY_ASSERT(pos < this->numItems_);
        this->items[pos].~T();
        memcpy(static_cast<void*>(this->items + pos),
               static_cast<const void*>(this->items + (this->numItems_ - 1)),
               sizeof(T));
        this->numItems_--;
    }
    void eraseQuick(u32 pos, u32 count) {
        PLY_ASSERT(pos + count <= this->numItems_);
        subst::destructArray(this->items + pos, count);
        memmove(static_cast<void*>(this->items + pos),
                static_cast<const void*>(this->items + this->numItems_ - count),
                count * sizeof(T)); // Underlying type is relocatable
        this->numItems_ -= count;
    }

    static Array adopt(T* items, u32 numItems) {
        return {items, numItems, numItems};
    }
    T* release() {
        T* items = this->items;
        this->items = nullptr;
        this->numItems_ = 0;
        this->allocated = 0;
        return items;
    }

    template <typename Arr0, typename Arr1, typename, typename>
    friend auto operator+(Arr0&& a, Arr1&& b);

    ArrayView<T> view() {
        return {this->items, this->numItems_};
    }
    ArrayView<const T> view() const {
        return {this->items, this->numItems_};
    }
    operator ArrayView<T>() {
        return {this->items, this->numItems_};
    }
    operator ArrayView<const T>() const {
        return {this->items, this->numItems_};
    }

    StringView stringView() const {
        return {(const char*) this->items,
                safeDemote<u32>(this->numItems_ * sizeof(T))};
    }
    MutStringView mutableStringView() const {
        return {(char*) this->items, safeDemote<u32>(this->numItems_ * sizeof(T))};
    }

    ArrayView<T> subView(u32 start) {
        return view().subView(start);
    }
    ArrayView<const T> subView(u32 start) const {
        return view().subView(start);
    }
    ArrayView<T> subView(u32 start, u32 numItems_) {
        return view().subView(start, numItems_);
    }
    ArrayView<const T> subView(u32 start, u32 numItems_) const {
        return view().subView(start, numItems_);
    }
};

namespace impl {
template <typename T>
struct InitListType<Array<T>> {
    using Type = ArrayView<const T>;
};

template <typename T>
struct ArrayTraits<Array<T>> {
    using ItemType = T;
    static constexpr bool IsOwner = true;
};
} // namespace impl

template <typename Arr0, typename Arr1, typename T0 = impl::ArrayViewType<Arr0>,
          typename T1 = impl::ArrayViewType<Arr1>>
auto operator+(Arr0&& a, Arr1&& b) {
    u32 numItemsA = ArrayView<T0>{a}.numItems;
    u32 numItemsB = ArrayView<T1>{b}.numItems;

    Array<std::remove_const_t<T0>> result;
    ((impl::BaseArray&) result).alloc(numItemsA + numItemsB, (u32) sizeof(T0));
    impl::moveOrCopyConstruct(result.items, std::forward<Arr0>(a));
    impl::moveOrCopyConstruct(result.items + numItemsA, std::forward<Arr1>(b));
    return result;
}

//  ▄▄▄▄▄ ▄▄                   ▄▄  ▄▄▄▄
//  ██    ▄▄ ▄▄  ▄▄  ▄▄▄▄   ▄▄▄██ ██  ██ ▄▄▄▄▄  ▄▄▄▄▄   ▄▄▄▄  ▄▄  ▄▄
//  ██▀▀  ██  ▀██▀  ██▄▄██ ██  ██ ██▀▀██ ██  ▀▀ ██  ▀▀  ▄▄▄██ ██  ██
//  ██    ██ ▄█▀▀█▄ ▀█▄▄▄  ▀█▄▄██ ██  ██ ██     ██     ▀█▄▄██ ▀█▄▄██
//                                                             ▄▄▄█▀

template <typename T, u32 Size>
struct FixedArray {
#if PLY_COMPILER_MSVC
#pragma warning(push)
#pragma warning( \
    disable : 4200) // nonstandard extension used: zero-sized array in struct/union
#endif
    T items[Size];
#if PLY_COMPILER_MSVC
#pragma warning(pop)
#endif

    FixedArray() = default;

    FixedArray(InitList<T> args) {
        PLY_ASSERT(Size == args.size());
        subst::constructArrayFrom(this->items, args.begin(), Size);
    }

    template <typename... Args>
    FixedArray(Args&&... args) {
        PLY_STATIC_ASSERT(Size == sizeof...(Args));
        impl::InitItems<T>::init(items, std::forward<Args>(args)...);
    }

    constexpr u32 numItems() const {
        return Size;
    }

    T& operator[](u32 i) {
        PLY_ASSERT(i < Size);
        return items[i];
    }

    const T& operator[](u32 i) const {
        PLY_ASSERT(i < Size);
        return items[i];
    }

    ArrayView<T> view() {
        return {items, Size};
    }

    ArrayView<const T> view() const {
        return {items, Size};
    }

    operator ArrayView<T>() {
        return {items, Size};
    }

    operator ArrayView<const T>() const {
        return {items, Size};
    }

    MutStringView mutableStringView() {
        return {reinterpret_cast<char*>(items), safeDemote<u32>(Size * sizeof(T))};
    }

    StringView stringView() const {
        return {reinterpret_cast<const char*>(items),
                safeDemote<u32>(Size * sizeof(T))};
    }

    T* begin() {
        return items;
    }

    T* end() {
        return items + Size;
    }

    const T* begin() const {
        return items;
    }

    const T* end() const {
        return items + Size;
    }
};

namespace impl {
template <typename T, u32 Size>
struct InitListType<FixedArray<T, Size>> {
    using Type = ArrayView<const T>;
};

template <typename T, u32 Size>
struct ArrayTraits<FixedArray<T, Size>> {
    using ItemType = T;
    static constexpr bool IsOwner = true;
};
} // namespace impl

//         ▄▄▄                       ▄▄  ▄▄   ▄▄
//   ▄▄▄▄   ██   ▄▄▄▄▄  ▄▄▄▄  ▄▄▄▄▄  ▄▄ ▄██▄▄ ██▄▄▄  ▄▄▄▄▄▄▄   ▄▄▄▄
//   ▄▄▄██  ██  ██  ██ ██  ██ ██  ▀▀ ██  ██   ██  ██ ██ ██ ██ ▀█▄▄▄
//  ▀█▄▄██ ▄██▄ ▀█▄▄██ ▀█▄▄█▀ ██     ██  ▀█▄▄ ██  ██ ██ ██ ██  ▄▄▄█▀
//               ▄▄▄█▀

// ┏━━━━━━━━┓
// ┃  find  ┃
// ┗━━━━━━━━┛
PLY_MAKE_WELL_FORMEDNESS_CHECK_2(IsComparable, std::declval<T0>() == std::declval<T1>());
PLY_MAKE_WELL_FORMEDNESS_CHECK_2(IsCallable, std::declval<T0>()(std::declval<T1>()));

template <typename T, typename U, std::enable_if_t<IsComparable<T, U>, int> = 0>
PLY_INLINE s32 find(ArrayView<const T> arr, const U& item) {
    for (u32 i = 0; i < arr.numItems; i++) {
        if (arr[i] == item)
            return i;
    }
    return -1;
}

template <typename T, typename Callback, std::enable_if_t<IsCallable<Callback, T>, int> = 0>
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

template <typename T, typename Callback, std::enable_if_t<IsCallable<Callback, T>, int> = 0>
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
          typename MappedItemType = std::decay_t<
              decltype(std::declval<MapFunc>()(std::declval<impl::ItemType<Iterable>>()))>>
Array<MappedItemType> map(Iterable&& iterable, MapFunc&& mapFunc) {
    Array<MappedItemType> result;
    // FIXME: Reserve memory for result when possible. Otherwise, use a typed ChunkBuffer.
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
PLY_NO_INLINE void sort(ArrayView<T> view, const IsLess& isLess = impl::defaultLess<T>) {
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
    // Now, everything to left of lo is <= pivot, and everything from hi onwards is >= pivot.
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

template <typename Arr, typename IsLess = decltype(impl::defaultLess<impl::ArrayViewType<Arr>>)>
PLY_INLINE void sort(Arr& arr, const IsLess& isLess = impl::defaultLess<impl::ArrayViewType<Arr>>) {
    using T = impl::ArrayViewType<Arr>;
    sort(ArrayView<T>{arr}, isLess);
}

} // namespace ply
