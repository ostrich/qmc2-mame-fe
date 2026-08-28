#include <QtScript/QScriptEngine>
#include <QtScriptTools/QScriptEngineDebugger>
#include <QTimer>
#include <QtTest>

class ScriptApi : public QObject
{
    Q_OBJECT

public slots:
    QString echo(const QString &value) { return value; }
    int add(int left, int right) { return left + right; }
};

class tst_QtScript : public QObject
{
    Q_OBJECT

private slots:
    void exposesLegacyGlobals()
    {
        QScriptEngine engine;
        ScriptApi api;
        QScriptValue object = engine.newQObject(&api);
        engine.globalObject().setProperty("scriptEngine", object);
        engine.globalObject().setProperty("qchdman", object);

        QScriptValue result = engine.evaluate(
            "scriptEngine.echo('compatible') + ':' + qchdman.add(20, 22)");

        QVERIFY2(!engine.hasUncaughtException(), qPrintable(engine.uncaughtException().toString()));
        QCOMPARE(result.toString(), QString("compatible:42"));
    }

    void reportsExceptions()
    {
        QScriptEngine engine;
        engine.evaluate("function fail() { throw new Error('fixture failure'); } fail();");

        QVERIFY(engine.hasUncaughtException());
        QVERIFY(engine.uncaughtException().toString().contains("fixture failure"));
        QVERIFY(!engine.uncaughtExceptionBacktrace().isEmpty());
    }

    void interruptsInfiniteEvaluation()
    {
        QScriptEngine engine;
        engine.setProcessEventsInterval(1);
        bool abortRequested = false;
        QTimer::singleShot(25, &engine, [&engine, &abortRequested]() {
            abortRequested = true;
            engine.abortEvaluation();
        });

        engine.evaluate("for (;;) {};");

        QVERIFY(abortRequested);
        QVERIFY(!engine.isEvaluating());
    }

    void debuggerAttachAndCleanShutdown()
    {
        QScriptEngine engine;
        QScriptEngineDebugger debugger;
        debugger.attachTo(&engine);

        QScriptValue result = engine.evaluate("function answer() { return 42; } answer();");
        QCOMPARE(result.toInt32(), 42);
        QVERIFY(!engine.hasUncaughtException());

        debugger.detach();
    }
};

QTEST_MAIN(tst_QtScript)
#include "tst_qtscript.moc"
