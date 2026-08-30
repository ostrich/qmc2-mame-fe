#include <QtTest>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFile>
#include <QPlainTextEdit>
#include <QTemporaryDir>
#include <QMetaMethod>
#include <QRegularExpression>
#include <QFileDialog>
#include <QInputDialog>
#include <QQueue>
#include <QTimer>
#include <algorithm>
#include <functional>

#include "mainwindow.h"
#include "qchdmansettings.h"
#include "scriptwidget.h"

quint64 runningProjects = 0;
quint64 runningScripts = 0;
MainWindow *mainWindow = 0;
QtChdmanGuiSettings *globalConfig = 0;

class DialogAutomator : public QObject
{
    Q_OBJECT

public:
    enum Kind { File, Folder, Text, Item, Integer, Double };
    struct Action { QString title; Kind kind; QVariant value; bool accept; };

    void enqueue(const QString &title, Kind kind, const QVariant &value, bool accept = true)
    {
        actions.enqueue(Action{title, kind, value, accept});
    }

    void start() { timer.start(5, this); }
    bool finished() const { return actions.isEmpty(); }

protected:
    void timerEvent(QTimerEvent *event) override
    {
        if (event->timerId() != timer.timerId() || actions.isEmpty())
            return;
        const Action action = actions.head();
        QStringList visibleDialogs;
        for (QWidget *widget : QApplication::topLevelWidgets()) {
            QDialog *dialog = qobject_cast<QDialog *>(widget);
            if (!dialog || !dialog->isVisible())
                continue;
            visibleDialogs.append(QStringLiteral("%1:%2").arg(QString::fromLatin1(dialog->metaObject()->className()), dialog->windowTitle()));
            if (dialog->windowTitle() != action.title)
                continue;
            if (!action.accept) {
                dialog->reject();
            } else if (QFileDialog *fileDialog = qobject_cast<QFileDialog *>(dialog)) {
                if (action.kind == Folder)
                    fileDialog->setDirectory(action.value.toString());
                fileDialog->selectFile(action.value.toString());
                dialog->done(QDialog::Accepted);
            } else if (QInputDialog *inputDialog = qobject_cast<QInputDialog *>(dialog)) {
                switch (action.kind) {
                case Text:
                case Item: inputDialog->setTextValue(action.value.toString()); break;
                case Integer: inputDialog->setIntValue(action.value.toInt()); break;
                case Double: inputDialog->setDoubleValue(action.value.toDouble()); break;
                default: break;
                }
                inputDialog->accept();
            }
            actions.dequeue();
            if (actions.isEmpty())
                timer.stop();
            return;
        }
        const QString signature = action.title + QLatin1Char('|') + visibleDialogs.join(QLatin1Char(','));
        if (signature != lastDiagnostic) {
            lastDiagnostic = signature;
            qWarning().noquote() << "waiting for dialog" << action.title << "visible:" << visibleDialogs;
        }
    }

private:
    QBasicTimer timer;
    QQueue<Action> actions;
    QString lastDiagnostic;
};

class QchdmanScriptTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void coreUtilities();
    void languageRuntimeBridge();
    void repeatedRuns();
    void inputDialogs();
    void versionOnePersistence();
    void projectProperties();
    void projectLifecycleAndFailures();
    void scriptInterruptionAndStress();
    void debuggerWorkflow();
    void structuredResultRoundTrip();
    void slotManifestComplete();

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
    QApplication::setStyle(QStringLiteral("Fusion"));
    QCoreApplication::setOrganizationName(QStringLiteral("qmc2-tests"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("invalid.example"));
    QCoreApplication::setApplicationName(QStringLiteral("qchdman-script-test"));
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsDirectory.path());
    globalConfig = new QtChdmanGuiSettings;
    globalConfig->setApplicationVersion(QCHDMAN_APP_VERSION);
    globalConfig->setPreferencesChdmanBinary(QCoreApplication::applicationFilePath());
    globalConfig->setPreferencesNativeFileDialogs(false);
    window = new MainWindow;
    mainWindow = window;
    QCoreApplication::processEvents();
    scriptWindow = window->createProjectWindow(QCHDMAN_MDI_SCRIPT);
    QVERIFY(scriptWindow);
    scriptWidget = qobject_cast<ScriptWidget *>(scriptWindow->widget());
    QVERIFY(scriptWidget);
    scriptWidget->setLogLimit(100);
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

void QchdmanScriptTest::slotManifestComplete()
{
    const QString manifestPath = QFINDTESTDATA("../slot-manifest.json");
    QVERIFY2(!manifestPath.isEmpty(), "could not find slot-manifest.json");
    QFile manifestFile(manifestPath);
    QVERIFY(manifestFile.open(QIODevice::ReadOnly));
    const QJsonObject manifest = QJsonDocument::fromJson(manifestFile.readAll()).object();
    QCOMPARE(manifest.value(QStringLiteral("format")).toInt(), 1);
    const QJsonArray rules = manifest.value(QStringLiteral("rules")).toArray();
    QVERIFY(!rules.isEmpty());

    QStringList publicSlots;
    const QMetaObject *metaObject = &ScriptEngine::staticMetaObject;
    for (int index = metaObject->methodOffset(); index < metaObject->methodCount(); ++index) {
        const QMetaMethod method = metaObject->method(index);
        const QString name = QString::fromLatin1(method.name());
        if (method.methodType() == QMetaMethod::Slot && method.access() == QMetaMethod::Public
                && !publicSlots.contains(name))
            publicSlots.append(name);
    }
    QCOMPARE(publicSlots.size(), 240);

    for (const QString &slot : publicSlots) {
        QStringList fixtures;
        for (const QJsonValue &value : rules) {
            const QJsonObject rule = value.toObject();
            const QRegularExpression expression(rule.value(QStringLiteral("pattern")).toString());
            QVERIFY2(expression.isValid(), qPrintable(expression.errorString()));
            if (expression.match(slot).hasMatch())
                fixtures.append(rule.value(QStringLiteral("fixture")).toString());
        }
        QCOMPARE(fixtures.size(), 1);
        QVERIFY2(!fixtures.first().isEmpty(), qPrintable(slot));
    }
}

