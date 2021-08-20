/*
* Copyright 2013-2020, GrammarSoft ApS
* Developed by Tino Didriksen <mail@tinodidriksen.com> for GrammarSoft ApS (https://grammarsoft.com/)
* Development funded by Tony Berber Sardinha (http://www2.lael.pucsp.br/~tony/), São Paulo Catholic University (http://pucsp.br/), CEPRIL (http://www2.lael.pucsp.br/corpora/), CNPq (http://cnpq.br/), FAPESP (http://fapesp.br/)
*
* This file is part of CG-3 IDE
*
* CG-3 IDE is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* CG-3 IDE is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with CG-3 IDE.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "GrammarHighlighter.hpp"
#include "inlines.hpp"

GrammarHighlighter::GrammarHighlighter(QTextDocument *parent) :
    QSyntaxHighlighter(parent),
    fmts(NUM_FORMATS),
    fmt_desc(NUM_FORMATS)
{
    fmt_desc[F_ERROR]     << "error"     << "Parse Errors"      << "LIZT Nauns = errors abound ;"     << "#ff0000" << "2" << "1";
    fmt_desc[F_COMMENT]   << "comment"   << "Comments"          << "# Comments with cats and dogs"    << "#404040" << "1" << "1";
    fmt_desc[F_DIRECTIVE] << "directive" << "Directives"        << "SELECT, REMOVE, SECTION, etc"     << "#0000ff" << "1" << "1";
    fmt_desc[F_TAG]       << "tag"       << "Tag"               << "\"<fluffy>\" \"bunny\" <waffle> @whogoesthere" << "#008000" << "1" << "1";
    fmt_desc[F_SETNAME]   << "setname"   << "Set Names"         << "DescriptiveNameForX"              << "#800080" << "1" << "1";
    fmt_desc[F_SETOP]     << "setop"     << "Set Operators"     << QString("+ - OR | ^ ").append(QChar(0x2206)).append(' ').append(QChar(0x2229)) << "#ff00ff" << "1" << "1";
    fmt_desc[F_TMPLNAME]  << "tmplname"  << "Template Names"    << "NameForT T:UsedHere"              << "#808000" << "1" << "1";
    fmt_desc[F_ANCHOR]    << "anchor"    << "Anchors"           << ":names :for :rules"               << "#000080" << "1" << "1";
    fmt_desc[F_CNTXMOD]   << "cntxmod"   << "Context Modifiers" << "NEGATE, NONE, NOT, ALL, etc"      << "#000000" << "1" << "1";
    fmt_desc[F_CNTXPOS]   << "cntxpos"   << "Context Position"  << "-1**W cclll r:somewhere"          << "#646400" << "1" << "1";
    fmt_desc[F_CNTXOP]    << "cntxop"    << "Context Operators" << "LINK, OR, BARRIER, CBARRIER, etc" << "#000000" << "1" << "1";
    fmt_desc[F_RULE_FLAG] << "ruleflag"  << "Rule Flags"        << "NEAREST, DELAYED, UNSAFE, etc"    << "#000080" << "1" << "1";
    fmt_desc[F_OPTIONAL]  << "optional"  << "Optional Keywords" << "SETS, TARGET, IF, END, etc"       << "#808080" << "1" << "1";
}

void GrammarHighlighter::clear() {
    set_lines.clear();
    tmpl_lines.clear();
    section_lines.clear();
    rehighlight();
}

bool GrammarHighlighter::SKIPWS(const QChar *& p, const QChar a, const QChar b) {
    while (*p != nullptr && *p != a && *p != b) {
        if (*p == '#' && !ISESC(p)) {
            state->stack << S_COMMENT;
            return false;
        }
        if (!ISSPACE(*p)) {
            break;
        }
        ++p;
    }
    return true;
}

bool GrammarHighlighter::SKIPTOWS(const QChar *& p, const QChar a, const bool allowhash) {
    while (*p != nullptr && !ISSPACE(*p)) {
        if (!allowhash && *p == '#' && !ISESC(p)) {
            state->stack << S_COMMENT;
            return false;
        }
        if (*p == ';' && !ISESC(p)) {
            break;
        }
        if (*p == a && !ISESC(p)) {
            break;
        }
        ++p;
    }
    return true;
}

void GrammarHighlighter::highlightBlock(const QString& text) {
    auto s = static_cast<GrammarState*>(currentBlock().previous().userData());
    if (s) {
        state = new GrammarState(*s);
        state->tokens.clear();
    }
    else {
        state = new GrammarState;
    }
    if (state->stack.empty()) {
        state->stack << S_NONE;
    }
    state->error.clear();
    setCurrentBlockUserData(state);

    auto p = text.constData();
    SKIPWS(p);

    if (!currentBlock().next().isValid() && state->stack.back() != S_NONE) {
        state->stack << S_ERROR;
        static QString ps(" ");
        p = ps.constData();
    }

    int oz = state->stack.size();
    size_t os = state->stack.back();
    const QChar *op = nullptr;
    for (size_t loops = 0 ; *p != nullptr ; op = p, oz = state->stack.size(), os = state->stack.isEmpty() ? S_ERROR : state->stack.back()) {
        if (op == p && oz == state->stack.size() && os == (state->stack.isEmpty() ? static_cast<size_t>(S_ERROR) : state->stack.back())) {
            ++loops;
        }
        else {
            loops = 0;
        }
        // 1000 was too low for the Greenlandic grammar
        if (loops >= 10000) {
            // We've been stuck trying to parse the same spot for 10000 iterations - time to give up...
            state->stack << S_ERROR;
        }

        if (state->stack.empty()) {
            state->stack << S_NONE;
        }
        size_t cs = state->stack.back();
        if (cs & S_ERROR) {
            const int index = p-text.constData(), length = text.length() - (p-text.constData());
            setFormat(index, length, fmts[F_ERROR]);
            p = text.constData() + text.length();
            state->error = tr("Parse error! Expected parse stack: ");
            state->stack.pop_back();
            while (!state->stack.isEmpty()) {
                state->error.append(QString("%1 ").arg(StateText(state->stack.back())));
                state->stack.pop_back();
            }
            continue;
        }
        if (cs & S_COMMENT) {
            const int index = p-text.constData(), length = text.length() - (p-text.constData());
            setFormat(index, length, fmts[F_COMMENT]);
            p = text.constData() + text.length();
            state->stack.pop_back();
            continue;
        }
        if (!SKIPWS(p)) {
            continue;
        }
        if (*p == nullptr) {
            break;
        }
        if (cs & S_SETNAME) {
            auto n = p;
            if (!SKIPTOWS(n, ')', true)) {
                continue;
            }
            while (n[-1] == ',' || n[-1] == ']') {
                --n;
            }
            if (p == n) {
                state->stack << S_ERROR;
                continue;
            }
            const int index = p-text.constData();
            setFormat(index, n - p, fmts[F_SETNAME]);
            state->tokens[index] = S_SETNAME;
            state->tokens[n - text.constData()] = S_NONE;
            QString name(p, n - p);
            if (!set_lines.contains(name)) {
                set_lines.insert(name, currentBlock().firstLineNumber());
            }
            set_lines[name] = std::min(set_lines[name], currentBlock().blockNumber());
            p = n;
            state->stack.pop_back();
            continue;
        }
        if (cs & S_TMPLNAME) {
            auto n = p;
            if (!SKIPTOWS(n, ')', true)) {
                continue;
            }
            while (n[-1] == ',' || n[-1] == ']') {
                --n;
            }
            if (p == n) {
                state->stack << S_ERROR;
                continue;
            }
            const int index = p-text.constData();
            setFormat(index, n - p, fmts[F_TMPLNAME]);
            state->tokens[index] = S_TMPLNAME;
            state->tokens[n - text.constData()] = S_NONE;
            QString name(p, n - p);
            if (!tmpl_lines.contains(name)) {
                tmpl_lines.insert(name, currentBlock().firstLineNumber());
            }
            tmpl_lines[name] = std::min(tmpl_lines[name], currentBlock().blockNumber());
            p = n;
            state->stack.pop_back();
            continue;
        }
        // Qt bug https://bugreports.qt.io/browse/QTBUG-27451 is to blame for this hack
        if (cs & S_EQUALS) {
            if (*p == '+') {
                ++p;
            }
            if (*p != '=') {
                state->stack << S_ERROR;
                continue;
            }
            ++p;
            state->stack.pop_back();
            continue;
        }
        if ((cs & S_TAG) && (cs & S_COMPOSITETAG)) {
            if (*p == '(') {
                ++p;
                state->stack.back() = S_COMPOSITETAG;
                parseCompositeTag(text, p);
            }
            else {
                state->stack.back() = S_TAG;
                parseTag(text, p);
            }
            continue;
        }
        if (cs & S_COMPOSITETAG) {
            if (*p == '(') {
                ++p;
                state->stack.back() = S_COMPOSITETAG;
                parseCompositeTag(text, p);
            }
            else {
                state->stack << S_ERROR;
            }
            continue;
        }
        if (cs & S_TAG) {
            if ((*p == '(' || *p == ')' || *p == ';') && !ISESC(p)) {
                state->stack << S_ERROR;
            }
            else {
                state->stack.back() = S_TAG;
                parseTag(text, p);
            }
            continue;
        }
        if (cs & S_TAGLIST_INLINE) {
            parseTagList(text, p);
            if (*p == ')' && !ISESC(p)) {
                state->stack.pop_back();
                ++p;
            }
            else if (*p == ';' && !ISESC(p)) {
                state->stack << S_ERROR;
            }
            continue;
        }
        if ((cs & S_TAGLIST) && *p != ';') {
            parseTagList(text, p);
            continue;
        }
        if (cs & S_IF) {
            state->stack.pop_back();
            if (ISCHR(p[0], 'I', 'i') && ISCHR(p[1], 'F', 'f') && !p[2].isLetterOrNumber()) {
                const int index = p-text.constData();
                setFormat(index, 2, fmts[F_OPTIONAL]);
                p += 2;
            }
            continue;
        }
        if (cs & S_EXCEPT) {
            state->stack.pop_back();
            if (ISCHR(p[0], 'E', 'e') && ISCHR(p[1], 'X', 'x') && ISCHR(p[2], 'C', 'c') && ISCHR(p[3], 'E', 'e')
                    && ISCHR(p[4], 'P', 'p') && ISCHR(p[5], 'T', 't') && !p[6].isLetterOrNumber()) {
                const int index = p-text.constData();
                setFormat(index, 6, fmts[F_DIRECTIVE]);
                p += 6;
                state->stack << S_SET_INLINE;
            }
            continue;
        }
        if (cs & S_TO_FROM) {
            state->stack.pop_back();
            if (ISCHR(p[0], 'T', 't') && ISCHR(p[1], 'O', 'o') && !p[2].isLetterOrNumber()) {
                const int index = p-text.constData();
                setFormat(index, 2, fmts[F_DIRECTIVE]);
                p += 2;
            }
            else if (ISCHR(p[0], 'F', 'f') && ISCHR(p[1], 'R', 'r') && ISCHR(p[2], 'O', 'o') && ISCHR(p[3], 'M', 'm')
                     && !p[4].isLetterOrNumber()) {
                const int index = p-text.constData();
                setFormat(index, 4, fmts[F_DIRECTIVE]);
                p += 4;
            }
            else {
                state->stack << S_ERROR;
            }
            continue;
        }
        if (cs & S_ONCE_ALWAYS) {
            state->stack.pop_back();
            if (ISCHR(p[0], 'O', 'o') && ISCHR(p[1], 'N', 'n') && ISCHR(p[2], 'C', 'c') && ISCHR(p[3], 'E', 'e')
                    && !p[4].isLetterOrNumber()) {
                const int index = p-text.constData();
                setFormat(index, 4, fmts[F_DIRECTIVE]);
                p += 4;
            }
            else if (ISCHR(p[0], 'A', 'a') && ISCHR(p[1], 'L', 'l') && ISCHR(p[2], 'W', 'w') && ISCHR(p[3], 'A', 'a')
                     && ISCHR(p[4], 'Y', 'y') && ISCHR(p[5], 'S', 's') && !p[6].isLetterOrNumber()) {
                const int index = p-text.constData();
                setFormat(index, 6, fmts[F_DIRECTIVE]);
                p += 6;
            }
            else {
                state->stack << S_ERROR;
            }
            continue;
        }
        if (cs & S_BEFORE_AFTER) {
            state->stack.pop_back();
            if (ISCHR(p[0], 'B', 'b') && ISCHR(p[1], 'E', 'e') && ISCHR(p[2], 'F', 'f') && ISCHR(p[3], 'O', 'o')
                    && ISCHR(p[4], 'R', 'r') && ISCHR(p[5], 'E', 'e') && !p[6].isLetterOrNumber()) {
                const int index = p-text.constData();
                setFormat(index, 6, fmts[F_DIRECTIVE]);
                p += 6;
            }
            else if (ISCHR(p[0], 'A', 'a') && ISCHR(p[1], 'F', 'f') && ISCHR(p[2], 'T', 't') && ISCHR(p[3], 'E', 'e')
                     && ISCHR(p[4], 'R', 'r') && !p[5].isLetterOrNumber()) {
                const int index = p-text.constData();
                setFormat(index, 5, fmts[F_DIRECTIVE]);
                p += 5;
            }
            else {
                state->stack << S_ERROR;
            }
            continue;
        }
        if (cs & S_WITH) {
            state->stack.pop_back();
            if (ISCHR(p[0], 'W', 'w') && ISCHR(p[1], 'I', 'i') && ISCHR(p[2], 'T', 't') && ISCHR(p[3], 'H', 'h')
                    && !p[4].isLetterOrNumber()) {
                const int index = p-text.constData();
                setFormat(index, 4, fmts[F_DIRECTIVE]);
                p += 4;
            }
            else {
                state->stack << S_ERROR;
            }
            continue;
        }
        if (cs & S_RULE_FLAG) {
            state->stack.pop_back();
            if (ISCHR(p[0], 'N', 'n') && ISCHR(p[1], 'E', 'e') && ISCHR(p[2], 'A', 'a') && ISCHR(p[3], 'R', 'r')
                    && ISCHR(p[4], 'E', 'e') && ISCHR(p[5], 'S', 's') && ISCHR(p[6], 'T', 't') && !p[7].isLetterOrNumber()) {
                const int index = p-text.constData();
                setFormat(index, 7, fmts[F_RULE_FLAG]);
                p += 7;
                state->stack << S_RULE_FLAG;
            }
            else if (ISCHR(p[0], 'A', 'a') && ISCHR(p[1], 'L', 'l') && ISCHR(p[2], 'L', 'l') && ISCHR(p[3], 'O', 'o')
                     && ISCHR(p[4], 'W', 'w') && ISCHR(p[5], 'L', 'l') && ISCHR(p[6], 'O', 'o') && ISCHR(p[7], 'O', 'o')
                     && ISCHR(p[8], 'P', 'p') && !p[9].isLetterOrNumber()) {
                const int index = p-text.constData();
                setFormat(index, 9, fmts[F_RULE_FLAG]);
                p += 9;
                state->stack << S_RULE_FLAG;
            }
            else if (ISCHR(p[0], 'D', 'd') && ISCHR(p[1], 'E', 'e') && ISCHR(p[2], 'L', 'l') && ISCHR(p[3], 'A', 'a')
                     && ISCHR(p[4], 'Y', 'y') && ISCHR(p[5], 'E', 'e') && ISCHR(p[6], 'D', 'd') && !p[7].isLetterOrNumber()) {
                const int index = p-text.constData();
                setFormat(index, 7, fmts[F_RULE_FLAG]);
                p += 7;
                state->stack << S_RULE_FLAG;
            }
            else if (ISCHR(p[0], 'I', 'i') && ISCHR(p[1], 'M', 'm') && ISCHR(p[2], 'M', 'm') && ISCHR(p[3], 'E', 'e')
                     && ISCHR(p[4], 'D', 'd') && ISCHR(p[5], 'I', 'i') && ISCHR(p[6], 'A', 'a') && ISCHR(p[7], 'T', 't')
                     && ISCHR(p[8], 'E', 'e') && !p[9].isLetterOrNumber()) {
                const int index = p-text.constData();
                setFormat(index, 9, fmts[F_RULE_FLAG]);
                p += 9;
                state->stack << S_RULE_FLAG;
            }
            else if (ISCHR(p[0], 'L', 'l') && ISCHR(p[1], 'O', 'o') && ISCHR(p[2], 'O', 'o') && ISCHR(p[3], 'K', 'k')
                     && ISCHR(p[4], 'D', 'd') && ISCHR(p[5], 'E', 'e') && ISCHR(p[6], 'L', 'l') && ISCHR(p[7], 'E', 'e')
                     && ISCHR(p[8], 'T', 't') && ISCHR(p[9], 'E', 'e') && ISCHR(p[10], 'D', 'd') && !p[11].isLetterOrNumber()) {
                const int index = p-text.constData();
                setFormat(index, 11, fmts[F_RULE_FLAG]);
                p += 11;
                state->stack << S_RULE_FLAG;
            }
            else if (ISCHR(p[0], 'L', 'l') && ISCHR(p[1], 'O', 'o') && ISCHR(p[2], 'O', 'o') && ISCHR(p[3], 'K', 'k')
                     && ISCHR(p[4], 'D', 'd') && ISCHR(p[5], 'E', 'e') && ISCHR(p[6], 'L', 'l') && ISCHR(p[7], 'A', 'a')
                     && ISCHR(p[8], 'Y', 'y') && ISCHR(p[9], 'E', 'e') && ISCHR(p[10], 'D', 'd') && !p[11].isLetterOrNumber()) {
                const int index = p-text.constData();
                setFormat(index, 11, fmts[F_RULE_FLAG]);
                p += 11;
                state->stack << S_RULE_FLAG;
            }
            else if (ISCHR(p[0], 'U', 'u') && ISCHR(p[1], 'N', 'n') && ISCHR(p[2], 'S', 's') && ISCHR(p[3], 'A', 'a')
                     && ISCHR(p[4], 'F', 'f') && ISCHR(p[5], 'E', 'e') && !p[6].isLetterOrNumber()) {
                const int index = p-text.constData();
                setFormat(index, 6, fmts[F_RULE_FLAG]);
                p += 6;
                state->stack << S_RULE_FLAG;
            }
            else if (ISCHR(p[0], 'S', 's') && ISCHR(p[1], 'A', 'a') && ISCHR(p[2], 'F', 'f') && ISCHR(p[3], 'E', 'e')
                     && !p[4].isLetterOrNumber()) {
                const int index = p-text.constData();
                setFormat(index, 4, fmts[F_RULE_FLAG]);
                p += 4;
                state->stack << S_RULE_FLAG;
            }
            else if (ISCHR(p[0], 'R', 'r') && ISCHR(p[1], 'E', 'e') && ISCHR(p[2], 'M', 'm') && ISCHR(p[3], 'E', 'e')
                     && ISCHR(p[4], 'M', 'm') && ISCHR(p[5], 'B', 'b') && ISCHR(p[6], 'E', 'e') && ISCHR(p[7], 'R', 'r')
                     && ISCHR(p[8], 'X', 'x') && !p[9].isLetterOrNumber()) {
                const int index = p-text.constData();
                setFormat(index, 9, fmts[F_RULE_FLAG]);
                p += 9;
                state->stack << S_RULE_FLAG;
            }
            else if (ISCHR(p[0], 'R', 'r') && ISCHR(p[1], 'E', 'e') && ISCHR(p[2], 'S', 's') && ISCHR(p[3], 'E', 'e')
                     && ISCHR(p[4], 'T', 't') && ISCHR(p[5], 'X', 'x') && !p[6].isLetterOrNumber()) {
                const int index = p-text.constData();
                setFormat(index, 6, fmts[F_RULE_FLAG]);
                p += 6;
                state->stack << S_RULE_FLAG;
            }
            else if (ISCHR(p[0], 'K', 'k') && ISCHR(p[1], 'E', 'e') && ISCHR(p[2], 'E', 'e') && ISCHR(p[3], 'P', 'p')
                     && ISCHR(p[4], 'O', 'o') && ISCHR(p[5], 'R', 'r') && ISCHR(p[6], 'D', 'd') && ISCHR(p[7], 'E', 'e')
                     && ISCHR(p[8], 'R', 'r') && !p[9].isLetterOrNumber()) {
                const int index = p-text.constData();
                setFormat(index, 9, fmts[F_RULE_FLAG]);
                p += 9;
                state->stack << S_RULE_FLAG;
            }
            else if (ISCHR(p[0], 'V', 'v') && ISCHR(p[1], 'A', 'a') && ISCHR(p[2], 'R', 'r') && ISCHR(p[3], 'Y', 'y')
                     && ISCHR(p[4], 'O', 'o') && ISCHR(p[5], 'R', 'r') && ISCHR(p[6], 'D', 'd') && ISCHR(p[7], 'E', 'e')
                     && ISCHR(p[8], 'R', 'r') && !p[9].isLetterOrNumber()) {
                const int index = p-text.constData();
                setFormat(index, 9, fmts[F_RULE_FLAG]);
                p += 9;
                state->stack << S_RULE_FLAG;
            }
            else if (ISCHR(p[0], 'E', 'e') && ISCHR(p[1], 'N', 'n') && ISCHR(p[2], 'C', 'c') && ISCHR(p[3], 'L', 'l')
                     && ISCHR(p[4], '_', '_') && ISCHR(p[5], 'I', 'i') && ISCHR(p[6], 'N', 'n') && ISCHR(p[7], 'N', 'n')
                     && ISCHR(p[8], 'E', 'e') && ISCHR(p[9], 'R', 'r') && !p[10].isLetterOrNumber()) {
                const int index = p-text.constData();
                setFormat(index, 10, fmts[F_RULE_FLAG]);
                p += 10;
                state->stack << S_RULE_FLAG;
            }
            else if (ISCHR(p[0], 'E', 'e') && ISCHR(p[1], 'N', 'n') && ISCHR(p[2], 'C', 'c') && ISCHR(p[3], 'L', 'l')
                     && ISCHR(p[4], '_', '_') && ISCHR(p[5], 'O', 'o') && ISCHR(p[6], 'U', 'u') && ISCHR(p[7], 'T', 't')
                     && ISCHR(p[8], 'E', 'e') && ISCHR(p[9], 'R', 'r') && !p[10].isLetterOrNumber()) {
                const int index = p-text.constData();
                setFormat(index, 10, fmts[F_RULE_FLAG]);
                p += 10;
                state->stack << S_RULE_FLAG;
            }
            else if (ISCHR(p[0], 'E', 'e') && ISCHR(p[1], 'N', 'n') && ISCHR(p[2], 'C', 'c') && ISCHR(p[3], 'L', 'l')
                     && ISCHR(p[4], '_', '_') && ISCHR(p[5], 'F', 'f') && ISCHR(p[6], 'I', 'i') && ISCHR(p[7], 'N', 'n')
                     && ISCHR(p[8], 'A', 'a') && ISCHR(p[9], 'L', 'l') && !p[10].isLetterOrNumber()) {
                const int index = p-text.constData();
                setFormat(index, 10, fmts[F_RULE_FLAG]);
                p += 10;
                state->stack << S_RULE_FLAG;
            }
            else if (ISCHR(p[0], 'E', 'e') && ISCHR(p[1], 'N', 'n') && ISCHR(p[2], 'C', 'c') && ISCHR(p[3], 'L', 'l')
                     && ISCHR(p[4], '_', '_') && ISCHR(p[5], 'A', 'a') && ISCHR(p[6], 'N', 'n') && ISCHR(p[7], 'Y', 'y')
                     && !p[8].isLetterOrNumber()) {
                const int index = p-text.constData();
                setFormat(index, 8, fmts[F_RULE_FLAG]);
                p += 8;
                state->stack << S_RULE_FLAG;
            }
            else if (ISCHR(p[0], 'A', 'a') && ISCHR(p[1], 'L', 'l') && ISCHR(p[2], 'L', 'l') && ISCHR(p[3], 'O', 'o')
                     && ISCHR(p[4], 'W', 'w') && ISCHR(p[5], 'C', 'c') && ISCHR(p[6], 'R', 'r') && ISCHR(p[7], 'O', 'o')
                     && ISCHR(p[8], 'S', 's') && ISCHR(p[9], 'S', 's') && !p[10].isLetterOrNumber()) {
                const int index = p-text.constData();
                setFormat(index, 10, fmts[F_RULE_FLAG]);
                p += 10;
                state->stack << S_RULE_FLAG;
            }
            /* WithChild and NoChild are handled separately
            else if (ISCHR(p[0], 'W', 'w') && ISCHR(p[1], 'I', 'i') && ISCHR(p[2], 'T', 't') && ISCHR(p[3], 'H', 'h')
                     && ISCHR(p[4], 'C', 'c') && ISCHR(p[5], 'H', 'h') && ISCHR(p[6], 'I', 'i') && ISCHR(p[7], 'L', 'l')
                     && ISCHR(p[8], 'D', 'd') && !p[9].isLetterOrNumber()) {
                const int index = p-text.constData();
                setFormat(index, 9, fmts[F_RULE_FLAG]);
                p += 9;
                state->stack << S_RULE_FLAG;
            }
            else if (ISCHR(p[0], 'N', 'n') && ISCHR(p[1], 'O', 'o') && ISCHR(p[2], 'C', 'c') && ISCHR(p[3], 'H', 'h')
                     && ISCHR(p[4], 'I', 'i') && ISCHR(p[5], 'L', 'l') && ISCHR(p[6], 'D', 'd') && !p[7].isLetterOrNumber()) {
                const int index = p-text.constData();
                setFormat(index, 7, fmts[F_RULE_FLAG]);
                p += 7;
                state->stack << S_RULE_FLAG;
            }
            //*/
            else if (ISCHR(p[0], 'I', 'i') && ISCHR(p[1], 'T', 't') && ISCHR(p[2], 'E', 'e') && ISCHR(p[3], 'R', 'r')
                     && ISCHR(p[4], 'A', 'a') && ISCHR(p[5], 'T', 't') && ISCHR(p[6], 'E', 'e') && !p[7].isLetterOrNumber()) {
                const int index = p-text.constData();
                setFormat(index, 7, fmts[F_RULE_FLAG]);
                p += 7;
                state->stack << S_RULE_FLAG;
            }
            else if (ISCHR(p[0], 'N', 'n') && ISCHR(p[1], 'O', 'o') && ISCHR(p[2], 'I', 'i') && ISCHR(p[3], 'T', 't')
                     && ISCHR(p[4], 'E', 'e') && ISCHR(p[5], 'R', 'r') && ISCHR(p[6], 'A', 'a') && ISCHR(p[7], 'T', 't')
                     && ISCHR(p[8], 'E', 'e') && !p[9].isLetterOrNumber()) {
                const int index = p-text.constData();
                setFormat(index, 9, fmts[F_RULE_FLAG]);
                p += 9;
                state->stack << S_RULE_FLAG;
            }
            else if (ISCHR(p[0], 'U', 'u') && ISCHR(p[1], 'N', 'n') && ISCHR(p[2], 'M', 'm') && ISCHR(p[3], 'A', 'a')
                     && ISCHR(p[4], 'P', 'p') && ISCHR(p[5], 'L', 'l') && ISCHR(p[6], 'A', 'a') && ISCHR(p[7], 'S', 's')
                     && ISCHR(p[8], 'T', 't') && !p[9].isLetterOrNumber()) {
                const int index = p-text.constData();
                setFormat(index, 9, fmts[F_RULE_FLAG]);
                p += 9;
                state->stack << S_RULE_FLAG;
            }
            else if (ISCHR(p[0], 'R', 'r') && ISCHR(p[1], 'E', 'e') && ISCHR(p[2], 'V', 'v') && ISCHR(p[3], 'E', 'e')
                     && ISCHR(p[4], 'R', 'r') && ISCHR(p[5], 'S', 's') && ISCHR(p[6], 'E', 'e') && !p[7].isLetterOrNumber()) {
                const int index = p-text.constData();
                setFormat(index, 7, fmts[F_RULE_FLAG]);
                p += 7;
                state->stack << S_RULE_FLAG;
            }
            else if (ISCHR(p[0], 'S', 's') && ISCHR(p[1], 'U', 'u') && ISCHR(p[2], 'B', 'b') && p[3] == ':' && (p[4].isDigit() || (p[4] == '-' && p[5].isDigit()))) {
                auto n = p+4;
                if (!SKIPTOWS(n, '(')) {
                    continue;
                }
                const int index = p-text.constData();
                setFormat(index, n - p, fmts[F_RULE_FLAG]);
                p = n;
                state->stack << S_RULE_FLAG;
            }
            continue;
        }
        if (cs & S_WITHCHILD) {
            state->stack.pop_back();
            if (ISCHR(p[0], 'W', 'w') && ISCHR(p[1], 'I', 'i') && ISCHR(p[2], 'T', 't') && ISCHR(p[3], 'H', 'h')
                    && ISCHR(p[4], 'C', 'c') && ISCHR(p[5], 'H', 'h') && ISCHR(p[6], 'I', 'i') && ISCHR(p[7], 'L', 'l')
                    && ISCHR(p[8], 'D', 'd') && !p[9].isLetterOrNumber()) {
                const int index = p-text.constData();
                setFormat(index, 9, fmts[F_RULE_FLAG]);
                p += 9;
                state->stack << S_SET_INLINE;
            }
            else if (ISCHR(p[0], 'N', 'n') && ISCHR(p[1], 'O', 'o') && ISCHR(p[2], 'C', 'c') && ISCHR(p[3], 'H', 'h')
                     && ISCHR(p[4], 'I', 'i') && ISCHR(p[5], 'L', 'l') && ISCHR(p[6], 'D', 'd') && !p[7].isLetterOrNumber()) {
                const int index = p-text.constData();
                setFormat(index, 7, fmts[F_RULE_FLAG]);
                p += 7;
            }
            continue;
        }
        if (cs & S_TARGET) {
            state->stack.pop_back();
            if (ISCHR(p[0], 'T', 't') && ISCHR(p[1], 'A', 'a') && ISCHR(p[2], 'R', 'r') && ISCHR(p[3], 'G', 'g')
                    && ISCHR(p[4], 'E', 'e') && ISCHR(p[5], 'T', 't') && !p[6].isLetterOrNumber()) {
                const int index = p-text.constData();
                setFormat(index, 6, fmts[F_OPTIONAL]);
                p += 6;
            }
            continue;
        }
        if (cs & S_LINK) {
            state->stack.pop_back();
            if (ISCHR(p[0], 'L', 'l') && ISCHR(p[1], 'I', 'i') && ISCHR(p[2], 'N', 'n') && ISCHR(p[3], 'K', 'k')
                     && !p[4].isLetterOrNumber()) {
                state->stack << S_CONTEXT;
                const int index = p-text.constData();
                setFormat(index, 4, fmts[F_CNTXOP]);
                p += 4;
            }
            continue;
        }
        if (cs & S_CONTEXT_OP) {
            state->stack.pop_back();
            if (ISCHR(p[0], 'O', 'o') && ISCHR(p[1], 'R', 'r') && !p[2].isLetterOrNumber()) {
                state->stack << S_CONTEXT_OP << S_PAR_STOP << S_CONTEXT << S_PAR_START;
                const int index = p-text.constData();
                setFormat(index, 2, fmts[F_CNTXOP]);
                p += 2;
            }
            continue;
        }
        if (cs & S_BARRIER) {
            state->stack.pop_back();
            if (ISCHR(p[0], 'B', 'b') && ISCHR(p[1], 'A', 'a') && ISCHR(p[2], 'R', 'r') && ISCHR(p[3], 'R', 'r')
                    && ISCHR(p[4], 'I', 'i') && ISCHR(p[5], 'E', 'e') && ISCHR(p[6], 'R', 'r') && !p[7].isLetterOrNumber()) {
                state->stack << S_SET_INLINE;
                const int index = p-text.constData();
                setFormat(index, 7, fmts[F_CNTXOP]);
                p += 7;
            }
            else if (ISCHR(p[0], 'C', 'c') && ISCHR(p[1], 'B', 'b') && ISCHR(p[2], 'A', 'a') && ISCHR(p[3], 'R', 'r')
                     && ISCHR(p[4], 'R', 'r') && ISCHR(p[5], 'I', 'i') && ISCHR(p[6], 'E', 'e') && ISCHR(p[7], 'R', 'r')
                     && !p[8].isLetterOrNumber()) {
                state->stack << S_SET_INLINE;
                const int index = p-text.constData();
                setFormat(index, 8, fmts[F_CNTXOP]);
                p += 8;
            }
            continue;
        }
        if (cs & S_SETOP) {
            state->stack.pop_back();
            if (*p == ',') {
                state->stack << S_SET_INLINE;
                ++p;
            }
            else if (ux_isSetOp(p)) {
                auto n = p;
                if (!SKIPTOWS(n, '(')) {
                    continue;
                }
                const int index = p-text.constData();
                setFormat(index, n - p, fmts[F_SETOP]);
                state->stack << S_SET_INLINE;
                p = n;
            }
            continue;
        }
        if (cs & S_SET_INLINE) {
            state->stack.pop_back();
            if (p[0] == 'T' && p[1] == ':') {
                state->stack << S_CONTEXT_TMPL;
            }
            else if (*p == '(') {
                state->stack << S_SETOP << S_TAGLIST_INLINE << S_TAG;
                ++p;
            }
            else {
                state->stack << S_SETOP << S_SETNAME;
            }
            continue;
        }
        if (cs & S_CONTEXT_TMPL) {
            state->stack.pop_back();
            auto n = p;
            auto m = p;
            if (!SKIPTOWS(n, '(')) {
                continue;
            }
            if (!SKIPTOWS(m, ')')) {
                continue;
            }
            n = std::min(n,m);
            const int index = p-text.constData();
            setFormat(index, n - p, fmts[F_TMPLNAME]);
            state->tokens[index] = S_TMPLNAME;
            state->tokens[n - text.constData()] = S_NONE;
            p = n;
            continue;
        }
        if (cs & S_CONTEXT_POS) {
            state->stack.pop_back();
            if (p[0] == 'T' && p[1] == ':') {
                state->stack.pop_back();
                state->stack << S_CONTEXT_TMPL;
            }
            else {
                auto n = p;
                if (!SKIPTOWS(n, '(')) {
                    continue;
                }
                const int index = p-text.constData();
                setFormat(index, n - p, fmts[F_CNTXPOS]);
                p = n;
            }
            continue;
        }
        if (cs & S_CONTEXT) {
            state->stack.pop_back();
            bool found = false;
            do {
                found = false;
                SKIPWS(p);
                if (ISCHR(p[0], 'N', 'n') && ISCHR(p[1], 'E', 'e') && ISCHR(p[2], 'G', 'g') && ISCHR(p[3], 'A', 'a')
                        && ISCHR(p[4], 'T', 't') && ISCHR(p[5], 'E', 'e') && !p[6].isLetterOrNumber()) {
                    const int index = p-text.constData();
                    setFormat(index, 6, fmts[F_CNTXMOD]);
                    p += 6;
                    found = true;
                }
                SKIPWS(p);
                if (ISCHR(p[0], 'N', 'n') && ISCHR(p[1], 'O', 'o') && ISCHR(p[2], 'N', 'n') && ISCHR(p[3], 'E', 'e')
                        && !p[4].isLetterOrNumber()) {
                    const int index = p-text.constData();
                    setFormat(index, 4, fmts[F_CNTXMOD]);
                    p += 4;
                    found = true;
                }
                SKIPWS(p);
                if (ISCHR(p[0], 'N', 'n') && ISCHR(p[1], 'O', 'o') && ISCHR(p[2], 'T', 't') && !p[3].isLetterOrNumber()) {
                    const int index = p-text.constData();
                    setFormat(index, 3, fmts[F_CNTXMOD]);
                    p += 3;
                    found = true;
                }
                SKIPWS(p);
                if (ISCHR(p[0], 'A', 'a') && ISCHR(p[1], 'L', 'l') && ISCHR(p[2], 'L', 'l') && !p[3].isLetterOrNumber()) {
                    const int index = p-text.constData();
                    setFormat(index, 3, fmts[F_CNTXMOD]);
                    p += 3;
                    found = true;
                }
            } while(found);

            if (*p == '[') {
                state->stack << S_LINK << S_SQBRACKET_STOP << S_SET_INLINE;
                ++p;
            }
            else if (*p == '(') {
                state->stack << S_LINK << S_CONTEXT_OP << S_PAR_STOP << S_CONTEXT;
                ++p;
            }
            else if (*p != ';') {
                state->stack << S_LINK << S_BARRIER << S_BARRIER << S_SET_INLINE << S_CONTEXT_POS;
            }
            continue;
        }
        if (cs & S_CONTEXT_LIST) {
            if (*p == '(') {
                state->stack << S_CONTEXT_OP << S_PAR_STOP << S_CONTEXT;
                ++p;
            }
            else {
                state->stack.pop_back();
            }
            continue;
        }
        if ((cs & S_PAR_START) && *p == '(') {
            ++p;
            state->stack.pop_back();
            continue;
        }
        if ((cs & S_PAR_STOP) && *p == ')') {
            ++p;
            state->stack.pop_back();
            continue;
        }
        if ((cs & S_SQBRACKET_STOP) && *p == ']') {
            ++p;
            state->stack.pop_back();
            continue;
        }
        if ((cs & S_SEMICOLON) && *p == ';') {
            ++p;
            state->stack.pop_back();
            continue;
        }
        if ((cs == S_NONE || cs == S_RULE) && parseNone(text, p)) {
            continue;
        }
        state->stack << S_ERROR;
    }
}

