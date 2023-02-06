/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-runtime/Precomp.h>

#if PLY_KERNEL_LINUX

#include <ply-runtime/thread/impl/Affinity_Linux.h>
#include <string>
#include <fstream>
#include <stdio.h>

namespace ply {

Affinity_Linux::Affinity_Linux() : m_isAccurate(false), m_numHWThreads(0) {
    std::ifstream f("/proc/cpuinfo");
    if (f.is_open()) {
        CoreInfoCollector collector;
        while (!f.eof()) {
            std::string line;
            std::getline(f, line);
            size_t colon = line.find_first_of(':');
            if (colon != std::string::npos) {
                size_t end_label =
                    line.find_last_not_of(" \t", colon > 0 ? colon - 1 : 0);
                std::string label =
                    line.substr(0, end_label != std::string::npos ? end_label + 1 : 0);
                int value;
                bool is_integer_value =
                    (sscanf(line.c_str() + colon + 1, " %d", &value) > 0);
                if (is_integer_value && label == "processor") {
                    collector.flush(*this);
                    collector.logical_processor = (s32) value;
                } else if (is_integer_value && label == "physical id") {
                    collector.core_id.physical = (s32) value;
                } else if (is_integer_value && label == "core id") {
                    collector.core_id.core = (s32) value;
                }
            }
        }
        collector.flush(*this);
    }
    m_isAccurate = (m_numHWThreads > 0);
    if (!m_isAccurate) {
        m_coreIndexToInfo.resize(1);
        m_coreIndexToInfo[0].hw_thread_index_to_logical_processor.push_back(0);
        m_numHWThreads = 1;
    }
}

bool Affinity_Linux::set_affinity(ureg core, ureg hw_thread) {
    u32 logical_processor =
        m_coreIndexToInfo[core].hw_thread_index_to_logical_processor[hw_thread];
    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);
    CPU_SET(logical_processor, &cpu_set);
    int rc = sched_setaffinity(0, sizeof(cpu_set), &cpu_set); // Note: untested!
    return (rc == 0);
}

} // namespace ply

#endif // PLY_KERNEL_LINUX
