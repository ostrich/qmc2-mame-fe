#include <QtTest>

#include "bigbytearray.h"

class TestBigByteArray : public QObject
{
	Q_OBJECT

private slots:
	void spansMultipleChunks();
	void rejectsInvalidRanges();
};

void TestBigByteArray::spansMultipleChunks()
{
	const QByteArray source("0123456789abcdefghijklmnopqrstuvwxyz");
	BigByteArray data(source.constData(), quint64(source.size()));

	QCOMPARE(data.chunks(), 5);
	QCOMPARE(data.mid(6, 24), source.mid(6, 24));
	QCOMPARE(data.mid(8, 16), source.mid(8, 16));
	QCOMPARE(data.mid(35, 1), source.mid(35, 1));
}

void TestBigByteArray::rejectsInvalidRanges()
{
	const QByteArray source("0123456789");
	BigByteArray data(source.constData(), quint64(source.size()));

	QTest::ignoreMessage(QtWarningMsg, "BigByteArray::mid(): range out of bounds");
	QVERIFY(data.mid(9, 2).isEmpty());
	QTest::ignoreMessage(QtWarningMsg, "BigByteArray::mid(): length must not exceed 2 GB");
	QVERIFY(data.mid(0, -1).isEmpty());
	QCOMPARE(data.mid(10, 0), QByteArray());
}

QTEST_APPLESS_MAIN(TestBigByteArray)
#include "tst_bigbytearray.moc"
