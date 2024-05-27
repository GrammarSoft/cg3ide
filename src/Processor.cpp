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

#include "Processor.hpp"
#include "ui_Processor.h"
#include "inlines.hpp"

Processor::Processor(const QString& paramname) :
	ui(new Ui::Processor),
	input_buffer(32768, 0),
	input_size(0),
	input_offset(0),
	split(false)
{
	ui->setupUi(this);

	ui->prgProgress->hide();
	ui->editLogPipe->hide();

	int tabwidth = QFontMetrics(ui->editLog->document()->defaultFont()).horizontalAdvance('x')*3;
	ui->editLog->setTabStopDistance(tabwidth);
	ui->editLogPipe->setTabStopDistance(tabwidth);
	ui->editLogCG->setTabStopDistance(tabwidth);

	QFile paramf(paramname);
	if (!paramf.open(QIODevice::ReadOnly)) {
		QMessageBox::critical(nullptr, tr("Bad Param Data!"), tr("Could not read %1!").arg(paramname));
		throw(-1);
	}

	QTextStream paramt(&paramf);
	setEncoding(paramt);

	while (!paramt.atEnd()) {
		auto tmp = paramt.readLine();
		tmp = tmp.trimmed();
		if (!tmp.isEmpty() && tmp.at(0) != '#' && tmp.contains('\t')) {
			auto ls = tmp.split('\t');
			if (ls.at(0) == "binary") {
				setBinary(ls.at(1));
			}
			else if (ls.at(0) == "grammar") {
				setGrammar(ls.at(1));
			}
			else if (ls.at(0) == "inputs") {
				auto ss = ls.at(1).split("|");
				for (auto& s : ss) {
					addInputFile(s);
				}
			}
			else if (ls.at(0) == "pipe") {
				setPipe(ls.at(1));
			}
			else if (ls.at(0) == "output_file") {
				setOutputFile(ls.at(1));
			}
			else if (ls.at(0) == "output_split") {
				setOutputSplit(QVariant(ls.at(1)).toBool());
			}
		}
	}

	setWindowTitle(QFileInfo(paramname).fileName() + tr(" - CG-3 IDE Processor"));
}

Processor::~Processor() {
}

void Processor::closeEvent(QCloseEvent *event) {
	on_btnAbortForce_clicked(true);
	QWidget::closeEvent(event);
}

bool Processor::addInputFile(const QString& name) {
	QFileInfo info(name);
	if (info.exists() && info.isReadable() && info.isFile()) {
		inputs.append(info);
		input_size += info.size();
		ui->prgProgress->setMaximum(input_size);
		ui->prgProgress->show();
		ui->editLog->appendPlainText(tr("Added file %1 as input").arg(name));
		return true;
	}
	return false;
}

void Processor::setBinary(const QString& b) {
	binary = b;
}

void Processor::setGrammar(const QString& g) {
	args = QStringList() << "-v" << "-g" << g;
}

void Processor::setPipe(const QString& p) {
	pipes = p;
}

void Processor::setOutputFile(const QString& o) {
	output_name = o;
}

void Processor::setOutputSplit(bool state) {
	split = state;
}

void Processor::doIt() {
	if (output_name.isEmpty()) {
		output_name = QFileDialog::getSaveFileName(this, tr("Output To"), inputs.empty() ? "" : inputs.front().filePath(), tr("Any File (*.*)"));
		if (output_name.isEmpty()) {
			QMessageBox::critical(this, tr("Output to nowhere?"), tr("You must select somewhere to output to."));
			close();
			return;
		}
	}

	process.reset(new QProcess);
	connect(process.data(), SIGNAL(started()), this, SLOT(process_started()));
	connect(process.data(), SIGNAL(error(QProcess::ProcessError)), this, SLOT(process_error(QProcess::ProcessError)));
	connect(process.data(), SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(process_finished(int,QProcess::ExitStatus)));
	connect(process.data(), SIGNAL(readyReadStandardOutput()), this, SLOT(process_readyReadStandardOutput()));
	connect(process.data(), SIGNAL(readyReadStandardError()), this, SLOT(process_readyReadStandardError()));
	process->setWorkingDirectory(QDir::tempPath());

	if (!pipes.isEmpty()) {
		ui->editLogPipe->show();
		pipe.reset(new QProcess);
		connect(pipe.data(), SIGNAL(started()), this, SLOT(pipe_started()));
		connect(pipe.data(), SIGNAL(error(QProcess::ProcessError)), this, SLOT(pipe_error(QProcess::ProcessError)));
		connect(pipe.data(), SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(pipe_finished(int,QProcess::ExitStatus)));
		connect(pipe.data(), SIGNAL(readyReadStandardError()), this, SLOT(readyReadStandardError()));
		pipe->setWorkingDirectory(QDir::tempPath());
		pipe->setStandardOutputProcess(process.data());
		#if defined(Q_OS_WIN)
		pipe->start("cmd", QStringList() << "/D" << "/Q" << "/C" << pipes);
		#else
		pipe->start("/bin/sh", QStringList() << "-c" << pipes);
		#endif
	}

	process->start(binary, args);

	if (!inputs.empty()) {
		timer.reset(new QTimer);
		connect(timer.data(), SIGNAL(timeout()), this, SLOT(timer_timeout()));
		timer->setSingleShot(true);
		timer->start(100);
	}
}

