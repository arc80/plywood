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
    Random();
    Random(u64 seed); // Explicit seed
    u64 next64();
    u32 next32() {
        return (u32) next64();
    }
    u16 next16() {
        return (u16) next64();
    }
    u8 next8() {
        return (u8) next64();
    }
    float next_float() {
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
        operator s64() const {
            return s64(ticks.count());
        }
    };

    struct Point {
        using Tick = std::chrono::high_resolution_clock::time_point;
        Tick tick;
        Point(s64 v = 0) : tick{Duration::Ticks{v}} {
        }
        Point(const Tick& tick) : tick{tick} {
        }
        Point operator+(Duration d) const {
            return {tick + d.ticks};
        }
        Duration operator-(Point b) const {
            return {tick - b.tick};
        }
        bool operator<(Point b) const {
            return tick < b.tick;
        }
        bool operator<=(Point b) const {
            return tick <= b.tick;
        }
        bool operator>(Point b) const {
            return tick > b.tick;
        }
        bool operator>=(Point b) const {
            return tick >= b.tick;
        }
        bool operator==(Point b) const {
            return tick == b.tick;
        }
    };

    static Point get() {
        return {std::chrono::high_resolution_clock::now()};
    }

    struct Converter {
        Converter() {
        }
        static float to_seconds(Duration duration) {
            return std::chrono::duration_cast<std::chrono::duration<float>>(
                       duration.ticks)
                .count();
        }
        Duration to_duration(float seconds) const {
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
    s8 time_zone_hour = 0;
    u8 time_zone_minute = 0;
    u32 microseconds = 0;

    // Number of microseconds since January 1, 1970 at 00:00:00 UTC.
    static s64 get_current_epoch_microseconds();

    // Conversion
    static DateTime from_epoch_microseconds(s64 us);
    s64 to_epoch_microseconds() const;
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

static TID get_current_thread_id() {
#if PLY_CPU_X64
    return ((DWORD*) __readgsqword(48))[18]; // Read directly from the TIB
#elif PLY_CPU_X86
    return ((DWORD*) __readfsdword(24))[9]; // Read directly from the TIB
#else
    return GetCurrentThreadID();
#endif
}

static PID get_current_process_id() {
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

    static TID get_current_thread_id() {
        return pthread_mach_thread_np(pthread_self());
    }

    static PID get_current_process_id() {
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

    static TID get_current_thread_id() {
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

    static PID get_current_process_id() {
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
    Affinity();

    bool is_accurate() const {
        return m_isAccurate;
    }

    u32 get_num_physical_cores() const {
        return static_cast<u32>(m_numPhysicalCores);
    }

    u32 get_num_hwthreads() const {
        return static_cast<u32>(m_numHWThreads);
    }

    u32 get_num_hwthreads_for_core(ureg core) const {
        PLY_ASSERT(core < m_numPhysicalCores);
        return static_cast<u32>(count_set_bits(m_physicalCoreMasks[core]));
    }

    bool set_affinity(ureg core, ureg hw_thread);
};

#elif PLY_KERNEL_LINUX
// ┏━━━━━━━━━┓
// ┃  Linux  ┃
// ┗━━━━━━━━━┛
class Affinity_Linux {
private:
    struct CoreInfo {
        std::vector<u32> hw_thread_index_to_logical_processor;
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

        s32 logical_processor;
        CoreID core_id;
        std::map<CoreID, u32> core_idto_index;

        CoreInfoCollector() : logical_processor(-1) {
        }

        void flush(Affinity_Linux& affinity) {
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

public:
    Affinity_Linux();

    bool is_accurate() const {
        return m_isAccurate;
    }

    u32 get_num_physical_cores() const {
        return m_coreIndexToInfo.size();
    }

    u32 get_num_hwthreads() const {
        return m_numHWThreads;
    }

    u32 get_num_hwthreads_for_core(ureg core) const {
        return m_coreIndexToInfo[core].hw_thread_index_to_logical_processor.size();
    }

    bool set_affinity(ureg core, ureg hw_thread);
};

#elif PLY_KERNEL_FREEBSD
// ┏━━━━━━━━━━━┓
// ┃  FreeBSD  ┃
// ┗━━━━━━━━━━━┛
class Affinity {
private:
    struct CoreInfo {
        std::vector<u32> hw_thread_index_to_logical_processor;
    };
    bool m_isAccurate;
    std::vector<CoreInfo> m_coreIndexToInfo;
    u32 m_numHWThreads;

public:
    Affinity();

    bool is_accurate() const {
        return m_isAccurate;
    }

    u32 get_num_physical_cores() const {
        return m_coreIndexToInfo.size();
    }

    u32 get_num_hwthreads() const {
        return m_numHWThreads;
    }

    u32 get_num_hwthreads_for_core(ureg core) const {
        return m_coreIndexToInfo[core].hw_thread_index_to_logical_processor.size();
    }

    bool set_affinity(ureg core, ureg hw_thread);
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
        size_t count_len = sizeof(count);
        if (sysctlbyname("hw.logicalcpu", &count, &count_len, NULL, 0) == 0) {
            if (count > 0) {
                m_numHWThreads = (u32) count;
                // Get # of physical cores
                size_t count_len = sizeof(count);
                if (sysctlbyname("hw.physicalcpu", &count, &count_len, NULL, 0) == 0) {
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

    bool is_accurate() const {
        return m_isAccurate;
    }

    u32 get_num_physical_cores() const {
        return m_numPhysicalCores;
    }

    u32 get_num_hwthreads() const {
        return m_numHWThreads;
    }

    u32 get_num_hwthreads_for_core(ureg core) const {
        PLY_ASSERT(core < m_numPhysicalCores);
        return m_hwThreadsPerCore;
    }

    bool set_affinity(ureg core, ureg hw_thread) {
        PLY_ASSERT(core < m_numPhysicalCores);
        PLY_ASSERT(hw_thread < m_hwThreadsPerCore);
        u32 index = core * m_hwThreadsPerCore + hw_thread;
        thread_t thread = mach_thread_self();
        thread_affinity_policy_data_t policy_info = {(integer_t) index};
        // Note: The following returns KERN_NOT_SUPPORTED on i_os. (Tested on i_os
        // 9.2.)
        kern_return_t result = thread_policy_set(thread, THREAD_AFFINITY_POLICY,
                                                 (thread_policy_t) &policy_info,
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
// ┃  create_default  ┃
// ┗━━━━━━━━━━━━━━━━━┛
template <typename T, std::enable_if_t<std::is_arithmetic<T>::value, int> = 0>
T create_default() {
    return 0;
}

template <typename T,
          std::enable_if_t<
              !std::is_arithmetic<T>::value && !std::is_same<T, void>::value, int> = 0>
T create_default() {
    return {};
}

template <typename T, std::enable_if_t<std::is_same<T, void>::value, int> = 0>
T create_default() {
}

// ┏━━━━━━━━━━━━━━━━━━━┓
// ┃  unsafe_construct  ┃
// ┗━━━━━━━━━━━━━━━━━━━┛
template <typename T,
          std::enable_if_t<std::is_default_constructible<T>::value, int> = 0>
void unsafe_construct(T* obj) {
    new (obj) T;
}

template <typename T,
          std::enable_if_t<!std::is_default_constructible<T>::value, int> = 0>
void unsafe_construct(T* obj) {
    // Not constructible
    PLY_FORCE_CRASH();
}

// ┏━━━━━━━━━━━━━━━━━━┓
// ┃  create_by_member  ┃
// ┗━━━━━━━━━━━━━━━━━━┛
PLY_MAKE_WELL_FORMEDNESS_CHECK_1(HasCreate, T0::create())

template <typename T, std::enable_if_t<HasCreate<T>, int> = 0>
T* create_by_member() {
    return T::create();
}

template <
    typename T,
    std::enable_if_t<!HasCreate<T> && std::is_default_constructible<T>::value, int> = 0>
T* create_by_member() {
    return new T;
}

template <typename T,
          std::enable_if_t<!HasCreate<T> && !std::is_default_constructible<T>::value,
                           int> = 0>
T* create_by_member() {
    // Not constructible
    PLY_FORCE_CRASH();
    return nullptr;
}

// ┏━━━━━━━━━━━━━━━━━━━┓
// ┃  destroy_by_member  ┃
// ┗━━━━━━━━━━━━━━━━━━━┛
PLY_MAKE_WELL_FORMEDNESS_CHECK_1(HasDestroyMember, std::declval<T0>().destroy())
PLY_MAKE_WELL_FORMEDNESS_CHECK_1(HasNamespaceDestroy, destroy((T0*) nullptr))

template <typename T, std::enable_if_t<HasDestroyMember<T>, int> = 0>
void destroy_by_member(T* obj) {
    if (obj) {
        obj->destroy();
    }
}

template <typename T,
          std::enable_if_t<!HasDestroyMember<T> && HasNamespaceDestroy<T>, int> = 0>
void destroy_by_member(T* obj) {
    if (obj) {
        destroy(obj);
    }
}

template <typename T,
          std::enable_if_t<!HasDestroyMember<T> && !HasNamespaceDestroy<T>, int> = 0>
void destroy_by_member(T* obj) {
    delete obj; // Passing nullptr to delete is allowed
}

// ┏━━━━━━━━━━━━━━━━━━━━┓
// ┃  destruct_by_member  ┃
// ┗━━━━━━━━━━━━━━━━━━━━┛
PLY_MAKE_WELL_FORMEDNESS_CHECK_1(HasDestruct, std::declval<T0>().destruct())

template <typename T, std::enable_if_t<HasDestruct<T>, int> = 0>
void destruct_by_member(T* obj) {
    obj->destruct();
}

template <typename T, std::enable_if_t<!HasDestruct<T>, int> = 0>
void destruct_by_member(T* obj) {
    obj->~T();
}

// ┏━━━━━━━━━━━━━━━━━━━━━━━┓
// ┃  unsafe_move_construct  ┃
// ┗━━━━━━━━━━━━━━━━━━━━━━━┛
template <typename T, std::enable_if_t<std::is_move_constructible<T>::value, int> = 0>
void unsafe_move_construct(T* dst, T* src) {
    new (dst) T{std::move(*src)};
}

template <typename T, std::enable_if_t<!std::is_move_constructible<T>::value, int> = 0>
void unsafe_move_construct(T* dst, T* src) {
    // Not move constructible
    PLY_FORCE_CRASH();
}

// ┏━━━━━━━━━━━━━━┓
// ┃  unsafe_copy  ┃
// ┗━━━━━━━━━━━━━━┛
template <typename T, std::enable_if_t<std::is_copy_assignable<T>::value, int> = 0>
void unsafe_copy(T* dst, const T* src) {
    *dst = *src;
}

template <typename T, std::enable_if_t<!std::is_copy_assignable<T>::value, int> = 0>
void unsafe_copy(T* dst, const T* src) {
    // Not copy assignable
    PLY_FORCE_CRASH();
}

// ┏━━━━━━━━━━━━━━┓
// ┃  unsafe_move  ┃
// ┗━━━━━━━━━━━━━━┛
template <typename T, std::enable_if_t<std::is_move_assignable<T>::value, int> = 0>
void unsafe_move(T* dst, T* src) {
    *dst = std::move(*src);
}

template <typename T, std::enable_if_t<!std::is_move_assignable<T>::value, int> = 0>
void unsafe_move(T* dst, T* src) {
    // Not move assignable
    PLY_FORCE_CRASH();
}

// ┏━━━━━━━━━━━━━━━━━━┓
// ┃  construct_array  ┃
// ┗━━━━━━━━━━━━━━━━━━┛
template <typename T,
          std::enable_if_t<std::is_trivially_default_constructible<T>::value, int> = 0>
void construct_array(T* items, s32 size) {
    // Trivially constructible
}

template <typename T,
          std::enable_if_t<!std::is_trivially_default_constructible<T>::value, int> = 0>
void construct_array(T* items, s32 size) {
    // Explicitly constructble
    while (size-- > 0) {
        new (items++) T;
    }
}

// ┏━━━━━━━━━━━━━━━━━┓
// ┃  destruct_array  ┃
// ┗━━━━━━━━━━━━━━━━━┛
template <typename T,
          std::enable_if_t<std::is_trivially_destructible<T>::value, int> = 0>
void destruct_array(T* items, s32 size) {
    // Trivially destructible
}

template <typename T,
          std::enable_if_t<!std::is_trivially_destructible<T>::value, int> = 0>
void destruct_array(T* items, s32 size) {
    // Explicitly destructble
    while (size-- > 0) {
        (items++)->~T();
    }
}

// ┏━━━━━━━━━━━━━━━━━━━━━━┓
// ┃  construct_array_from  ┃
// ┗━━━━━━━━━━━━━━━━━━━━━━┛
template <typename T,
          std::enable_if_t<std::is_trivially_copy_constructible<T>::value, int> = 0>
void construct_array_from(T* dst, const T* src, s32 size) {
    // Trivially copy constructible
    memcpy(dst, src, sizeof(T) * size);
}

template <typename T, typename U,
          std::enable_if_t<std::is_constructible<T, const U&>::value, int> = 0>
void construct_array_from(T* dst, const U* src, s32 size) {
    // Invoke constructor explicitly on each item
    while (size-- > 0) {
        // Use parentheses instead of curly braces to avoid narrowing conversion errors.
        new (dst++) T(*src++);
    }
}

// ┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
// ┃  unsafe_construct_array_from  ┃
// ┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛
template <typename T, typename U,
          std::enable_if_t<std::is_constructible<T, const U&>::value, int> = 0>
void unsafe_construct_array_from(T* dst, const U* src, s32 size) {
    construct_array_from(dst, src, size);
}

template <typename T, typename U,
          std::enable_if_t<!std::is_constructible<T, const U&>::value, int> = 0>
void unsafe_construct_array_from(T* dst, const U* src, s32 size) {
    PLY_FORCE_CRASH();
}

// ┏━━━━━━━━━━━━━━━━━━━━━━┓
// ┃  move_construct_array  ┃
// ┗━━━━━━━━━━━━━━━━━━━━━━┛
template <typename T,
          std::enable_if_t<std::is_trivially_move_constructible<T>::value, int> = 0>
void move_construct_array(T* dst, const T* src, s32 size) {
    // Trivially move constructible
    memcpy(dst, src, sizeof(T) * size);
}

template <typename T, typename U,
          std::enable_if_t<std::is_constructible<T, U&&>::value, int> = 0>
void move_construct_array(T* dst, U* src, s32 size) {
    // Explicitly move constructible
    while (size-- > 0) {
        new (dst++) T{std::move(*src++)};
    }
}

// ┏━━━━━━━━━━━━━┓
// ┃  copy_array  ┃
// ┗━━━━━━━━━━━━━┛
template <typename T,
          std::enable_if_t<std::is_trivially_copy_assignable<T>::value, int> = 0>
void copy_array(T* dst, const T* src, s32 size) {
    // Trivially copy assignable
    memcpy(dst, src, sizeof(T) * size);
}

template <typename T,
          std::enable_if_t<!std::is_trivially_copy_assignable<T>::value, int> = 0>
void copy_array(T* dst, const T* src, s32 size) {
    // Explicitly copy assignable
    while (size-- > 0) {
        *dst++ = *src++;
    }
}

// ┏━━━━━━━━━━━━━┓
// ┃  move_array  ┃
// ┗━━━━━━━━━━━━━┛
template <typename T,
          std::enable_if_t<std::is_trivially_move_assignable<T>::value, int> = 0>
void move_array(T* dst, const T* src, s32 size) {
    // Trivially move assignable
    memcpy(dst, src, sizeof(T) * size);
}

template <typename T,
          std::enable_if_t<!std::is_trivially_move_assignable<T>::value, int> = 0>
void move_array(T* dst, T* src, s32 size) {
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
    static void init(T* items, Arg&& arg, RemainingArgs&&... remaining_args) {
        *items = T{std::forward<Arg>(arg)};
        init(items + 1, std::forward<RemainingArgs>(remaining_args)...);
    }
};
} // namespace impl

//   ▄▄▄▄   ▄▄                   ▄▄
//  ██  ██ ▄██▄▄  ▄▄▄▄  ▄▄▄▄▄▄▄  ▄▄  ▄▄▄▄
//  ██▀▀██  ██   ██  ██ ██ ██ ██ ██ ██
//  ██  ██  ▀█▄▄ ▀█▄▄█▀ ██ ██ ██ ██ ▀█▄▄▄
//

// clang-format off
inline void signal_fence_consume() { std::atomic_signal_fence(std::memory_order_acquire); }
inline void signal_fence_acquire() { std::atomic_signal_fence(std::memory_order_acquire); }
inline void signal_fence_release() { std::atomic_signal_fence(std::memory_order_release); }
inline void signal_fence_seq_cst() { std::atomic_signal_fence(std::memory_order_seq_cst); }

inline void thread_fence_consume() { std::atomic_thread_fence(std::memory_order_acquire); }
inline void thread_fence_acquire() { std::atomic_thread_fence(std::memory_order_acquire); }
inline void thread_fence_release() { std::atomic_thread_fence(std::memory_order_release); }
inline void thread_fence_seq_cst() { std::atomic_thread_fence(std::memory_order_seq_cst); }
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
    Atomic(const Atomic& other) : std::atomic<T>{other.load_nonatomic()} {
    }
    // Hide operator=
    void operator=(T value) = delete;
    void operator=(const Atomic& other) {
        store_nonatomic(other.load_nonatomic());
    }
    T load_nonatomic() const {
        return std::atomic<T>::load(std::memory_order_relaxed);
    }
    T load(MemoryOrder memory_order) const {
        return std::atomic<T>::load((std::memory_order) memory_order);
    }
    void store_nonatomic(T value) {
        return std::atomic<T>::store(value, std::memory_order_relaxed);
    }
    void store(T value, MemoryOrder memory_order) {
        return std::atomic<T>::store(value, (std::memory_order) memory_order);
    }
    T compare_exchange(T expected, T desired, MemoryOrder memory_order) {
        std::atomic<T>::compare_exchange_strong(expected, desired,
                                                (std::memory_order) memory_order);
        return expected; // modified by reference by compare_exchange_strong
    }
    bool compare_exchange_strong(T& expected, T desired, MemoryOrder memory_order) {
        return std::atomic<T>::compare_exchange_strong(
            expected, desired, (std::memory_order) memory_order);
    }
    bool compare_exchange_weak(T& expected, T desired, MemoryOrder success,
                               MemoryOrder failure) {
        return std::atomic<T>::compare_exchange_weak(expected, desired,
                                                     (std::memory_order) success,
                                                     (std::memory_order) failure);
    }
    T exchange(T desired, MemoryOrder memory_order) {
        return std::atomic<T>::exchange(desired, (std::memory_order) memory_order);
    }
    T fetch_add(T operand, MemoryOrder memory_order) {
        return std::atomic<T>::fetch_add(operand, (std::memory_order) memory_order);
    }
    T fetch_sub(T operand, MemoryOrder memory_order) {
        return std::atomic<T>::fetch_sub(operand, (std::memory_order) memory_order);
    }
    T fetch_and(T operand, MemoryOrder memory_order) {
        return std::atomic<T>::fetch_and(operand, (std::memory_order) memory_order);
    }
    T fetch_or(T operand, MemoryOrder memory_order) {
        return std::atomic<T>::fetch_or(operand, (std::memory_order) memory_order);
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
    LockType& get_mutex() {
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

    bool try_lock() {
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

    Mutex& get_mutex() {
        return *(Mutex*) m_buffer;
    }

    void lazy_init() {
        // We use the thread-safe DCLI pattern via spinlock in case threads are spawned
        // during static initialization of global C++ objects. In that case, any of them
        // could call lazy_init().
        while (m_spinLock.compare_exchange(false, true, Acquire)) {
            // FIXME: Implement reusable AdaptiveBackoff class and apply it here
        }
        if (!m_initFlag.load_nonatomic()) {
            new (&get_mutex()) Mutex;
            m_initFlag.store(true, Release);
        }
        m_spinLock.store(false, Release);
    }

public:
    // Manual initialization is needed if not created at global scope:
    void zero_init() {
        memset(static_cast<void*>(this), 0, sizeof(*this));
    }

    // There should be no threads racing to lock when the destructor is called.
    // It's valid to attempt to lock after the destructor, though.
    // This permits Mutex_LazyInit to be used at global scope where destructors are
    // called in an arbitrary order.
    ~Mutex_LazyInit() {
        if (m_initFlag.load_nonatomic()) {
            get_mutex().Mutex::~Mutex();
            zero_init();
        }
    }

    void lock() {
        if (!m_initFlag.load(Acquire))
            lazy_init();
        get_mutex().lock();
    }

    void unlock() {
        PLY_ASSERT(m_initFlag.load_nonatomic());
        get_mutex().unlock();
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

    void lock_exclusive() {
        AcquireSRWLockExclusive(&m_rwLock);
    }

    void unlock_exclusive() {
        ReleaseSRWLockExclusive(&m_rwLock);
    }

    void lock_shared() {
        AcquireSRWLockShared(&m_rwLock);
    }

    void unlock_shared() {
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

    void lock_exclusive() {
        pthread_rwlock_wrlock(&m_rwLock);
    }

    void unlock_exclusive() {
        pthread_rwlock_unlock(&m_rwLock);
    }

    void lock_shared() {
        pthread_rwlock_rdlock(&m_rwLock);
    }

    void unlock_shared() {
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
        m_lock.lock_shared();
    }

    ~SharedLockGuard() {
        m_lock.unlock_shared();
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
        m_lock.lock_exclusive();
    }

    ~ExclusiveLockGuard() {
        m_lock.unlock_exclusive();
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

    void timed_wait(LockGuard<Mutex>& guard, ureg wait_millis) {
        if (wait_millis > 0)
            m_condVar.wait_for(guard, std::chrono::milliseconds(wait_millis));
    }

    void wake_one() {
        m_condVar.notify_one();
    }

    void wake_all() {
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

    Thread() = default;

    template <typename Callable>
    Thread(Callable&& callable) : thread{std::forward<Callable>(callable)} {
    }

    ~Thread() {
        if (this->thread.joinable())
            this->thread.detach();
    }

    bool is_valid() const {
        return this->thread.joinable();
    }

    void join() {
        this->thread.join();
    }

    template <typename Callable>
    void run(Callable&& callable) {
        if (this->thread.joinable())
            this->thread.detach();
        this->thread = std::thread(std::forward<Callable>(callable));
    }

    static void sleep_millis(ureg millis) {
        std::this_thread::sleep_for(std::chrono::milliseconds(millis));
    }
};

//  ▄▄▄▄▄▄ ▄▄                              ▄▄ ▄▄                        ▄▄▄
//    ██   ██▄▄▄  ▄▄▄▄▄   ▄▄▄▄   ▄▄▄▄   ▄▄▄██ ██     ▄▄▄▄   ▄▄▄▄  ▄▄▄▄   ██
//    ██   ██  ██ ██  ▀▀ ██▄▄██  ▄▄▄██ ██  ██ ██    ██  ██ ██     ▄▄▄██  ██
//    ██   ██  ██ ██     ▀█▄▄▄  ▀█▄▄██ ▀█▄▄██ ██▄▄▄ ▀█▄▄█▀ ▀█▄▄▄ ▀█▄▄██ ▄██▄
//

// Used as the return value of ThreadLocal::set_in_scope()
template <template <typename> class TL, typename T>
class ThreadLocalScope {
private:
    TL<T>* var;
    T old_value;

public:
    ThreadLocalScope(TL<T>* var, T new_value) : var{var} {
        this->old_value = var->load();
        var->store(new_value);
    }

    ThreadLocalScope(const ThreadLocalScope&) = delete;
    ThreadLocalScope(ThreadLocalScope&& other) {
        this->var = other->var;
        this->old_value = std::move(other.old_value);
        other->var = nullptr;
    }

    ~ThreadLocalScope() {
        if (this->var) {
            this->var->store(this->old_value);
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
    ThreadLocal() {
        m_tlsIndex = TlsAlloc();
        PLY_ASSERT(m_tlsIndex != TLS_OUT_OF_INDEXES);
    }

    ThreadLocal(const ThreadLocal&) = delete;

    ~ThreadLocal() {
        BOOL rc = TlsFree(m_tlsIndex);
        PLY_ASSERT(rc != 0);
        PLY_UNUSED(rc);
    }

    template <typename U = T, std::enable_if_t<std::is_pointer<U>::value, int> = 0>
    U load() const {
        LPVOID value = TlsGetValue(m_tlsIndex);
        PLY_ASSERT(value != 0 || GetLastError() == ERROR_SUCCESS);
        return (T) value;
    }

    template <
        typename U = T,
        std::enable_if_t<std::is_enum<U>::value || std::is_integral<U>::value, int> = 0>
    U load() const {
        LPVOID value = TlsGetValue(m_tlsIndex);
        PLY_ASSERT(value != 0 || GetLastError() == ERROR_SUCCESS);
        return (T) (uptr) value;
    }

    void store(T value) {
        BOOL rc = TlsSetValue(m_tlsIndex, (LPVOID) value);
        PLY_ASSERT(rc != 0);
        PLY_UNUSED(rc);
    }

    // In C++11, you can write auto scope = my_tlvar.set_in_scope(value);
    using Scope = ThreadLocalScope<ThreadLocal, T>;
    Scope set_in_scope(T value) {
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
    ThreadLocal() {
        int rc = pthread_key_create(&m_tlsKey, NULL);
        PLY_ASSERT(rc == 0);
        PLY_UNUSED(rc);
    }

    ThreadLocal(const ThreadLocal&) = delete;

    ~ThreadLocal() {
        int rc = pthread_key_delete(m_tlsKey);
        PLY_ASSERT(rc == 0);
        PLY_UNUSED(rc);
    }

    template <typename U = T, std::enable_if_t<std::is_pointer<U>::value, int> = 0>
    U load() const {
        void* value = pthread_getspecific(m_tlsKey);
        return (T) value;
    }

    template <
        typename U = T,
        std::enable_if_t<std::is_enum<U>::value || std::is_integral<U>::value, int> = 0>
    U load() const {
        void* value = pthread_getspecific(m_tlsKey);
        return (T) (uptr) value;
    }

    template <
        typename U = T,
        std::enable_if_t<std::is_enum<U>::value || std::is_integral<U>::value, int> = 0>
    void store(U value) {
        int rc = pthread_setspecific(m_tlsKey, (void*) (uptr) value);
        PLY_ASSERT(rc == 0);
        PLY_UNUSED(rc);
    }

    // In C++11, you can write auto scope = my_tlvar.set_in_scope(value);
    using Scope = ThreadLocalScope<ThreadLocal, T>;
    Scope set_in_scope(T value) {
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
    ply::RaceDetectGuard PLY_UNIQUE_VARIABLE(race_detect_guard)(name)
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
        uptr allocation_granularity;
        uptr page_size;
    };

    static const Info& get_info();

    static bool alloc(char*& out_addr, uptr num_bytes);
    static bool reserve(char*& out_addr, uptr num_bytes);
    static void commit(char* addr, uptr num_bytes);
    static void decommit(char* addr, uptr num_bytes);
    // On Windows, free() is only able to free a single entire region returned by
    // alloc() or reserve(). For this reason, dlmalloc doesn't use MemPage_Win32.
    static void free(char* addr, uptr num_bytes);
};

//  ▄▄  ▄▄
//  ██  ██  ▄▄▄▄   ▄▄▄▄  ▄▄▄▄▄
//  ██▀▀██ ██▄▄██  ▄▄▄██ ██  ██
//  ██  ██ ▀█▄▄▄  ▀█▄▄██ ██▄▄█▀
//                       ██

struct HeapStats {
    ureg peak_system_bytes;
    ureg system_bytes;
    ureg in_use_bytes;
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
    size_t in_use_bytes;
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
    void zero_init();
    void* alloc(ureg size);
    void* realloc(void* ptr, ureg new_size);
    void free(void* ptr);
    void* alloc_aligned(ureg size, ureg alignment);
    void free_aligned(void* ptr);
    HeapStats get_stats();
    ureg get_in_use_bytes() const;
    ureg get_size(void* ptr);
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
    u32 num_items = 0;

    // Constructors
    ArrayView() = default;
    ArrayView(T* items, u32 num_items) : items{items}, num_items{num_items} {
    }
    template <typename U = T, std::enable_if_t<std::is_const<U>::value, int> = 0>
    ArrayView(std::initializer_list<T> init)
        : items{init.begin()}, num_items{check_cast<u32>(init.size())} {
        PLY_ASSERT((uptr) init.end() - (uptr) init.begin() == sizeof(T) * init.size());
    }
    template <u32 N>
    ArrayView(T (&s)[N]) : items{s}, num_items{N} {
    }

    // Conversion
    operator ArrayView<const T>() const {
        return {this->items, this->num_items};
    }
    static ArrayView<const T> from(StringView view);
    static ArrayView<T> from(MutStringView view);
    StringView string_view() const;
    MutStringView mutable_string_view();

    // Indexing
    T& operator[](u32 index) {
        PLY_ASSERT(index < num_items);
        return items[index];
    }
    const T& operator[](u32 index) const {
        PLY_ASSERT(index < num_items);
        return items[index];
    }
    T& back(s32 offset = -1) {
        PLY_ASSERT(u32(num_items + offset) < num_items);
        return items[num_items + offset];
    }
    const T& back(s32 offset = -1) const {
        PLY_ASSERT(u32(num_items + offset) < num_items);
        return items[num_items + offset];
    }

    void offset_head(u32 ofs) {
        PLY_ASSERT(ofs <= num_items);
        items += ofs;
        num_items -= ofs;
    }
    void offset_back(s32 ofs) {
        PLY_ASSERT((u32) -ofs <= num_items);
        num_items += ofs;
    }

    explicit operator bool() const {
        return this->num_items > 0;
    }
    bool is_empty() const {
        return num_items == 0;
    }
    u32 size_bytes() const {
        return num_items * u32(sizeof(T));
    }
    ArrayView sub_view(u32 start) const {
        PLY_ASSERT(start <= num_items);
        return {items + start, num_items - start};
    }
    ArrayView sub_view(u32 start, u32 num_items) const {
        PLY_ASSERT(start <= this->num_items); // FIXME: Support different end parameters
        PLY_ASSERT(start + num_items <= this->num_items);
        return {items + start, num_items};
    }
    ArrayView shortened_by(u32 num_items) const {
        PLY_ASSERT(num_items <= this->num_items);
        return {this->items, this->num_items - num_items};
    }

    // Range-for support
    T* begin() const {
        return items;
    }
    T* end() const {
        return items + num_items;
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
void move_or_copy_construct(T* dst, Arr&& src) {
    ArrayView<ArrayViewType<Arr>> src_view{src};
    subst::move_construct_array(dst, src_view.items, src_view.num_items);
}
template <typename T, typename Arr,
          std::enable_if_t<!ArrayTraits<Arr>::IsOwner, int> = 0>
void move_or_copy_construct(T* dst, Arr&& src) {
    ArrayView<ArrayViewType<Arr>> src_view{src};
    subst::construct_array_from(dst, src_view.items, src_view.num_items);
}

} // namespace impl

template <typename T0, typename T1>
bool operator==(ArrayView<T0> a, ArrayView<T1> b) {
    if (a.num_items != b.num_items)
        return false;
    for (u32 i = 0; i < a.num_items; i++) {
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

inline bool is_white(char cp) {
    return (cp == ' ') || (cp == '\t') || (cp == '\r') || (cp == '\n');
}

inline bool is_ascii_letter(char cp) {
    return (cp >= 'A' && cp <= 'Z') || (cp >= 'a' && cp <= 'z');
}

inline bool is_decimal_digit(char cp) {
    return (cp >= '0' && cp <= '9');
}

struct StringView {
    const char* bytes = nullptr;
    u32 num_bytes = 0;

    StringView() = default;
    StringView(const char* s)
        : bytes{s}, num_bytes{(u32) std::char_traits<char>::length(s)} {
        PLY_ASSERT(s[num_bytes] ==
                   0); // Sanity check; num_bytes must fit within 32-bit field
    }
    template <typename U, u32 N,
              std::enable_if_t<std::is_same<U, std::decay_t<decltype(*u8"")>>::value,
                               bool> = false>
    StringView(const U (&s)[N]) : bytes{s}, num_bytes{N} {
    }
    u32 num_codepoints(UnicodeType decoder_type) const;
    template <typename U, typename = std::enable_if_t<std::is_same<U, char>::value>>
    StringView(const U& c) : bytes{&c}, num_bytes{1} {
    }
    StringView(const char* bytes, u32 num_bytes) : bytes{bytes}, num_bytes{num_bytes} {
    }

    static StringView from_range(const char* start_byte, const char* end_byte) {
        return {start_byte, check_cast<u32>(end_byte - start_byte)};
    }
    const char& operator[](u32 index) const {
        PLY_ASSERT(index < this->num_bytes);
        return this->bytes[index];
    }
    const char& back(s32 ofs = -1) const {
        PLY_ASSERT(u32(-ofs - 1) < this->num_bytes);
        return this->bytes[this->num_bytes + ofs];
    }
    void offset_head(u32 num_bytes) {
        PLY_ASSERT(num_bytes <= this->num_bytes);
        this->bytes += num_bytes;
        this->num_bytes -= num_bytes;
    }

    void offset_back(s32 ofs) {
        PLY_ASSERT((u32) -ofs <= this->num_bytes);
        this->num_bytes += ofs;
    }
    template <typename T>
    T to(const T& default_value = subst::create_default<T>()) const;
    explicit operator bool() const {
        return this->num_bytes != 0;
    }
    bool is_empty() const {
        return this->num_bytes == 0;
    }
    StringView sub_str(u32 start) const {
        PLY_ASSERT(start <= num_bytes);
        return {this->bytes + start, this->num_bytes - start};
    }
    StringView sub_str(u32 start, u32 num_bytes) const {
        PLY_ASSERT(start <= this->num_bytes);
        PLY_ASSERT(start + num_bytes <= this->num_bytes);
        return {this->bytes + start, num_bytes};
    }
    bool contains(const char* cur_byte) const {
        return uptr(cur_byte - this->bytes) <= this->num_bytes;
    }
    StringView left(u32 num_bytes) const {
        PLY_ASSERT(num_bytes <= this->num_bytes);
        return {this->bytes, num_bytes};
    }
    StringView shortened_by(u32 num_bytes) const {
        PLY_ASSERT(num_bytes <= this->num_bytes);
        return {this->bytes, this->num_bytes - num_bytes};
    }
    StringView right(u32 num_bytes) const {
        PLY_ASSERT(num_bytes <= this->num_bytes);
        return {this->bytes + this->num_bytes - num_bytes, num_bytes};
    }
    s32 find_byte(char match_byte, u32 start_pos = 0) const {
        for (u32 i = start_pos; i < this->num_bytes; i++) {
            if (this->bytes[i] == match_byte)
                return i;
        }
        return -1;
    }
    template <typename MatchFunc>
    s32 find_byte(const MatchFunc& match_func, u32 start_pos = 0) const {
        for (u32 i = start_pos; i < this->num_bytes; i++) {
            if (match_func(this->bytes[i]))
                return i;
        }
        return -1;
    }

    s32 rfind_byte(char match_byte, u32 start_pos) const {
        s32 i = start_pos;
        for (; i >= 0; i--) {
            if (this->bytes[i] == match_byte)
                break;
        }
        return i;
    }
    template <typename MatchFunc>
    s32 rfind_byte(const MatchFunc& match_func, u32 start_pos) const {
        s32 i = start_pos;
        for (; i >= 0; i--) {
            if (match_func(this->bytes[i]))
                break;
        }
        return i;
    }
    template <typename MatchFuncOrChar>
    s32 rfind_byte(const MatchFuncOrChar& match_func_or_byte) const {
        return this->rfind_byte(match_func_or_byte, this->num_bytes - 1);
    }

    bool starts_with(StringView arg) const;
    bool ends_with(StringView arg) const;

    StringView trim(bool (*match_func)(char) = is_white, bool left = true,
                    bool right = true) const;
    StringView ltrim(bool (*match_func)(char) = is_white) const {
        return this->trim(match_func, true, false);
    }
    StringView rtrim(bool (*match_func)(char) = is_white) const {
        return this->trim(match_func, false, true);
    }
    String join(ArrayView<const StringView> comps) const;
    Array<StringView> split_byte(char sep) const;
    String upper_asc() const;
    String lower_asc() const;
    String reversed_bytes() const;
    String filter_bytes(char (*filter_func)(char)) const;

    bool includes_null_terminator() const {
        return (this->num_bytes > 0) ? (this->bytes[this->num_bytes - 1] == 0) : false;
    }
    HybridString with_null_terminator() const;
    StringView without_null_terminator() const;

    const char* begin() const {
        return this->bytes;
    }
    const char* end() const {
        return this->bytes + this->num_bytes;
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
    u32 num_bytes = 0;

    MutStringView() = default;
    MutStringView(char* bytes, u32 num_bytes) : bytes{bytes}, num_bytes{num_bytes} {
    }

    char* end() {
        return this->bytes + this->num_bytes;
    }
    static MutStringView from_range(char* start_byte, char* end_byte) {
        return {start_byte, check_cast<u32>(end_byte - start_byte)};
    }
    operator const StringView&() const {
        return reinterpret_cast<const StringView&>(*this);
    }
    void offset_head(u32 num_bytes) {
        PLY_ASSERT(num_bytes <= this->num_bytes);
        this->bytes += num_bytes;
        this->num_bytes -= num_bytes;
    }
    void offset_back(s32 ofs) {
        PLY_ASSERT((u32) -ofs <= this->num_bytes);
        this->num_bytes += ofs;
    }
};

template <typename T>
ArrayView<const T> ArrayView<T>::from(StringView view) {
    u32 num_items = view.num_bytes / sizeof(T); // Divide by constant is fast
    return {(const T*) view.bytes, num_items};
}

template <typename T>
ArrayView<T> ArrayView<T>::from(MutStringView view) {
    u32 num_items = view.num_bytes / sizeof(T); // Divide by constant is fast
    return {(T*) view.bytes, num_items};
}

template <typename T>
StringView ArrayView<T>::string_view() const {
    return {(const char*) items, check_cast<u32>(num_items * sizeof(T))};
}

template <typename T>
MutStringView ArrayView<T>::mutable_string_view() {
    return {(char*) items, check_cast<u32>(num_items * sizeof(T))};
}

namespace subst {
template <typename T>
void destruct_view_as(StringView view) {
    subst::destruct_array<T>((T*) view.bytes, view.num_bytes / (u32) sizeof(T));
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
s32 find(ArrayView<const T> arr, const U& item) {
    for (u32 i = 0; i < arr.num_items; i++) {
        if (arr[i] == item)
            return i;
    }
    return -1;
}

template <typename T, typename Callback,
          std::enable_if_t<IsCallable<Callback, T>, int> = 0>
s32 find(ArrayView<const T> arr, const Callback& callback) {
    for (u32 i = 0; i < arr.num_items; i++) {
        if (callback(arr[i]))
            return i;
    }
    return -1;
}

template <typename Arr, typename Arg, typename T = impl::ArrayViewType<Arr>>
s32 find(const Arr& arr, const Arg& arg) {
    return find(ArrayView<const T>{arr}, arg);
}

template <typename T, typename U, std::enable_if_t<IsComparable<T, U>, int> = 0>
s32 rfind(ArrayView<const T> arr, const U& item) {
    for (s32 i = check_cast<s32>(arr.num_items - 1); i >= 0; i--) {
        if (arr[i] == item)
            return i;
    }
    return -1;
}

template <typename T, typename Callback,
          std::enable_if_t<IsCallable<Callback, T>, int> = 0>
s32 rfind(ArrayView<const T> arr, const Callback& callback) {
    for (s32 i = check_cast<s32>(arr.num_items - 1); i >= 0; i--) {
        if (callback(arr[i]))
            return i;
    }
    return -1;
}

template <typename Arr, typename Arg, typename T = impl::ArrayViewType<Arr>>
s32 rfind(const Arr& arr, const Arg& arg) {
    return rfind(ArrayView<const T>{arr}, arg);
}

// ┏━━━━━━━┓
// ┃  map  ┃
// ┗━━━━━━━┛
template <typename Iterable, typename MapFunc,
          typename MappedItemType = std::decay_t<decltype(std::declval<MapFunc>()(
              std::declval<impl::ItemType<Iterable>>()))>>
Array<MappedItemType> map(Iterable&& iterable, MapFunc&& map_func) {
    Array<MappedItemType> result;
    // FIXME: Reserve memory for result when possible. Otherwise, use a typed
    // ChunkBuffer.
    for (auto&& item : iterable) {
        result.append(map_func(item));
    }
    return result;
}

// ┏━━━━━━━━┓
// ┃  sort  ┃
// ┗━━━━━━━━┛
namespace impl {
template <typename T>
bool default_less(const T& a, const T& b) {
    return a < b;
}
} // namespace impl

template <typename T, typename IsLess = decltype(impl::default_less<T>)>
void sort(ArrayView<T> view, const IsLess& is_less = impl::default_less<T>) {
    if (view.num_items <= 1)
        return;
    u32 lo = 0;
    u32 hi = view.num_items - 1;
    u32 pivot = view.num_items / 2;
    for (;;) {
        while (lo < hi && is_less(view[lo], view[pivot])) {
            lo++;
        }
        while (lo < hi && is_less(view[pivot], view[hi])) {
            hi--;
        }
        if (lo >= hi)
            break;
        // view[lo] is >= pivot
        // All slots to left of lo are < pivot
        // view[hi] <= pivot
        // All slots to the right of hi are > pivot
        PLY_ASSERT(!is_less(view[lo], view[pivot]));
        PLY_ASSERT(!is_less(view[pivot], view[hi]));
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
        if (!is_less(view[lo - 1], view[pivot])) {
            lo--;
        } else {
            sort(view.sub_view(0, lo), is_less);
            break;
        }
    }
    while (hi + 1 < view.num_items) {
        if (!is_less(view[pivot], view[hi])) {
            hi++;
        } else {
            sort(view.sub_view(hi), is_less);
            break;
        }
    }
}

template <typename Arr,
          typename IsLess = decltype(impl::default_less<impl::ArrayViewType<Arr>>)>
void sort(Arr& arr,
          const IsLess& is_less = impl::default_less<impl::ArrayViewType<Arr>>) {
    using T = impl::ArrayViewType<Arr>;
    sort(ArrayView<T>{arr}, is_less);
}

} // namespace ply

#include <ply-runtime/container.h>
#include <ply-runtime/io.h>
#include <ply-runtime/network.h>
