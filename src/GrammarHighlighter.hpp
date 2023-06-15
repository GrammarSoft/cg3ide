/*
* Copyright 2013-2023, GrammarSoft ApS
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

#pragma once
#ifndef GRAMMARHIGHLIGHTER_HPP_cc7194f1bd3a13d1dca4d5a1c31f83d81877a7f7
#define GRAMMARHIGHLIGHTER_HPP_cc7194f1bd3a13d1dca4d5a1c31f83d81877a7f7

#include "GrammarState.hpp"
#include <QtWidgets>
#include <set>

class GrammarHighlighter : public QSyntaxHighlighter {
    Q_OBJECT

public:
    GrammarHighlighter(QTextDocument *parent = nullptr);
    QMap<QString,int> set_lines;
    QMap<QString,int> tmpl_lines;
    std::set<int> section_lines;

public slots:
    void clear();

protected:
    void highlightBlock(const QString& text);

public:
    enum {
        F_ERROR,
        F_COMMENT,
        F_DIRECTIVE,
        F_TAG,
        F_SETNAME,
        F_SETOP,
        F_TMPLNAME,
        F_ANCHOR,
        F_CNTXMOD,
        F_CNTXPOS,
        F_CNTXOP,
        F_RULE_FLAG,
        F_OPTIONAL,
        NUM_FORMATS
    };
    QVector<QTextCharFormat> fmts;
    QVector<QStringList> fmt_desc;

private:
    inline bool SKIPWS(const QChar *& p, const QChar a = QChar(0), const QChar b = QChar(0));
    inline bool SKIPTOWS(const QChar *& p, const QChar a = QChar(0), const bool allowhash = false);

    inline bool parseTag(const QString& text, const QChar *& p);
    inline bool parseCompositeTag(const QString& text, const QChar *& p);
    inline bool parseTagList(const QString& text, const QChar *& p);
    inline bool parseAnchorish(const QString& text, const QChar *& p);
    inline bool parseSectionDirective(const QString& text, const QChar *& p, int length);
    inline bool parseRuleDirective(const QString& text, const QChar *& p, int length);
    inline bool parseNone(const QString& text, const QChar *& p);

    GrammarState *state;
};

#endif // GRAMMARHIGHLIGHTER_HPP_cc7194f1bd3a13d1dca4d5a1c31f83d81877a7f7