void Processor::timer_timeout() {
	auto p = pipe ? pipe.data() : process.data();

	if (p->state() == 0) {
		ui->editLog->appendPlainText(tr("The pipe / process ended prematurely"));
		return;
	}

	if (input.isOpen()) {
		qint64 pos = 0;
		if ((pos = input.read(&input_buffer.data()[input_offset], input_buffer.size()-input_offset)) == 0) {
			if (split) {
				input_buffer.append("\n<STREAMCMD:FLUSH>\n");
			}
			input.close();
			ui->editLog->appendPlainText(tr("Closed input file %1").arg(input.fileName()));
		}
		input_offset += pos;
	}

	auto written = p->write(input_buffer.data(), input_offset);
	if (written == 0) {
	}
	else if (written == -1) {
		ui->editLog->appendPlainText(tr("Failed to write data to pipe / process"));
	}
	else if (written == input_offset) {
		input_offset = 0;
	}
	else {
		input_buffer = input_buffer.mid(written, input_offset-written);
		input_buffer.resize(32768);
	}

	if (!input.isOpen() && !inputs.empty()) {
		auto file = inputs.front().filePath();
		input.setFileName(file);
		if (!input.open(QIODevice::ReadOnly)) {
			ui->editLog->appendPlainText(tr("Failed to open input file %1").arg(file));
			return;
		}
		ui->editLog->appendPlainText(tr("Opened input file %1").arg(file));
		outputs.append(inputs.front());
		inputs.pop_front();
	}

	if (input.isOpen()) {
		timer->start(100);
	}
	else {
		p->closeWriteChannel();
	}
}

void Processor::process_started() {
	ui->editLogCG->appendPlainText(tr("Launched CG-3"));
}

void Processor::process_error(QProcess::ProcessError error) {
	ui->editLogCG->appendPlainText(tr("CG-3 reported error %1").arg(error));
}

void Processor::process_finished(int code, QProcess::ExitStatus status) {
	ui->editLogCG->appendPlainText(tr("CG-3 exited with code %1 and status %2").arg(code).arg(status));

	process_readyReadStandardOutput();
	output.close();
	ui->editLog->appendPlainText(tr("Closed output file %1").arg(output.fileName()));

	if (code == 0 && status == QProcess::NormalExit) {
		ui->editLogCG->appendPlainText("\n" + tr("All done!"));
		ui->editLog->appendPlainText("\n" + tr("All done!"));
	}
}

void Processor::process_readyReadStandardOutput() {
	if (!output.isOpen() && !outputs.isEmpty()) {
		auto file = output_name;
		if (split) {
			file = QFileInfo(output_name).path() + outputs.front().fileName() + QFileInfo(output_name).fileName();
		}
		output.setFileName(file);
		if (!output.open(QIODevice::WriteOnly|QIODevice::Truncate)) {
			ui->editLog->appendPlainText(tr("Failed to open output file %1").arg(file));
			on_btnAbortForce_clicked(true);
			return;
		}
		ui->editLog->appendPlainText(tr("Opened output file %1").arg(file));
		outputs.pop_front();
	}

	process->setReadChannel(QProcess::StandardOutput);
	QString line;
	while (!(line = process->readLine(32768)).isEmpty()) {
		output.write(line.toUtf8());
		if (split && line.at(0) == '<' && line == "<STREAMCMD:FLUSH>\n") {
			output.close();
			ui->editLog->appendPlainText(tr("Closed output file %1").arg(output.fileName()));
		}
	}
}

void Processor::process_readyReadStandardError() {
	process->setReadChannel(QProcess::StandardError);
	QString line;
	while (!(line = process->readLine(32768)).isEmpty()) {
		ui->editLogCG->appendPlainText(line.trimmed());
	}
}

void Processor::pipe_started() {
	ui->editLogPipe->appendPlainText(tr("Launched pipe"));
}

void Processor::pipe_error(QProcess::ProcessError error) {
	ui->editLogPipe->appendPlainText(tr("Pipe reported error %1").arg(error));
}

void Processor::pipe_finished(int code, QProcess::ExitStatus status) {
	ui->editLogPipe->appendPlainText(tr("Pipe exited with code %1 and status %2").arg(code).arg(status));
}

void Processor::pipe_readyReadStandardError() {
	pipe->setReadChannel(QProcess::StandardError);
	QString line;
	while (!(line = pipe->readLine(32768)).isEmpty()) {
		ui->editLogPipe->appendPlainText(line.trimmed());
	}
}

void Processor::on_btnAbort_clicked(bool) {
	if (pipe) {
		ui->editLog->appendPlainText(tr("Sending friendly terminate signal to pipe..."));
		pipe->terminate();
	}
	if (process) {
		ui->editLog->appendPlainText(tr("Sending friendly terminate signal to CG-3..."));
		process->terminate();
	}
}

void Processor::on_btnAbortForce_clicked(bool) {
	on_btnAbort_clicked(true);
	if (pipe) {
		ui->editLog->appendPlainText(tr("Sending kill signal to pipe..."));
		pipe->kill();
	}
	if (process) {
		ui->editLog->appendPlainText(tr("Sending kill signal to CG-3..."));
		process->kill();
	}
}

void Processor::on_btnClose_clicked(bool) {
	on_btnAbortForce_clicked(true);
	close();
}

int main(int argc, char *argv[]) {
	QApplication app(argc, argv);

	app.setOrganizationDomain("grammarsoft.com");
	app.setOrganizationName("GrammarSoft ApS");
	app.setApplicationName("CG-3 IDE Processor");
	app.setQuitOnLastWindowClosed(true);

	QSettings::setDefaultFormat(QSettings::IniFormat);

	auto args = app.arguments();
	args.pop_front();

	if (args.empty()) {
		QMessageBox::critical(nullptr, "Missing params file!", "The first and only argument to this program must be a file with parameters!");
		return -1;
	}

	auto w = new Processor(args.first());
	w->show();

	QTimer::singleShot(250, w, SLOT(doIt()));

	return app.exec();
}