bool GrammarHighlighter::parseTag(const QString& text, const QChar *& p) {
    bool warn_space = false;
    auto n = p;
    if (*n == '"') {
        ++n;
        SKIPTO_NOSPAN(n, '"');
        if (*n != '"') {
            state->stack << S_ERROR;
            return false;
        }
        if (n[-1].isSpace() && !ISESC(&n[-1])) {
            warn_space = true;
        }
    }
    if (!SKIPTOWS(n, ')', true)) {
        return false;
    }
    const int index = p-text.constData(), length = n-p;
    setFormat(index, length, fmts[F_TAG]);
    p = n;
    state->tokens[index] = S_TAG;
    state->tokens[n - text.constData()] = S_NONE;
    state->stack.pop_back();

    if (warn_space) {
        QString tag = text.mid(index, length);
        if (tag.count('"') >= 3) {
            state->warnings.insert(qMakePair(index, length), tr("Sure you didn't mean %1 (missing quote)? Can silence with \\ before the space.").arg(tag.replace(QRegExp("\\s\""), "\" \"")));
        }
    }

    return true;
}

bool GrammarHighlighter::parseCompositeTag(const QString& text, const QChar *& p) {
    while (*p != nullptr && *p != ';' && *p != ')') {
        state->stack << S_TAG;
        if (!parseTag(text, p)) {
            return false;
        }
        if (!SKIPWS(p, ';', ')')) {
            return false;
        }
    }
    if (*p == ')') {
        ++p;
        state->stack.pop_back();
    }
    return true;
}

