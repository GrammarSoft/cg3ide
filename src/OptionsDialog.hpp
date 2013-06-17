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
    explicit OptionsDialog(QWidget *parent = 0);
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
