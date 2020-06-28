/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <pylon/Core.h>
#include <pylon/Node.h>
#include <ply-runtime/io/text/StringWriter.h>
#include <ply-runtime/io/text/FileLocationMap.h>

namespace pylon {

struct ParseError {
    struct Scope {
        enum Type { Object, Property, Duplicate, Array };
        u32 fileOfs;
        Type type;
        StringView name;
        u32 index;

        static PLY_INLINE Scope object(u32 fileOfs) {
            return {fileOfs, Object, {}, 0};
        }
        static PLY_INLINE Scope property(u32 fileOfs, StringView name) {
            return {fileOfs, Property, name, 0};
        }
        static PLY_INLINE Scope duplicate(u32 fileOfs) {
            return {fileOfs, Duplicate, {}, 0};
        }
        static PLY_INLINE Scope array(u32 fileOfs, u32 index) {
            return {fileOfs, Array, {}, index};
        }
    };

    u32 fileOfs;
    HybridString message;
    const Array<Scope>& context;
};

class Parser {
private:
    struct Token {
        enum Type {
            Invalid,
            OpenCurly,
            CloseCurly,
            OpenSquare,
            CloseSquare,
            Colon,
            Equals,
            Comma,
            Semicolon,
            Text,
            Junk,
            NewLine,
            EndOfFile
        };
        Type type = Invalid;
        u32 fileOfs = 0;
        HybridString text;

        PLY_INLINE bool isValid() const {
            return type != Type::Invalid;
        }
    };

    Functor<void(const ParseError& err)> errorCallback;
    FileLocationMap fileLocMap;
    bool anyError_ = false;
    ConstBufferView srcView;
    u32 readOfs = 0;
    s32 nextUnit = 0;
    u32 tabSize = 4;
    Token pushBackToken;
    Array<ParseError::Scope> context;

    PLY_INLINE void pushBack(Token&& token) {
        pushBackToken = std::move(token);
    }

    struct ScopeHandler {
        Parser& parser;
        u32 index;

        PLY_INLINE ScopeHandler(Parser& parser, ParseError::Scope&& scope)
            : parser{parser}, index{parser.context.numItems()} {
            parser.context.append(std::move(scope));
        }
        PLY_INLINE ~ScopeHandler() {
            // parser.context can be empty when ParseError is thrown
            if (!parser.context.isEmpty()) {
                PLY_ASSERT(parser.context.numItems() == index + 1);
                parser.context.pop();
            }
        }
        PLY_INLINE ParseError::Scope& get() {
            return parser.context[index];
        }
    };

    void error(u32 fileOfs, HybridString&& message);
    void advanceChar();
    Token readPlainToken(Token::Type type);
    bool readEscapedHex(OutStream* outs, u32 escapeFileOfs);
    Token readQuotedString();
    Token readLiteral();
    Token readToken(bool tokenizeNewLine = false);
    static HybridString toString(const Token& token);
    static HybridString toString(const Node& node);
    Node readObject(const Token& startToken);
    Node readArray(const Token& startToken);
    Node readExpression(Token&& firstToken, const Token* afterToken = nullptr);

public:
    PLY_INLINE void setTabSize(int tabSize_) {
        tabSize = tabSize_;
    }
    PLY_INLINE void setErrorCallback(Functor<void(const ParseError& err)>&& cb) {
        this->errorCallback = std::move(cb);
    }
    PLY_INLINE bool anyError() const {
        return this->anyError_;
    }

    struct Result {
        Node root;
        FileLocationMap fileLocMap;
    };

    void dumpError(const ParseError& error, StringWriter& sw) const;

    Result parse(ConstBufferView srcView_);

    PLY_INLINE Result parse(StringView srcView_) {
        return parse(srcView_.bufferView());
    }
};

} // namespace pylon
