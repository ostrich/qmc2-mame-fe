#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcessEnvironment>
#include <QTextStream>
#include <QThread>

#ifdef Q_OS_WIN
#include <qt_windows.h>
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

    QJsonArray arguments;
    const QStringList applicationArguments = app.arguments();
    for (int i = 1; i < applicationArguments.size(); ++i)
        arguments.append(applicationArguments.at(i));
    if (applicationArguments.value(1) == QStringLiteral("verify"))
        delay += environment.value(QStringLiteral("QCHDMAN_FAKE_VERIFY_STAGGER_MS"), QStringLiteral("0")).toInt();

    if (!recordPath.isEmpty()) {
        QFile record(recordPath);
        if (!record.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
            return 125;
        QJsonObject invocation;
        invocation.insert(QStringLiteral("arguments"), arguments);
        invocation.insert(QStringLiteral("mode"), mode);
        record.write(QJsonDocument(invocation).toJson(QJsonDocument::Compact));
        record.write("\n");
    }

    if (delay > 0)
        QThread::msleep(static_cast<unsigned long>(delay));

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
        for (;;)
            QThread::msleep(100);
    }
    if (mode == QStringLiteral("exit"))
        return environment.value(QStringLiteral("QCHDMAN_FAKE_EXIT_CODE"), QStringLiteral("1")).toInt();
    return 0;
}