void QchdmanScriptTest::coreUtilities()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QFile first(directory.filePath(QStringLiteral("a.txt")));
    QFile second(directory.filePath(QStringLiteral("b.bin")));
    QVERIFY(first.open(QIODevice::WriteOnly));
    QVERIFY(second.open(QIODevice::WriteOnly));
    first.close();
    second.close();
    QVERIFY(QDir().mkpath(directory.filePath(QStringLiteral("sub"))));

    QString root = directory.path();
    root.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
    root.replace(QLatin1Char('\''), QStringLiteral("\\'"));
#if defined(Q_OS_WIN)
    const QString failingCommand = QStringLiteral("exit /b 7");
#else
    const QString failingCommand = QStringLiteral("exit 7");
#endif
    const QString script = QStringLiteral(
        "var root = '%1';"
        "var entries = scriptEngine.dirEntryList(root, '*.txt', true, true);"
        "var subdirs = scriptEngine.dirSubDirList(root, '', true, true);"
        "scriptEngine.dirStartEntryList(root, '*.txt', false);"
        "var iterated = []; while (scriptEngine.dirHasNextEntry()) iterated.push(scriptEngine.dirNextEntry());"
        "var made = scriptEngine.createPath(root + '/new/child');"
        "var removed = scriptEngine.removePath(root + '/new/child');"
        "scriptEngine.progressSetRange(10, 20); scriptEngine.progressSetValue(17);"
        "scriptEngine.dumpHardDiskTemplates();"
        "scriptEngine.log('ordinary utility log');"
        "var result = {"
        "entries: entries, subdirs: subdirs, iteratedCount: iterated.length,"
        "made: made, removed: removed, shellExit: scriptEngine.runShellCommand('%2', false),"
        "detached: scriptEngine.runShellCommand('', true),"
        "knownOS: scriptEngine.operatingSystemName().length > 0,"
        "hasCPU: scriptEngine.numberOfCPUs() > 0, progress: scriptEngine.progressGetValue()"
        "};"
        "scriptEngine.log('QCHDMAN_TEST_RESULT ' + JSON.stringify(result));")
        .arg(root, failingCommand);
    const QJsonObject result = QJsonDocument::fromJson(runFixture(script).toUtf8()).object();
    QCOMPARE(result.value(QStringLiteral("entries")).toArray(), QJsonArray{QStringLiteral("a.txt")});
    QCOMPARE(result.value(QStringLiteral("subdirs")).toArray(), QJsonArray{QStringLiteral("sub")});
    QCOMPARE(result.value(QStringLiteral("iteratedCount")).toInt(), 1);
    QCOMPARE(result.value(QStringLiteral("made")).toBool(), true);
    QCOMPARE(result.value(QStringLiteral("removed")).toBool(), true);
    QCOMPARE(result.value(QStringLiteral("shellExit")).toInt(), 7);
    QCOMPARE(result.value(QStringLiteral("detached")).toInt(), 0);
    QCOMPARE(result.value(QStringLiteral("knownOS")).toBool(), true);
    QCOMPARE(result.value(QStringLiteral("hasCPU")).toBool(), true);
    QCOMPARE(result.value(QStringLiteral("progress")).toInt(), 17);
    QJsonObject observation;
    observation.insert(QStringLiteral("id"), QStringLiteral("utilities/core"));
    observation.insert(QStringLiteral("result"), result);
    observations.append(observation);
}

void QchdmanScriptTest::languageRuntimeBridge()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QFile file(directory.filePath(QStringLiteral("bridge.txt")));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.close();
    QString root = directory.path();
    root.replace(QLatin1Char('\\'), QStringLiteral("\\\\")).replace(QLatin1Char('\''), QStringLiteral("\\'"));
    const QString script = QStringLiteral(
        "var bridgeSignal = null;"
        "function bridgeCallback(id) { bridgeSignal = id; }"
        "scriptEngine.projectStarted.connect(bridgeCallback);"
        "scriptEngine.projectStarted('signal-id');"
        "scriptEngine.projectStarted.disconnect(bridgeCallback);"
        "var bridgeCaught = false, bridgeSyntax = false, bridgeStack = false;"
        "try { throw new TypeError('caught'); } catch (e) { bridgeCaught = e instanceof TypeError; bridgeStack = typeof e.stack === 'string'; }"
        "try { eval('function ('); } catch (e) { bridgeSyntax = e instanceof SyntaxError; }"
        "eval('function bridgeNested(v) { return v + 1; }');"
        "var bridgeEntries = scriptEngine.dirEntryList('%1');"
        "if (typeof gc === 'function') gc();"
        "scriptEngine.log('QCHDMAN_TEST_RESULT ' + JSON.stringify({"
        "sameGlobal: scriptEngine === qchdman, signal: bridgeSignal,"
        "primitive: typeof scriptEngine.progressGetValue() === 'number',"
        "containerLength: bridgeEntries.length, containerJoin: bridgeEntries.join(','),"
        "defaultArguments: bridgeEntries[0] === 'bridge.txt', nested: bridgeNested(41),"
        "caught: bridgeCaught, syntax: bridgeSyntax, stackAvailable: bridgeStack,"
        "gcAvailable: typeof gc === 'function'"
        "}));").arg(root);
    const QJsonObject result = QJsonDocument::fromJson(runFixture(script).toUtf8()).object();
    QCOMPARE(result.value(QStringLiteral("sameGlobal")).toBool(), false);
    QCOMPARE(result.value(QStringLiteral("signal")).toString(), QStringLiteral("signal-id"));
    QCOMPARE(result.value(QStringLiteral("primitive")).toBool(), true);
    QCOMPARE(result.value(QStringLiteral("containerLength")).toInt(), 1);
    QCOMPARE(result.value(QStringLiteral("containerJoin")).toString(), QStringLiteral("bridge.txt"));
    QCOMPARE(result.value(QStringLiteral("defaultArguments")).toBool(), true);
    QCOMPARE(result.value(QStringLiteral("nested")).toInt(), 42);
    QCOMPARE(result.value(QStringLiteral("caught")).toBool(), true);
    QCOMPARE(result.value(QStringLiteral("syntax")).toBool(), true);
    QJsonObject observation;
    observation.insert(QStringLiteral("id"), QStringLiteral("runtime/language-bridge"));
    observation.insert(QStringLiteral("result"), result);
    observations.append(observation);
}

