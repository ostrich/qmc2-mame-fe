#ifndef ARCHIVEFILE_H
#define ARCHIVEFILE_H

#include <QObject>
#include <QString>
#include <QList>
#include <QHash>
#include <QStringList>
#include <QDateTime>
#include <QByteArray>
#include <QIODevice>

#include <archive.h>
#include <archive_entry.h>

#include "bigbytearray.h"
#include "macros.h"

#ifndef QMC2_ARCHIVE_MAX_ENTRY_SIZE
#define QMC2_ARCHIVE_MAX_ENTRY_SIZE QMC2_QBYTEARRAY_LIMIT
#endif

class ArchiveEntryMetaData
{
	public:
		explicit ArchiveEntryMetaData(QString name = QString(), quint64 size = 0, QDateTime date = QDateTime())
		{
			setName(name);
			setSize(size);
			setDate(date);
		}

		void setName(QString name) { m_name = name; }
		QString &name() { return m_name; }
		void setSize(quint64 size) { m_size = size; }
		quint64 size() { return m_size; }
		void setDate(QDateTime date) { m_date = date; }
		QDateTime &date() { return m_date; }

	private:
		QString m_name;
		quint64 m_size;
		QDateTime m_date;
};

class ArchiveFile : public QObject
{
	Q_OBJECT

	public:
		explicit ArchiveFile(QString fileName = QString(), bool sequential = false, bool deflate = true, QObject *parent = 0);
		~ArchiveFile();

		QString fileName() { return m_fileName; }
		QList<ArchiveEntryMetaData> &entryList() { return m_entryList; }
		bool isOpen() { return m_archive != 0; }
		bool open(QIODevice::OpenMode openMode = QIODevice::ReadOnly, QString fileName = QString());
		bool reopen();
		void close();
		bool readMode() { return m_openMode == QIODevice::ReadOnly; }
		bool writeMode() { return m_openMode == QIODevice::WriteOnly; }
		bool seekNextEntry(ArchiveEntryMetaData *metaData, bool *reset = 0);
		bool seekEntry(uint index);
		bool seekEntry(const QString &name) { int index = indexOfName(name); return index >= 0 ? seekEntry(index) : false; }
		bool hasError() { return m_lastStatus <= ARCHIVE_FAILED; }
		bool hasWarning() { return m_lastStatus == ARCHIVE_WARN; }
		qint64 readEntry(QByteArray &buffer);
		bool createEntry(QString name, size_t size);
		qint64 writeEntryData(const QByteArray &buffer);
		qint64 writeEntryDataBig(const BigByteArray &buffer);
		void closeEntry();
		QString errorString() { return isOpen() && archive_error_string(m_archive) ? QString::fromUtf8(archive_error_string(m_archive)) : QString(); }
		int errorCode() { return m_lastStatus; }
		void createEntryList();

	private:
		int indexOfName(const QString &name) { return m_nameToIndexCache.contains(name) ? m_nameToIndexCache.value(name) : -1; }

		struct archive *m_archive;
		struct archive_entry *m_entry;
		QList<ArchiveEntryMetaData> m_entryList;
		QHash<QString, int> m_nameToIndexCache;
		QString m_fileName;
		bool m_sequential;
		bool m_deflate;
		QIODevice::OpenMode m_openMode;
		int m_lastStatus;
};

#endif
