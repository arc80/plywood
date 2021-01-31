/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/Precomp.h>
#include <ply-runtime/filesystem/FileSystem.h>
#include <ply-runtime/io/Pipe.h>

namespace ply {

ThreadLocal<FSResult> FileSystem::lastResult_;

namespace details {
struct WalkImpl : FileSystem::Walk::Impl {
    struct StackItem {
        String path;
        Array<String> dirNames;
        u32 dirIndex;
    };

    FileSystem* fs = nullptr;
    u32 flags = 0;
    Array<StackItem> stack;

    PLY_NO_INLINE void visit(StringView dirPath) {
        this->triple.dirPath = dirPath;
        this->triple.dirNames.clear();
        this->triple.files.clear();
        for (DirectoryEntry& entry : this->fs->listDir(dirPath, this->flags)) {
            if (entry.isDir) {
                this->triple.dirNames.append(std::move(entry.name));
            } else {
                WalkTriple::FileInfo& file = this->triple.files.append();
                file.name = std::move(entry.name);
                file.fileSize = entry.fileSize;
                file.creationTime = entry.creationTime;
                file.accessTime = entry.accessTime;
                file.modificationTime = entry.modificationTime;
            }
        }
    }

    static PLY_NO_INLINE void destructImpl(FileSystem::Walk::Impl* impl_) {
        WalkImpl* walk = static_cast<WalkImpl*>(impl_);
        walk->stack.~Array();
    }