void QchdmanScriptTest::repeatedRuns()
{
    const QJsonObject first = QJsonDocument::fromJson(runFixture(QStringLiteral(
        "var qchdmanRepeatedCounter = (typeof qchdmanRepeatedCounter === 'undefined' ? 0 : qchdmanRepeatedCounter) + 1;"
        "scriptEngine.log('QCHDMAN_TEST_RESULT ' + JSON.stringify({counter: qchdmanRepeatedCounter}));")).toUtf8()).object();
    const QJsonObject second = QJsonDocument::fromJson(runFixture(QStringLiteral(
        "qchdmanRepeatedCounter += 1;"
        "scriptEngine.log('QCHDMAN_TEST_RESULT ' + JSON.stringify({counter: qchdmanRepeatedCounter}));")).toUtf8()).object();
    QCOMPARE(first.value(QStringLiteral("counter")).toInt(), 1);
    QCOMPARE(second.value(QStringLiteral("counter")).toInt(), 2);
    QJsonObject result;
    result.insert(QStringLiteral("first"), first);
    result.insert(QStringLiteral("second"), second);
    QJsonObject observation;
    observation.insert(QStringLiteral("id"), QStringLiteral("runtime/repeated-runs"));
    observation.insert(QStringLiteral("result"), result);
    observations.append(observation);
}

void QchdmanScriptTest::inputDialogs()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString filePath = directory.filePath(QStringLiteral("chosen.txt"));
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("fixture");
    file.close();

    DialogAutomator accepted;
    accepted.enqueue(QStringLiteral("file-title"), DialogAutomator::File, filePath);
    accepted.enqueue(QStringLiteral("folder-title"), DialogAutomator::Folder, directory.path());
    accepted.enqueue(QStringLiteral("text-title"), DialogAutomator::Text, QStringLiteral("chosen text"));
    accepted.enqueue(QStringLiteral("item-title"), DialogAutomator::Item, QStringLiteral("two"));
    accepted.enqueue(QStringLiteral("int-title"), DialogAutomator::Integer, 23);
    accepted.enqueue(QStringLiteral("double-title"), DialogAutomator::Double, 4.25);
    accepted.enqueue(QStringLiteral("cancel-file"), DialogAutomator::File, QVariant(), false);
    accepted.enqueue(QStringLiteral("cancel-folder"), DialogAutomator::Folder, QVariant(), false);
    accepted.enqueue(QStringLiteral("cancel-text"), DialogAutomator::Text, QVariant(), false);
    accepted.enqueue(QStringLiteral("cancel-item"), DialogAutomator::Item, QVariant(), false);
    accepted.enqueue(QStringLiteral("cancel-int"), DialogAutomator::Integer, QVariant(), false);
    accepted.enqueue(QStringLiteral("cancel-double"), DialogAutomator::Double, QVariant(), false);
    accepted.start();
    QString escapedFile = filePath;
    QString escapedFolder = directory.path();
    escapedFile.replace(QLatin1Char('\\'), QStringLiteral("\\\\")).replace(QLatin1Char('\''), QStringLiteral("\\'"));
    escapedFolder.replace(QLatin1Char('\\'), QStringLiteral("\\\\")).replace(QLatin1Char('\''), QStringLiteral("\\'"));
    const QString acceptedScript = QStringLiteral(
        "var r = {}, oks = [];"
        "r.file = scriptEngine.inputGetFilePath('%1', 'Text (*.txt)', 'file-title'); oks.push(scriptEngine.inputOk());"
        "r.folder = scriptEngine.inputGetFolderPath('%2', 'folder-title'); oks.push(scriptEngine.inputOk());"
        "r.text = scriptEngine.inputGetStringValue('initial', 'text-title', 'label'); oks.push(scriptEngine.inputOk());"
        "r.item = scriptEngine.inputGetListItem('one', ['one','two'], false, 'item-title', 'label'); oks.push(scriptEngine.inputOk());"
        "r.integer = scriptEngine.inputGetIntValue(1, 'int-title', 'label'); oks.push(scriptEngine.inputOk());"
        "r.doubleValue = scriptEngine.inputGetDoubleValue(1.0, 2, 'double-title', 'label'); oks.push(scriptEngine.inputOk());"
        "r.oks = oks; var cancelled = [];"
        "scriptEngine.inputGetFilePath('', '', 'cancel-file'); cancelled.push(scriptEngine.inputOk());"
        "scriptEngine.inputGetFolderPath('', 'cancel-folder'); cancelled.push(scriptEngine.inputOk());"
        "scriptEngine.inputGetStringValue('', 'cancel-text', 'label'); cancelled.push(scriptEngine.inputOk());"
        "scriptEngine.inputGetListItem('', ['one'], false, 'cancel-item', 'label'); cancelled.push(scriptEngine.inputOk());"
        "scriptEngine.inputGetIntValue(0, 'cancel-int', 'label'); cancelled.push(scriptEngine.inputOk());"
        "scriptEngine.inputGetDoubleValue(0, 1, 'cancel-double', 'label'); cancelled.push(scriptEngine.inputOk());"
        "r.cancelled = cancelled; scriptEngine.log('QCHDMAN_TEST_RESULT ' + JSON.stringify(r));")
        .arg(escapedFile, escapedFolder);
    const QJsonObject acceptedResult = QJsonDocument::fromJson(runFixture(acceptedScript).toUtf8()).object();
    QVERIFY(accepted.finished());
    QCOMPARE(acceptedResult.value(QStringLiteral("file")).toString(), filePath);
    QCOMPARE(QDir::cleanPath(acceptedResult.value(QStringLiteral("folder")).toString()), QDir::cleanPath(directory.path()));
    QCOMPARE(acceptedResult.value(QStringLiteral("text")).toString(), QStringLiteral("chosen text"));
    QCOMPARE(acceptedResult.value(QStringLiteral("item")).toString(), QStringLiteral("two"));
    QCOMPARE(acceptedResult.value(QStringLiteral("integer")).toInt(), 23);
    QCOMPARE(acceptedResult.value(QStringLiteral("doubleValue")).toDouble(), 4.25);
    const QJsonArray acceptedFlags{true, true, true, true, true, true};
    QCOMPARE(acceptedResult.value(QStringLiteral("oks")).toArray(), acceptedFlags);
    const QJsonArray cancelledFlags{false, false, false, false, false, false};
    QCOMPARE(acceptedResult.value(QStringLiteral("cancelled")).toArray(), cancelledFlags);
    QJsonObject normalizedResult = acceptedResult;
    normalizedResult.insert(QStringLiteral("file"), QStringLiteral("<TMP>/chosen.txt"));
    normalizedResult.insert(QStringLiteral("folder"), QStringLiteral("<TMP>"));
    QJsonObject observation;
    observation.insert(QStringLiteral("id"), QStringLiteral("utilities/dialogs"));
    observation.insert(QStringLiteral("result"), normalizedResult);
    observations.append(observation);
}

