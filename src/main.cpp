#include "GrammarEditor.hpp"
#include "inlines.hpp"
#include <QtGui>
#include <ctime>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));

    app.setOrganizationDomain("grammarsoft.com");
    app.setOrganizationName("GrammarSoft ApS");
    app.setApplicationName("CG-3 IDE");
    app.setQuitOnLastWindowClosed(true);

    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings settings;

    if (settings.value("cg3/autodetect", true).toBool()) {
        settings.setValue("cg3/autodetect", true);

        QString latest = findLatestCG3();
        settings.remove("cg3/binary");
        if (!latest.isEmpty()) {
            settings.setValue("cg3/binary", latest);
        }
    }

    qsrand(static_cast<uint>(time(0)));

    QStringList args = app.arguments();
    args.pop_front();

    if (args.empty()) {
        GrammarEditor *w = new GrammarEditor;
        w->show();
    }
    else {
        bool first = true;
        foreach (QString arg, args) {
            if (first) {
                GrammarEditor *w = new GrammarEditor;
                w->show();
                w->open(arg);
                first = false;
            }
            else {
                launchEditor(arg);
            }
        }
    }
    
    return app.exec();
}
