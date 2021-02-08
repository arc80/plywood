/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <web-markdown/Core.h>
#include <web-markdown/Markdown.h>
#include <ply-runtime/string/TextEncoding.h>
#include <ply-runtime/algorithm/Find.h>

namespace ply {
namespace markdown {

// A helper class that keeps track of the indentation level while consuming input.
struct LineConsumer {
    ViewInStream vins;
    u32 outerIndent = 0;
    u32 indent = 0;

    PLY_NO_INLINE bool consumeSpaceOrTab() {
        if (this->vins.numBytesAvailable() == 0)
            return false;
        char c = *this->vins.curByte;
        if (c == ' ') {
            this->indent++;
        } else if (c == '\t') {
            u32 tabSize = 4;
            this->indent += tabSize - (this->indent % tabSize);
        } else {
            return false;
        }
        this->vins.advanceByte();
        return true;
    }

    PLY_INLINE u32 innerIndent() const {
        return this->indent - this->outerIndent;
    }

    PLY_INLINE StringView trimmedRemainder() const {
        return this->vins.viewAvailable().trim(isWhite);
    }

    PLY_INLINE LineConsumer(StringView line) : vins{line} {
    }
};

//-----------------------------------------------------------------
// Code to parse block elements (first pass)
//-----------------------------------------------------------------
String extractCodeLine(StringView line, u32 fromIndent) {
    u32 indent = 0;
    for (u32 i = 0; i < line.numBytes; i++) {
        if (indent == fromIndent) {
            return line.subStr(i);
        }
        u8 c = line[i];
        PLY_ASSERT(c < 128);              // No high code points
        PLY_ASSERT(c >= 32 || c == '\t'); // No control characters
        if (c == '\t') {
            u32 tabSize = 4;
            u32 newIndent = indent + tabSize - (indent % tabSize);
            if (newIndent > fromIndent) {
                return StringView{" "} * (newIndent - fromIndent) + line.subStr(i + 1);
            }
            indent = newIndent;
        } else {
            indent++;
        }
    }
    PLY_ASSERT(0);
    return {};
}

struct Parser {
    // blockNodes and leafNode, together, represent the stack of elements for the current line.
    // blockNodes[0] holds the root Node::Document.
    // blockNodes[1 and greater] can only hold a Node::BlockQuote or Node::ListItem.
    // leafNode represents the top of the stack where text goes. It can either hold a
    // Node::Paragraph or Node::CodeBlock.
    Array<Node*> blockNodes;
    Node* leafNode = nullptr;

    // Only used if leafNode is CodeBlock:
    u32 numBlankLinesInCodeBlock = 0;

    // This flag indicates that some Lists on the stack have their isLooseIfContinued flag set:
    // (Alternatively, we *could* store the number of such Lists on the stack, and eliminate the
    // isLooseIfContinued flag completely, but it would complicate matchExistingIndentation a little
    // bit. Sticking with this approach for now.)
    bool checkListContinuations = false;

