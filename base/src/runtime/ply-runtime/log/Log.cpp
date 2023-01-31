/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/Precomp.h>
#include <ply-runtime/log/Log.h>

namespace ply {

CPUTimer::Point LogChannel::startTime = CPUTimer::get();
CPUTimer::Converter LogChannel::converter;

PLY_NO_INLINE LogChannel::LineHandler::LineHandler(StringView channelName) {
    CPUTimer::Point now = CPUTimer::get();
    TID tid = getCurrentThreadID();
    this->mout << (now - startTime); // Timestamp
    this->mout.format(" 0x{}[{}] ", hex(tid), channelName);
}

PLY_NO_INLINE LogChannel::LineHandler::~LineHandler() {
    this->mout << StringView{"\n", 2}; // include null terminator
    Logger::log(this->mout.moveToString());
}

} // namespace ply
