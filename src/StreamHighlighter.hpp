#pragma once
#ifndef STREAMHIGHLIGHTER_HPP_cc7194f1bd3a13d1dca4d5a1c31f83d81877a7f7
#define STREAMHIGHLIGHTER_HPP_cc7194f1bd3a13d1dca4d5a1c31f83d81877a7f7

#include <QtWidgets>

class StreamHighlighter : public QSyntaxHighlighter {
    Q_OBJECT

public:
    StreamHighlighter(QTextDocument *parent = 0);

protected:
    void highlightBlock(const QString &text);

private:
    struct Tag {
        QRegExp rx;
        QTextCharFormat fmt;

        Tag(const char *r) : rx(r) {
        }
    };
    QList<Tag> tagPatterns;

    QList<QRegExp> rxs;
    QList<QTextCharFormat> fmts;
};

#endif // STREAMHIGHLIGHTER_HPP_cc7194f1bd3a13d1dca4d5a1c31f83d81877a7f7
