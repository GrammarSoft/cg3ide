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

#ifndef PROCESSOR_HPP
#define PROCESSOR_HPP

#include <QtWidgets>

namespace Ui {
class Processor;
}

class Processor : public QWidget {
    Q_OBJECT
    
public:
    explicit Processor(const QString&);
    ~Processor();

    bool addInputFile(const QString&);
    void setBinary(const QString&);
    void setGrammar(const QString&);
    void setPipe(const QString&);
    void setOutputFile(const QString&);
    void setOutputSplit(bool);

public slots:
    void doIt();

protected:
    void closeEvent(QCloseEvent *event);

private slots:
    void timer_timeout();
    void process_started();
    void process_error(QProcess::ProcessError);
    void process_finished(int, QProcess::ExitStatus);
    void process_readyReadStandardOutput();
    void process_readyReadStandardError();
    void pipe_started();
    void pipe_error(QProcess::ProcessError);
    void pipe_finished(int, QProcess::ExitStatus);
    void pipe_readyReadStandardError();

    void on_btnAbort_clicked(bool);
    void on_btnAbortForce_clicked(bool);
    void on_btnClose_clicked(bool);

private:
    QScopedPointer<Ui::Processor> ui;
    QFileInfoList inputs, outputs;
    QFile input, output;
    QByteArray input_buffer;
    qint64 input_size, input_offset;
    QStringList args;
    QString binary;
    QString pipes;
    QString output_name;
    QScopedPointer<QProcess> process, pipe;
    QScopedPointer<QTimer> timer;
    bool split;
};

#endif // PROCESSOR_HPP