    // This is called at the start of each line. It figures out which of the existing elements we
    // are still inside by consuming indentation and blockquote '>' markers that match the current
    // element stack, then truncates any unmatched BlockQuote nodes from the stack. (Unmatched
    // ListItem nodes are not truncated here because list items are allowed to contain blank lines.)
    // Returns true if there is more text on the current line.
    PLY_NO_INLINE bool matchExistingIndentation(StringView line, LineConsumer& lc) {
        u32 keepStackDepth = 1;
        for (;;) {
            while (lc.consumeSpaceOrTab()) {
            }
            if (keepStackDepth >= this->blockNodes.numItems())
                break;
            Node* node = this->blockNodes[keepStackDepth];
            if (node->type == Node::BlockQuote) {
                if (lc.vins.numBytesAvailable() > 0 && *lc.vins.curByte == '>' &&
                    lc.innerIndent() <= 3) {
                    // Continue the current blockquote
                    lc.vins.advanceByte();
                    lc.indent++;
                    lc.outerIndent = lc.indent;
                    keepStackDepth++;
                    // Consume optional space and include it in outerIndent:
                    if (lc.consumeSpaceOrTab()) {
                        lc.outerIndent++;
                    }
                    continue;
                }
            } else if (node->type == Node::ListItem) {
                if (lc.innerIndent() >= node->indentOrLevel) {
                    // Continue the current list item
                    keepStackDepth++;
                    lc.outerIndent += node->indentOrLevel;
                    continue;
                }
            } else {
                // blockNodes indices >= 1 should only hold BlockQuote and ListItem
                PLY_ASSERT(0);
            }
            break;
        }

        // Is remainder of line blank?
        if (lc.trimmedRemainder().isEmpty()) {
            // Yes. Terminate paragraph if any
            if (this->leafNode && this->leafNode->type == Node::Paragraph) {
                this->leafNode = nullptr;
                PLY_ASSERT(this->numBlankLinesInCodeBlock == 0);
            }
            // Truncate non-continued blockquotes
            while (keepStackDepth < this->blockNodes.numItems() &&
                   this->blockNodes[keepStackDepth]->type == Node::ListItem) {
                keepStackDepth++;
            }
            PLY_ASSERT(keepStackDepth >= this->blockNodes.numItems() ||
                       this->blockNodes[keepStackDepth]->type == Node::BlockQuote);
            if (keepStackDepth < this->blockNodes.numItems()) {
                this->blockNodes.resize(keepStackDepth);
                this->leafNode = nullptr;
                this->numBlankLinesInCodeBlock = 0;
            }
            if (this->leafNode) {
                // At this point, the only possible leaf node is a CodeBlock, because Paragraphs are
                // terminated above, and Headings don't persist across lines.
                PLY_ASSERT(this->leafNode->type == Node::CodeBlock);
                // Count blank lines in CodeBlocks
                if (lc.indent - lc.outerIndent > 4) {
                    // Add intermediate blank lines
                    // FIXME: Could this be unified with the code below? (Code simplification)
                    for (u32 i = 0; i < this->numBlankLinesInCodeBlock; i++) {
                        this->leafNode->rawLines.append("\n");
                    }
                    this->numBlankLinesInCodeBlock = 0;
                    String codeLine = extractCodeLine(line, lc.outerIndent + 4);
                    this->leafNode->rawLines.append(std::move(codeLine));
                } else {
                    this->numBlankLinesInCodeBlock++;
                }
            } else {
                // There's no leaf node and the remainder of the line is blank.
                // Walk the stack and set the "isLooseIfContinued" flag on all Lists.
                for (Node* node : this->blockNodes) {
                    if (node->type == Node::ListItem) {
                        PLY_ASSERT(node->parent->type == Node::List);
                        if (!node->parent->isLoose) {
                            node->parent->isLooseIfContinued = true;
                            this->checkListContinuations = true;
                        }
                    }
                }
            }
            return false;
        }

        // No. There's more text on the current line
        if (keepStackDepth < this->blockNodes.numItems()) {
            this->blockNodes.resize(keepStackDepth);
            this->leafNode = nullptr;
            this->numBlankLinesInCodeBlock = 0;
        }
        return true;
    }