void QchdmanScriptTest::versionOnePersistence()
{
    const QString scriptSource = QStringLiteral(
        "# legacy fixture\n"
        "ApplicationVersion = 0.244\n"
        "UnknownHeader = ignored\n"
        "ScriptFormatVersion = 1\n"
        "ECMAScript [\n"
        "var greeting = 'Grüße 世界';\n"
        "scriptEngine.log(greeting);\n"
        "]\n");
    scriptWidget->fromString(scriptSource);
    const QString canonicalScript = scriptWidget->toString();
    const QString expectedScript = QStringLiteral(
        "# Qt CHDMAN GUI script file -- please do not edit manually\n"
        "ApplicationVersion = 0.244\n"
        "ScriptFormatVersion = 1\n"
        "ECMAScript [\n"
        "var greeting = 'Grüße 世界';\n"
        "scriptEngine.log(greeting);\n"
        "]\n");
    QCOMPARE(canonicalScript, expectedScript);
    scriptWidget->fromString(QStringLiteral("truncated input without marker\n"));
    QCOMPARE(scriptWidget->toString(), expectedScript);

    ProjectWindow *projectWindow = window->createProjectWindow(QCHDMAN_MDI_PROJECT);
    QVERIFY(projectWindow);
    const QString projectSource = QStringLiteral(
        "# legacy fixture\n"
        "ApplicationVersion = 0.244\n"
        "ProjectFormatVersion = 1\n"
        "ProjectType = Info\n"
        "UnknownField = ignored\n"
        "InfoInputFile = /tmp/Grüße 世界.chd\n"
        "InfoVerbose = 1\n");
    projectWindow->projectWidget->fromString(projectSource);
    const QString canonicalProject = projectWindow->projectWidget->toString();
    const QString expectedProject = QStringLiteral(
        "# Qt CHDMAN GUI project file -- please do not edit manually\n"
        "ApplicationVersion = 0.244\n"
        "ProjectFormatVersion = 1\n"
        "ProjectType = Info\n"
        "InfoInputFile = /tmp/Grüße 世界.chd\n"
        "InfoVerbose = 1\n");
    QCOMPARE(canonicalProject, expectedProject);

    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString projectPath = directory.filePath(QStringLiteral("roundtrip.prj"));
    projectWindow->projectWidget->saveAs(projectPath);
    ProjectWindow *loadedWindow = window->createProjectWindow(QCHDMAN_MDI_PROJECT);
    QVERIFY(loadedWindow);
    loadedWindow->projectWidget->load(projectPath);
    QCOMPARE(loadedWindow->projectWidget->toString(), expectedProject);

    const QString beforeMalformed = loadedWindow->projectWidget->toString();
    loadedWindow->projectWidget->fromString(QStringLiteral(
        "ProjectFormatVersion = 1\nProjectType = UnknownType\nInfoInputFile = changed\n"));
    QCOMPARE(loadedWindow->projectWidget->toString(), beforeMalformed);

    window->mdiArea()->removeSubWindow(loadedWindow);
    delete loadedWindow;
    window->mdiArea()->removeSubWindow(projectWindow);
    delete projectWindow;

    QJsonObject result;
    result.insert(QStringLiteral("script"), canonicalScript);
    result.insert(QStringLiteral("project"), canonicalProject);
    result.insert(QStringLiteral("fileRoundTrip"), true);
    result.insert(QStringLiteral("malformedPreserved"), true);
    QJsonObject observation;
    observation.insert(QStringLiteral("id"), QStringLiteral("persistence/version-1"));
    observation.insert(QStringLiteral("result"), result);
    observations.append(observation);
}

