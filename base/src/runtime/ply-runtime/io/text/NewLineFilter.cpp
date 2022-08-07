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
    static Funcs Funcs_;
    OptionallyOwned<InStream> ins;
    NewLineFilter filter;
    InPipe_NewLineFilter();
};

PLY_NO_INLINE void InPipe_NewLineFilter_destroy(InPipe* inPipe_) {
    InPipe_NewLineFilter* inPipe = static_cast<InPipe_NewLineFilter*>(inPipe_);
    destruct(inPipe->ins);
}

PLY_NO_INLINE u32 InPipe_NewLineFilter_readSome(InPipe* inPipe_, MutableStringView buf) {
    InPipe_NewLineFilter* inPipe = static_cast<InPipe_NewLineFilter*>(inPipe_);
    PLY_ASSERT(buf.numBytes > 0);

    NewLineFilter::Params params;
    params.dstByte = buf.bytes;
    params.dstEndByte = buf.bytes + buf.numBytes;
    for (;;) {
        params.srcByte = inPipe->ins->curByte;
        params.srcEndByte = inPipe->ins->endByte;
        inPipe->filter.process(&params);

        inPipe->ins->curByte = params.srcByte;
        u32 numBytesWritten = safeDemote<u32>(params.dstByte - buf.bytes);
        if (numBytesWritten > 0)
            return numBytesWritten;

        PLY_ASSERT(inPipe->ins->numBytesAvailable() == 0);
        if (!inPipe->ins->tryMakeBytesAvailable())
            return 0;
    }
}

InPipe::Funcs InPipe_NewLineFilter::Funcs_ = {
    InPipe_NewLineFilter_destroy,
    InPipe_NewLineFilter_readSome,
    InPipe::getFileSize_Unsupported,
};

PLY_NO_INLINE InPipe_NewLineFilter::InPipe_NewLineFilter() : InPipe{&Funcs_} {
}

Owned<InPipe> createInNewLineFilter(OptionallyOwned<InStream>&& ins) {
    InPipe_NewLineFilter* inPipe = new InPipe_NewLineFilter;
    inPipe->ins = std::move(ins);
    return inPipe;
}

//-----------------------------------------------------------------------

struct OutPipe_NewLineFilter : OutPipe {
    static Funcs Funcs_;
    OptionallyOwned<OutStream> outs;
    NewLineFilter filter;
    OutPipe_NewLineFilter();
};

PLY_NO_INLINE void OutPipe_NewLineFilter_destroy(OutPipe* outPipe_) {
    OutPipe_NewLineFilter* outPipe = static_cast<OutPipe_NewLineFilter*>(outPipe_);
    destruct(outPipe->outs);
}

PLY_NO_INLINE bool OutPipe_NewLineFilter_write(OutPipe* outPipe_, StringView buf) {
    OutPipe_NewLineFilter* outPipe = static_cast<OutPipe_NewLineFilter*>(outPipe_);
    u32 desiredTotalBytesRead = buf.numBytes;
    u32 totalBytesRead = 0;
    for (;;) {
        outPipe->outs->tryMakeBytesAvailable();

        // If tryMakeBytesAvailable fails, process() will do nothing and we'll simply return below:
        NewLineFilter::Params params;
        params.srcByte = buf.bytes;
        params.srcEndByte = buf.bytes + buf.numBytes;
        params.dstByte = outPipe->outs->curByte;
        params.dstEndByte = outPipe->outs->endByte;
        outPipe->filter.process(&params);
        outPipe->outs->curByte = params.dstByte;
        u32 numBytesRead = safeDemote<u32>(params.srcByte - buf.bytes);
        if (numBytesRead == 0) {
            PLY_ASSERT(totalBytesRead <= desiredTotalBytesRead);
            return totalBytesRead >= desiredTotalBytesRead;
        }
        totalBytesRead += numBytesRead;
        buf.offsetHead(numBytesRead);
    }
}

PLY_NO_INLINE bool OutPipe_NewLineFilter_flush(OutPipe* outPipe_, bool toDevice) {
    OutPipe_NewLineFilter* outPipe = static_cast<OutPipe_NewLineFilter*>(outPipe_);
    return outPipe->outs->flush(toDevice);
};

OutPipe::Funcs OutPipe_NewLineFilter::Funcs_ = {
    OutPipe_NewLineFilter_destroy,
    OutPipe_NewLineFilter_write,
    OutPipe_NewLineFilter_flush,
    OutPipe::seek_Empty,
};

PLY_NO_INLINE OutPipe_NewLineFilter::OutPipe_NewLineFilter() : OutPipe{&Funcs_} {
}

Owned<OutPipe> createOutNewLineFilter(OptionallyOwned<OutStream>&& outs, bool writeCRLF) {
    OutPipe_NewLineFilter* outPipe = new OutPipe_NewLineFilter;
    outPipe->outs = std::move(outs);
    outPipe->filter.crlf = writeCRLF;
    return outPipe;
}

} // namespace ply
