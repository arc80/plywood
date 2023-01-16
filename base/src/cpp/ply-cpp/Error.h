/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-cpp/Core.h>
#include <ply-cpp/LinearLocation.h>
#include <ply-cpp/PPVisitedFiles.h>

namespace ply {
namespace cpp {

struct BaseError {
    virtual ~BaseError() {
    }
    virtual void writeMessage(OutStream& out, const PPVisitedFiles* visitedFiles) const = 0;
};

} // namespace cpp
} // namespace ply
