/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-runtime/Precomp.h>
#include <ply-runtime/string/WString.h>

namespace ply {

#if PLY_TARGET_WIN32
Path_t Path{true};
#elif PLY_TARGET_POSIX
Path_t Path{false};
#else
#error "Unsupported target!"
#endif

Path_t WindowsPath{true};
Path_t PosixPath{false};

PLY_NO_INLINE Tuple<StringView, StringView> Path_t::split(StringView path) const {
    s32 lastSepIndex = path.rfindByte([&](char c) { return this->isSepByte(c); });
    if (lastSepIndex >= 0) {
        s32 prefixLen =
            path.rfindByte([&](char c) { return !this->isSepByte(c); }, lastSepIndex) + 1;
        if (path.left(prefixLen) == this->getDriveLetter(path)) {
            prefixLen++; // If prefix is the root, include a separator character
        }
        return {path.left(prefixLen), path.subStr(lastSepIndex + 1)};
    } else {
        return {String{}, path};
    }
}

PLY_NO_INLINE Array<StringView> Path_t::splitFull(StringView path) const {
    Array<StringView> result;
    if (this->hasDriveLetter(path)) {
        if (this->isAbsolute(path)) {
            // Root with drive letter
            result.append(path.left(3));
            path.offsetHead(3);
            while (path.numBytes > 0 && this->isSepByte(path[0])) {
                path.offsetHead(1);
            }
        } else {
            // Drive letter only
            result.append(path.left(2));
            path.offsetHead(2);
        }
    } else if (path.numBytes > 0 && this->isSepByte(path[0])) {
        // Starts with path separator
        result.append(path.left(1));
        path.offsetHead(1);
        while (path.numBytes > 0 && this->isSepByte(path[0])) {
            path.offsetHead(1);
        }
    }
    if (path.numBytes > 0) {
        for (;;) {
            PLY_ASSERT(path.numBytes > 0);
            PLY_ASSERT(!this->isSepByte(path[0]));
            s32 sepPos = path.findByte([&](char c) { return this->isSepByte(c); });
            if (sepPos < 0) {
                result.append(path);
                break;
            }
            result.append(path.left(sepPos));
            path.offsetHead(sepPos);
            s32 nonSepPos = path.findByte([&](char c) { return !this->isSepByte(c); });
            if (nonSepPos < 0) {
                // Empty final component
                result.append({});
                break;
            }
            path.offsetHead(nonSepPos);
        }
    }
    return result;
}

PLY_DLL_ENTRY Tuple<StringView, StringView> Path_t::splitExt(StringView path) const {
    StringView lastComp = path;
    s32 slashPos = lastComp.rfindByte([&](char c) { return this->isSepByte(c); });
    if (slashPos >= 0) {
        lastComp = lastComp.subStr(slashPos + 1);
    }
    s32 dotPos = lastComp.rfindByte([](u32 c) { return c == '.'; });
    if (dotPos < 0 || dotPos == 0) {
        dotPos = lastComp.numBytes;
    }
    return {path.shortenedBy(lastComp.numBytes - dotPos), lastComp.subStr(dotPos)};
}

struct PathCompIterator {
    char firstComp[3] = {0};

    PLY_NO_INLINE void iterateOver(const Path_t* pathFmt, ArrayView<const StringView> components,
                                   const Func<void(StringView)>& callback) {
        s32 absoluteIndex = -1;
        s32 driveLetterIndex = -1;
        for (s32 i = components.numItems - 1; i >= 0; i--) {
            if (absoluteIndex < 0 && pathFmt->isAbsolute(components[i])) {
                absoluteIndex = i;
            }
            if (pathFmt->hasDriveLetter(components[i])) {
                driveLetterIndex = i;
                break;
            }
        }

        // Special first component if there's a drive letter and/or absolute component:
        if (driveLetterIndex >= 0) {
            firstComp[0] = components[driveLetterIndex][0];
            firstComp[1] = ':';
            if (absoluteIndex >= 0) {
                firstComp[2] = pathFmt->sepByte();
                callback(StringView{firstComp, 3});
            } else {
                callback(StringView{firstComp, 2});
            }
        }

        // Choose component to start iterating from:
        u32 i = driveLetterIndex >= 0 ? driveLetterIndex : 0;
        if (absoluteIndex >= 0) {
            PLY_ASSERT((u32) absoluteIndex >= i);
            i = absoluteIndex;
            if (driveLetterIndex < 0) {
                char sepByte = pathFmt->sepByte();
                callback(StringView{&sepByte, 1});
            }
        }

        // Iterate over components. Remember, we've already sent the drive letter and/or initial
        // slash as its own component (if any).
        for (; i < components.numItems; i++) {
            StringView comp = components[i];
            if ((s32) i == driveLetterIndex) {
                comp = comp.subStr(2);
            }

            s32 nonSep = comp.findByte([pathFmt](char c) { return !pathFmt->isSepByte(c); });
            while (nonSep >= 0) {
                s32 sep =
                    comp.findByte([pathFmt](char c) { return pathFmt->isSepByte(c); }, nonSep + 1);
                if (sep < 0) {
                    callback(comp.subStr(nonSep));
                    break;
                } else {
                    callback(comp.subStr(nonSep, sep - nonSep));
                    nonSep = comp.findByte([pathFmt](char c) { return !pathFmt->isSepByte(c); },
                                           sep + 1);
                }
            }
        }
    }

