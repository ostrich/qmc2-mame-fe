#include <QtTest>
#include <QFile>
#include <QTemporaryDir>

#include "sevenzipfile.h"

class TestSevenZip : public QObject
{
	Q_OBJECT

private slots:
	void rejectsInvalidIndex();
	void resetsStateWhenReopened();
	void recoversAfterMalformedArchive();

private:
	QString writeFixture(QTemporaryDir &directory, const QString &name, const QByteArray &encoded);
};

static const QByteArray fixtureA = QByteArrayLiteral(
	"N3q8ryccAATyxoNZSAIAAAAAAABqAAAAAAAAAA2v6yJRVCArPSBjb3JlIG5ldHdvcmsgdGVzdGxpYgpDT05GSUcgKz0gdGVzdGNhc2UgY29uc29sZSBjKysxNwpURU1QTEFURSA9IGFwcApUQVJHRVQgPSB0c3RfZnRwCmdyZWF0ZXJUaGFuKFFNQUtFX0dDQ19NQUpPUl9WRVJTSU9OLCAxNSk6IFFNQUtFX0NYWEZMQUdTX1dBUk5fT04gKz0gLVduby1zZmluYWUtaW5jb21wbGV0ZQpTT1VSQ0VTICs9IHRzdF9mdHAuY3BwIC4uLy4uL3NyYy9mdHByZXBseS5jcHAKSEVBREVSUyArPSAuLi8uLi9zcmMvZnRwcmVwbHkuaApJTkNMVURFUEFUSCArPSAuLi8uLi9zcmMKIXdpbjMyIHsKCUNPTkZJRyArPSBsaW5rX3BrZ2NvbmZpZwoJUEtHQ09ORklHICs9IGxpYmN1cmwKfSBlbHNlIHsKCVZDUEtHX1BSRUZJWCA9ICQkKFZDUEtHX1JPT1QpCglpc0VtcHR5KFZDUEtHX1BSRUZJWCk6IFZDUEtHX1BSRUZJWCA9ICQkKFZDUEtHX0lOU1RBTExBVElPTl9ST09UKQoJQ1VSTF9ST09UID0gJCRWQ1BLR19QUkVGSVgvaW5zdGFsbGVkL3g2NC13aW5kb3dzCglJTkNMVURFUEFUSCArPSAkJENVUkxfUk9PVC9pbmNsdWRlCglMSUJTICs9IC9MSUJQQVRIOiQkQ1VSTF9ST09UL2xpYiBsaWJjdXJsLmxpYgp9CgEEBgABCYJIAAcLAQABAQAMgkgACAoBLEeenwAABQEZDAAAAAAAAAAAAAAAABElAHQAZQBzAHQAcwAvAGYAdABwAC8AZgB0AHAALgBwAHIAbwAAABQKAQBqH/txjzbdARUGAQAggKSBAAA=");
static const QByteArray fixtureB = QByteArrayLiteral(
	"N3q8ryccAASZ/6HdrwAAAAAAAACKAAAAAAAAADjtJsNRVCArPSBjb3JlIHRlc3RsaWIgeG1sCkNPTkZJRyArPSB0ZXN0Y2FzZSBjb25zb2xlIGMrKzE3ClRFTVBMQVRFID0gYXBwClRBUkdFVCA9IHRzdF94bWxtYWNoaW5lClNPVVJDRVMgKz0gdHN0X3htbG1hY2hpbmUuY3BwIC4uLy4uL3NyYy94bWxtYWNoaW5lLmNwcApJTkNMVURFUEFUSCArPSAuLi8uLi9zcmMKAQQGAAEJgK8ABwsBAAEBAAyArwAICgH9uQ0TAAAFARkMAAAAAAAAAAAAAAAAEUEAdABlAHMAdABzAC8AeABtAGwAbQBhAGMAaABpAG4AZQAvAHgAbQBsAG0AYQBjAGgAaQBuAGUALgBwAHIAbwAAABkCAAAUCgEAos5TzoA23QEVBgEAIICkgQAA");

QString TestSevenZip::writeFixture(QTemporaryDir &directory, const QString &name, const QByteArray &encoded)
{
	const QString path = directory.filePath(name);
	QFile file(path);
	if ( !file.open(QIODevice::WriteOnly) || file.write(QByteArray::fromBase64(encoded)) < 0 )
		return QString();
	return path;
}

void TestSevenZip::rejectsInvalidIndex()
{
	QTemporaryDir directory;
	QVERIFY(directory.isValid());
	SevenZipFile archive(writeFixture(directory, "a.7z", fixtureA));
	QVERIFY(archive.open());
	QByteArray data("unchanged");
	QCOMPARE(archive.read(1, &data), quint64(0));
	QVERIFY(archive.lastError().contains("out of range"));
}

void TestSevenZip::resetsStateWhenReopened()
{
	QTemporaryDir directory;
	QVERIFY(directory.isValid());
	const QString first = writeFixture(directory, "a.7z", fixtureA);
	const QString second = writeFixture(directory, "b.7z", fixtureB);
	SevenZipFile archive(first);
	QVERIFY(archive.open());
	QByteArray data;
	QVERIFY(archive.read(0, &data) > 0);
	QVERIFY(data.contains("TARGET = tst_ftp"));
	QVERIFY(archive.open(second));
	QVERIFY(archive.read(0, &data) > 0);
	QVERIFY(data.contains("TARGET = tst_xmlmachine"));
}

void TestSevenZip::recoversAfterMalformedArchive()
{
	QTemporaryDir directory;
	QVERIFY(directory.isValid());
	const QString malformed = directory.filePath("bad.7z");
	QFile badFile(malformed);
	QVERIFY(badFile.open(QIODevice::WriteOnly));
	QCOMPARE(badFile.write("not an archive"), qint64(14));
	badFile.close();

	SevenZipFile archive(malformed);
	QVERIFY(!archive.open());
	QVERIFY(!archive.open());
	QVERIFY(archive.open(writeFixture(directory, "valid.7z", fixtureA)));
}

QTEST_APPLESS_MAIN(TestSevenZip)
#include "tst_sevenzip.moc"