bool GrammarHighlighter::parseTagList(const QString& text, const QChar *& p) {
    while (*p != nullptr && *p != ';' && *p != ')') {
        if (!SKIPWS(p, ';', ')')) {
            return false;
        }
        if (*p != nullptr && *p != ';' && *p != ')') {
            if (*p == '(') {
                ++p;
                state->stack << S_COMPOSITETAG;
                if (!parseCompositeTag(text, p)) {
                    return false;
                }
            }
            else {
                state->stack << S_TAG;
                if (!parseTag(text, p)) {
                    return false;
                }
            }
        }
    }
    return true;
}

bool GrammarHighlighter::parseAnchorish(const QString& text, const QChar *& p) {
    auto n = p;
    if (!SKIPTOWS(n, 0, true)) {
        return false;
    }
    const int index = p-text.constData();
    setFormat(index, n-p, fmts[F_ANCHOR]);
    p = n;
    return true;
}

bool GrammarHighlighter::parseSectionDirective(const QString& text, const QChar *& p, int length) {
    section_lines.insert(currentBlock().blockNumber());

    const int index = p-text.constData();
    setFormat(index, length, fmts[F_DIRECTIVE]);
    p += length;
    const QChar *s = p;
    SKIPLN(s);
    ::SKIPWS(s);
    if (SKIPWS(p) && p != s) {
        if (!parseAnchorish(text, p)) {
            return false;
        }
        state->stack << S_SEMICOLON;
    }
    return true;
}

