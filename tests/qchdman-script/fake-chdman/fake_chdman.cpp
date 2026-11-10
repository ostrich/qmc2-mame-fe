#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QElapsedTimer>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLockFile>
#include <QProcessEnvironment>
#include <QTextStream>
#include <QThread>

#ifdef Q_OS_WIN
#include <qt_windows.h>
#else
#include <csignal>
#endif

static void writeText(FILE *stream, const QString &text)
{
    QTextStream output(stream, QIODevice::WriteOnly);
    output << text;
    output.flush();
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    const QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    const QString recordPath = environment.value(QStringLiteral("QCHDMAN_FAKE_RECORD"));
    const QString mode = environment.value(QStringLiteral("QCHDMAN_FAKE_MODE"), QStringLiteral("success"));
    int delay = environment.value(QStringLiteral("QCHDMAN_FAKE_DELAY_MS"), QStringLiteral("0")).toInt();
#ifndef Q_OS_WIN
    if (mode == QStringLiteral("wait"))
        std::signal(SIGTERM, SIG_IGN);
#endif

    QJsonArray arguments;
    const QStringList applicationArguments = app.arguments();
    for (int i = 1; i < applicationArguments.size(); ++i)
        arguments.append(applicationArguments.at(i));
    if (applicationArguments.value(1) == QStringLiteral("verify"))
        delay += environment.value(QStringLiteral("QCHDMAN_FAKE_VERIFY_STAGGER_MS"), QStringLiteral("0")).toInt();

    if (!recordPath.isEmpty()) {
        // QFile's append mode does not serialize independent Windows writers.
        // Hold the lock until the complete record has been flushed and closed.
        QLockFile lock(recordPath + QStringLiteral(".lock"));
        if (!lock.tryLock(30000))
            return 125;
        QFile record(recordPath);
        if (!record.open(QIODevice::WriteOnly | QIODevice::Append))
            return 125;
        QJsonObject invocation;
        invocation.insert(QStringLiteral("arguments"), arguments);
        invocation.insert(QStringLiteral("mode"), mode);
        const QByteArray line = QJsonDocument(invocation).toJson(QJsonDocument::Compact) + '\n';
        if (record.write(line) != line.size() || !record.flush())
            return 125;
        record.close();
    }

    if (delay > 0)
        QThread::msleep(static_cast<unsigned long>(delay));

    const QString verifyGate = environment.value(QStringLiteral("QCHDMAN_FAKE_VERIFY_GATE"));
    if (applicationArguments.value(1) == QStringLiteral("verify") && !verifyGate.isEmpty()) {
        QElapsedTimer deadline;
        deadline.start();
        while (!QFileInfo::exists(verifyGate)) {
            if (deadline.elapsed() >= 30000)
                return 124;
            QThread::msleep(5);
        }
    }

    const QString stdoutText = environment.value(QStringLiteral("QCHDMAN_FAKE_STDOUT"));
    const QString stderrText = environment.value(QStringLiteral("QCHDMAN_FAKE_STDERR"));
    if (!stdoutText.isEmpty())
        writeText(stdout, stdoutText);
    if (!stderrText.isEmpty())
        writeText(stderr, stderrText);

    if (mode == QStringLiteral("progress")) {
        writeText(stdout, QStringLiteral("Compressing, 25.0% complete\nCompressing, 100.0% complete\n"));
        return 0;
    }
    if (mode == QStringLiteral("crash")) {
#ifdef Q_OS_WIN
        // Qt classifies NT exception status codes as QProcess::CrashExit. Using
        // ExitProcess avoids Windows Error Reporting blocking an unattended run.
        ExitProcess(0xC0000005u);
#else
        qFatal("fake chdman crash requested");
#endif
    }
    if (mode == QStringLiteral("wait")) {
        // On Unix the handler above exercises qchdman's forced-stop fallback.
        // Windows console processes already ignore QProcess::terminate() when
        // they do not own a window or run a message loop.
        for (;;)
            QThread::msleep(100);
    }
    if (mode == QStringLiteral("exit"))
        return environment.value(QStringLiteral("QCHDMAN_FAKE_EXIT_CODE"), QStringLiteral("1")).toInt();
    return 0;
}
