/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#pragma once
#include <ply-cpp/Core.h>
#include <ply-cpp/Parser.h>

namespace ply {
namespace cpp {

struct RestorePoint {
    Parser* parser = nullptr;
    bool was_previously_enabled = false;
    u32 saved_token_queue_pos = 0;
    u32 saved_error_count = 0;

    RestorePoint(Parser* parser) : parser{parser} {
        // Restore points can be nested. For example, when parsing the parameters of the
        // ply::Initializer constructor, there is a restore point when the constructor
        // is optimistically parsed, and another restore point after 'void' when we
        // optimistically try to parse a parameter list:
        //      struct Initializer {
        //          Initializer(void (*init)()) {
        //          ^                ^
        //          |                `---- second restore point
        //          `---- first restore point
        if (!parser->restore_point_enabled) {
            PLY_ASSERT(parser->token_queue_pos == 0);
        }
        this->was_previously_enabled = parser->restore_point_enabled;
        parser->restore_point_enabled = true;
        this->saved_token_queue_pos = parser->token_queue_pos;
        this->saved_error_count = parser->raw_error_count;
    }
    ~RestorePoint() {
        if (this->parser) {
            this->cancel();
        }
    }
    bool error_occurred() const {
        return this->parser->raw_error_count != this->saved_error_count;
    }
    void backtrack() {
        PLY_ASSERT(this->parser); // Must not have been canceled
        this->parser->token_queue_pos = this->saved_token_queue_pos;
        this->parser->raw_error_count = this->saved_error_count;
    }
    void cancel() {
        PLY_ASSERT(this->parser);            // Must not have been canceled
        PLY_ASSERT(!this->error_occurred()); // no errors occurred
        this->parser->restore_point_enabled = this->was_previously_enabled;
        if (this->saved_token_queue_pos == 0) {
            PLY_ASSERT(!this->was_previously_enabled);
            this->parser->token_queue.erase(0, this->parser->token_queue_pos);
            this->parser->token_queue_pos = 0;
        }
        this->parser = nullptr;
    }
};

} // namespace cpp
} // namespace ply
