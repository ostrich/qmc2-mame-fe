#include <QtTest>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFile>
#include <QPlainTextEdit>
#include <QTemporaryDir>

#include "mainwindow.h"
#include "qchdmansettings.h"
#include "scriptwidget.h"

quint64 runningProjects = 0;
quint64 runningScripts = 0;
MainWindow *mainWindow = 0;
QtChdmanGuiSettings *globalConfig = 0;

class QchdmanScriptTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void structuredResultRoundTrip();

private:
    QString runFixture(const QString &body);
    QTemporaryDir settingsDirectory;
    MainWindow *window = 0;
    ProjectWindow *scriptWindow = 0;
    ScriptWidget *scriptWidget = 0;
    QJsonArray observations;
};

void QchdmanScriptTest::initTestCase()
{
    QVERIFY2(settingsDirectory.isValid(), "could not create isolated settings directory");
    QCoreApplication::setOrganizationName(QStringLiteral("qmc2-tests"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("invalid.example"));
    QCoreApplication::setApplicationName(QStringLiteral("qchdman-script-test"));
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsDirectory.path());
    globalConfig = new QtChdmanGuiSettings;
    globalConfig->setApplicationVersion(QCHDMAN_APP_VERSION);
    window = new MainWindow;
    mainWindow = window;
    scriptWindow = window->createProjectWindow(QCHDMAN_MDI_SCRIPT);
    QVERIFY(scriptWindow);
    scriptWidget = qobject_cast<ScriptWidget *>(scriptWindow->widget());
    QVERIFY(scriptWidget);
}

void QchdmanScriptTest::cleanupTestCase()
{
    const QString resultPath = qEnvironmentVariable("QCHDMAN_TEST_RESULTS");
    if (!resultPath.isEmpty()) {
        QFile resultFile(resultPath);
        QVERIFY2(resultFile.open(QIODevice::WriteOnly | QIODevice::Truncate),
                 qPrintable(resultFile.errorString()));
        QJsonObject document;
        document.insert(QStringLiteral("format"), 1);
        document.insert(QStringLiteral("fixtures"), observations);
        resultFile.write(QJsonDocument(document).toJson(QJsonDocument::Indented));
    }
    window->mdiArea()->removeSubWindow(scriptWindow);
    delete scriptWindow;
    scriptWindow = 0;
    scriptWidget = 0;
    delete window;
    window = 0;
    mainWindow = 0;
    delete globalConfig;
    globalConfig = 0;
}

QString QchdmanScriptTest::runFixture(const QString &body)
{
    const QList<QPlainTextEdit *> editors = scriptWidget->findChildren<QPlainTextEdit *>();
    QPlainTextEdit *log = 0;
    for (QPlainTextEdit *editor : editors) {
        if (editor->isReadOnly()) {
            log = editor;
            break;
        }
    }
    if (!log) {
        QTest::qFail("could not locate the production script log", __FILE__, __LINE__);
        return QString();
    }
    log->clear();
    scriptWidget->engine()->runScript(body);
    QCoreApplication::processEvents();
    const QString prefix = QStringLiteral("QCHDMAN_TEST_RESULT ");
    for (const QString &line : log->toPlainText().split(QLatin1Char('\n'))) {
        if (line.contains(prefix))
            return line.mid(line.indexOf(prefix) + prefix.size());
    }
    return QString();
}

void QchdmanScriptTest::structuredResultRoundTrip()
{
    const QString json = runFixture(QStringLiteral(
        "scriptEngine.log('QCHDMAN_TEST_RESULT ' + JSON.stringify({"
        "value: 42, ok: true,"
        "same: scriptEngine === qchdman,"
        "hasLog: typeof scriptEngine.log === 'function',"
        "hasProjectCreate: typeof qchdman.projectCreate === 'function'"
        "}));"));
    QVERIFY2(!json.isEmpty(), "fixture did not report a structured result");
    const QJsonDocument document = QJsonDocument::fromJson(json.toUtf8());
    QVERIFY(document.isObject());
    QCOMPARE(document.object().value(QStringLiteral("value")).toInt(), 42);
    QCOMPARE(document.object().value(QStringLiteral("ok")).toBool(), true);
    QCOMPARE(document.object().value(QStringLiteral("same")).toBool(), false);
    QCOMPARE(document.object().value(QStringLiteral("hasLog")).toBool(), true);
    QCOMPARE(document.object().value(QStringLiteral("hasProjectCreate")).toBool(), true);
    QJsonObject observation;
    observation.insert(QStringLiteral("id"), QStringLiteral("globals/structured-result"));
    observation.insert(QStringLiteral("result"), document.object());
    observations.append(observation);
}

QTEST_MAIN(QchdmanScriptTest)
#include "tst_qchdman_script.moc"
