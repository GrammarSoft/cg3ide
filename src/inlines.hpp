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
#ifndef INLINES_HPP_cc7194f1bd3a13d1dca4d5a1c31f83d81877a7f7
#define INLINES_HPP_cc7194f1bd3a13d1dca4d5a1c31f83d81877a7f7

#include <QtWidgets>
#include <stdint.h>

#define CG_DIRECTIVES_OR "SETS|LIST|SET|DELIMITERS|SOFT-DELIMITERS|PREFERRED-TARGETS" \
    "|MAPPING-PREFIX|MAPPINGS|CONSTRAINTS|CORRECTIONS|SECTION|BEFORE-SECTIONS" \
    "|AFTER-SECTIONS|NULL-SECTION|START|END|ANCHOR|TEMPLATE|STATIC-SETS|SUBREADINGS" \
    "|PARENTHESES|REOPEN-MAPPINGS|STRICT-TAGS|OPTIONS"
#define CG_RULES_OR "ADD|MAP|REPLACE|SELECT|REMOVE|IFF|APPEND|SUBSTITUTE|REMVARIABLE" \
    "|SETVARIABLE|DELIMIT|MATCH|SETPARENT|SETCHILD|ADDRELATION|SETRELATION" \
    "|REMRELATION|ADDRELATIONS|SETRELATIONS|REMRELATIONS|MOVE|MOVE-AFTER" \
    "|MOVE-BEFORE|SWITCH|REMCOHORT|UNMAP|COPY|ADDCOHORT|ADDCOHORT-AFTER" \
    "|ADDCOHORT-BEFORE|EXTERNAL|EXTERNAL-ONCE|EXTERNAL-ALWAYS|EXECUTE|JUMP"
#define CG_RULEFLAGS_OR "NEAREST|ALLOWLOOP|DELAYED|IMMEDIATE|LOOKDELETED|LOOKDELAYED" \
    "|UNSAFE|SAFE|REMEMBERX|RESETX|KEEPORDER|VARYORDER|ENCL_INNER|ENCL_OUTER|ENCL_FINAL" \
    "|ENCL_ANY|ALLOWCROSS|WITHCHILD|NOCHILD|ITERATE|NOITERATE|UNMAPLAST|REVERSE|SUB"
#define CG_RESERVED_OR "\\||TO|OR|+|-|NOT|NEGATE|ALL|NONE|LINK|BARRIER|CBARRIER|TARGET" \
    "|AND|IF|_S_DELIMITERS_|_S_SOFT_DELIMITERS_|_LEFT_|_RIGHT_|_PAREN_|_TARGET_|_MARK_" \
    "|_ATTACHTO_|AFTER|BEFORE|WITH|ONCE|ALWAYS|FROM|_ENCL_|\\x2206|\\x2229"

#define CG_DIRECTIVES_RX "\\b(" CG_DIRECTIVES_OR ")(\\s|$)"
#define CG_RULES_RX "\\b(" CG_RULES_OR ")(:\\w+)?\\s"
#define CG_RULEFLAGS_RX "\\b(" CG_RULEFLAGS_OR ")(:\\w+)?\\s"
#define CG_RESERVED_RX "\\b(" CG_RESERVED_OR ")\\b"

#define CG_TRACE_RX "^(" CG_RULES_OR ")(\\(\\w+(,\\w+)?\\))?(:\\w+)+$"
#define CG_READING_RX "^;?\\s+(\".+\")|(- )"
#define CG_READING_RX2 "^;?\\s+"

template<typename T>
inline void settingSetOrDef(QSettings& settings, const QString& key, const T& def, const T& value) {
    if (settings.value(key, def).template value<T>() != value) {
        if (value != def) {
            settings.setValue(key, value);
        }
        else {
            settings.remove(key);
        }
    }
}

inline bool launchEditor(const QStringList& args = QStringList()) {
    QString exe = QCoreApplication::applicationFilePath();
    QFileInfo exi(exe);
    if (exi.exists() && exi.isExecutable() && QProcess::startDetached(exe, args)) {
        return true;
    }
    return false;
}

