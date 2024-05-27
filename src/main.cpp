/*
* Copyright 2013-2024, GrammarSoft ApS
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

	auto args = app.arguments();
	args.pop_front();

	if (args.empty()) {
		auto w = new GrammarEditor;
		w->show();
	}
	else {
		bool first = true;
		for (auto& arg : args) {
			if (first) {
				auto w = new GrammarEditor;
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
