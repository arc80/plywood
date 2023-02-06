/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-runtime/Precomp.h>
#include <ply-runtime/io/text/NewLineFilter.h>

namespace ply {

struct NewLineFilter {
    struct Params {
        const char* src_byte = nullptr;
        const char* src_end_byte = nullptr;
        char* dst_byte = nullptr;
        char* dst_end_byte = nullptr;
    };

    bool crlf = false; // If true, outputs \r\n instead of \n
    bool needs_lf = false;

    void process(Params* params) {
        while (params->dst_byte < params->dst_end_byte) {
            u8 c = 0;
            if (this->needs_lf) {
                c = '\n';
                this->needs_lf = false;
            } else {
                for (;;) {
                    if (params->src_byte >= params->src_end_byte)
                        return; // src has been consumed
                    c = *params->src_byte++;
                    if (c == '\r') {
                        // Output nothing
                    } else {
                        if (c == '\n' && this->crlf) {
                            c = '\r';
                            this->needs_lf = true;
                        }
                        break;
                    }
                }
            }
            *params->dst_byte++ = c;
        }
    }
};

//-----------------------------------------------------------------------

struct InPipe_NewLineFilter : InPipe {
    InStream in;
    NewLineFilter filter;

    InPipe_NewLineFilter(InStream&& in) : in{std::move(in)} {
    }
    virtual u32 read(MutStringView buf) override;
};

u32 InPipe_NewLineFilter::read(MutStringView buf) {
    PLY_ASSERT(buf.num_bytes > 0);

    NewLineFilter::Params params;
    params.dst_byte = buf.bytes;
    params.dst_end_byte = buf.bytes + buf.num_bytes;
    for (;;) {
        params.src_byte = this->in.cur_byte;
        params.src_end_byte = this->in.end_byte;
        this->filter.process(&params);

        this->in.cur_byte = params.src_byte;
        u32 num_bytes_written = safe_demote<u32>(params.dst_byte - buf.bytes);
        if (num_bytes_written > 0)
            return num_bytes_written;

        PLY_ASSERT(this->in.num_bytes_readable() == 0);
        if (!this->in.ensure_readable())
            return 0;
    }
}

Owned<InPipe> create_in_new_line_filter(InStream&& in) {
    return new InPipe_NewLineFilter{std::move(in)};
}

//-----------------------------------------------------------------------

struct OutPipe_NewLineFilter : OutPipe {
    NewLineFilter filter;

    OutPipe_NewLineFilter(OutStream&& out, bool write_crlf) {
        this->child_stream = std::move(out);
        this->filter.crlf = write_crlf;
    }
    virtual bool write(StringView buf) override;
    virtual void flush(bool hard);
};

bool OutPipe_NewLineFilter::write(StringView buf) {
    u32 desired_total_bytes_read = buf.num_bytes;
    u32 total_bytes_read = 0;
    for (;;) {
        this->child_stream.ensure_writable();

        // If try_make_bytes_available fails, process() will do nothing and we'll simply
        // return below:
        NewLineFilter::Params params;
        params.src_byte = buf.bytes;
        params.src_end_byte = buf.bytes + buf.num_bytes;
        params.dst_byte = this->child_stream.cur_byte;
        params.dst_end_byte = this->child_stream.end_byte;
        this->filter.process(&params);
        this->child_stream.cur_byte = params.dst_byte;
        u32 num_bytes_read = safe_demote<u32>(params.src_byte - buf.bytes);
        if (num_bytes_read == 0) {
            PLY_ASSERT(total_bytes_read <= desired_total_bytes_read);
            return total_bytes_read >= desired_total_bytes_read;
        }
        total_bytes_read += num_bytes_read;
        buf.offset_head(num_bytes_read);
    }
}

void OutPipe_NewLineFilter::flush(bool hard) {
    // Forward flush command down the output chain.
    this->child_stream.flush(hard);
};

Owned<OutPipe> create_out_new_line_filter(OutStream&& out, bool write_crlf) {
    return new OutPipe_NewLineFilter{std::move(out), write_crlf};
}

} // namespace ply