    // This function consumes new blockquote '>' markers and list item markers such as '*' that
    // *don't* match existing block elements on the current stack. It creates new block elements for
    // each marker encountered.
    void parseNewMarkers(LineConsumer& lc) {
        PLY_ASSERT(!lc.trimmedRemainder().isEmpty()); // Not called if remainder of line is blank

        // Attempt to parse new Node markers
        while (lc.vins.numBytesAvailable() > 0) {
            if (lc.innerIndent() >= 4)
                break;

            auto savePoint = lc.vins.savePoint();
            u32 savedIndent = lc.indent;

            // This code block will handle any list markers encountered:
            auto gotListMarker = [&](s32 markerNumber, char punc) {
                bool isOrdered = (markerNumber >= 0);
                this->leafNode = nullptr;
                this->numBlankLinesInCodeBlock = 0;
                Node* listNode = nullptr;
                Node* parentCtr = this->blockNodes.back();
                PLY_ASSERT(parentCtr->isContainerBlock());
                if (!parentCtr->children.isEmpty()) {
                    Node* potentialParent = parentCtr->children.back();
                    if (potentialParent->type == Node::List &&
                        potentialParent->isOrderedList() == isOrdered &&
                        potentialParent->listPunc == punc) {
                        // Add item to existing list
                        listNode = potentialParent;
                    }
                } else if (parentCtr->type == Node::ListItem) {
                    // Begin new list as a sublist of existing list
                    parentCtr = parentCtr->parent;
                    PLY_ASSERT(parentCtr->type == Node::List);
                }
                if (!listNode) {
                    // Begin new list
                    listNode = new Node{parentCtr, Node::List};
                    listNode->listStartNumber = markerNumber;
                    listNode->listPunc = punc;
                }
                Node* listItem = new Node{listNode, Node::ListItem};
                listItem->indentOrLevel = lc.outerIndent;
                this->blockNodes.append(listItem);
            };

            char c = *lc.vins.curByte;
            PLY_ASSERT(!isWhite(c));
            if (c == '>') {
                // Begin a new blockquote
                this->blockNodes.append(new Node{this->blockNodes.back(), Node::BlockQuote});
                // Consume optional space after '>'
                lc.vins.advanceByte();
                lc.indent++;
                lc.outerIndent = lc.indent;
                if (lc.consumeSpaceOrTab()) {
                    lc.outerIndent++;
                }
            } else if (c == '*' || c == '-' || c == '+') {
                lc.vins.advanceByte();
                lc.indent++;
                u32 indentAfterStar = lc.indent;
                if (!lc.consumeSpaceOrTab())
                    goto notMarker;
                if (this->leafNode && lc.trimmedRemainder().isEmpty()) {
                    // If the list item interrupts a paragraph, it must not begin with a blank line.
                    goto notMarker;
                }

                // It's an unordered list item.
                lc.outerIndent = indentAfterStar + 1;
                gotListMarker(-1, c);
            } else if (isDecimalDigit(c)) {
                u64 num = lc.vins.parse<u64>();
                if (this->leafNode && num != 1) {
                    // If list item interrupts a paragraph, the start number must be 1.
                    goto notMarker;
                }
                uptr markerLength = (lc.vins.curByte - savePoint.startByte);
                if (markerLength > 9)
                    goto notMarker; // marker too long
                lc.indent += safeDemote<u32>(markerLength);
                if (lc.vins.numBytesAvailable() < 2)
                    goto notMarker;
                char punc = *lc.vins.curByte;
                // FIXME: support alternate punctuator ')'.
                // If the punctuator doesn't match, it should start a new list.
                if (punc != '.' && punc != ')')
                    goto notMarker;
                lc.vins.advanceByte();
                lc.indent++;
                u32 indentAfterMarker = lc.indent;
                if (!lc.consumeSpaceOrTab())
                    goto notMarker;
                if (this->leafNode && lc.trimmedRemainder().isEmpty()) {
                    // If the list item interrupts a paragraph, it must not begin with a blank line.
                    goto notMarker;
                }

                // It's an ordered list item.
                // 32-bit demotion is safe because we know the marker is 9 digits or less:
                lc.outerIndent = indentAfterMarker + 1;
                gotListMarker(safeDemote<s32>(num), punc);
            } else {
                goto notMarker;
            }

            // Consume whitespace
            while (lc.consumeSpaceOrTab()) {
            }
            continue;

        notMarker:
            lc.vins.restore(savePoint);
            lc.indent = savedIndent;
            break;
        }
    }