void QchdmanScriptTest::projectProperties()
{
    const QStringList projectTypes{
        QStringLiteral("Info"), QStringLiteral("Verify"), QStringLiteral("Copy"),
        QStringLiteral("CreateRaw"), QStringLiteral("CreateHD"), QStringLiteral("CreateCD"),
        QStringLiteral("CreateLD"), QStringLiteral("ExtractRaw"), QStringLiteral("ExtractHD"),
        QStringLiteral("ExtractCD"), QStringLiteral("ExtractLD"), QStringLiteral("DumpMeta"),
        QStringLiteral("AddMeta"), QStringLiteral("DelMeta")
    };
    QString script = QStringLiteral("var propertyResults = {}, propertyBoundaries = {}, propertyCalls = []; ");
    for (const QString &type : projectTypes) {
        const QString id = QStringLiteral("properties-%1").arg(type.toLower());
        script += QStringLiteral("scriptEngine.projectCreate('%1','%2');").arg(id, type);
    }

    int setterCount = 0;
    int getterCount = 0;
    const QMetaObject *metaObject = &ScriptEngine::staticMetaObject;
    QSet<QString> methodNames;
    for (int index = metaObject->methodOffset(); index < metaObject->methodCount(); ++index)
        methodNames.insert(QString::fromLatin1(metaObject->method(index).name()));
    QSet<QString> seen;
    for (int index = metaObject->methodOffset(); index < metaObject->methodCount(); ++index) {
        const QMetaMethod method = metaObject->method(index);
        const QString name = QString::fromLatin1(method.name());
        if (method.methodType() != QMetaMethod::Slot || seen.contains(name))
            continue;
        seen.insert(name);
        if (!name.startsWith(QStringLiteral("projectSet")) && !name.startsWith(QStringLiteral("projectGet")))
            continue;
        QString type;
        for (const QString &candidate : projectTypes) {
            if (name.mid(QStringLiteral("projectSet").size()).startsWith(candidate)
                    || name.mid(QStringLiteral("projectGet").size()).startsWith(candidate)) {
                type = candidate;
                break;
            }
        }
        if (type.isEmpty())
            continue;
        const QString id = QStringLiteral("properties-%1").arg(type.toLower());
        if (name.startsWith(QStringLiteral("projectGet"))) {
            script += QStringLiteral("propertyResults['%1']=scriptEngine['%1']('%2');propertyCalls.push('%1');")
                    .arg(name, id);
            ++getterCount;
            continue;
        }

        QStringList arguments{id};
        const QList<QByteArray> parameterTypes = method.parameterTypes();
        for (int parameter = 1; parameter < parameterTypes.size(); ++parameter) {
            const QByteArray parameterType = parameterTypes.at(parameter);
            if (parameterType == "bool") {
                arguments.append(QStringLiteral("true"));
            } else if (parameterType == "int") {
                arguments.append(QStringLiteral("7"));
            } else if (parameterType == "QString") {
                QString value = QStringLiteral("value-%1-%2").arg(type.toLower(), QString::number(parameter));
                if (name.endsWith(QStringLiteral("Compressors")))
                    value = QStringLiteral("zlib,lzma");
                arguments.append(QStringLiteral("'%1'").arg(value));
                continue;
            } else {
                QFAIL(qPrintable(QStringLiteral("unsupported script parameter type %1 in %2")
                                 .arg(QString::fromLatin1(parameterType), name)));
            }
            if (parameterTypes.at(parameter) != "QString")
                continue;
        }
        // Quote the project id while leaving the generated typed values intact.
        arguments[0] = QStringLiteral("'%1'").arg(id);
        const QString getterName = QString(name).replace(QStringLiteral("projectSet"), QStringLiteral("projectGet"));
        if (parameterTypes.size() == 2 && methodNames.contains(getterName)) {
            QStringList boundaryValues;
            if (parameterTypes.at(1) == "bool") {
                boundaryValues << QStringLiteral("false") << QStringLiteral("true");
            } else if (parameterTypes.at(1) == "int") {
                boundaryValues << QStringLiteral("-1") << QStringLiteral("0") << QStringLiteral("2147483647");
            } else if (name.endsWith(QStringLiteral("Compressors"))) {
                boundaryValues << QStringLiteral("''") << QStringLiteral("'none'") << QStringLiteral("'zlib,lzma'");
            } else if (name.endsWith(QStringLiteral("Tag"))) {
                boundaryValues << QStringLiteral("''") << QStringLiteral("'ABCDE'");
            } else {
                boundaryValues << QStringLiteral("''") << QStringLiteral("'Grüße 世界\\nline'");
            }
            script += QStringLiteral("propertyBoundaries['%1']=[];").arg(name);
            for (const QString &boundary : boundaryValues) {
                script += QStringLiteral("scriptEngine['%1']('%2',%3);propertyBoundaries['%1'].push(scriptEngine['%4']('%2'));")
                        .arg(name, id, boundary, getterName);
            }
        }
        script += QStringLiteral("scriptEngine['%1'](%2);propertyCalls.push('%1');")
                .arg(name, arguments.join(QLatin1Char(',')));
        ++setterCount;
    }
    script += QStringLiteral(
        "scriptEngine.log('QCHDMAN_TEST_RESULT '+JSON.stringify({calls:propertyCalls,boundaries:propertyBoundaries,values:propertyResults}));");

    const QJsonObject result = QJsonDocument::fromJson(runFixture(script).toUtf8()).object();
    const QJsonArray calls = result.value(QStringLiteral("calls")).toArray();
    const QJsonObject boundaries = result.value(QStringLiteral("boundaries")).toObject();
    const QJsonObject values = result.value(QStringLiteral("values")).toObject();
    QCOMPARE(calls.size(), setterCount + getterCount);
    QCOMPARE(values.size(), getterCount);
    QCOMPARE(boundaries.size(), getterCount);
    QVERIFY(setterCount > 80);
    QVERIFY(getterCount > 80);
    QCOMPARE(boundaries.value(QStringLiteral("projectSetCopyForce")).toArray(), QJsonArray({false, true}));
    QCOMPARE(boundaries.value(QStringLiteral("projectSetCopyCompressors")).toArray(),
             QJsonArray({QString(), QStringLiteral("none"), QStringLiteral("zlib,lzma")}));
    QCOMPARE(boundaries.value(QStringLiteral("projectSetAddMetaTag")).toArray(),
             QJsonArray({QString(), QStringLiteral("ABCD")}));
    QCOMPARE(boundaries.value(QStringLiteral("projectSetCopyProcessors")).toArray(),
             QJsonArray({0, 0, 9999}));
    QCOMPARE(boundaries.value(QStringLiteral("projectSetCreateHDCylinders")).toArray(),
             QJsonArray({-1, 0, 999999999}));
    QCOMPARE(boundaries.value(QStringLiteral("projectSetInfoInputFile")).toArray(),
             QJsonArray({QString(), QStringLiteral("Grüße 世界\nline")}));
    for (const QJsonValue &call : calls)
        QVERIFY2(!call.toString().isEmpty(), "empty property call name");

    const QMap<QString, QString> expectedCommands{
        {QStringLiteral("Info"), QStringLiteral("info")},
        {QStringLiteral("Verify"), QStringLiteral("verify")},
        {QStringLiteral("Copy"), QStringLiteral("copy")},
        {QStringLiteral("CreateRaw"), QStringLiteral("createraw")},
        {QStringLiteral("CreateHD"), QStringLiteral("createhd")},
        {QStringLiteral("CreateCD"), QStringLiteral("createcd")},
        {QStringLiteral("CreateLD"), QStringLiteral("createld")},
        {QStringLiteral("ExtractRaw"), QStringLiteral("extractraw")},
        {QStringLiteral("ExtractHD"), QStringLiteral("extracthd")},
        {QStringLiteral("ExtractCD"), QStringLiteral("extractcd")},
        {QStringLiteral("ExtractLD"), QStringLiteral("extractld")},
        {QStringLiteral("DumpMeta"), QStringLiteral("dumpmeta")},
        {QStringLiteral("AddMeta"), QStringLiteral("addmeta")},
        {QStringLiteral("DelMeta"), QStringLiteral("delmeta")}
    };
    QJsonObject commands;
    for (QWidget *widget : QApplication::allWidgets()) {
        ProjectWidget *projectWidget = qobject_cast<ProjectWidget *>(widget);
        if (!projectWidget)
            continue;
        if (!projectWidget->isScriptElement)
            continue;
        projectWidget->on_toolButtonRun_clicked(true);
        if (!expectedCommands.contains(projectWidget->projectTypeName))
            continue;
        QVERIFY(!projectWidget->arguments.isEmpty());
        QCOMPARE(projectWidget->arguments.first(), expectedCommands.value(projectWidget->projectTypeName));
        QVERIFY2(!projectWidget->arguments.contains(QString()), qPrintable(projectWidget->projectTypeName));
        commands.insert(projectWidget->projectTypeName,
                        QJsonArray::fromStringList(projectWidget->arguments));
    }
    QCOMPARE(commands.size(), expectedCommands.size());

    scriptWidget->engine()->destroyProjects();
    QJsonObject observation;
    observation.insert(QStringLiteral("id"), QStringLiteral("projects/properties"));
    QJsonObject observedResult = result;
    observedResult.insert(QStringLiteral("commands"), commands);
    observation.insert(QStringLiteral("result"), observedResult);
    observations.append(observation);
}

