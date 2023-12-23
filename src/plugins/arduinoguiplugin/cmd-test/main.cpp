#include "mainwindow.h"

#include <QApplication>
#include <QProcess>
#include <QDebug>

void readTest() {
    QProcess process;
//    process.start("type", QStringList{"whereis"});
    process.startDetached( "/home/linuxbrew/.linuxbrew/bin/arduino-cli" );
//    process.start("find /home -type f -name file-to-search");
    if( !process.waitForStarted() || !process.waitForFinished() ) {
        return;
    }

    qDebug() << process.readAllStandardError();
    qDebug() << process.readAllStandardOutput();
}

void qprocTest(QApplication a){
    auto proc = new QProcess(a.instance());

    auto stdOutReadyCallBack = [&proc](){
        auto output = QString(proc->readAllStandardOutput());
        qDebug() << output;

        proc->close();
        delete proc;
    };

    auto errorOccuredCallback = [&proc](QProcess::ProcessError error){
        switch (error) {
        case QProcess::FailedToStart:
            qDebug() << "The process failed to start.";
            break;
        case QProcess::Crashed:
            qDebug() << "The process crashed some time after starting successfully.";
            break;
        case QProcess::Timedout:
            qDebug() << "The last waitFor...() function timed out";
            break;
        case QProcess::WriteError:
            qDebug() << "An error occurred when attempting to write to the process.";
            break;
        case QProcess::ReadError:
            qDebug() << "An error occurred when attempting to read from the process.";
            break;
        case QProcess::UnknownError:
            qDebug() << "An unknown error occurred. ";
            break;
        default:
            break;
        }
    };

    auto stateChangedCallback = [&proc](QProcess::ProcessState state){
        switch (state) {
        case QProcess::NotRunning:
            qDebug() << "The process is not running.";
            break;
        case QProcess::Starting:
            qDebug() << "The process is starting, but the program has not yet been invoked.";
            break;
        case QProcess::Running:
            qDebug() << "The process is running and is ready for reading and writing.";
            break;
        default:
            break;
        }

        proc->close();
    };
    QObject::connect(proc,
                     &QProcess::readyReadStandardOutput,
                     a.instance(),
                     stdOutReadyCallBack);

    QObject::connect(proc,
                     &QProcess::errorOccurred,
                     a.instance(),
                     errorOccuredCallback);

    QObject::connect(proc,
                     &QProcess::stateChanged,
                     a.instance(),
                     stateChangedCallback);

    proc->open(QProcess::ReadOnly);
    proc->setProgram("sh");
    proc->setArguments(QStringList{"-c", "arduino-cli"});
//    proc->startDetached();
    proc->startDetached("/bin/bash", QStringList{"-c", "whereis arduino-cli", "\n"});
    proc->waitForStarted();
    proc->waitForFinished();

    proc->close();
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;

//    readTest();

    w.show();
    return a.exec();
}