    PLY_NO_INLINE void parseParagraphText(StringView line, LineConsumer& lc) {
        StringView remainingText = lc.trimmedRemainder();
        bool hasPara = this->leafNode && this->leafNode->type == Node::Paragraph;
        if (!hasPara && lc.innerIndent() >= 4) {
            // Potentially begin or append to code Node
            if (remainingText && !this->leafNode) {
                this->leafNode = new Node{this->blockNodes.back(), Node::CodeBlock};
                PLY_ASSERT(this->numBlankLinesInCodeBlock == 0);
            }
            if (this->leafNode) {
                PLY_ASSERT(this->leafNode->type == Node::CodeBlock);
                // Add intermediate blank lines
                for (u32 i = 0; i < this->numBlankLinesInCodeBlock; i++) {
                    this->leafNode->rawLines.append("\n");
                }
                this->numBlankLinesInCodeBlock = 0;
                String codeLine = extractCodeLine(line, lc.outerIndent + 4);
                this->leafNode->rawLines.append(std::move(codeLine));
            }
        } else {
            if (remainingText) {
                // We're going to create or extend a leaf node.
                // First, check if any Lists should be marked loose:
                if (this->checkListContinuations) {
                    // Yes, we should mark some (possibly zero) lists loose. It's impossible for a
                    // leaf node to exist at this point:
                    PLY_ASSERT(!this->leafNode);
                    for (Node* node : this->blockNodes) {
                        if (node->type == Node::ListItem) {
                            PLY_ASSERT(node->parent->type == Node::List);
                            if (node->parent->isLooseIfContinued) {
                                node->parent->isLoose = true;
                                node->parent->isLooseIfContinued = false;
                            }
                        }
                    }
                    this->checkListContinuations = false;
                }

                if (*lc.vins.curByte == '#' && lc.innerIndent() <= 3) {
                    // Attempt to parse a heading
                    auto savePoint = lc.vins.savePoint();
                    StringView poundSeq =
                        lc.vins.readView(fmt::Callback{[](char c) { return c == '#'; }});
                    StringView space = lc.vins.readView<fmt::Whitespace>();
                    if (poundSeq.numBytes <= 6 &&
                        (!space.isEmpty() || lc.vins.numBytesAvailable() == 0)) {
                        // Got a heading
                        Node* headingNode = new Node{this->blockNodes.back(), Node::Heading};
                        headingNode->indentOrLevel = poundSeq.numBytes;
                        if (StringView remainingText = lc.trimmedRemainder()) {
                            headingNode->rawLines.append(remainingText);
                        }
                        this->leafNode = nullptr;
                        return;
                    }
                    lc.vins.restore(savePoint);
                }
                // If this->leafNode already exists, it's a lazy paragraph continuation
                if (!hasPara) {
                    // Begin new paragraph
                    this->leafNode = new Node{this->blockNodes.back(), Node::Paragraph};
                    this->numBlankLinesInCodeBlock = 0;
                }
                this->leafNode->rawLines.append(remainingText);
            } else {
                PLY_ASSERT(!this->leafNode); // Should already be cleared by this point
            }
        }
    }

    PLY_NO_INLINE Owned<Node> parse(StringView src) {
        ViewInStream vins{src};
        Owned<Node> document = new Node{nullptr, Node::Document};

        // Initialize stack
        this->blockNodes = {document.get()};
        this->leafNode = nullptr;
        this->numBlankLinesInCodeBlock = 0;

        while (StringView line = vins.readView<fmt::Line>()) {
            LineConsumer lc{line};
            if (!matchExistingIndentation(line, lc))
                continue; // Remainder of line is blank

            parseNewMarkers(lc);
            parseParagraphText(line, lc);
        }

        return document;
    }
};

//-----------------------------------------------------------------
// Code to parse inline elements (second pass)
//-----------------------------------------------------------------
struct InlineConsumer {
    ArrayView<const String> rawLines;
    StringView rawLine;
    u32 lineIndex = 0;
    u32 i = 0;

    InlineConsumer(ArrayView<const String> rawLines) : rawLines{rawLines} {
        PLY_ASSERT(rawLines.numItems > 0);
        rawLine = rawLines[0];
        PLY_ASSERT(rawLine);
    }

    enum ValidIndexResult { SameLine, NextLine, End };