void QchdmanScriptTest::projectLifecycleAndFailures()
{
    const QString fakeChdman = qEnvironmentVariable("QCHDMAN_FAKE_CHDMAN");
    QVERIFY2(QFileInfo::exists(fakeChdman),
             "QCHDMAN_FAKE_CHDMAN must name the built fake-chdman executable");
    globalConfig->setPreferencesChdmanBinary(fakeChdman);
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString recordPath = directory.filePath(QStringLiteral("invocations.jsonl"));
    qputenv("QCHDMAN_FAKE_RECORD", QFile::encodeName(recordPath));
    qputenv("QCHDMAN_FAKE_MODE", "success");
    qputenv("QCHDMAN_FAKE_DELAY_MS", "25");
    qputenv("QCHDMAN_FAKE_STDERR", "Compressing, 25.0% complete...\nCompression complete ... final ratio = 1.00\n");

    const QJsonObject success = QJsonDocument::fromJson(runFixture(QStringLiteral(
        "var started=[],finished=[];"
        "scriptEngine.projectStarted.connect(function(id){started.push(id);});"
        "scriptEngine.projectFinished.connect(function(id){finished.push(id);});"
        "scriptEngine.projectCreate('parallel-a','Copy');"
        "scriptEngine.projectCreate('parallel-b','Verify');"
        "scriptEngine.projectSetCopyInputFile('parallel-a','input a.chd');"
        "scriptEngine.projectSetCopyOutputFile('parallel-a','output a.chd');"
        "scriptEngine.projectSetVerifyInputFile('parallel-b','input b.chd');"
        "scriptEngine.runProjects('parallel-a, parallel-b');"
        "var peak=scriptEngine.runningProjects();"
        "scriptEngine.waitForRunningProjects(2);scriptEngine.syncProjects();"
        "scriptEngine.log('QCHDMAN_TEST_RESULT '+JSON.stringify({"
        "started:started,finished:finished,peak:peak,running:scriptEngine.runningProjects(),"
        "aStatus:scriptEngine.projectStatus('parallel-a'),bStatus:scriptEngine.projectStatus('parallel-b'),"
        "aRc:scriptEngine.projectReturnCode('parallel-a'),bRc:scriptEngine.projectReturnCode('parallel-b')}));"))
        .toUtf8()).object();
    QCOMPARE(success.value(QStringLiteral("started")).toArray().size(), 2);
    QCOMPARE(success.value(QStringLiteral("finished")).toArray().size(), 2);
    QCOMPARE(success.value(QStringLiteral("peak")).toInt(), 2);
    QCOMPARE(success.value(QStringLiteral("running")).toInt(), 0);
    QCOMPARE(success.value(QStringLiteral("aStatus")).toString(), QStringLiteral("finished"));
    QCOMPARE(success.value(QStringLiteral("bStatus")).toString(), QStringLiteral("finished"));
    QCOMPARE(success.value(QStringLiteral("aRc")).toInt(), 0);
    QCOMPARE(success.value(QStringLiteral("bRc")).toInt(), 0);

    qputenv("QCHDMAN_FAKE_MODE", "exit");
    qputenv("QCHDMAN_FAKE_EXIT_CODE", "7");
    qputenv("QCHDMAN_FAKE_DELAY_MS", "0");
    qunsetenv("QCHDMAN_FAKE_STDERR");
    const QJsonObject nonzero = QJsonDocument::fromJson(runFixture(QStringLiteral(
        "scriptEngine.projectCreate('nonzero','Info');scriptEngine.runProjects('nonzero');scriptEngine.syncProjects('nonzero');"
        "scriptEngine.log('QCHDMAN_TEST_RESULT '+JSON.stringify({status:scriptEngine.projectStatus('nonzero'),rc:scriptEngine.projectReturnCode('nonzero')}));"))
        .toUtf8()).object();
    QCOMPARE(nonzero.value(QStringLiteral("status")).toString(), QStringLiteral("finished"));
    QCOMPARE(nonzero.value(QStringLiteral("rc")).toInt(), 7);

    qputenv("QCHDMAN_FAKE_MODE", "crash");
    const QJsonObject crashed = QJsonDocument::fromJson(runFixture(QStringLiteral(
        "scriptEngine.projectCreate('crash','Info');scriptEngine.runProjects('crash');scriptEngine.syncProjects('crash');"
        "scriptEngine.log('QCHDMAN_TEST_RESULT '+JSON.stringify({status:scriptEngine.projectStatus('crash')}));"))
        .toUtf8()).object();
    QCOMPARE(crashed.value(QStringLiteral("status")).toString(), QStringLiteral("crashed"));

    qputenv("QCHDMAN_FAKE_MODE", "wait");
    const QJsonObject stopped = QJsonDocument::fromJson(runFixture(QStringLiteral(
        "scriptEngine.projectCreate('stopped','Info');scriptEngine.runProjects('stopped');"
        "var running=scriptEngine.runningProjects();scriptEngine.stopProjects('stopped');"
        "scriptEngine.log('QCHDMAN_TEST_RESULT '+JSON.stringify({running:running,status:scriptEngine.projectStatus('stopped'),left:scriptEngine.runningProjects()}));"))
        .toUtf8()).object();
    QCOMPARE(stopped.value(QStringLiteral("running")).toInt(), 1);
    QCOMPARE(stopped.value(QStringLiteral("status")).toString(), QStringLiteral("terminated"));
    QCOMPARE(stopped.value(QStringLiteral("left")).toInt(), 0);

    globalConfig->setPreferencesChdmanBinary(directory.filePath(QStringLiteral("missing-chdman")));
    const QJsonObject missing = QJsonDocument::fromJson(runFixture(QStringLiteral(
        "scriptEngine.projectCreate('missing','Info');scriptEngine.runProjects('missing');"
        "scriptEngine.log('QCHDMAN_TEST_RESULT '+JSON.stringify({status:scriptEngine.projectStatus('missing'),rc:scriptEngine.projectReturnCode('missing')}));"))
        .toUtf8()).object();
    QCOMPARE(missing.value(QStringLiteral("status")).toString(), QStringLiteral("error"));
    QCOMPARE(missing.value(QStringLiteral("rc")).toInt(), -1);

    globalConfig->setPreferencesChdmanBinary(fakeChdman);
    const QString projectPath = directory.filePath(QStringLiteral("lifecycle.prj"));
    QFile projectFile(projectPath);
    QVERIFY(projectFile.open(QIODevice::WriteOnly | QIODevice::Text));
    projectFile.write("ProjectFormatVersion = 1\nProjectType = Info\nInfoInputFile = from-file.chd\n");
    projectFile.close();
    QString escapedPath = projectPath;
    escapedPath.replace(QLatin1Char('\\'), QStringLiteral("\\\\")).replace(QLatin1Char('\''), QStringLiteral("\\'"));
    const QString lifecycleScript = QStringLiteral(
        "var serialized='ProjectFormatVersion = 1\\nProjectType = Verify\\nVerifyInputFile = from-string.chd\\n';"
        "scriptEngine.projectCreate('base','Info');scriptEngine.projectClone('base','clone');"
        "scriptEngine.projectSetType('clone','Copy');"
        "scriptEngine.projectCreateFromString('string',serialized);"
        "scriptEngine.projectCreateFromFile('file','%1');"
        "var before={base:scriptEngine.projectGetType('base'),clone:scriptEngine.projectGetType('clone'),"
        "string:scriptEngine.projectGetType('string'),file:scriptEngine.projectGetType('file')};"
        "scriptEngine.projectDestroy('base');scriptEngine.destroyProjects('clone,string');"
        "scriptEngine.log('QCHDMAN_TEST_RESULT '+JSON.stringify({before:before,base:scriptEngine.projectStatus('base'),"
        "clone:scriptEngine.projectStatus('clone'),file:scriptEngine.projectStatus('file')}));").arg(escapedPath);
    const QJsonObject lifecycle = QJsonDocument::fromJson(runFixture(lifecycleScript).toUtf8()).object();
    QCOMPARE(lifecycle.value(QStringLiteral("before")).toObject().value(QStringLiteral("base")).toString(), QStringLiteral("Info"));
    QCOMPARE(lifecycle.value(QStringLiteral("before")).toObject().value(QStringLiteral("clone")).toString(), QStringLiteral("Copy"));
    QCOMPARE(lifecycle.value(QStringLiteral("before")).toObject().value(QStringLiteral("string")).toString(), QStringLiteral("Verify"));
    QCOMPARE(lifecycle.value(QStringLiteral("before")).toObject().value(QStringLiteral("file")).toString(), QStringLiteral("Info"));
    QCOMPARE(lifecycle.value(QStringLiteral("base")).toString(), QStringLiteral("unknown"));
    QCOMPARE(lifecycle.value(QStringLiteral("clone")).toString(), QStringLiteral("unknown"));
    QCOMPARE(lifecycle.value(QStringLiteral("file")).toString(), QStringLiteral("idle"));

    QFile record(recordPath);
    QVERIFY(record.open(QIODevice::ReadOnly | QIODevice::Text));
    QList<QJsonObject> invocationObjects;
    while (!record.atEnd()) {
        const QJsonDocument invocation = QJsonDocument::fromJson(record.readLine());
        QVERIFY(invocation.isObject());
        if (invocation.object().value(QStringLiteral("mode")).toString() != QStringLiteral("wait"))
            invocationObjects.append(invocation.object());
    }
    std::sort(invocationObjects.begin(), invocationObjects.end(), [](const QJsonObject &left, const QJsonObject &right) {
        return QJsonDocument(left).toJson(QJsonDocument::Compact)
                < QJsonDocument(right).toJson(QJsonDocument::Compact);
    });
    QJsonArray invocations;
    for (const QJsonObject &invocation : invocationObjects)
        invocations.append(invocation);
    QCOMPARE(invocations.size(), 4);

    qunsetenv("QCHDMAN_FAKE_RECORD");
    qunsetenv("QCHDMAN_FAKE_MODE");
    qunsetenv("QCHDMAN_FAKE_DELAY_MS");
    qunsetenv("QCHDMAN_FAKE_EXIT_CODE");
    QJsonObject result;
    result.insert(QStringLiteral("success"), success);
    result.insert(QStringLiteral("nonzero"), nonzero);
    result.insert(QStringLiteral("crashed"), crashed);
    result.insert(QStringLiteral("stopped"), stopped);
    result.insert(QStringLiteral("missing"), missing);
    result.insert(QStringLiteral("lifecycle"), lifecycle);
    result.insert(QStringLiteral("invocations"), invocations);
    QJsonObject observation;
    observation.insert(QStringLiteral("id"), QStringLiteral("projects/lifecycle"));
    observation.insert(QStringLiteral("result"), result);
    observations.append(observation);
}

