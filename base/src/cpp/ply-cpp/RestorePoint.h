/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-cpp/Core.h>
#include <ply-cpp/Parser.h>

namespace ply {
namespace cpp {

struct RestorePoint {
    Parser* parser = nullptr;
    bool wasPreviouslyEnabled = false;
    u32 savedTokenQueuePos = 0;
    u32 savedErrorCount = 0;

    RestorePoint(Parser* parser) : parser{parser} {
        // Restore points can be nested. For example, when parsing the parameters of the
        // ply::Initializer constructor, there is a restore point when the constructor is
        // optimistically parsed, and another restore point after 'void' when we optimistically try
        // to parse a parameter list:
        //      struct Initializer {
        //          Initializer(void (*init)()) {
        //          ^                ^
        //          |                `---- second restore point
        //          `---- first restore point
        if (!parser->restorePointEnabled) {
            PLY_ASSERT(parser->tokenQueuePos == 0);
        }
        this->wasPreviouslyEnabled = parser->restorePointEnabled;
        parser->restorePointEnabled = true;
        this->savedTokenQueuePos = parser->tokenQueuePos;
        this->savedErrorCount = parser->rawErrorCount;
    }
    ~RestorePoint() {
        if (this->parser) {
            this->cancel();
        }
    }
    PLY_INLINE bool errorOccurred() const {
        return this->parser->rawErrorCount != this->savedErrorCount;
    }
    void backtrack() {
        PLY_ASSERT(this->parser); // Must not have been canceled
        this->parser->tokenQueuePos = this->savedTokenQueuePos;
        this->parser->rawErrorCount = this->savedErrorCount;
    }
    void cancel() {
        PLY_ASSERT(this->parser);           // Must not have been canceled
        PLY_ASSERT(!this->errorOccurred()); // no errors occurred
        this->parser->restorePointEnabled = this->wasPreviouslyEnabled;
        if (this->savedTokenQueuePos == 0) {
            PLY_ASSERT(!this->wasPreviouslyEnabled);
            this->parser->tokenQueue.erase(0, this->parser->tokenQueuePos);
            this->parser->tokenQueuePos = 0;
        }
        this->parser = nullptr;
    }
};

} // namespace cpp
} // namespace ply