inline bool launchEditor(const QString& arg) {
    return launchEditor(QStringList() << arg);
}

inline void curGotoLine(QTextCursor& cur, int line=0) {
    const QTextBlock &block = cur.document()->findBlockByNumber(line);
    if (block.isValid()) {
        cur.setPosition(block.position());
    }
}

inline void editorGotoLine(QPlainTextEdit *editor, int line=0) {
    QTextCursor cur = editor->textCursor();
    curGotoLine(cur, line);
    editor->setTextCursor(cur);
    editor->centerCursor();
    editor->setFocus();
}

inline QString fileGetContents(const QString& name, qint64 max = 0) {
    QFile file(name);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::information(0, QCoreApplication::tr("Open failed!"), QCoreApplication::tr("Failed to open %1 for reading!").arg(name));
        return QString::null;
    }

    QTextStream in(&file);
    in.setCodec("UTF-8");
    in.setAutoDetectUnicode(true);
    if (max) {
        return in.read(max);
    }
    return in.readAll();
}

inline bool filePutContents(const QString& name, const QString& data) {
    QFile file(name);
    file.remove();
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::information(0, QCoreApplication::tr("Open failed!"), QCoreApplication::tr("Failed to open %1 for writing!").arg(name));
        return false;
    }

    QTextStream out(&file);
    out.setCodec("UTF-8");
    out << data;
    out.flush();
    file.close();
    return true;
}

inline size_t queryCG3Version(const QString& filename) {
    size_t ver = 0;
    QProcess cg3p;
    cg3p.setProcessChannelMode(QProcess::MergedChannels);
    cg3p.start(filename, QStringList() << "--version");
    if (!cg3p.waitForStarted()) {
        return ver;
    }

    if (!cg3p.waitForFinished()) {
        return ver;
    }

    QString result = cg3p.readAll();
    QRegExp rx("version \\d+\\.\\d+\\.\\d+\\.(\\d+)");
    if (rx.indexIn(result) != -1) {
        ver = QVariant(rx.capturedTexts().last()).toUInt();
    }
    return ver;
}

inline QString findLatestCG3() {
    QSettings settings;

    QProgressDialog progress;
    progress.setLabelText("Detecting most recent CG-3 binary...");
    progress.show();

    QStringList paths;

    if (settings.contains("cg3/binary")) {
        paths.append(QFileInfo(settings.value("cg3/binary").toString()).dir().path());
    }

    #if defined(Q_OS_WIN)
    paths.append(QCoreApplication::instance()->applicationDirPath());
    paths.append(QCoreApplication::instance()->applicationDirPath() + "/cg3/win32");
    #elif defined(Q_OS_MAC)
    paths.append(QCoreApplication::instance()->applicationDirPath() + "/../Resources/CG-3");
    #else
    paths.append(QCoreApplication::instance()->applicationDirPath() + "/cg3/linux");
    #endif

    #if defined(Q_OS_WIN)
    paths.append(QProcessEnvironment::systemEnvironment().value("PATH").split(';'));
    #else
    paths.append(QProcessEnvironment::systemEnvironment().value("PATH").split(':'));
    paths.append("/usr/bin");
    paths.append("/usr/local/bin");
    paths.append("/opt/bin");
    paths.append("/opt/local/bin");
    paths.append(QDir::home().filePath("/bin"));
    #endif

    progress.setMaximum(paths.size());

    QFileInfoList cg3s;
    foreach (QString path, paths) {
        progress.setValue(progress.value()+1);

        QDir cg3(path);
        #if defined(Q_OS_WIN)
        #define CG3_BINARY_NAME "vislcg3.exe"
        #else
        #define CG3_BINARY_NAME "vislcg3"
        #endif
        if (cg3.exists(CG3_BINARY_NAME)) {
            progress.setLabelText("Trying " + cg3.filePath(CG3_BINARY_NAME));
            QFileInfo exe(cg3.filePath(CG3_BINARY_NAME));
            if (exe.isExecutable()) {
                cg3s.append(exe);
            }
        }
    }

    progress.setLabelText("Querying found CG-3 binaries for versions...");
    progress.setValue(0);
    progress.setMaximum(cg3s.size());

    QFileInfo latest;
    size_t vlatest = 0;
    foreach (QFileInfo cg3, cg3s) {
        progress.setValue(progress.value()+1);
        progress.setLabelText("Querying " + cg3.filePath());

        size_t ver = queryCG3Version(cg3.filePath());
        if (ver > vlatest) {
            latest = cg3;
            vlatest = ver;
        }
    }

    return latest.filePath();
}

