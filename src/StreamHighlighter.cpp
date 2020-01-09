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

#include "StreamHighlighter.hpp"
#include "inlines.hpp"

StreamHighlighter::StreamHighlighter(QTextDocument *parent) :
    QSyntaxHighlighter(parent)
{
    rxs.append(QRegExp("^\"<.*>\"$"));
    rxs.append(QRegExp(CG_READING_RX));
    rxs.last().setMinimal(true);
    rxs.append(QRegExp(CG_READING_RX2));

    fmts.append(QTextCharFormat());
    fmts.last().setForeground(Qt::darkGreen);
    fmts.last().setFontWeight(QFont::Bold);

    tagPatterns.append(Tag("^\".+\"$"));
    tagPatterns.last().fmt.setForeground(Qt::darkBlue);
    tagPatterns.append(Tag("^<\\S+>$"));
    tagPatterns.last().fmt.setForeground(Qt::darkRed);
    tagPatterns.append(Tag("^[-A-Z0-9/]+$"));
    tagPatterns.last().fmt.setForeground(Qt::blue);
    tagPatterns.append(Tag(CG_TRACE_RX));
    tagPatterns.last().fmt.setForeground(Qt::darkMagenta);
    tagPatterns.last().fmt.setFontItalic(true);
}

void StreamHighlighter::highlightBlock(const QString &text) {
    int index = 0;
    if ((index = rxs[0].indexIn(text)) != -1) {
        setFormat(0, text.size(), fmts[0]);
    }
    else if (rxs[1].indexIn(text) != -1 && (index = rxs[2].indexIn(text)) != -1) {
        index += rxs[2].matchedLength();
        auto tags = text.mid(index).split(' ');
        auto glob = text.left(index);
        glob.reserve(text.length());

        for (auto& tag : tags) {
            for (int i=0 ; i<tagPatterns.size() ; ++i) {
                if (tagPatterns[i].rx.indexIn(tag) != -1) {
                    setFormat(glob.length(), tag.length(), tagPatterns[i].fmt);
                    break;
                }
            }
            glob.append(tag).append(' ');
        }
    }
}
