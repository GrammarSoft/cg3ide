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
        QStringList tags = text.mid(index).split(' ');
        QString glob = text.left(index);
        glob.reserve(text.length());

        foreach (QString tag, tags) {
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