    ValidIndexResult validIndex() {
        if (this->i >= this->rawLine.numBytes) {
            if (this->lineIndex >= this->rawLines.numItems) {
                return End;
            }
            this->i = 0;
            this->lineIndex++;
            if (this->lineIndex >= this->rawLines.numItems) {
                this->rawLine = {};
                return End;
            }
            this->rawLine = this->rawLines[this->lineIndex];
            PLY_ASSERT(this->rawLine);
            return NextLine;
        }
        return SameLine;
    }
};

String getCodeSpan(InlineConsumer& ic, u32 endTickCount) {
    MemOutStream mout;
    for (;;) {
        InlineConsumer::ValidIndexResult res = ic.validIndex();
        if (res == InlineConsumer::End)
            return {};
        if (res == InlineConsumer::NextLine) {
            mout << ' ';
        }
        char c = ic.rawLine[ic.i];
        ic.i++;
        if (c == '`') {
            u32 tickCount = 1;
            for (; ic.i < ic.rawLine.numBytes && ic.rawLine[ic.i] == '`'; ic.i++) {
                tickCount++;
            }
            if (tickCount == endTickCount) {
                String result = mout.moveToString();
                PLY_ASSERT(result);
                if (result[0] == ' ' && result.back() == ' ' &&
                    result.findByte([](char c) { return c != ' '; }) >= 0) {
                    result = result.subStr(1, result.numBytes - 2);
                }
                return result;
            }
            mout << ic.rawLine.subStr(ic.i - tickCount, tickCount);
        } else {
            mout << c;
        }
    }
}

// FIXME: Recognize all Unicode punctuation
PLY_INLINE bool isAscPunc(char c) {
    return (c >= 0x21 && c <= 0x2f) || (c >= 0x3a && c <= 0x40) || (c >= 0x5b && c <= 0x60) ||
           (c >= 0x7b && c <= 0x7e);
}

struct Delimiter {
    enum Type {
        RawText,
        Stars,
        Underscores,
        OpenLink,
        InlineElem,
    };

    Type type = RawText;
    bool leftFlanking = false;  // Stars & Underscores only
    bool rightFlanking = false; // Stars & Underscores only
    bool active = true;         // OpenLink only
    HybridString text;
    Owned<Node> element; // InlineElem only, and it'll be an inline node type