bool GrammarHighlighter::parseRuleDirective(const QString& text, const QChar *& p, int length) {
    if (state->stack.back() == S_RULE) {
        state->stack.pop_back();
    }
    const int index = p-text.constData();
    setFormat(index, length, fmts[F_DIRECTIVE]);
    p += length;
    if (*p == ':') {
        ++p;
        parseAnchorish(text, p);
    }
    return true;
}

bool GrammarHighlighter::parseNone(const QString& text, const QChar *& p) {
    while (*p != nullptr) {
        // DELIMITERS
        if (ISCHR(*p,'D','d') && ISCHR(*(p+9),'S','s') && ISCHR(*(p+1),'E','e') && ISCHR(*(p+2),'L','l')
            && ISCHR(*(p+3),'I','i') && ISCHR(*(p+4),'M','m') && ISCHR(*(p+5),'I','i') && ISCHR(*(p+6),'T','t')
            && ISCHR(*(p+7),'E','e') && ISCHR(*(p+8),'R','r')
            && !ISSTRING(p, 9)) {
            const int index = p-text.constData(), length = 10;
            setFormat(index, length, fmts[F_DIRECTIVE]);
            p += 10;
            state->stack << (S_SEMICOLON|S_TAGLIST) << (S_TAG|S_COMPOSITETAG) << S_EQUALS;
            return true;
        }
        // SOFT-DELIMITERS
        else if (ISCHR(*p,'S','s') && ISCHR(*(p+14),'S','s') && ISCHR(*(p+1),'O','o') && ISCHR(*(p+2),'F','f')
            && ISCHR(*(p+3),'T','t') && ISCHR(*(p+4),'-','_')
            && ISCHR(*(p+5),'D','d') && ISCHR(*(p+6),'E','e') && ISCHR(*(p+7),'L','l')
            && ISCHR(*(p+8),'I','i') && ISCHR(*(p+9),'M','m') && ISCHR(*(p+10),'I','i') && ISCHR(*(p+11),'T','t')
            && ISCHR(*(p+12),'E','e') && ISCHR(*(p+13),'R','r')
            && !ISSTRING(p, 14)) {
            const int index = p-text.constData(), length = 15;
            setFormat(index, length, fmts[F_DIRECTIVE]);
            p += 15;
            state->stack << (S_SEMICOLON|S_TAGLIST) << (S_TAG|S_COMPOSITETAG) << S_EQUALS;
            return true;
        }
        // MAPPING-PREFIX
        else if (ISCHR(*p,'M','m') && ISCHR(*(p+13),'X','x') && ISCHR(*(p+1),'A','a') && ISCHR(*(p+2),'P','p')
            && ISCHR(*(p+3),'P','p') && ISCHR(*(p+4),'I','i')
            && ISCHR(*(p+5),'N','n') && ISCHR(*(p+6),'G','g') && ISCHR(*(p+7),'-','_')
            && ISCHR(*(p+8),'P','p') && ISCHR(*(p+9),'R','r') && ISCHR(*(p+10),'E','e') && ISCHR(*(p+11),'F','f')
            && ISCHR(*(p+12),'I','i')
            && !ISSTRING(p, 13)) {
            const int index = p-text.constData(), length = 14;
            setFormat(index, length, fmts[F_DIRECTIVE]);
            p += 14;
            state->stack << S_SEMICOLON << S_TAG << S_EQUALS;
            return true;
        }
        // PREFERRED-TARGETS
        else if (ISCHR(*p,'P','p') && ISCHR(*(p+16),'S','s') && ISCHR(*(p+1),'R','r') && ISCHR(*(p+2),'E','e')
            && ISCHR(*(p+3),'F','f') && ISCHR(*(p+4),'E','e')
            && ISCHR(*(p+5),'R','r') && ISCHR(*(p+6),'R','r') && ISCHR(*(p+7),'E','e')
            && ISCHR(*(p+8),'D','d') && ISCHR(*(p+9),'-','_') && ISCHR(*(p+10),'T','t') && ISCHR(*(p+11),'A','a')
            && ISCHR(*(p+12),'R','r') && ISCHR(*(p+13),'G','g') && ISCHR(*(p+14),'E','e') && ISCHR(*(p+15),'T','t')
            && !ISSTRING(p, 16)) {
            const int index = p-text.constData(), length = 17;
            setFormat(index, length, fmts[F_DIRECTIVE]);
            p += 17;
            state->stack << (S_SEMICOLON|S_TAGLIST) << (S_TAG|S_COMPOSITETAG) << S_EQUALS;
            return true;
        }
        // STATIC-SETS
        else if (ISCHR(*p,'S','s') && ISCHR(*(p+10),'S','s') && ISCHR(*(p+1),'T','t') && ISCHR(*(p+2),'A','a')
            && ISCHR(*(p+3),'T','t') && ISCHR(*(p+4),'I','i') && ISCHR(*(p+5),'C','c') && ISCHR(*(p+6),'-','-')
            && ISCHR(*(p+7),'S','s') && ISCHR(*(p+8),'E','e') && ISCHR(*(p+9),'T','t')
            && !ISSTRING(p, 10)) {
            const int index = p-text.constData(), length = 11;
            setFormat(index, length, fmts[F_DIRECTIVE]);
            p += 11;
            state->stack << (S_SEMICOLON|S_TAGLIST) << S_TAG << S_EQUALS; // ToDo: Set name list, not tag list
            return true;
        }
        // REOPEN-MAPPINGS
        else if (ISCHR(*p, 'R', 'r') && ISCHR(*(p + 14), 'S', 's') && ISCHR(*(p + 1), 'E', 'e') && ISCHR(*(p + 2), 'O', 'o')
            && ISCHR(*(p + 3), 'P', 'p') && ISCHR(*(p + 4), 'E', 'e') && ISCHR(*(p + 5), 'N', 'n') && ISCHR(*(p + 6), '-', '_')
            && ISCHR(*(p + 7), 'M', 'm') && ISCHR(*(p + 8), 'A', 'a') && ISCHR(*(p + 9), 'P', 'p') && ISCHR(*(p + 10), 'P', 'p')
            && ISCHR(*(p + 11), 'I', 'i') && ISCHR(*(p + 12), 'N', 'n') && ISCHR(*(p + 13), 'G', 'g')
            && !ISSTRING(p, 14)) {
            const int index = p-text.constData(), length = 15;
            setFormat(index, length, fmts[F_DIRECTIVE]);
            p += 15;
            state->stack << (S_SEMICOLON|S_TAGLIST) << S_TAG << S_EQUALS;
            return true;
        }
        // OPTIONS
        else if (ISCHR(*p, 'O', 'o') && ISCHR(*(p + 6), 'S', 's') && ISCHR(*(p + 1), 'P', 'p') && ISCHR(*(p + 2), 'T', 't')
            && ISCHR(*(p + 3), 'I', 'i') && ISCHR(*(p + 4), 'O', 'o') && ISCHR(*(p + 5), 'N', 'n')
            && !ISSTRING(p, 6)) {
            const int index = p-text.constData(), length = 7;
            setFormat(index, length, fmts[F_DIRECTIVE]);
            p += 7;
            state->stack << (S_SEMICOLON|S_TAGLIST) << S_TAG << S_EQUALS;
            return true;
        }
        // STRICT-TAGS
        else if (ISCHR(*p, 'S', 's') && ISCHR(*(p + 10), 'S', 's') && ISCHR(*(p + 1), 'T', 't') && ISCHR(*(p + 2), 'R', 'r')
            && ISCHR(*(p + 3), 'I', 'i') && ISCHR(*(p + 4), 'C', 'c') && ISCHR(*(p + 5), 'T', 't')
            && ISCHR(*(p + 6), '-', '-') && ISCHR(*(p + 7), 'T', 't') && ISCHR(*(p + 8), 'A', 'a') && ISCHR(*(p + 9), 'G', 'g')
            && !ISSTRING(p, 10)) {
            const int index = p-text.constData(), length = 11;
            setFormat(index, length, fmts[F_DIRECTIVE]);
            p += 11;
            state->stack << (S_SEMICOLON|S_TAGLIST) << S_TAG << S_EQUALS;
            return true;
        }
        // ADDRELATIONS
        else if (ISCHR(*p,'A','a') && ISCHR(*(p+11),'S','s') && ISCHR(*(p+1),'D','d') && ISCHR(*(p+2),'D','d')
            && ISCHR(*(p+3),'R','r') && ISCHR(*(p+4),'E','e') && ISCHR(*(p+5),'L','l') && ISCHR(*(p+6),'A','a')
            && ISCHR(*(p+7),'T','t') && ISCHR(*(p+8),'I','i') && ISCHR(*(p+9),'O','o') && ISCHR(*(p+10),'N','n')
            && !ISSTRING(p, 11)) {
            parseRuleDirective(text, p, 12);
            state->stack << S_SEMICOLON << S_CONTEXT_LIST << S_TO_FROM << S_CONTEXT_LIST << S_IF << S_SET_INLINE << S_TARGET << S_SET_INLINE << S_SET_INLINE << S_RULE_FLAG;
            return true;
        }
        // SETRELATIONS
        else if (ISCHR(*p,'S','s') && ISCHR(*(p+11),'S','s') && ISCHR(*(p+1),'E','e') && ISCHR(*(p+2),'T','t')
            && ISCHR(*(p+3),'R','r') && ISCHR(*(p+4),'E','e') && ISCHR(*(p+5),'L','l') && ISCHR(*(p+6),'A','a')
            && ISCHR(*(p+7),'T','t') && ISCHR(*(p+8),'I','i') && ISCHR(*(p+9),'O','o') && ISCHR(*(p+10),'N','n')
            && !ISSTRING(p, 11)) {
            parseRuleDirective(text, p, 12);
            state->stack << S_SEMICOLON << S_CONTEXT_LIST << S_TO_FROM << S_CONTEXT_LIST << S_IF << S_SET_INLINE << S_TARGET << S_SET_INLINE << S_SET_INLINE << S_RULE_FLAG;
            return true;
        }
        // REMRELATIONS
        else if (ISCHR(*p,'R','r') && ISCHR(*(p+11),'S','s') && ISCHR(*(p+1),'E','e') && ISCHR(*(p+2),'M','m')
            && ISCHR(*(p+3),'R','r') && ISCHR(*(p+4),'E','e') && ISCHR(*(p+5),'L','l') && ISCHR(*(p+6),'A','a')
            && ISCHR(*(p+7),'T','t') && ISCHR(*(p+8),'I','i') && ISCHR(*(p+9),'O','o') && ISCHR(*(p+10),'N','n')
            && !ISSTRING(p, 11)) {
            parseRuleDirective(text, p, 12);
            state->stack << S_SEMICOLON << S_CONTEXT_LIST << S_TO_FROM << S_CONTEXT_LIST << S_IF << S_SET_INLINE << S_TARGET << S_SET_INLINE << S_SET_INLINE << S_RULE_FLAG;
            return true;
        }
        // ADDRELATION
        else if (ISCHR(*p,'A','a') && ISCHR(*(p+10),'N','n') && ISCHR(*(p+1),'D','d') && ISCHR(*(p+2),'D','d')
            && ISCHR(*(p+3),'R','r') && ISCHR(*(p+4),'E','e') && ISCHR(*(p+5),'L','l') && ISCHR(*(p+6),'A','a')
            && ISCHR(*(p+7),'T','t') && ISCHR(*(p+8),'I','i') && ISCHR(*(p+9),'O','o')
            && !ISSTRING(p, 10)) {
            parseRuleDirective(text, p, 11);
            state->stack << S_SEMICOLON << S_CONTEXT_LIST << S_TO_FROM << S_CONTEXT_LIST << S_IF << S_SET_INLINE << S_TARGET << S_SET_INLINE << S_RULE_FLAG;
            return true;
        }
        // SETRELATION
        else if (ISCHR(*p,'S','s') && ISCHR(*(p+10),'N','n') && ISCHR(*(p+1),'E','e') && ISCHR(*(p+2),'T','t')
            && ISCHR(*(p+3),'R','r') && ISCHR(*(p+4),'E','e') && ISCHR(*(p+5),'L','l') && ISCHR(*(p+6),'A','a')
            && ISCHR(*(p+7),'T','t') && ISCHR(*(p+8),'I','i') && ISCHR(*(p+9),'O','o')
            && !ISSTRING(p, 10)) {
            parseRuleDirective(text, p, 11);
            state->stack << S_SEMICOLON << S_CONTEXT_LIST << S_TO_FROM << S_CONTEXT_LIST << S_IF << S_SET_INLINE << S_TARGET << S_SET_INLINE << S_RULE_FLAG;
            return true;
        }
        // REMRELATION
        else if (ISCHR(*p,'R','r') && ISCHR(*(p+10),'N','n') && ISCHR(*(p+1),'E','e') && ISCHR(*(p+2),'M','m')
            && ISCHR(*(p+3),'R','r') && ISCHR(*(p+4),'E','e') && ISCHR(*(p+5),'L','l') && ISCHR(*(p+6),'A','a')
            && ISCHR(*(p+7),'T','t') && ISCHR(*(p+8),'I','i') && ISCHR(*(p+9),'O','o')
            && !ISSTRING(p, 10)) {
            parseRuleDirective(text, p, 11);
            state->stack << S_SEMICOLON << S_CONTEXT_LIST << S_TO_FROM << S_CONTEXT_LIST << S_IF << S_SET_INLINE << S_TARGET << S_SET_INLINE << S_RULE_FLAG;
            return true;
        }
        // SETVARIABLE
        else if (ISCHR(*p,'S','s') && ISCHR(*(p+10),'E','e') && ISCHR(*(p+1),'E','e') && ISCHR(*(p+2),'T','t')
            && ISCHR(*(p+3),'V','v') && ISCHR(*(p+4),'A','a') && ISCHR(*(p+5),'R','r') && ISCHR(*(p+6),'I','i')
            && ISCHR(*(p+7),'A','a') && ISCHR(*(p+8),'B','b') && ISCHR(*(p+9),'L','l')
            && !ISSTRING(p, 10)) {
            parseRuleDirective(text, p, 11);
            state->stack << S_SEMICOLON << S_CONTEXT_LIST << S_IF << S_SET_INLINE << S_TARGET << S_SET_INLINE << S_SET_INLINE << S_RULE_FLAG;
            return true;
        }
        // REMVARIABLE
        else if (ISCHR(*p,'R','r') && ISCHR(*(p+10),'E','e') && ISCHR(*(p+1),'E','e') && ISCHR(*(p+2),'M','m')
            && ISCHR(*(p+3),'V','v') && ISCHR(*(p+4),'A','a') && ISCHR(*(p+5),'R','r') && ISCHR(*(p+6),'I','i')
            && ISCHR(*(p+7),'A','a') && ISCHR(*(p+8),'B','b') && ISCHR(*(p+9),'L','l')
            && !ISSTRING(p, 10)) {
            parseRuleDirective(text, p, 11);
            state->stack << S_SEMICOLON << S_CONTEXT_LIST << S_IF << S_SET_INLINE << S_TARGET << S_SET_INLINE << S_RULE_FLAG;
            return true;
        }
        // SETPARENT
        else if (ISCHR(*p,'S','s') && ISCHR(*(p+8),'T','t') && ISCHR(*(p+1),'E','e') && ISCHR(*(p+2),'T','t')
            && ISCHR(*(p+3),'P','p') && ISCHR(*(p+4),'A','a') && ISCHR(*(p+5),'R','r') && ISCHR(*(p+6),'E','e')
            && ISCHR(*(p+7),'N','n')
            && !ISSTRING(p, 8)) {
            parseRuleDirective(text, p, 9);
            state->stack << S_SEMICOLON << S_CONTEXT_LIST << S_TO_FROM << S_CONTEXT_LIST << S_IF << S_SET_INLINE << S_TARGET << S_RULE_FLAG;
            return true;
        }
        // SETCHILD
        else if (ISCHR(*p,'S','s') && ISCHR(*(p+7),'D','d') && ISCHR(*(p+1),'E','e') && ISCHR(*(p+2),'T','t')
            && ISCHR(*(p+3),'C','c') && ISCHR(*(p+4),'H','h') && ISCHR(*(p+5),'I','i') && ISCHR(*(p+6),'L','l')
            && !ISSTRING(p, 7)) {
            parseRuleDirective(text, p, 8);
            state->stack << S_SEMICOLON << S_CONTEXT_LIST << S_TO_FROM << S_CONTEXT_LIST << S_IF << S_SET_INLINE << S_TARGET << S_RULE_FLAG;
            return true;
        }
        // EXTERNAL
        else if (ISCHR(*p,'E','e') && ISCHR(*(p+7),'L','l') && ISCHR(*(p+1),'X','x') && ISCHR(*(p+2),'T','t')
            && ISCHR(*(p+3),'E','e') && ISCHR(*(p+4),'R','r') && ISCHR(*(p+5),'N','n') && ISCHR(*(p+6),'A','a')
            && !ISSTRING(p, 7)) {
            parseRuleDirective(text, p, 8);
            state->stack << S_SEMICOLON << S_CONTEXT_LIST << S_IF << S_SET_INLINE << S_TARGET << S_RULE_FLAG << S_TAG << S_ONCE_ALWAYS;
            return true;
        }
        // REMCOHORT
        else if (ISCHR(*p,'R','r') && ISCHR(*(p+8),'T','t') && ISCHR(*(p+1),'E','e') && ISCHR(*(p+2),'M','m')
            && ISCHR(*(p+3),'C','c') && ISCHR(*(p+4),'O','o') && ISCHR(*(p+5),'H','h') && ISCHR(*(p+6),'O','o')
            && ISCHR(*(p+7),'R','r')
            && !ISSTRING(p, 8)) {
            parseRuleDirective(text, p, 9);
            state->stack << S_SEMICOLON << S_CONTEXT_LIST << S_IF << S_SET_INLINE << S_TARGET << S_RULE_FLAG;
            return true;
        }
        // ADDCOHORT
        else if (ISCHR(*p,'A','a') && ISCHR(*(p+8),'T','t') && ISCHR(*(p+1),'D','d') && ISCHR(*(p+2),'D','d')
            && ISCHR(*(p+3),'C','c') && ISCHR(*(p+4),'O','o') && ISCHR(*(p+5),'H','h') && ISCHR(*(p+6),'O','o')
            && ISCHR(*(p+7),'R','r')
            && !ISSTRING(p, 8)) {
            parseRuleDirective(text, p, 9);
            state->stack << S_SEMICOLON << S_CONTEXT_LIST << S_IF << S_SET_INLINE << S_TARGET << S_BEFORE_AFTER << S_SET_INLINE << S_RULE_FLAG;
            return true;
        }
        // SETS
        else if (ISCHR(*p,'S','s') && ISCHR(*(p+3),'S','s') && ISCHR(*(p+1),'E','e') && ISCHR(*(p+2),'T','t')
            && !ISSTRING(p, 3)) {
            const int index = p-text.constData(), length = 4;
            setFormat(index, length, fmts[F_OPTIONAL]);
            p += 4;
            return true;
        }
        // LIST
        else if (ISCHR(*p,'L','l') && ISCHR(*(p+3),'T','t') && ISCHR(*(p+1),'I','i') && ISCHR(*(p+2),'S','s')
            && !ISSTRING(p, 3)) {
            const int index = p-text.constData(), length = 4;
            setFormat(index, length, fmts[F_DIRECTIVE]);
            p += 4;
            state->stack << (S_SEMICOLON|S_TAGLIST) << (S_TAG|S_COMPOSITETAG) << S_EQUALS << S_SETNAME;
            return true;
        }
        // SET
        else if (ISCHR(*p,'S','s') && ISCHR(*(p+2),'T','t') && ISCHR(*(p+1),'E','e')
            && !ISSTRING(p, 2)) {
            const int index = p-text.constData(), length = 3;
            setFormat(index, length, fmts[F_DIRECTIVE]);
            p += 3;
            state->stack << S_SEMICOLON << S_SET_INLINE << S_EQUALS << S_SETNAME;
            return true;
        }
        // MAPPINGS
        else if (ISCHR(*p,'M','m') && ISCHR(*(p+7),'S','s') && ISCHR(*(p+1),'A','a') && ISCHR(*(p+2),'P','p')
            && ISCHR(*(p+3),'P','p') && ISCHR(*(p+4),'I','i') && ISCHR(*(p+5),'N','n') && ISCHR(*(p+6),'G','g')
            && !ISSTRING(p, 7)) {
            return parseSectionDirective(text, p, 8);
        }
        // CORRECTIONS
        else if (ISCHR(*p,'C','c') && ISCHR(*(p+10),'S','s') && ISCHR(*(p+1),'O','o') && ISCHR(*(p+2),'R','r')
            && ISCHR(*(p+3),'R','r') && ISCHR(*(p+4),'E','e') && ISCHR(*(p+5),'C','c') && ISCHR(*(p+6),'T','t')
            && ISCHR(*(p+7),'I','i') && ISCHR(*(p+8),'O','o') && ISCHR(*(p+9),'N','n')
            && !ISSTRING(p, 10)) {
            return parseSectionDirective(text, p, 11);
        }
        // BEFORE-SECTIONS
        else if (ISCHR(*p,'B','b') && ISCHR(*(p+14),'S','s') && ISCHR(*(p+1),'E','e') && ISCHR(*(p+2),'F','f')
            && ISCHR(*(p+3),'O','o') && ISCHR(*(p+4),'R','r') && ISCHR(*(p+5),'E','e') && ISCHR(*(p+6),'-','_')
            && ISCHR(*(p+7),'S','s') && ISCHR(*(p+8),'E','e') && ISCHR(*(p+9),'C','c') && ISCHR(*(p+10),'T','t')
            && ISCHR(*(p+11),'I','i') && ISCHR(*(p+12),'O','o') && ISCHR(*(p+13),'N','n')
            && !ISSTRING(p, 14)) {
            return parseSectionDirective(text, p, 15);
        }
        // SECTION
        else if (ISCHR(*p,'S','s') && ISCHR(*(p+6),'N','n') && ISCHR(*(p+1),'E','e') && ISCHR(*(p+2),'C','c')
            && ISCHR(*(p+3),'T','t') && ISCHR(*(p+4),'I','i') && ISCHR(*(p+5),'O','o')
            && !ISSTRING(p, 6)) {
            return parseSectionDirective(text, p, 7);
        }
        // CONSTRAINTS
        else if (ISCHR(*p,'C','c') && ISCHR(*(p+10),'S','s') && ISCHR(*(p+1),'O','o') && ISCHR(*(p+2),'N','n')
            && ISCHR(*(p+3),'S','s') && ISCHR(*(p+4),'T','t') && ISCHR(*(p+5),'R','r') && ISCHR(*(p+6),'A','a')
            && ISCHR(*(p+7),'I','i') && ISCHR(*(p+8),'N','n') && ISCHR(*(p+9),'T','t')
            && !ISSTRING(p, 10)) {
            return parseSectionDirective(text, p, 11);
        }
        // AFTER-SECTIONS
        else if (ISCHR(*p,'A','a') && ISCHR(*(p+13),'S','s') && ISCHR(*(p+1),'F','f') && ISCHR(*(p+2),'T','t')
            && ISCHR(*(p+3),'E','e') && ISCHR(*(p+4),'R','r') && ISCHR(*(p+5),'-','_')
            && ISCHR(*(p+6),'S','s') && ISCHR(*(p+7),'E','e') && ISCHR(*(p+8),'C','c') && ISCHR(*(p+9),'T','t')
            && ISCHR(*(p+10),'I','i') && ISCHR(*(p+11),'O','o') && ISCHR(*(p+12),'N','n')
            && !ISSTRING(p, 13)) {
            return parseSectionDirective(text, p, 14);
        }
        // NULL-SECTION
        else if (ISCHR(*p,'N','n') && ISCHR(*(p+11),'N','n') && ISCHR(*(p+1),'U','u') && ISCHR(*(p+2),'L','l')
            && ISCHR(*(p+3),'L','l') && ISCHR(*(p+4),'-','-') && ISCHR(*(p+5),'S','s')
            && ISCHR(*(p+6),'E','e') && ISCHR(*(p+7),'C','c') && ISCHR(*(p+8),'T','t') && ISCHR(*(p+9),'I','i')
            && ISCHR(*(p+10),'O','o')
            && !ISSTRING(p, 11)) {
            return parseSectionDirective(text, p, 12);
        }
        // SUBREADINGS
        else if (ISCHR(*p,'S','s') && ISCHR(*(p+10),'S','s') && ISCHR(*(p+1),'U','u') && ISCHR(*(p+2),'B','b')
            && ISCHR(*(p+3),'R','r') && ISCHR(*(p+4),'E','e') && ISCHR(*(p+5),'A','a')
            && ISCHR(*(p+6),'D','d') && ISCHR(*(p+7),'I','i') && ISCHR(*(p+8),'N','n') && ISCHR(*(p+9),'G','g')
            && !ISSTRING(p, 10)) {
            const int index = p-text.constData(), length = 11;
            setFormat(index, length, fmts[F_DIRECTIVE]);
            p += 11;
            SKIPWS(p, '=');
            if (*p != '=') {
                state->stack << S_ERROR;
                return false;
            }
            ++p;
            SKIPWS(p);
            auto n = p;
            if (!SKIPTOWS(n, 0, true)) {
                return false;
            }
            if (n == p+3 && (ISCHR(*p,'L','l') || ISCHR(*p,'R','r')) && ISCHR(*(p+1),'T','t') && (ISCHR(*(p+2),'R','r') || ISCHR(*(p+2),'L','l'))) {
                const int index = p-text.constData(), length = 3;
                setFormat(index, length, fmts[F_DIRECTIVE]);
            }
            else {
                state->stack << S_ERROR;
                return false;
            }
            p = n;
            state->stack << S_SEMICOLON;
            return true;
        }
        // ANCHOR
        else if (ISCHR(*p,'A','a') && ISCHR(*(p+5),'R','r') && ISCHR(*(p+1),'N','n') && ISCHR(*(p+2),'C','c')
            && ISCHR(*(p+3),'H','h') && ISCHR(*(p+4),'O','o')
            && !ISSTRING(p, 5)) {
            const int index = p-text.constData(), length = 6;
            setFormat(index, length, fmts[F_DIRECTIVE]);
            p += 6;
            SKIPWS(p);
            if (!parseAnchorish(text, p)) {
                return false;
            }
            state->stack << S_SEMICOLON;
            return true;
        }
        // INCLUDE
        else if (IS_ICASE(p, "INCLUDE", "include")) {
            const int index = p-text.constData(), length = 7;
            setFormat(index, length, fmts[F_DIRECTIVE]);
            p += 7;
            SKIPWS(p);
            auto n = p;
            if (!SKIPTOWS(n, 0, true)) {
                return false;
            }
            if (IS_ICASE(p, "STATIC", "static")) {
                setFormat(p-text.constData(), n-p, fmts[F_DIRECTIVE]);
                p = n;
                SKIPWS(p);
                n = p;
                if (!SKIPTOWS(n, 0, true)) {
                    return false;
                }
            }
            setFormat(p-text.constData(), n-p, fmts[F_TAG]);
            p = n;
            state->stack << S_SEMICOLON;
            return true;
        }
        // IFF
        else if (ISCHR(*p,'I','i') && ISCHR(*(p+2),'F','f') && ISCHR(*(p+1),'F','f')
            && !ISSTRING(p, 2)) {
            parseRuleDirective(text, p, 3);
            state->stack << S_SEMICOLON << S_CONTEXT_LIST << S_IF << S_SET_INLINE << S_TARGET << S_RULE_FLAG;
            return true;
        }
        // MAP
        else if (ISCHR(*p,'M','m') && ISCHR(*(p+2),'P','p') && ISCHR(*(p+1),'A','a')
            && !ISSTRING(p, 2)) {
            parseRuleDirective(text, p, 3);
            state->stack << S_SEMICOLON << S_CONTEXT_LIST << S_IF << S_SET_INLINE << S_TARGET << S_SET_INLINE << S_RULE_FLAG;
            return true;
        }
        // ADD
        else if (ISCHR(*p,'A','a') && ISCHR(*(p+2),'D','d') && ISCHR(*(p+1),'D','d')
            && !ISSTRING(p, 2)) {
            parseRuleDirective(text, p, 3);
            state->stack << S_SEMICOLON << S_CONTEXT_LIST << S_IF << S_SET_INLINE << S_TARGET << S_SET_INLINE << S_RULE_FLAG;
            return true;
        }
        // APPEND
        else if (ISCHR(*p,'A','a') && ISCHR(*(p+5),'D','d') && ISCHR(*(p+1),'P','p') && ISCHR(*(p+2),'P','p')
            && ISCHR(*(p+3),'E','e') && ISCHR(*(p+4),'N','n')
            && !ISSTRING(p, 5)) {
            parseRuleDirective(text, p, 6);
            state->stack << S_SEMICOLON << S_CONTEXT_LIST << S_IF << S_SET_INLINE << S_TARGET << S_SET_INLINE << S_RULE_FLAG;
            return true;
        }
        // SELECT
        else if (ISCHR(*p,'S','s') && ISCHR(*(p+5),'T','t') && ISCHR(*(p+1),'E','e') && ISCHR(*(p+2),'L','l')
            && ISCHR(*(p+3),'E','e') && ISCHR(*(p+4),'C','c')
            && !ISSTRING(p, 5)) {
            parseRuleDirective(text, p, 6);
            state->stack << S_SEMICOLON << S_CONTEXT_LIST << S_IF << S_SET_INLINE << S_TARGET << S_RULE_FLAG;
            return true;
        }
        // REMOVE
        else if (ISCHR(*p,'R','r') && ISCHR(*(p+5),'E','e') && ISCHR(*(p+1),'E','e') && ISCHR(*(p+2),'M','m')
            && ISCHR(*(p+3),'O','o') && ISCHR(*(p+4),'V','v')
            && !ISSTRING(p, 5)) {
            parseRuleDirective(text, p, 6);
            state->stack << S_SEMICOLON << S_CONTEXT_LIST << S_IF << S_SET_INLINE << S_TARGET << S_RULE_FLAG;
            return true;
        }
        // REPLACE
        else if (ISCHR(*p,'R','r') && ISCHR(*(p+6),'E','e') && ISCHR(*(p+1),'E','e') && ISCHR(*(p+2),'P','p')
            && ISCHR(*(p+3),'L','l') && ISCHR(*(p+4),'A','a') && ISCHR(*(p+5),'C','c')
            && !ISSTRING(p, 6)) {
            parseRuleDirective(text, p, 7);
            state->stack << S_SEMICOLON << S_CONTEXT_LIST << S_IF << S_SET_INLINE << S_TARGET << S_SET_INLINE << S_RULE_FLAG;
            return true;
        }
        // DELIMIT
        else if (ISCHR(*p,'D','d') && ISCHR(*(p+6),'T','t') && ISCHR(*(p+1),'E','e') && ISCHR(*(p+2),'L','l')
            && ISCHR(*(p+3),'I','i') && ISCHR(*(p+4),'M','m') && ISCHR(*(p+5),'I','i')
            && !ISSTRING(p, 6)) {
            parseRuleDirective(text, p, 7);
            state->stack << S_SEMICOLON << S_CONTEXT_LIST << S_IF << S_SET_INLINE << S_TARGET << S_RULE_FLAG;
            return true;
        }
        // SUBSTITUTE
        else if (ISCHR(*p,'S','s') && ISCHR(*(p+9),'E','e') && ISCHR(*(p+1),'U','u') && ISCHR(*(p+2),'B','b')
            && ISCHR(*(p+3),'S','s') && ISCHR(*(p+4),'T','t') && ISCHR(*(p+5),'I','i') && ISCHR(*(p+6),'T','t')
            && ISCHR(*(p+7),'U','u') && ISCHR(*(p+8),'T','t')
            && !ISSTRING(p, 9)) {
            parseRuleDirective(text, p, 10);
            state->stack << S_SEMICOLON << S_CONTEXT_LIST << S_IF << S_SET_INLINE << S_TARGET << S_SET_INLINE << S_SET_INLINE << S_RULE_FLAG;
            return true;
        }
        // COPY
        else if (ISCHR(*p,'C','c') && ISCHR(*(p+3),'Y','y') && ISCHR(*(p+1),'O','o') && ISCHR(*(p+2),'P','p')
            && !ISSTRING(p, 3)) {
            parseRuleDirective(text, p, 4);
            state->stack << S_SEMICOLON << S_CONTEXT_LIST << S_IF << S_SET_INLINE << S_TARGET << S_EXCEPT << S_SET_INLINE << S_RULE_FLAG;
            return true;
        }
        // JUMP
        else if (ISCHR(*p,'J','j') && ISCHR(*(p+3),'P','p') && ISCHR(*(p+1),'U','u') && ISCHR(*(p+2),'M','m')
            && !ISSTRING(p, 3)) {
            parseRuleDirective(text, p, 4);
            state->stack << S_SEMICOLON << S_CONTEXT_LIST << S_IF << S_SET_INLINE << S_TARGET << S_SET_INLINE << S_RULE_FLAG;
            return true;
        }
        // MOVE
        else if (ISCHR(*p,'M','m') && ISCHR(*(p+3),'E','e') && ISCHR(*(p+1),'O','o') && ISCHR(*(p+2),'V','v')
            && !ISSTRING(p, 3)) {
            parseRuleDirective(text, p, 4);
            state->stack << S_SEMICOLON << S_CONTEXT_LIST << S_WITHCHILD << S_BEFORE_AFTER << S_CONTEXT_LIST << S_IF << S_SET_INLINE << S_TARGET << S_WITHCHILD << S_RULE_FLAG;
            return true;
        }
        // SWITCH
        else if (ISCHR(*p,'S','s') && ISCHR(*(p+5),'H','h') && ISCHR(*(p+1),'W','w') && ISCHR(*(p+2),'I','i')
            && ISCHR(*(p+3),'T','t') && ISCHR(*(p+4),'C','c')
            && !ISSTRING(p, 5)) {
            parseRuleDirective(text, p, 6);
            state->stack << S_SEMICOLON << S_CONTEXT_LIST << S_WITH << S_CONTEXT_LIST << S_IF << S_SET_INLINE << S_TARGET << S_RULE_FLAG;
            return true;
        }
        // EXECUTE
        else if (ISCHR(*p,'E','e') && ISCHR(*(p+6),'E','e') && ISCHR(*(p+1),'X','x') && ISCHR(*(p+2),'E','e')
            && ISCHR(*(p+3),'C','c') && ISCHR(*(p+4),'U','u') && ISCHR(*(p+5),'T','t')
            && !ISSTRING(p, 6)) {
            parseRuleDirective(text, p, 7);
            state->stack << S_SEMICOLON << S_CONTEXT_LIST << S_IF << S_SET_INLINE << S_TARGET << S_SET_INLINE << S_SET_INLINE << S_RULE_FLAG;
            return true;
        }
        // UNMAP
        else if (ISCHR(*p,'U','u') && ISCHR(*(p+4),'P','p') && ISCHR(*(p+1),'N','n') && ISCHR(*(p+2),'M','m')
            && ISCHR(*(p+3),'A','a')
            && !ISSTRING(p, 4)) {
            parseRuleDirective(text, p, 5);
            state->stack << S_SEMICOLON << S_CONTEXT_LIST << S_IF << S_SET_INLINE << S_TARGET << S_RULE_FLAG;
            return true;
        }
        // TEMPLATE
        else if (ISCHR(*p,'T','t') && ISCHR(*(p+7),'E','e') && ISCHR(*(p+1),'E','e') && ISCHR(*(p+2),'M','m')
            && ISCHR(*(p+3),'P','p') && ISCHR(*(p+4),'L','l') && ISCHR(*(p+5),'A','a') && ISCHR(*(p+6),'T','t')
            && !ISSTRING(p, 7)) {
            const int index = p-text.constData(), length = 8;
            setFormat(index, length, fmts[F_DIRECTIVE]);
            p += 8;
            state->stack << S_SEMICOLON << S_CONTEXT << S_EQUALS << S_TMPLNAME;
            return true;
        }
        // PARENTHESES
        else if (ISCHR(*p,'P','p') && ISCHR(*(p+10),'S','s') && ISCHR(*(p+1),'A','a') && ISCHR(*(p+2),'R','r')
            && ISCHR(*(p+3),'E','e') && ISCHR(*(p+4),'N','n') && ISCHR(*(p+5),'T','t') && ISCHR(*(p+6),'H','h')
            && ISCHR(*(p+7),'E','e') && ISCHR(*(p+8),'S','s') && ISCHR(*(p+9),'E','e')
            && !ISSTRING(p, 10)) {
            const int index = p-text.constData(), length = 11;
            setFormat(index, length, fmts[F_DIRECTIVE]);
            p += 11;
            state->stack << (S_SEMICOLON|S_TAGLIST) << S_COMPOSITETAG << S_EQUALS;
            return true;
        }
        // END
        else if (ISCHR(*p,'E','e') && ISCHR(*(p+2),'D','d') && ISCHR(*(p+1),'N','n')) {
            if (p == text.constData() || ISNL(*(p-1)) || ISSPACE(*(p-1))) {
                if (*(p+3) == nullptr || ISNL(*(p+3)) || ISSPACE(*(p+3))) {
                    const int index = p-text.constData(), length = 3;
                    setFormat(index, length, fmts[F_OPTIONAL]);
                    p += 3;
                    return true;
                }
            }
        }
        if (*p == '"') {
            if (!parseTag(text, p)) {
                return false;
            }
            state->stack << S_RULE;
            return true;
        }
        if (!SKIPTOWS(p)) {
            return false;
        }
        state->stack << S_ERROR;
        return true;
    }
    return true;
}
