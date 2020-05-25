<% title "Manipulating Paths" %>
<% synopsis 
Path manipulation.
%>

Plywood provides the `WindowsPath` and `PosixPath` classes for manipulating filesystem paths. All member functions are static and accept `StringView` arguments:

    PosixPath::join("path/to", "file");     // "path/to/file"
    WindowsPath::split("path\\to\\file");   // {"path\\to", "file"}

Plywood also provides `NativePath`, a type alias for either `WindowsPath` or `PosixPath` depending on the target platform:

    NativePath::join(PLY_WORKSPACE_FOLDER, "repos/plywood/docs");

You'll likely use `NativePath` most often, but there's nothing to stop you from using `WindowsPath` on POSIX or `PosixPath` on Windows.

The `WindowsPath` class recognizes both forward and backslash characters as path separators. Its `join()`, `normalize()` and `makeRelative()` functions normalize all path separators to backslashes.

The `PosixPath` class only treats forward slashes as path separators.

Neither of these classes interact with the filesystem in any way. These path manipulation functions operate purely on strings.

You can also manipulate paths indirectly using a `PathFormat` object, which can be obtained by calling `format()`. All `WindowsPath` and `PosixPath` member functions are static, and all `PathFormat` member functions are non-static.

## Normalized Paths

The `join()`, `normalize()` and `makeRelative()` member functions all return **normalized paths**. A normalized path is one in which redundant path separators are collapsed, references to the current directory `"."` are discarded unless it is the only component, and references to the parent directory `".."` collapse the parent directory if there is one. A normalized `WindowsPath` contains no forward slashes and uses only backslashes as path separators. An empty string is normalized to `"."`. Normalized paths may or may not include a trailing path separator; see each function for further details.

## Unicode Support

UTF-8 is the preferred character encoding for paths, but for practical purposes, the only characters treated specially are `/ \ . :` and ASCII letters. Therefore, these path functions work just as well with ISO 8859-1, Windows-1252 and pure ASCII strings.

The `FileSystem` functions, on the other hand, do expect UTF-8 strings.

See [Unicode Support](Unicode) for more information.

## Header File

`#include <ply-runtime/filesystem/Path.h>`

Also included from `<ply-runtime/Base.h>`.

## Member Functions

<% member static const char& [strong sepByte]() %>

`WindowsPath` returns a backslash. `PosixPath` returns a forward slash.

<% member static bool [strong isSepByte](char c) %>

`WindowsPath` returns `true` if `c` is a forward or backslash. `PosixPath` returns `true` if `c` is a forward slash.

<% member static bool [strong endsWithSep](StringView path) %>

`WindowsPath` returns `true` if `path` ends with a forward or backslash. `PosixPath` returns `true` if `path` ends with a forward slash.

<% member static bool [strong hasDriveLetter](StringView path) %>

`WindowsPath` returns `true` if `path` begins with a drive letter prefix such as `"C:"`. `PosixPath` always returns `false`.

<% member static StringView [strong getDriveLetter](StringView path) %>

If `path` begins with a drive letter such as `"C:"`, `WindowsPath` returns the drive letter including the colon. `PosixPath` always returns an empty string.

<% member static bool [strong isAbsolute](StringView path) %>

`WindowsPath` returns `true` if `path` begins with a drive letter and path separator. `PosixPath` returns `true` if `path` begins with `'/'`.

    WindowsPath::isAbsolute("C:\\");       // true
    WindowsPath::isAbsolute("d:/my/path"); // true
    WindowsPath::isAbsolute("C:file");     // false
    WindowsPath::isAbsolute("\\my\\path"); // false
    PosixPath::isAbsolute("/my/path");     // true

An empty string satisfies neither `isAbsolute()` nor `isRelative()`. For `WindowsPath`, a path that begin with a drive letter or path separator only (such as `"C:file"` and `"\\my\\path"`) satisfies neither `isAbsolute()` nor `isRelative()`.

You can make a path absolute using `FileSystem::getAbsolutePath()`.

<% member static bool [strong isRelative](StringView path) %>

Returns `true` if `path` is non-empty and does not begin with a drive letter or path separator.

    WindowsPath::isRelative("file");       // true
    WindowsPath::isRelative("my\\path");   // true
    WindowsPath::isRelative("C:file");     // false
    WindowsPath::isRelative("\\my\\path"); // false
    PosixPath::isRelative("my/path");      // true

An empty string satisfies neither `isAbsolute()` nor `isRelative()`. For `WindowsPath`, a path that begin with a drive letter or path separator only (such as `"C:file"` and `"\\my\\path"`) satisfies neither `isAbsolute()` nor `isRelative()`.

<% member static Tuple<StringView, StringView> [strong split](StringView path) %>

Splits `path` into a `Tuple` where `second` is the last path component and `first` is everything leading up to that. Similar to `os.path.split()` in Python. `second` will never contain a path separator; if `path` ends with a path separator, `second` will be empty. If there is no path separator in `path`, `first` will be empty. Trailing slashes are stripped from `first` unless it is the root. `WindowsPath::split()` does not convert forward slashes to backslashes.

    PosixPath::split("path/to/file");       // {"path/to", "file"}
    PosixPath::split("path/to/folder/");    // {"path/to/folder", ""}
    PosixPath::split("/file_in_root");      // {"/", "file_in_root"}
    PosixPath::split("file");               // {"", "file"}
    WindowsPath::split("path\\to\\file");   // {"path\\to", "file"}
    WindowsPath::split("path/to/file");     // {"path/to", "file"}

