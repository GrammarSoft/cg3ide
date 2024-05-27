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

#include "OptionsDialog.hpp"
#include "GrammarEditor.hpp"
#include "ui_GrammarEditor.h"
#include "ui_OptionsDialog.h"
#include "inlines.hpp"

OptionsDialog::OptionsDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::OptionsDialog)
{
	ui->setupUi(this);
	setWindowModality(Qt::WindowModal);

	QSettings settings;

	ui->optCheckGrammar->setChecked(settings.value("cg3/checkgrammar", true).toBool());
	ui->optPreviewOutput->setChecked(settings.value("cg3/previewoutput", true).toBool());
	ui->optBinary->setText(settings.value("cg3/binary", "").toString());
	ui->optLiveDelay->setText(settings.value("cg3/livedelay", 2000).toString());
	ui->optMaxInputLines->setText(settings.value("cg3/maxinputlines", 1000).toString());
	ui->optMaxInputChars->setText(settings.value("cg3/maxinputchars", 60000).toString());

	bin_auto = settings.value("cg3/autodetect", true).toBool();
	updateRevision(settings.value("cg3/binary", "").toString());

	auto& ge = *dynamic_cast<GrammarEditor*>(parent);
	const auto& font = ge.ui->editGrammar->font();
	ui->editFont->setFont(font);
	ui->editFont->setText(QString("%1 (%2pt%3%4)").arg(font.family()).arg(font.pointSize()).arg(font.bold() ? tr(", bold") : "").arg(font.italic() ? tr(", italic") : ""));

	const auto& gh = *ge.stxGrammar.data();
	for (int i=0 ; i<gh.fmt_desc.size() ; ++i) {
		auto& desc = gh.fmt_desc[i];
		auto nf = font;
		auto lbl = new QLabel(desc[1]);
		auto edt = new QLineEdit(desc[2]);
		auto hbox = new QHBoxLayout;

		auto col = new QPushButton(tr("Change Color"));
		hbox->addWidget(col);
		connect(col, SIGNAL(clicked()), this, SLOT(colorClicked()));

		hbox->addWidget(new QLabel(QString("<b>%1</b>:").arg(tr("Bold"))));

		auto bld = new QCheckBox;
		bld->setTristate(true);
		hbox->addWidget(bld);
		connect(bld, SIGNAL(stateChanged(int)), this, SLOT(boldToggled(int)));

		hbox->addWidget(new QLabel(QString("<i>%1</i>:").arg(tr("Italic"))));

		auto ita = new QCheckBox;
		ita->setTristate(true);
		hbox->addWidget(ita);
		connect(ita, SIGNAL(stateChanged(int)), this, SLOT(italicToggled(int)));

		QPalette pl;
		pl.setColor(QPalette::Text, settings.value(QString("editor/highlight_%1_color").arg(desc[0]), desc[3]).value<QColor>());
		edt->setPalette(pl);

		int bld_s = settings.value(QString("editor/highlight_%1_bold").arg(desc[0]), desc[5]).toInt();
		bool bld_v = (bld_s == 1) ? font.bold() : (bld_s == 2);
		nf.setBold(bld_v);
		bld->setCheckState(static_cast<Qt::CheckState>(bld_s));

		int ita_s = settings.value(QString("editor/highlight_%1_italic").arg(desc[0]), desc[4]).toInt();
		bool ita_v = (ita_s == 1) ? font.bold() : (ita_s == 2);
		nf.setItalic(ita_v);
		ita->setCheckState(static_cast<Qt::CheckState>(ita_s));

		edt->setFont(nf);
		ui->gridSyntax->addWidget(lbl, i+1, 0);
		ui->gridSyntax->addWidget(edt, i+1, 1);
		ui->gridSyntax->addLayout(hbox, i+1, 2);

		cols.push_back(col);
		blds.push_back(bld);
		itas.push_back(ita);
	}
}

OptionsDialog::~OptionsDialog() {
}

void OptionsDialog::accept() {
	QSettings settings;

	settingSetOrDef(settings, "cg3/checkgrammar", true, ui->optCheckGrammar->isChecked());
	settingSetOrDef(settings, "cg3/previewoutput", true, ui->optPreviewOutput->isChecked());
	settingSetOrDef(settings, "cg3/binary", QString(""), ui->optBinary->text().trimmed());
	settingSetOrDef(settings, "cg3/autodetect", true, bin_auto);

	int delay = std::max(ui->optLiveDelay->text().trimmed().toInt(), 150);
	settingSetOrDef(settings, "cg3/livedelay", 2000, delay);
	int lines = std::max(ui->optMaxInputLines->text().trimmed().toInt(), 20);
	settingSetOrDef(settings, "cg3/maxinputlines", 1000, lines);
	int chars = std::max(ui->optMaxInputChars->text().trimmed().toInt(), 500);
	settingSetOrDef(settings, "cg3/maxinputchars", 60000, chars);

	settingSetOrDef(settings, "editor/font", QString(""), ui->editFont->font().toString());

	const auto& gh = *dynamic_cast<GrammarEditor*>(parent())->stxGrammar.data();
	for (int i=0 ; i<gh.fmt_desc.size() ; ++i) {
		auto& desc = gh.fmt_desc[i];
		auto edt = dynamic_cast<QLineEdit*>(ui->gridSyntax->itemAtPosition(i+1, 1)->widget());
		settingSetOrDef(settings, QString("editor/highlight_%1_color").arg(desc[0]), desc[3], edt->palette().text().color().name());
		settingSetOrDef(settings, QString("editor/highlight_%1_bold").arg(desc[0]), QVariant(desc[5]).toInt(), static_cast<int>(blds[i]->checkState()));
		settingSetOrDef(settings, QString("editor/highlight_%1_italic").arg(desc[0]), QVariant(desc[4]).toInt(), static_cast<int>(itas[i]->checkState()));
	}

	dynamic_cast<GrammarEditor*>(parent())->reOptions();

	close();
}

