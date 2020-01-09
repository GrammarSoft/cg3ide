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

#pragma once
#ifndef OPTIONSDIALOG_HPP_cc7194f1bd3a13d1dca4d5a1c31f83d81877a7f7
#define OPTIONSDIALOG_HPP_cc7194f1bd3a13d1dca4d5a1c31f83d81877a7f7

#include <QtWidgets>

namespace Ui {
class OptionsDialog;
}

class OptionsDialog : public QDialog {
    Q_OBJECT

public:
    explicit OptionsDialog(QWidget *parent = nullptr);
    ~OptionsDialog();

private slots:
    void accept();
    void colorClicked();
    void italicToggled(int);
    void boldToggled(int);

    void on_btnBinaryManual_clicked(bool);
    void on_btnBinaryAuto_clicked(bool);

    void on_btnFont_clicked(bool);

private:
    void updateRevision(const QString&);

    QScopedPointer<Ui::OptionsDialog> ui;
    bool bin_auto;

    QVector<QPushButton*> cols;
    QVector<QCheckBox*> blds;
    QVector<QCheckBox*> itas;
};

#endif // OPTIONSDIALOG_HPP_cc7194f1bd3a13d1dca4d5a1c31f83d81877a7f7
