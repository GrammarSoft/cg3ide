#pragma once
#ifndef TYPES_HPP_cc7194f1bd3a13d1dca4d5a1c31f83d81877a7f7
#define TYPES_HPP_cc7194f1bd3a13d1dca4d5a1c31f83d81877a7f7

#include <QtWidgets>

struct CGChecker {
    QString txtGrammar;
    QString binGrammar;
    QString inputText;
    QString inputFile;
    QScopedPointer<QProcess> process;
};

#endif // TYPES_HPP_cc7194f1bd3a13d1dca4d5a1c31f83d81877a7f7
