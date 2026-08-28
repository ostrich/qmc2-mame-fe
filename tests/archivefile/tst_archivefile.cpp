#include <QtTest>

#include <cstring>
#include <utility>
#include <QFile>
#include <QTemporaryDir>

#include "archivefile.h"

class TestArchiveFile : public QObject
{
	Q_OBJECT

private slots:
	void roundTripAndBounds();
	void rejectsOversizedEntry();
	void rejectsInvalidOperations();

private:
	QByteArray tarWithDeclaredSize(quint64 size) const;
};

QByteArray TestArchiveFile::tarWithDeclaredSize(quint64 size) const
{
	QByteArray header(512, '\0');
	auto put = [&header](int offset, int length, const QByteArray &value) {
		memcpy(header.data() + offset, value.constData(), size_t(qMin(length, value.size())));
	};
	put(0, 100, "large.bin");
	put(100, 8, "0000644");
	put(108, 8, "0000000");
	put(116, 8, "0000000");
	put(124, 12, QByteArray::number(size, 8).rightJustified(11, '0') + '\0');
	put(136, 12, "00000000000");
	memset(header.data() + 148, ' ', 8);
	header[156] = '0';
	put(257, 6, "ustar");
	put(263, 2, "00");
	quint64 checksum = 0;
	for (char byte : std::as_const(header))
		checksum += uchar(byte);
	put(148, 8, QByteArray::number(checksum, 8).rightJustified(6, '0') + QByteArray("\0 ", 2));
	return header + QByteArray(1024, '\0');
}

void TestArchiveFile::roundTripAndBounds()
{
	QTemporaryDir directory;
	QVERIFY(directory.isValid());
	const QString path = directory.filePath("fixture.zip");
	{
		ArchiveFile writer(path);
		QVERIFY(writer.open(QIODevice::WriteOnly));
		QVERIFY(writer.createEntry("hello.txt", 5));
		QCOMPARE(writer.writeEntryData("hello"), qint64(5));
		writer.closeEntry();
	}

	ArchiveFile reader(path);
	QVERIFY(reader.open());
	QCOMPARE(reader.entryList().size(), 1);
	QVERIFY(!reader.seekEntry(1));
	QVERIFY(reader.seekEntry(0));
	QByteArray data;
	QCOMPARE(reader.readEntry(data), qint64(5));
	QCOMPARE(data, QByteArray("hello"));
}

void TestArchiveFile::rejectsOversizedEntry()
{
	QTemporaryDir directory;
	QVERIFY(directory.isValid());
	const QString path = directory.filePath("oversized.tar");
	QFile file(path);
	QVERIFY(file.open(QIODevice::WriteOnly));
	QCOMPARE(file.write(tarWithDeclaredSize(1024)), qint64(1536));
	file.close();

	ArchiveFile reader(path, true);
	QVERIFY(reader.open());
	ArchiveEntryMetaData metadata;
	QVERIFY(reader.seekNextEntry(&metadata));
	QCOMPARE(metadata.size(), quint64(1024));
	QByteArray data("old data");
	QCOMPARE(reader.readEntry(data), qint64(-1));
	QVERIFY(data.isEmpty());
	QVERIFY(reader.hasError());
	QVERIFY(reader.errorString().contains("in-memory limit"));
}

void TestArchiveFile::rejectsInvalidOperations()
{
	QTemporaryDir directory;
	ArchiveFile archive(directory.filePath("unused.zip"));
	ArchiveEntryMetaData metadata;
	QVERIFY(!archive.seekNextEntry(&metadata));
	QVERIFY(!archive.seekNextEntry(nullptr));
	QVERIFY(!archive.seekEntry(0));
	QVERIFY(!archive.createEntry("bad", 1));
	QCOMPARE(archive.writeEntryData("bad"), qint64(-1));
}

QTEST_APPLESS_MAIN(TestArchiveFile)
#include "tst_archivefile.moc"
