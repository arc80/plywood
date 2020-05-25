/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-build-common/Core.h>
#include <ply-build-repo/ErrorHandler.h>

namespace ply {
namespace build {

thread_local Functor<void(ErrorHandler::Level, HybridString&&)> ErrorHandler::current;

void ErrorHandler::log(ErrorHandler::Level errorLevel, HybridString&& error) {
    if (ErrorHandler::current) {
        ErrorHandler::current.call(errorLevel, std::move(error));
    }
    if (errorLevel == ErrorHandler::Fatal) {
        exit(1);
    }
};

} // namespace build
} // namespace ply
