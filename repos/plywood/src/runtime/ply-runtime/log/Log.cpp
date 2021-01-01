/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/Precomp.h>
#include <ply-runtime/log/Log.h>
#include <ply-runtime/thread/TID.h>

namespace ply {

CPUTimer::Point LogChannel::startTime = CPUTimer::get();
CPUTimer::Converter LogChannel::converter;

PLY_NO_INLINE LogChannel::LineHandler::LineHandler(const StringView channelName) {
    CPUTimer::Point now = CPUTimer::get();
    TID::TID tid = TID::getCurrentThreadID();
    this->sw << (now - startTime); // Timestamp
    this->sw.format(" 0x{}[{}] ", String::from(fmt::Hex(tid)), channelName);
}

PLY_NO_INLINE LogChannel::LineHandler::~LineHandler() {
    this->sw.write({"\n", 2}); // include null terminator
    Logger::log(this->sw.moveToString());
}

} // namespace ply
