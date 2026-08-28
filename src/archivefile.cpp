#include "archivefile.h"

#include <cerrno>

namespace {
constexpr qint64 archiveReadChunkSize = 64 * 1024;

quint64 safeEntrySize(struct archive_entry *entry)
{
	const la_int64_t size = archive_entry_size(entry);
	return size > 0 ? quint64(size) : 0;
}
}

ArchiveFile::ArchiveFile(QString fileName, bool sequential, bool deflate, QObject *parent) :
	QObject(parent),
	m_archive(0),
	m_entry(0),
	m_fileName(fileName),
	m_sequential(sequential),
	m_deflate(deflate),
	m_openMode(QIODevice::ReadOnly),
	m_lastStatus(ARCHIVE_OK)
{
}

ArchiveFile::~ArchiveFile()
{
	if ( isOpen() )
		close();
}

bool ArchiveFile::open(QIODevice::OpenMode openMode, QString fileName)
{
	if ( isOpen() )
		close();
	if ( !fileName.isEmpty() )
		m_fileName = fileName;
	m_openMode = openMode;
	m_entry = 0;
	m_lastStatus = ARCHIVE_OK;
	switch ( m_openMode ) {
		case QIODevice::WriteOnly:
			m_archive = archive_write_new();
			if ( !m_archive ) {
				m_lastStatus = ARCHIVE_FATAL;
				return false;
			}
			m_lastStatus = archive_write_set_format_zip(m_archive);
			if ( m_lastStatus >= ARCHIVE_WARN ) {
				if ( m_deflate )
					m_lastStatus = archive_write_set_options(m_archive, "compression=deflate");
				else
					m_lastStatus = archive_write_set_options(m_archive, "compression=store");
			}
			if ( m_lastStatus < ARCHIVE_WARN ) {
				archive_write_free(m_archive);
				m_archive = 0;
				return false;
			}
			m_lastStatus = archive_write_open_filename(m_archive, m_fileName.toUtf8().constData());
			if ( m_lastStatus != ARCHIVE_OK ) {
				archive_write_free(m_archive);
				m_archive = 0;
				return false;
			} else
				return true;
		case QIODevice::ReadOnly:
		default:
			m_archive = archive_read_new();
			if ( !m_archive ) {
				m_lastStatus = ARCHIVE_FATAL;
				return false;
			}
			m_lastStatus = archive_read_support_filter_all(m_archive);
			if ( m_lastStatus >= ARCHIVE_WARN )
				m_lastStatus = archive_read_support_format_all(m_archive);
			if ( m_lastStatus >= ARCHIVE_WARN )
				m_lastStatus = archive_read_open_filename(m_archive, m_fileName.toUtf8().constData(), QMC2_ARCHIVE_BLOCK_SIZE);
			if ( m_lastStatus != ARCHIVE_OK ) {
				archive_read_free(m_archive);
				m_archive = 0;
				return false;
			} else {
				if ( !m_sequential )
					createEntryList();
				return true;
			}
	}
}

void ArchiveFile::close()
{
	entryList().clear();
	m_nameToIndexCache.clear();
	if ( isOpen() ) {
		switch ( m_openMode ) {
			case QIODevice::WriteOnly:
				if ( m_entry )
					closeEntry();
				archive_write_close(m_archive);
				archive_write_free(m_archive);
				break;
			case QIODevice::ReadOnly:
			default:
				archive_read_close(m_archive);
				archive_read_free(m_archive);
				break;
		}
		m_archive = 0;
		m_entry = 0;
	}
}

bool ArchiveFile::reopen()
{
	if ( !isOpen() || !readMode() )
		return false;
	archive_read_free(m_archive);
	m_archive = archive_read_new();
	m_entry = 0;
	if ( !m_archive ) {
		m_lastStatus = ARCHIVE_FATAL;
		return false;
	}
	m_lastStatus = archive_read_support_filter_all(m_archive);
	if ( m_lastStatus >= ARCHIVE_WARN )
		m_lastStatus = archive_read_support_format_all(m_archive);
	if ( m_lastStatus >= ARCHIVE_WARN )
		m_lastStatus = archive_read_open_filename(m_archive, m_fileName.toUtf8().constData(), QMC2_ARCHIVE_BLOCK_SIZE);
	return m_lastStatus == ARCHIVE_OK;
}

qint64 ArchiveFile::writeEntryDataBig(const BigByteArray &buffer)
{
	qint64 len = 0;
	for (int i = 0; i < buffer.chunks(); i++) {
		const qint64 written = writeEntryData(buffer.chunk(i));
		if ( written < 0 || written != buffer.chunk(i).size() )
			return -1;
		len += written;
	}
	return len;
}

qint64 ArchiveFile::writeEntryData(const QByteArray &buffer)
{
	if ( !isOpen() || !writeMode() || !m_entry )
		return -1;
	const la_ssize_t written = archive_write_data(m_archive, buffer.constData(), size_t(buffer.size()));
	if ( written < 0 )
		m_lastStatus = ARCHIVE_FATAL;
	return qint64(written);
}

