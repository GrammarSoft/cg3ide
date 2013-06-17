#pragma once
#ifndef GRAMMARHIGHLIGHTER_HPP_cc7194f1bd3a13d1dca4d5a1c31f83d81877a7f7
#define GRAMMARHIGHLIGHTER_HPP_cc7194f1bd3a13d1dca4d5a1c31f83d81877a7f7

#include "GrammarState.hpp"
#include <QtWidgets>

class GrammarHighlighter : public QSyntaxHighlighter {
    Q_OBJECT

public:
    GrammarHighlighter(QTextDocument *parent = 0);
    QMap<QString,int> set_lines;
    QMap<QString,int> tmpl_lines;

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
    inline bool SKIPWS(const QChar *& p, const QChar a = 0, const QChar b = 0);
    inline bool SKIPTOWS(const QChar *& p, const QChar a = 0, const bool allowhash = false);

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