    static PLY_NO_INLINE void nextImpl(FileSystem::Walk::Impl* impl_) {
        WalkImpl* walk = static_cast<WalkImpl*>(impl_);
        if (!walk->triple.dirNames.isEmpty()) {
            StackItem& item = walk->stack.append();
            item.path = std::move(walk->triple.dirPath);
            item.dirNames = std::move(walk->triple.dirNames);
            item.dirIndex = 0;
        } else {
            walk->triple.dirPath.clear();
            walk->triple.dirNames.clear();
            walk->triple.files.clear();
        }
        while (!walk->stack.isEmpty()) {
            StackItem& item = walk->stack.back();
            if (item.dirIndex < item.dirNames.numItems()) {
                walk->visit(walk->fs->pathFormat().join(item.path, item.dirNames[item.dirIndex]));
                item.dirIndex++;
                return;
            }
            walk->stack.pop();
        }
        // End of walk
        PLY_ASSERT(walk->triple.dirPath.isEmpty());
    }
};
} // namespace details

PLY_NO_INLINE FileSystem::Walk FileSystem::walk(StringView top, u32 flags) {
    details::WalkImpl* walk = new details::WalkImpl;
    walk->destruct = details::WalkImpl::destructImpl;
    walk->next = details::WalkImpl::nextImpl;
    walk->fs = this;
    walk->flags = flags;
    walk->visit(top);
    return walk;
}

PLY_NO_INLINE FSResult FileSystem::makeDirs(StringView path) {
    if (path == this->pathFormat().getDriveLetter(path)) {
        return FileSystem::setLastResult(FSResult::OK);
    }
    ExistsResult er = this->exists(path);
    if (er == ExistsResult::Directory) {
        return FileSystem::setLastResult(FSResult::AlreadyExists);
    } else if (er == ExistsResult::File) {
        return FileSystem::setLastResult(FSResult::AccessDenied);
    } else {
        auto split = this->pathFormat().split(path);
        if (!split.first.isEmpty() && !split.second.isEmpty()) {
            FSResult r = makeDirs(split.first);
            if (r != FSResult::OK && r != FSResult::AlreadyExists)
                return r;
        }
        return this->makeDir(path);
    }
}

PLY_NO_INLINE Owned<InStream> FileSystem::openStreamForRead(StringView path) {
    Owned<InPipe> inPipe = this->funcs->openPipeForRead(this, path);
    if (!inPipe)
        return nullptr;
    return new InStream{std::move(inPipe)};
}

PLY_NO_INLINE Owned<OutStream> FileSystem::openStreamForWrite(StringView path) {
    Owned<OutPipe> outPipe = this->funcs->openPipeForWrite(this, path);
    if (!outPipe)
        return nullptr;
    return new OutStream{std::move(outPipe)};
}

PLY_NO_INLINE Owned<StringReader> FileSystem::openTextForRead(StringView path,
                                                              const TextFormat& textFormat) {
    Owned<InStream> ins = this->openStreamForRead(path);
    if (!ins)
        return nullptr;
    return textFormat.createImporter(std::move(ins));
}

PLY_DLL_ENTRY Tuple<Owned<StringReader>, TextFormat>
FileSystem::openTextForReadAutodetect(StringView path) {
    Owned<InStream> ins = this->openStreamForRead(path);
    if (!ins)
        return {nullptr, TextFormat{}};
    TextFormat textFormat = TextFormat::autodetect(ins);
    return {textFormat.createImporter(std::move(ins)), textFormat};
}

PLY_NO_INLINE String FileSystem::loadBinary(StringView path) {
    String result;
    Owned<InPipe> inPipe = this->openPipeForRead(path);
    if (inPipe) {
        u64 fileSize = inPipe->getFileSize();
        // Files >= 4GB cannot be loaded this way:
        result.resize(safeDemote<u32>(fileSize));
        inPipe->read({result.bytes, result.numBytes});
    }
    return result;
}

PLY_NO_INLINE String FileSystem::loadText(StringView path, const TextFormat& textFormat) {
    Owned<StringReader> sr = this->openTextForRead(path, textFormat);
    String contents;
    if (sr) {
        contents = sr->readRemainingContents();
    }
    return contents;
}

PLY_NO_INLINE Tuple<String, TextFormat> FileSystem::loadTextAutodetect(StringView path) {
    Tuple<Owned<StringReader>, TextFormat> tuple = this->openTextForReadAutodetect(path);
    String contents;
    if (tuple.first) {
        contents = tuple.first->readRemainingContents();
    }
    return {contents, tuple.second};
}

PLY_NO_INLINE Owned<OutStream> FileSystem::openTextForWrite(StringView path,
                                                            const TextFormat& textFormat) {
    Owned<OutStream> outs = this->openStreamForWrite(path);
    if (!outs)
        return nullptr;
    return textFormat.createExporter(std::move(outs));
}

PLY_NO_INLINE FSResult FileSystem::makeDirsAndSaveBinaryIfDifferent(StringView path,
                                                                    StringView view) {
    // FIXME: This could be optimized
    // We don't really need to load the existing file as a binary. We could read and compare it to
    // view incrementally.

    // Load existing contents
    String existingContents = this->loadBinary(path);
    FSResult existingResult = this->lastResult();
    if (existingResult == FSResult::OK && existingContents == view) {
        return FileSystem::setLastResult(FSResult::Unchanged);
    }
    if (existingResult != FSResult::OK && existingResult != FSResult::NotFound) {
        return existingResult;
    }
    existingContents.clear();

    // Create intermediate directories
    FSResult result = this->makeDirs(this->pathFormat().split(path).first);
    if (result != FSResult::OK && result != FSResult::AlreadyExists) {
        return result;
    }

    // Save file
    // FIXME: Use temporary file
    Owned<OutPipe> outPipe = this->openPipeForWrite(path);
    result = this->lastResult();
    if (result != FSResult::OK) {
        return result;
    }
    outPipe->write(view);
    return result;
}

PLY_NO_INLINE FSResult FileSystem::makeDirsAndSaveTextIfDifferent(StringView path,
                                                                  StringView strContents,
                                                                  const TextFormat& textFormat) {
    // FIXME: This could be optimized
    // We don't really need to convert strContents to raw data as a String. We could create an
    // exporter and compare to the existing file incrementally.

    MemOutStream memOut;
    Owned<OutStream> outs = textFormat.createExporter(borrow(&memOut));
    *outs << strContents;
    outs.clear();
    String rawContents = memOut.moveToString();
    return this->makeDirsAndSaveBinaryIfDifferent(path, rawContents);
}

} // namespace ply