void QchdmanScriptTest::scriptInterruptionAndStress()
{
    const QString fakeChdman = qEnvironmentVariable("QCHDMAN_FAKE_CHDMAN");
    QVERIFY(QFileInfo::exists(fakeChdman));
    globalConfig->setPreferencesChdmanBinary(fakeChdman);
    qputenv("QCHDMAN_FAKE_MODE", "wait");
    QElapsedTimer elapsed;
    elapsed.start();
    for (int iteration = 0; iteration < 5; ++iteration) {
        QTimer::singleShot(20, scriptWidget->engine(), [this]() {
            scriptWidget->engine()->stopScript();
        });
        const QString script = (iteration % 2 == 0)
                ? QStringLiteral("while (true) {}")
                : QStringLiteral("scriptEngine.projectCreate('active','Info');scriptEngine.runProjects('active');while (true) {}");
        scriptWidget->engine()->runScript(script);
        QVERIFY(scriptWidget->engine()->externalStop);
        QCOMPARE(scriptWidget->engine()->runningProjects(), 0);
        const QJsonObject recovery = QJsonDocument::fromJson(runFixture(QStringLiteral(
            "scriptEngine.log('QCHDMAN_TEST_RESULT '+JSON.stringify({ok:true,running:scriptEngine.runningProjects()}));"))
            .toUtf8()).object();
        QCOMPARE(recovery.value(QStringLiteral("ok")).toBool(), true);
        QCOMPARE(recovery.value(QStringLiteral("running")).toInt(), 0);
    }
    QVERIFY(elapsed.elapsed() < 5000);
    qunsetenv("QCHDMAN_FAKE_MODE");
    QJsonObject observation;
    observation.insert(QStringLiteral("id"), QStringLiteral("runtime/interruption-stress"));
    observation.insert(QStringLiteral("result"), QJsonObject{
        {QStringLiteral("iterations"), 5},
        {QStringLiteral("activeProcessIterations"), 2},
        {QStringLiteral("recovered"), true}
    });
    observations.append(observation);
}

