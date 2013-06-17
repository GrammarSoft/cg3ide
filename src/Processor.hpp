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
