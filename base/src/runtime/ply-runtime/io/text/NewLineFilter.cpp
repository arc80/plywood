/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/Precomp.h>
#include <ply-runtime/io/text/NewLineFilter.h>

namespace ply {

struct NewLineFilter {
    struct Params {
        const char* srcByte = nullptr;
        const char* srcEndByte = nullptr;
        char* dstByte = nullptr;
        char* dstEndByte = nullptr;
    };

    bool crlf = false; // If true, outputs \r\n instead of \n
    bool needsLF = false;

    void process(Params* params) {
        while (params->dstByte < params->dstEndByte) {
            u8 c = 0;
            if (this->needsLF) {
                c = '\n';
                this->needsLF = false;
            } else {
                for (;;) {
                    if (params->srcByte >= params->srcEndByte)
                        return; // src has been consumed
                    c = *params->srcByte++;
                    if (c == '\r') {
                        // Output nothing
                    } else {
                        if (c == '\n' && this->crlf) {
                            c = '\r';
                            this->needsLF = true;
                        }
                        break;
                    }
                }
            }
            *params->dstByte++ = c;
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
    PLY_ASSERT(buf.numBytes > 0);

    NewLineFilter::Params params;
    params.dstByte = buf.bytes;
    params.dstEndByte = buf.bytes + buf.numBytes;
    for (;;) {
        params.srcByte = this->in.cur_byte;
        params.srcEndByte = this->in.end_byte;
        this->filter.process(&params);

        this->in.cur_byte = params.srcByte;
        u32 numBytesWritten = safeDemote<u32>(params.dstByte - buf.bytes);
        if (numBytesWritten > 0)
            return numBytesWritten;

        PLY_ASSERT(this->in.num_bytes_readable() == 0);
        if (!this->in.ensure_readable())
            return 0;
    }
}

Owned<InPipe> createInNewLineFilter(InStream&& in) {
    return new InPipe_NewLineFilter{std::move(in)};
}

//-----------------------------------------------------------------------

struct OutPipe_NewLineFilter : OutPipe {
    OutStream out;
    NewLineFilter filter;

    OutPipe_NewLineFilter(OutStream&& out, bool writeCRLF) : out{std::move(out)} {
        this->filter.crlf = writeCRLF;
    }
    virtual bool write(StringView buf) override;
    virtual void flush(bool hard);
};

bool OutPipe_NewLineFilter::write(StringView buf) {
    u32 desiredTotalBytesRead = buf.numBytes;
    u32 totalBytesRead = 0;
    for (;;) {
        this->out.ensure_writable();

        // If tryMakeBytesAvailable fails, process() will do nothing and we'll simply
        // return below:
        NewLineFilter::Params params;
        params.srcByte = buf.bytes;
        params.srcEndByte = buf.bytes + buf.numBytes;
        params.dstByte = this->out.cur_byte;
        params.dstEndByte = this->out.end_byte;
        this->filter.process(&params);
        this->out.cur_byte = params.dstByte;
        u32 numBytesRead = safeDemote<u32>(params.srcByte - buf.bytes);
        if (numBytesRead == 0) {
            PLY_ASSERT(totalBytesRead <= desiredTotalBytesRead);
            return totalBytesRead >= desiredTotalBytesRead;
        }
        totalBytesRead += numBytesRead;
        buf.offsetHead(numBytesRead);
    }
}

void OutPipe_NewLineFilter::flush(bool hard) {
    this->out.flush(hard);
};

Owned<OutPipe> createOutNewLineFilter(OutStream&& out, bool writeCRLF) {
    return new OutPipe_NewLineFilter{std::move(out), writeCRLF};
}

} // namespace ply