<% member static Array<StringView> [strong splitFull](StringView path) %>

Splits `path` into an array of items where each item contains a single path component. When `WindowsPath` is given an absolute path, it returns the drive letter and first path separator as the first component. When `PosixPath` is given an absolute path, it returns `"/"` as the first component. None of the other components contain a path separator. If `path` ends with a path separator, the last component is an empty string.

    PosixPath::splitFull("path/to/file");       // {"path", "to", "file"}
    PosixPath::splitFull("path/to/folder/");    // {"path", "to", "folder", ""}
    PosixPath::splitFull("/file_in_root");      // {"/", "file_in_root"}
    WindowsPath::splitFull("path\\to/file");    // {"path", "to", "file"}
    WindowsPath::splitFull("C:\\file_in_root"); // {"C:\\", "file_in_root"}

<% member static Tuple<StringView, StringView> [strong splitExt](StringView path) %>

Splits `path` into a `Tuple` such that `first + second == path`, and `second` is empty or begins with a period and contains at most one period. Similar to `os.path.splitext()` in Python. Leading periods on the basename, as in `".gitignore"`, are ignored.

    PosixPath::splitExt("file.txt");           // {"file", ".txt"}
    PosixPath::splitExt("file");               // {"file"}
    PosixPath::splitExt("file.tar.gz");        // {"file.tar", ".gz"}
    PosixPath::splitExt("path/to/file.txt");   // {"path/to/file", ".txt"}
    PosixPath::splitExt("folder.tmp/");        // {"folder.tmp/", ""}
    PosixPath::splitExt("folder.tmp/file");    // {"folder.tmp/file", ""}
    PosixPath::splitExt(".gitignore");         // {".gitignore", ""}
    PosixPath::splitExt("path/to/.gitignore"); // {"path/to/.gitignore", ""}

<% member static String [strong join]([em components...]) %>

Joins one or more path components together and normalizes the result. Note that this function is not the same as `os.path.join()` in Python. It's closer (but not identical) to `os.path.normpath(os.path.join())` in Python.

The returned path has a trailing slash if and only if the last component has a trailing slash or is an empty string. If any argument is an absolute path, all previous arguments are thrown away except for any drive letter that was encountered. For `WindowsPath`, if any argument contains a drive letter, all previous arguments are thrown away. 

    PosixPath::join("path/to", "file");          // "path/to/file"
    PosixPath::join("path/to", "folder/");       // "path/to/folder/"
    PosixPath::join("path/to", "folder", "");    // "path/to/folder/"
    PosixPath::join("path/to", "./file");        // "path/to/file"
    PosixPath::join("path/to/file", "../other"); // "path/to/other"
    WindowsPath::join("C:\\", "bad/slash");      // "C:\\bad\\slash"

<% member static String [strong normalize]([em components...]) %>

Equivalent to `join()`.

<% member static bool [strong isNormalized](StringView path) %>

Equivalent to `path == normalize(path)`.

<% member static String [strong makeRelative](StringView ancestor, StringView descendant) %>

Returns a new path `relative` such that `join(ancestor, relative)` is equivalent to `descendant`. If `ancestor` is an absolute path, `descendant` must also be an absolute path, and if `ancestor` is a relative path, `descendant` must also be a relative path. The returned path is normalized. The returned path has a trailing slash if and only if `descendant` has a trailing slash.

    PosixPath::makeRelative("/path/to", "/path/to/file");        // "file"
    PosixPath::makeRelative("/path/to/", "/path/to/file");       // "file"
    PosixPath::makeRelative("/path/to", "/path/to/a/b");         // "a/b"
    PosixPath::makeRelative("/path/to/file", "/path/to/other");  // "../other"
    PosixPath::makeRelative("/path/to/file", "/path/to/other/"); // "../other/"
    PosixPath::makeRelative("path/to", "path/to");               // "."
    PosixPath::makeRelative("path/to/", "path/to");              // "."
    PosixPath::makeRelative("path/to", "path/to/");              // "./"

<% member static PathFormat [strong format]() %>

Returns a `PathFormat` object that exposes the same functionality as `WindowsPath` or `PosixPath`, but using non-static member functions instead of static member functions. You can pass the `PathFormat` object to other functions to perform platform-specific path manipulation indirectly.

<% member static HybridString [strong from]<[em SrcFormat]>(StringView path) %>

Converts the path separators in `path`. `PosixPath::from<WindowsPath>()` converts all backslashes to forward slashes. `WindowsPath::from<PosixPath>()` and `WindowsPath::from<WindowsPath>()` convert all forward slashes to backslashes. Otherwise the returned string is identical.

    // Returns "path\\to\\file":
    WindowsPath::from<PosixPath>("path/to/file");

    // Returns "C:/path/to/file":
    PosixPath::from<WindowsPath>("C:\\path\\to\\file");

<% member static HybridString [strong from](const PathFormat& srcFormat, StringView path) %>

Same as the previous function, but the source format is determined by function argument instead of template argument.

    // Returns "path\\to\\file":
    WindowsPath::from(PosixPath::format(), "path/to/file");

    // Returns "C:/path/to/file":
    PosixPath::from(WindowsPath::format(), "C:\\path\\to\\file");

<% endMembers %>