void OptionsDialog::updateRevision(const QString& filename) {
	size_t ver = 0;
	if (!filename.isEmpty()) {
		ver = queryCG3Version(filename);
	}
	if (ver) {
		ui->lblRevision->setText(QString("<i>%1: %2</i>").arg(tr("CG-3 Revision")).arg(ver));
	}
	else {
		ui->lblRevision->setText(QString("<i>%1: (%2)</i>").arg(tr("CG-3 Revision").arg(tr("no valid binary"))));
	}
}

void OptionsDialog::colorClicked() {
	for (int i=0 ; i<cols.size() ; ++i) {
		if (cols[i] == sender()) {
			const auto& gh = *dynamic_cast<GrammarEditor*>(parent())->stxGrammar.data();
			auto edt = dynamic_cast<QLineEdit*>(ui->gridSyntax->itemAtPosition(i+1, 1)->widget());
			auto col = QColorDialog::getColor(edt->palette().text().color(), this, tr("Select color for \"%1\"").arg(gh.fmt_desc[i][1]));
			if (col.isValid()) {
				QPalette pl;
				pl.setColor(QPalette::Text, col);
				edt->setPalette(pl);
			}
		}
	}
}

void OptionsDialog::boldToggled(int state) {
	for (int i=0 ; i<blds.size() ; ++i) {
		if (blds[i] == sender()) {
			auto edt = dynamic_cast<QLineEdit*>(ui->gridSyntax->itemAtPosition(i+1, 1)->widget());
			auto font = edt->font();
			font.setBold((state == Qt::PartiallyChecked) ? ui->editFont->font().bold() : (state == Qt::Checked));
			edt->setFont(font);
		}
	}
}

void OptionsDialog::italicToggled(int state) {
	for (int i=0 ; i<itas.size() ; ++i) {
		if (itas[i] == sender()) {
			auto edt = dynamic_cast<QLineEdit*>(ui->gridSyntax->itemAtPosition(i+1, 1)->widget());
			auto font = edt->font();
			font.setItalic((state == Qt::PartiallyChecked) ? ui->editFont->font().italic() : (state == Qt::Checked));
			edt->setFont(font);
		}
	}
}

void OptionsDialog::on_btnBinaryManual_clicked(bool) {
#if defined(Q_OS_MAC)
	auto filename = QFileDialog::getOpenFileName(this, tr("Locate vislcg3"), ui->optBinary->text(), tr("CG-3 Binary (* *.*)"));
#else
	auto filename = QFileDialog::getOpenFileName(this, tr("Locate vislcg3"), ui->optBinary->text(), tr("CG-3 Binary (cg3 cg3.exe vislcg3 vislcg3.exe);;Any File (*.*)"));
#endif
	if (filename.isEmpty()) {
		return;
	}

	size_t ver = queryCG3Version(filename);
	if (!ver) {
		QMessageBox::information(this, tr("Invalid CG-3!"), tr("The proposed binary \"%1\" was not suitable!").arg(filename));
		return;
	}

	bin_auto = false;
	ui->optBinary->setText(filename);
	updateRevision(filename);
}

void OptionsDialog::on_btnBinaryAuto_clicked(bool) {
	bin_auto = true;

	auto latest = findLatestCG3();
	if (latest.isEmpty()) {
		QMessageBox::information(this, tr("No CG-3 binary!"), tr("Could not locate a suitable CG-3 binary in $PATH or custom locations!"));
		return;
	}
	ui->optBinary->setText(latest);
	updateRevision(latest);
}

void OptionsDialog::on_btnFont_clicked(bool) {
	bool ok = false;
	auto font = QFontDialog::getFont(&ok, ui->editFont->font(), this);
	if (ok) {
		ui->editFont->setFont(font);
		ui->editFont->setText(QString("%1 (%2pt%3%4)").arg(font.family()).arg(font.pointSize()).arg(font.bold() ? tr(", bold") : "").arg(font.italic() ? tr(", italic") : ""));

		const auto& gh = *dynamic_cast<GrammarEditor*>(parent())->stxGrammar.data();
		for (int i=0 ; i<gh.fmt_desc.size() ; ++i) {
			auto edt = dynamic_cast<QLineEdit*>(ui->gridSyntax->itemAtPosition(i+1, 1)->widget());
			auto nf = font;
			if (blds[i]->checkState() != Qt::PartiallyChecked) {
				nf.setBold(blds[i]->checkState() == Qt::Checked);
			}
			if (itas[i]->checkState() != Qt::PartiallyChecked) {
				nf.setItalic(itas[i]->checkState() == Qt::Checked);
			}
			edt->setFont(nf);
		}
	}
}