void QchdmanScriptTest::debuggerWorkflow()
{
    int pauses = 0;
    bool widgetsAvailable = false;
    bool actionsAvailable = false;
    bool sourceVisible = false;
    bool consoleAvailable = false;
    bool consoleEvaluated = false;
    bool breakpointActionTriggered = false;
    bool active = true;
    std::function<void()> pollDebugger;
    pollDebugger = [&]() {
        if (!active)
            return;
        QScriptEngineDebugger *debugger = scriptWidget->engine()->debuggerForTest();
        if (!debugger) {
            QTimer::singleShot(10, this, pollDebugger);
            return;
        }
        QAction *continueAction = debugger->action(QScriptEngineDebugger::ContinueAction);
        if (!continueAction || !continueAction->isEnabled()) {
            QTimer::singleShot(10, this, pollDebugger);
            return;
        }
        QWidget *code = debugger->widget(QScriptEngineDebugger::CodeWidget);
        QWidget *stack = debugger->widget(QScriptEngineDebugger::StackWidget);
        QWidget *locals = debugger->widget(QScriptEngineDebugger::LocalsWidget);
        QWidget *console = debugger->widget(QScriptEngineDebugger::ConsoleWidget);
        widgetsAvailable = code && stack && locals && console
                && debugger->widget(QScriptEngineDebugger::ScriptsWidget)
                && debugger->widget(QScriptEngineDebugger::BreakpointsWidget)
                && debugger->widget(QScriptEngineDebugger::ErrorLogWidget);
        actionsAvailable = debugger->action(QScriptEngineDebugger::StepIntoAction)
                && debugger->action(QScriptEngineDebugger::StepOverAction)
                && debugger->action(QScriptEngineDebugger::StepOutAction)
                && debugger->action(QScriptEngineDebugger::ToggleBreakpointAction);
        if (code) {
            for (QPlainTextEdit *editor : code->findChildren<QPlainTextEdit *>())
                sourceVisible = sourceVisible || editor->toPlainText().contains(QStringLiteral("debuggerWorkflowInner"));
        }
        consoleAvailable = consoleAvailable || (console && console->findChild<QLineEdit *>());
        if (pauses == 0 && console) {
            if (QLineEdit *lineEdit = console->findChild<QLineEdit *>()) {
                lineEdit->setText(QStringLiteral("1 + 2"));
                QTest::keyClick(lineEdit, Qt::Key_Return);
                QCoreApplication::processEvents();
                lineEdit->setText(QStringLiteral("break 3"));
                QTest::keyClick(lineEdit, Qt::Key_Return);
                consoleEvaluated = true;
            }
            if (code) {
                for (QPlainTextEdit *editor : code->findChildren<QPlainTextEdit *>()) {
                    editor->moveCursor(QTextCursor::Start);
                    editor->moveCursor(QTextCursor::Down);
                }
            }
            debugger->action(QScriptEngineDebugger::ToggleBreakpointAction)->trigger();
            breakpointActionTriggered = true;
            QCoreApplication::processEvents();
        }
        QAction *action = continueAction;
        if (pauses == 0)
            action = debugger->action(QScriptEngineDebugger::StepOverAction);
        else if (pauses == 1)
            action = debugger->action(QScriptEngineDebugger::StepIntoAction);
        else if (pauses == 2)
            action = debugger->action(QScriptEngineDebugger::StepOutAction);
        ++pauses;
        QTimer::singleShot(10, this, pollDebugger);
        action->trigger();
    };
    QTimer::singleShot(10, this, pollDebugger);
    const QJsonObject result = QJsonDocument::fromJson(runFixture(QStringLiteral(
        "function debuggerWorkflowInner(value) { var localValue = value + 1; return localValue; }\n"
        "debugger;\nvar debuggerWorkflowResult = debuggerWorkflowInner(41);\ndebugger;\n"
        "scriptEngine.log('QCHDMAN_TEST_RESULT '+JSON.stringify({value:debuggerWorkflowResult}));"))
        .toUtf8()).object();
    active = false;
    QCOMPARE(result.value(QStringLiteral("value")).toInt(), 42);
    QVERIFY(pauses >= 4);
    QVERIFY(widgetsAvailable);
    QVERIFY(actionsAvailable);
    QVERIFY(sourceVisible);
    QVERIFY(consoleAvailable);
    QVERIFY(consoleEvaluated);
    QVERIFY(breakpointActionTriggered);
    QScriptEngineDebugger *debugger = scriptWidget->engine()->debuggerForTest();
    QVERIFY(debugger);
    debugger->detach();
    debugger->attachTo(scriptWidget->engine()->scriptEngineForTest());

    QJsonObject observation;
    observation.insert(QStringLiteral("id"), QStringLiteral("debugger/workflow"));
    observation.insert(QStringLiteral("result"), QJsonObject{
        {QStringLiteral("value"), 42},
        {QStringLiteral("minimumPauses"), 4},
        {QStringLiteral("widgets"), widgetsAvailable},
        {QStringLiteral("actions"), actionsAvailable},
        {QStringLiteral("source"), sourceVisible},
        {QStringLiteral("console"), consoleAvailable},
        {QStringLiteral("consoleEvaluation"), consoleEvaluated},
        {QStringLiteral("breakpointAction"), breakpointActionTriggered},
        {QStringLiteral("reattached"), true}
    });
    observations.append(observation);
}

QTEST_MAIN(QchdmanScriptTest)
#include "tst_qchdman_script.moc"
