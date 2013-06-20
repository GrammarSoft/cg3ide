/*
* Copyright (C) 2013, GrammarSoft ApS
* Developed by Tino Didriksen <mail@tinodidriksen.com> for GrammarSoft ApS (http://grammarsoft.com/)
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

#pragma once
#ifndef GRAMMARSTATE_HPP_cc7194f1bd3a13d1dca4d5a1c31f83d81877a7f7
#define GRAMMARSTATE_HPP_cc7194f1bd3a13d1dca4d5a1c31f83d81877a7f7

#include <QtWidgets>
#include <stdint.h>

enum enum_state {
    S_NONE           =         0,
    S_TAGLIST        = (1 <<  0),
    S_TAGLIST_INLINE = (1 <<  1),
    S_COMPOSITETAG   = (1 <<  2),
    S_EQUALS         = (1 <<  3),
    S_SEMICOLON      = (1 <<  4),
    S_COMMENT        = (1 <<  5),
    S_TAG            = (1 <<  6),
    S_SETNAME        = (1 <<  7),
    S_SETOP          = (1 <<  8),
    S_SET_INLINE     = (1 <<  9),
    S_CONTEXT        = (1 << 10),
    S_RULE           = (1 << 11),
    S_TMPLNAME       = (1 << 12),
    S_ERROR          = (1 << 13),
    S_RULE_FLAG      = (1 << 14),
    S_CONTEXT_POS    = (1 << 15),
    S_CONTEXT_TMPL   = (1 << 16),
    S_CONTEXT_OP     = (1 << 17),
    S_PAR_START      = (1 << 18),
    S_PAR_STOP       = (1 << 19),
    S_BARRIER        = (1 << 20),
    S_LINK           = (1 << 21),
    S_SQBRACKET_STOP = (1 << 22),
    S_TARGET         = (1 << 23),
    S_IF             = (1 << 24),
    S_CONTEXT_LIST   = (1 << 25),
    S_TO_FROM        = (1 << 26),
    S_BEFORE_AFTER   = (1 << 27),
    S_WITH           = (1 << 28),
    S_WITHCHILD      = (1 << 29),
    S_ONCE_ALWAYS    = (1 << 30)
};
Q_DECLARE_FLAGS(State, enum_state)
Q_FLAGS(State)
Q_DECLARE_OPERATORS_FOR_FLAGS(State)

inline QString StateText(State state) {
    QString rv;
    if (state & S_NONE) {
        rv.append("|S_NONE");
    }
    if (state & S_TAGLIST) {
        rv.append("|S_TAGLIST");
    }
    if (state & S_TAGLIST_INLINE) {
        rv.append("|S_TAGLIST_INLINE");
    }
    if (state & S_COMPOSITETAG) {
        rv.append("|S_COMPOSITETAG");
    }
    if (state & S_EQUALS) {
        rv.append("|S_EQUALS");
    }
    if (state & S_SEMICOLON) {
        rv.append("|S_SEMICOLON");
    }
    if (state & S_COMMENT) {
        rv.append("|S_COMMENT");
    }
    if (state & S_TAG) {
        rv.append("|S_TAG");
    }
    if (state & S_SETNAME) {
        rv.append("|S_SETNAME");
    }
    if (state & S_SETOP) {
        rv.append("|S_SETOP");
    }
    if (state & S_SET_INLINE) {
        rv.append("|S_SET_INLINE");
    }
    if (state & S_CONTEXT) {
        rv.append("|S_CONTEXT");
    }
    if (state & S_RULE) {
        rv.append("|S_RULE");
    }
    if (state & S_TMPLNAME) {
        rv.append("|S_TMPLNAME");
    }
    if (state & S_ERROR) {
        rv.append("|S_ERROR");
    }
    if (state & S_CONTEXT_POS) {
        rv.append("|S_CONTEXT_POS");
    }
    if (state & S_CONTEXT_TMPL) {
        rv.append("|S_CONTEXT_TMPL");
    }
    if (state & S_CONTEXT_OP) {
        rv.append("|S_CONTEXT_OP");
    }
    if (state & S_PAR_START) {
        rv.append("|S_PAR_START");
    }
    if (state & S_PAR_STOP) {
        rv.append("|S_PAR_STOP");
    }
    if (state & S_BARRIER) {
        rv.append("|S_BARRIER");
    }
    if (state & S_LINK) {
        rv.append("|S_LINK");
    }
    if (state & S_SQBRACKET_STOP) {
        rv.append("|S_SQBRACKET_STOP");
    }
    if (state & S_TARGET) {
        rv.append("|S_TARGET");
    }
    if (state & S_IF) {
        rv.append("|S_IF");
    }
    if (state & S_CONTEXT_LIST) {
        rv.append("|S_CONTEXT_LIST");
    }
    if (state & S_TO_FROM) {
        rv.append("|S_TO_FROM");
    }
    if (state & S_BEFORE_AFTER) {
        rv.append("|S_BEFORE_AFTER");
    }
    if (state & S_WITH) {
        rv.append("|S_WITH");
    }
    if (state & S_WITHCHILD) {
        rv.append("|S_WITHCHILD");
    }
    if (state & S_ONCE_ALWAYS) {
        rv.append("|S_ONCE_ALWAYS");
    }
    if (state & S_RULE_FLAG) {
        rv.append("|S_RULE_FLAG");
    }
    return rv.trimmed().replace(QRegExp("^\\|"), "");
}

class GrammarState : public QTextBlockUserData {
public:
    GrammarState() {
    }

    GrammarState(const QVector<State>& stack) : stack(stack) {
    }

    QVector<State> stack;
    QString error;

    typedef QMap<int,State> tokens_t;
    tokens_t tokens;

    typedef QMultiMap<QPair<int,int>,QString> warnings_t;
    warnings_t warnings;
};

#endif // GRAMMARSTATE_HPP_cc7194f1bd3a13d1dca4d5a1c31f83d81877a7f7