    // Note: Keep the PathCompIterator alive while using the return value
    PLY_NO_INLINE Array<StringView> getNormalizedComps(const Path_t* pathFmt,
                                                       ArrayView<const StringView> components) {
        Array<StringView> normComps;
        u32 upCount = 0;
        this->iterateOver(pathFmt, components, [&](StringView comp) { //
            if (comp == "..") {
                if (normComps.numItems() > upCount) {
                    normComps.pop();
                } else {
                    PLY_ASSERT(normComps.numItems() == upCount);
                    normComps.append("..");
                }
            } else if (comp != "." && !comp.isEmpty()) {
                normComps.append(comp);
            }
        });
        return normComps;
    }
};

PLY_NO_INLINE String Path_t::joinArray(ArrayView<const StringView> components) const {
    PathCompIterator compIter;
    Array<StringView> normComps = compIter.getNormalizedComps(this, components);
    if (normComps.isEmpty()) {
        if (components.numItems > 0 && components.back().isEmpty()) {
            return StringView{"."} + this->sepByte();
        } else {
            return ".";
        }
    } else {
        MemOutStream out;
        bool needSep = false;
        for (StringView comp : normComps) {
            if (needSep) {
                out << this->sepByte();
            } else {
                if (comp.numBytes > 0) {
                    needSep = !this->isSepByte(comp[comp.numBytes - 1]);
                }
            }
            out << comp;
        }
        if ((components.back().isEmpty() || this->isSepByte(components.back().back())) && needSep) {
            out << this->sepByte();
        }
        return out.moveToString();
    }
}

PLY_NO_INLINE String Path_t::makeRelative(StringView ancestor, StringView descendant) const {
    // This function requires either both absolute paths or both relative paths:
    PLY_ASSERT(this->isAbsolute(ancestor) == this->isAbsolute(descendant));

    // FIXME: Implement fastpath when descendant starts with ancestor and there are no ".", ".."
    // components.

    PathCompIterator ancestorCompIter;
    Array<StringView> ancestorComps = ancestorCompIter.getNormalizedComps(this, {ancestor});
    PathCompIterator descendantCompIter;
    Array<StringView> descendantComps = descendantCompIter.getNormalizedComps(this, {descendant});

    // Determine number of matching components
    u32 mc = 0;
    while (mc < ancestorComps.numItems() && mc < descendantComps.numItems()) {
        if (ancestorComps[mc] != descendantComps[mc])
            break;
        mc++;
    }

    // Determine number of ".." to output (will be 0 if drive letters mismatch)
    u32 upFolders = 0;
    if (!this->isAbsolute(ancestor) || mc > 0) {
        upFolders = ancestorComps.numItems() - mc;
    }

    // Form relative path (or absolute path if drive letters mismatch)
    MemOutStream out;
    bool needSep = false;
    for (u32 i = 0; i < upFolders; i++) {
        if (needSep) {
            out << this->sepByte();
        }
        out << "..";
        needSep = true;
    }
    for (u32 i = mc; i < descendantComps.numItems(); i++) {
        if (needSep) {
            out << this->sepByte();
        }
        out << descendantComps[i];
        needSep = !this->isSepByte(descendantComps[i].back());
    }

    // .
    if (out.get_seek_pos() == 0) {
        out << ".";
        needSep = true;
    }

    // Trailing slash
    if (descendant.numBytes > 0 && this->isSepByte(descendant.back()) && needSep) {
        out << this->sepByte();
    }

    return out.moveToString();
}

PLY_NO_INLINE HybridString Path_t::from(const Path_t& srcFormat, StringView srcPath) const {
    if (this->isWindows == srcFormat.isWindows)
        return srcPath;
    String result = srcPath;
    char* bytes = result.bytes;
    for (u32 i = 0; i < result.numBytes; i++) {
        if (srcFormat.isSepByte(bytes[i])) {
            bytes[i] = this->sepByte();
        }
    }
    return result;
}

PLY_NO_INLINE WString win32PathArg(StringView path, bool allowExtended) {
    ViewInStream path_in{path};
    MemOutStream out;
    if (allowExtended && WindowsPath.isAbsolute(path)) {
        out << ArrayView<const char16_t>{u"\\\\?\\", 4}.stringView();
    }
    while (true) {
        s32 codepoint = Unicode{UTF8}.decode_point(path_in);
        if (codepoint < 0)
            break;
        if (codepoint == '/') {
            codepoint = '\\'; // Fix slashes.
        }
        Unicode{UTF16_LE}.encode_point(out, codepoint);
    }
    out.raw_write<u16>(0); // Null terminator.
    return WString::moveFromString(out.moveToString());
}

} // namespace ply
