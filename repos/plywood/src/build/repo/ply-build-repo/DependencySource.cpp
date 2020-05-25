/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-build-common/Core.h>
#include <ply-build-repo/DependencySource.h>
#include <ply-build-repo/Repo.h>
#include <ply-build-repo/ErrorHandler.h>

namespace ply {
namespace build {

String DependencySource::getFullyQualifiedName() const {
    return String::format("{}.{}", this->repo->repoName, this->name);
}

// Logs an error and returns empty if nameWithDots is invalid
Array<StringView> splitName(StringView nameWithDots, StringView typeForErrorMsg) {
    Array<StringView> result;
    StringViewReader svr{nameWithDots};
    while (svr.numBytesAvailable() > 0) {
        if (svr.getSeekPos() > 0) {
            if (svr.curByte[0] != '.') {
                ErrorHandler::log(
                    ErrorHandler::Error,
                    String::format("Invalid {} '{}'\n", typeForErrorMsg, nameWithDots));
                return {};
            }
            svr.advanceByte();
        }
        StringView identifier = svr.readView<fmt::Identifier>(fmt::WithDash);
        if (!identifier) {
            ErrorHandler::log(ErrorHandler::Error,
                              String::format("Invalid {} '{}'\n", typeForErrorMsg, nameWithDots));
            return {};
        }
        result.append(identifier);
    }
    if (!result) {
        ErrorHandler::log(ErrorHandler::Error,
                          String::format("Invalid {} '{}'\n", typeForErrorMsg, nameWithDots));
        return {};
    }
    return result;
}

} // namespace build
} // namespace ply
