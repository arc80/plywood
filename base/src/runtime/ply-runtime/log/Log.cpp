/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-runtime/Precomp.h>
#include <ply-runtime/log/Log.h>

namespace ply {

CPUTimer::Point LogChannel::start_time = CPUTimer::get();
CPUTimer::Converter LogChannel::converter;

PLY_NO_INLINE LogChannel::LineHandler::LineHandler(StringView channel_name) {
    CPUTimer::Point now = CPUTimer::get();
    TID tid = get_current_thread_id();
    this->mout << (now - start_time); // Timestamp
    this->mout.format(" 0x{}[{}] ", hex(tid), channel_name);
}

PLY_NO_INLINE LogChannel::LineHandler::~LineHandler() {
    this->mout << StringView{"\n", 2}; // include null terminator
    Logger::log(this->mout.move_to_string());
}

} // namespace ply