inline bool ISSPACE(const QChar c) {
    if (c <= 0xFF && c != 0x09 && c != 0x0A && c != 0x0D && c != 0x20 && c != 0xA0) {
        return false;
    }
    return (c == 0x20 || c == 0x09 || c == 0x0A || c == 0x0D || c == 0xA0 || c.isSpace());
}

inline bool ISSTRING(const QChar *p, const uint32_t c) {
    if (*(p-1) == '"' && *(p+c+1) == '"') {
        return true;
    }
    if (*(p-1) == '<' && *(p+c+1) == '>') {
        return true;
    }
    return false;
}

inline bool ISNL(const QChar c) {
    return (
       c == 0x2028 // Unicode Line Seperator
    || c == 0x2029 // Unicode Paragraph Seperator
    || c == 0x0085 // EBCDIC NEL
    || c == 0x000C // Form Feed
    || c == 0x000A // ASCII \n
    );
}

inline bool ISESC(const QChar *p) {
    uint32_t a = 1;
    while (*(p-a) != 0 && *(p-a) == '\\') {
        ++a;
    }
    return (a%2==0);
}

inline bool ISCHR(const QChar p, const QChar a, const QChar b) {
    return (p != 0 && (p == a || p == b));
}

inline void BACKTONL(const QChar *& p) {
    while (*p != 0 && !ISNL(*p) && (*p != ';' || ISESC(p))) {
        --p;
    }
    if (*p == 0) {
        return;
    }
    ++p;
}

inline void SKIPLN(const QChar *& p) {
    while (*p != 0 && !ISNL(*p)) {
        ++p;
    }
    if (*p == 0) {
        return;
    }
    ++p;
}

inline void SKIPWS(const QChar *& p, const QChar a = 0, const QChar b = 0) {
    while (*p != 0 && *p != a && *p != b) {
        if (*p == '#' && !ISESC(p)) {
            SKIPLN(p);
        }
        if (!ISSPACE(*p)) {
            break;
        }
        ++p;
    }
}

inline void SKIPTOWS(const QChar *& p, const QChar a = 0, const bool allowhash = false) {
    while (*p != 0 && !ISSPACE(*p)) {
        if (!allowhash && *p == '#' && !ISESC(p)) {
            SKIPLN(p);
        }
        if (*p == ';' && !ISESC(p)) {
            break;
        }
        if (*p == a && !ISESC(p)) {
            break;
        }
        ++p;
    }
}

inline void SKIPTO(const QChar *& p, const QChar a) {
    while (*p != 0 && (*p != a || ISESC(p))) {
        ++p;
    }
}

inline void SKIPTO_NOSPAN(const QChar *& p, const QChar a) {
    while (*p != 0 && (*p != a || ISESC(p))) {
        if (ISNL(*p)) {
            break;
        }
        ++p;
    }
}

inline bool ux_isSetOp(const QChar *it) {
    switch (it[1].unicode()) {
    case 0:
    case ' ':
    case '(':
    case '\n':
    case '\r':
    case '\t':
        switch (it[0].unicode()) {
        case '|':
        case '+':
        case '-':
        case '^':
        case 8745:
        case 8710:
            return true;
        default:
            break;
        }
        break;
    case 'R':
    case 'r':
        switch (it[0].unicode()) {
        case 'O':
        case 'o':
            switch (it[2].unicode()) {
            case 0:
            case ' ':
            case '(':
            case '\n':
            case '\r':
            case '\t':
                return true;
            default:
                break;
            }
            break;
        default:
            break;
        }
        break;
    default:
        break;
    }
    return false;
}

#endif // INLINES_HPP_cc7194f1bd3a13d1dca4d5a1c31f83d81877a7f7