bool ArchiveFile::seekNextEntry(ArchiveEntryMetaData *metaData, bool *reset)
{
	if ( !isOpen() || !readMode() || !metaData )
		return false;
	if ( !m_sequential ) {
		if ( reset != 0 && *reset ) {
			if ( !reopen() )
				return false;
			*reset = false;
		}
	}
	m_lastStatus = archive_read_next_header(m_archive, &m_entry);
	if ( m_lastStatus == ARCHIVE_OK ) {
		const char *path = archive_entry_pathname_utf8(m_entry);
		*metaData = ArchiveEntryMetaData(path ? QString::fromUtf8(path) : QString(), safeEntrySize(m_entry), QDateTime::fromSecsSinceEpoch(archive_entry_mtime(m_entry)));
		return true;
	} else
		return false;
}

bool ArchiveFile::seekEntry(uint index)
{
	if ( !isOpen() || !readMode() || index >= uint(entryList().size()) )
		return false;
	ArchiveEntryMetaData metadata = entryList()[index];
	if ( !reopen() )
		return false;
	while ( (m_lastStatus = archive_read_next_header(m_archive, &m_entry)) == ARCHIVE_OK ) {
		const char *path = archive_entry_pathname_utf8(m_entry);
		if ( path && metadata.name().compare(QString::fromUtf8(path), Qt::CaseSensitive) == 0 )
			return true;
	}
	return false;
}

qint64 ArchiveFile::readEntry(QByteArray &buffer)
{
	buffer.clear();
	if ( !isOpen() || writeMode() || !m_entry )
		return 0;
	const la_int64_t declaredSize = archive_entry_size(m_entry);
	const quint64 maximumSize = qMin(quint64(QMC2_ARCHIVE_MAX_ENTRY_SIZE), quint64(QByteArray::maxSize()));
	if ( declaredSize > 0 && quint64(declaredSize) > maximumSize ) {
		archive_set_error(m_archive, EFBIG, "archive entry exceeds QMC2's in-memory limit");
		m_lastStatus = ARCHIVE_FATAL;
		return -1;
	}
	QByteArray chunk(archiveReadChunkSize, Qt::Uninitialized);
	for (;;) {
		const la_ssize_t len = archive_read_data(m_archive, chunk.data(), size_t(chunk.size()));
		if ( len == 0 )
			return buffer.size();
		if ( len < 0 ) {
			m_lastStatus = ARCHIVE_FATAL;
			buffer.clear();
			return -1;
		}
		if ( quint64(buffer.size()) > maximumSize - quint64(len) ) {
			archive_set_error(m_archive, EFBIG, "archive entry exceeds QMC2's in-memory limit");
			m_lastStatus = ARCHIVE_FATAL;
			buffer.clear();
			return -1;
		}
		buffer.append(chunk.constData(), qsizetype(len));
	}
}

bool ArchiveFile::createEntry(QString name, size_t size)
{
	if ( !isOpen() || !writeMode() || m_entry )
		return false;
	m_entry = archive_entry_new();
	if ( !m_entry ) {
		m_lastStatus = ARCHIVE_FATAL;
		return false;
	}
	archive_entry_set_pathname(m_entry, name.toUtf8().constData());
	archive_entry_set_size(m_entry, size);
	archive_entry_set_filetype(m_entry, AE_IFREG);
	archive_entry_set_perm(m_entry, 0644);
	archive_entry_set_mtime(m_entry, QDateTime::currentDateTime().toSecsSinceEpoch(), 0);
	m_lastStatus = archive_write_header(m_archive, m_entry);
	if ( m_lastStatus < ARCHIVE_WARN ) {
		archive_entry_free(m_entry);
		m_entry = 0;
		return false;
	}
	return true;
}

void ArchiveFile::closeEntry()
{
	if ( isOpen() && writeMode() && m_entry ) {
		m_lastStatus = archive_write_finish_entry(m_archive);
		archive_entry_free(m_entry);
		m_entry = 0;
	}
}

void ArchiveFile::createEntryList()
{
	entryList().clear();
	m_nameToIndexCache.clear();
	if ( !isOpen() )
		return;
	struct archive_entry *entry;
	int counter = 0;
	while ( (m_lastStatus = archive_read_next_header(m_archive, &entry)) == ARCHIVE_OK ) {
		const char *path = archive_entry_pathname_utf8(entry);
		QString entryName(path ? QString::fromUtf8(path) : QString());
		entryList() << ArchiveEntryMetaData(entryName, safeEntrySize(entry), QDateTime::fromSecsSinceEpoch(archive_entry_mtime(entry)));
		m_nameToIndexCache.insert(entryName, counter++);
		archive_read_data_skip(m_archive);
	}
}