    PLY_INLINE Delimiter() = default;
    PLY_INLINE Delimiter(Type type, HybridString&& text) : type{type}, text{std::move(text)} {
    }
    PLY_INLINE Delimiter(Owned<Node>&& elem) : type{InlineElem}, element{std::move(elem)} {
    }
    static PLY_NO_INLINE Delimiter makeRun(Type type, StringView rawLine, u32 start, u32 numBytes) {
        bool precededByWhite = (start == 0) || isWhite(rawLine[start - 1]);
        bool followedByWhite =
            (start + numBytes >= rawLine.numBytes) || isWhite(rawLine[start + numBytes]);
        bool precededByPunc = (start > 0) && isAscPunc(rawLine[start - 1]);
        bool followedByPunc =
            (start + numBytes < rawLine.numBytes) && isAscPunc(rawLine[start + numBytes]);

        Delimiter result{type, rawLine.subStr(start, numBytes)};
        result.leftFlanking =
            !followedByWhite &&
            (!followedByPunc || (followedByPunc && (precededByWhite || precededByPunc)));
        result.rightFlanking =
            !precededByWhite &&
            (!precededByPunc || (precededByPunc && (followedByWhite || followedByPunc)));
        return result;
    }
};

Tuple<bool, String> parseLinkDestination(InlineConsumer& ic) {
    // FIXME: Support < > destinations
    // FIXME: Support link titles

    // Skip initial whitespace
    for (;;) {
        InlineConsumer::ValidIndexResult res = ic.validIndex();
        if (res == InlineConsumer::End) {
            return {false, String{}};
        }
        if (!isWhite(ic.rawLine[ic.i]))
            break;
        ic.i++;
    }

    MemOutStream mout;
    u32 parenNestLevel = 0;
    for (;;) {
        InlineConsumer::ValidIndexResult res = ic.validIndex();
        if (res != InlineConsumer::SameLine)
            break;

        char c = ic.rawLine[ic.i];
        if (c == '\\') {
            ic.i++;
            if (ic.validIndex() != InlineConsumer::SameLine) {
                mout << '\\';
                break;
            }
            c = ic.rawLine[ic.i];
            if (!isAscPunc(c)) {
                mout << '\\';
            }
            mout << c;
        } else if (c == '(') {
            ic.i++;
            mout << c;
            parenNestLevel++;
        } else if (c == ')') {
            if (parenNestLevel > 0) {
                ic.i++;
                mout << c;
                parenNestLevel--;
            } else {
                break;
            }
        } else if (c >= 0 && c <= 32) {
            break;
        } else {
            ic.i++;
            mout << c;
        }
    }

    if (parenNestLevel != 0) {
        return {false, String{}};
    }

    // Skip trailing whitespace
    for (;;) {
        InlineConsumer::ValidIndexResult res = ic.validIndex();
        if (res == InlineConsumer::End) {
            return {false, String{}};
        }
        char c = ic.rawLine[ic.i];
        if (c == ')') {
            ic.i++;
            return {true, mout.moveToString()};
        } else if (!isWhite(c)) {
            return {false, String{}};
        }
        ic.i++;
    }
}

Array<Owned<Node>> convertToInlineElems(ArrayView<Delimiter> delimiters) {
    Array<Owned<Node>> elements;
    for (Delimiter& delimiter : delimiters) {
        if (delimiter.type == Delimiter::InlineElem) {
            elements.append(std::move(delimiter.element));
        } else {
            if (!(elements.numItems() > 0 && elements.back()->type == Node::Text)) {
                elements.append(new Node{nullptr, Node::Text});
            }
            elements.back()->text += delimiter.text;
        }
    }
    return elements;
}

Array<Owned<Node>> processEmphasis(Array<Delimiter>& delimiters, u32 bottomPos) {
    u32 starOpener = bottomPos;
    u32 underscoreOpener = bottomPos;
    for (u32 pos = bottomPos; pos < delimiters.numItems(); pos++) {
        auto handleCloser = [&](Delimiter::Type type, u32& openerPos) {
            for (u32 j = pos; j > openerPos;) {
                --j;
                if (delimiters[j].type == type && delimiters[j].leftFlanking) {
                    u32 spanLength =
                        min(delimiters[j].text.numBytes, delimiters[pos].text.numBytes);
                    PLY_ASSERT(spanLength > 0);
                    Owned<Node> elem =
                        new Node{nullptr, spanLength >= 2 ? Node::Strong : Node::Emphasis};
                    elem->addChildren(
                        convertToInlineElems(delimiters.subView(j + 1, pos - j - 1)));
                    u32 delimsToSubtract = min(spanLength, 2u);
                    delimiters[j].text.numBytes -= delimsToSubtract;
                    delimiters[pos].text.numBytes -= delimsToSubtract;
                    // We're going to delete from j to pos inclusive, so leave remaining delimiters
                    // if any
                    if (!delimiters[j].text.isEmpty()) {
                        j++;
                    }
                    if (!delimiters[pos].text.isEmpty()) {
                        pos--;
                    }
                    delimiters.erase(j, pos + 1 - j);
                    delimiters.insert(j) = std::move(elem);
                    pos = j;
                    starOpener = min(starOpener, pos + 1);
                    underscoreOpener = min(starOpener, pos + 1);
                    return;
                }
            }
            // None found
            openerPos = pos + 1;
        };
        if (delimiters[pos].type == Delimiter::Stars && delimiters[pos].rightFlanking) {
            handleCloser(Delimiter::Stars, starOpener);
        } else if (delimiters[pos].type == Delimiter::Underscores &&
                   delimiters[pos].rightFlanking) {
            handleCloser(Delimiter::Underscores, underscoreOpener);
        }
    }
    Array<Owned<Node>> result = convertToInlineElems(delimiters.subView(bottomPos));
    delimiters.resize(bottomPos);
    return result;
}

Array<Owned<Node>> expandInlineElements(ArrayView<const String> rawLines) {
    Array<Delimiter> delimiters;
    InlineConsumer ic{rawLines};
    u32 flushedIndex = 0;
    auto flushText = [&] {
        if (ic.i > flushedIndex) {
            delimiters.append(
                {Delimiter::RawText, ic.rawLine.subStr(flushedIndex, ic.i - flushedIndex)});
            flushedIndex = ic.i;
        }
    };
    for (;;) {
        if (ic.i >= ic.rawLine.numBytes) {
            flushText();
            ic.i = 0;
            flushedIndex = 0;
            ic.lineIndex++;
            if (ic.lineIndex >= ic.rawLines.numItems)
                break;
            ic.rawLine = ic.rawLines[ic.lineIndex];
            delimiters.append(Owned<Node>::create(nullptr, Node::SoftBreak));
        }

        char c = ic.rawLine[ic.i];
        if (c == '`') {
            flushText();
            u32 tickCount = 1;
            for (ic.i++; ic.i < ic.rawLine.numBytes && ic.rawLine[ic.i] == '`'; ic.i++) {
                tickCount++;
            }
            // Try consuming code span
            InlineConsumer backup = ic;
            String codeStr = getCodeSpan(ic, tickCount);
            if (codeStr) {
                Owned<Node> codeSpan = new Node{nullptr, Node::CodeSpan};
                codeSpan->text = std::move(codeStr);
                delimiters.append(std::move(codeSpan));
                flushedIndex = ic.i;
            } else {
                ic = backup;
                flushText();
            }
        } else if (c == '*') {
            flushText();
            u32 runLength = 1;
            for (ic.i++; ic.i < ic.rawLine.numBytes && ic.rawLine[ic.i] == '*'; ic.i++) {
                runLength++;
            }
            delimiters.append(
                Delimiter::makeRun(Delimiter::Stars, ic.rawLine, ic.i - runLength, runLength));
            flushedIndex = ic.i;
        } else if (c == '_') {
            flushText();
            u32 runLength = 1;
            for (ic.i++; ic.i < ic.rawLine.numBytes && ic.rawLine[ic.i] == '_'; ic.i++) {
                runLength++;
            }
            delimiters.append(Delimiter::makeRun(Delimiter::Underscores, ic.rawLine,
                                                 ic.i - runLength, runLength));
            flushedIndex = ic.i;
        } else if (c == '[') {
            flushText();
            delimiters.append({Delimiter::OpenLink, ic.rawLine.subStr(ic.i, 1)});
            ic.i++;
            flushedIndex = ic.i;
        } else if (c == ']') {
            // Try to parse an inline link
            flushText();
            ic.i++;
            if (!(ic.i < ic.rawLine.numBytes && ic.rawLine[ic.i] == '('))
                continue; // No parenthesis

            // Got opening parenthesis
            ic.i++;

            // Look for preceding OpenLink delimiter
            s32 openLink = rfind(delimiters, [](const Delimiter& delim) {
                return delim.type == Delimiter::OpenLink;
            });
            if (openLink < 0)
                continue; // No preceding OpenLink delimiter

            // Found a preceding OpenLink delimiter
            // Try to parse link destination
            InlineConsumer backup = ic;
            Tuple<bool, String> linkDest = parseLinkDestination(ic);
            if (!linkDest.first) {
                // Couldn't parse link destination
                ic = backup;
                continue;
            }

            // Successfully parsed link destination
            Owned<Node> elem = new Node{nullptr, Node::Link};
            elem->text = std::move(linkDest.second);
            elem->addChildren(processEmphasis(delimiters, openLink + 1));
            delimiters.resize(openLink);
            delimiters.append(std::move(elem));
            flushedIndex = ic.i;
        } else {
            ic.i++;
        }
    }

    return processEmphasis(delimiters, 0);
}

static PLY_NO_INLINE void doInlines(Node* node) {
    if (node->isContainerBlock()) {
        PLY_ASSERT(node->rawLines.isEmpty());
        for (Node* child : node->children) {
            doInlines(child);
        }
    } else {
        PLY_ASSERT(node->isLeafBlock());
        if (node->type != Node::CodeBlock) {
            node->addChildren(expandInlineElements(node->rawLines));
            node->rawLines.clear();
        }
    }
}

//-----------------------------------------------------------------
// Main parse function
//-----------------------------------------------------------------
Owned<Node> parse(StringView src) {
    // First pass: Block elements
    Owned<Node> document = Parser{}.parse(src);

    // Second pass: Inline elements
    doInlines(document);

    return document;
}

} // namespace markdown
} // namespace ply
