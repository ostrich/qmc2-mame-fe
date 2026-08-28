#include <QCoreApplication>
#include <QLocale>
#include <QRegularExpression>
#include <QTextDocument>

#include <curl/curl.h>
#include <atomic>
#include <limits>
#include <mutex>

#include "ftpreply.h"

static QString humanReadableSize(quint64 value)
{
	static const char *suffixes[] = { " KB", " MB", " GB", " TB" };
	qreal scaled = value;
	int suffix = 0;
	do {
		scaled /= 1024.0;
		if ( scaled < 1024.0 || suffix == 3 )
			break;
		++suffix;
	} while ( true );
	return QLocale().toString(scaled, 'f', 2) + QCoreApplication::translate("ROMAlyzer", suffixes[suffix]);
}

static constexpr qsizetype maximumFtpListingSize = 16 * 1024 * 1024;

class CurlFtpTransfer : public QThread
{
public:
	CurlFtpTransfer(FtpReply *reply, const QUrl &url, bool directoryRequest)
		: QThread(reply), reply(reply), url(url), directoryRequest(directoryRequest) { }

	void cancel() { cancelled.store(true); }

protected:
	void run() override
	{
		static std::once_flag curlInit;
		std::call_once(curlInit, []() { curl_global_init(CURL_GLOBAL_DEFAULT); });
		CURL *curl = curl_easy_init();
		if ( !curl ) {
			finish(CURLE_FAILED_INIT, QStringLiteral("Unable to initialize FTP transport"), -1);
			return;
		}
		const QByteArray encodedUrl = url.toEncoded(QUrl::FullyEncoded);
		curl_easy_setopt(curl, CURLOPT_URL, encodedUrl.constData());
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);
#if LIBCURL_VERSION_NUM >= 0x075500
		curl_easy_setopt(curl, CURLOPT_PROTOCOLS_STR, "ftp,ftps");
		curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS_STR, "ftp,ftps");
#else
		curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_FTP | CURLPROTO_FTPS);
		curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS, CURLPROTO_FTP | CURLPROTO_FTPS);
#endif
		curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30L);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &CurlFtpTransfer::writeCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
		curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, &CurlFtpTransfer::progressCallback);
		curl_easy_setopt(curl, CURLOPT_XFERINFODATA, this);
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
		curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer);
		if ( !url.userName().isEmpty() ) {
			const QByteArray credentials = url.userName().toUtf8() + ':' + url.password().toUtf8();
			curl_easy_setopt(curl, CURLOPT_USERPWD, credentials.constData());
		}
		const CURLcode result = curl_easy_perform(curl);
		curl_off_t contentLength = -1;
		curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &contentLength);
		QString errorText;
		if ( result != CURLE_OK )
			errorText = errorBuffer[0] ? QString::fromUtf8(errorBuffer) : QString::fromUtf8(curl_easy_strerror(result));
		curl_easy_cleanup(curl);
		finish(result, errorText, qint64(contentLength));
	}

private:
	static size_t writeCallback(char *data, size_t size, size_t count, void *userdata)
	{
		auto *self = static_cast<CurlFtpTransfer *>(userdata);
		if ( size != 0 && count > size_t((std::numeric_limits<qsizetype>::max)()) / size )
			return 0;
		const qsizetype length = qsizetype(size * count);
		if ( self->directoryRequest && length > maximumFtpListingSize - self->listing.size() )
			return 0;
		const QByteArray chunk(data, length);
		if ( self->directoryRequest )
			self->listing += chunk;
		else
			QMetaObject::invokeMethod(self->reply, "receiveData", Qt::QueuedConnection, Q_ARG(QByteArray, chunk));
		return size_t(length);
	}

	static int progressCallback(void *userdata, curl_off_t total, curl_off_t current, curl_off_t, curl_off_t)
	{
		auto *self = static_cast<CurlFtpTransfer *>(userdata);
		if ( self->cancelled.load() )
			return 1;
		QMetaObject::invokeMethod(self->reply, "receiveProgress", Qt::QueuedConnection,
			Q_ARG(qint64, qint64(current)), Q_ARG(qint64, qint64(total)));
		return 0;
	}

	void finish(CURLcode result, const QString &errorText, qint64 contentLength)
	{
		const QString payload = directoryRequest ? QString::fromUtf8(listing) : errorText;
		QMetaObject::invokeMethod(reply, "receiveFinished", Qt::QueuedConnection,
			Q_ARG(int, int(result)), Q_ARG(QString, payload), Q_ARG(qint64, contentLength));
	}

	FtpReply *reply;
	QUrl url;
	bool directoryRequest;
	std::atomic_bool cancelled = false;
	QByteArray listing;
	char errorBuffer[CURL_ERROR_SIZE] = {};
};

FtpReply::FtpReply(const QUrl &url)
	: QNetworkReply(), transfer(nullptr), offset(0), expectedSize(-1), directoryRequest(url.path().endsWith('/'))
{
	setUrl(url);
	setOperation(QNetworkAccessManager::GetOperation);
	open(ReadOnly | Unbuffered);
	transfer = new CurlFtpTransfer(this, url, directoryRequest);
	transfer->start();
}

FtpReply::~FtpReply()
{
	abort();
	transfer->wait();
}

void FtpReply::receiveData(const QByteArray &data)
{
	content += data;
	emit readyRead();
}

void FtpReply::receiveProgress(qint64 received, qint64 total)
{
	if ( total >= 0 ) {
		expectedSize = total;
		setHeader(QNetworkRequest::ContentLengthHeader, total);
	}
	emit downloadProgress(received, total);
}

void FtpReply::receiveFinished(int result, const QString &payload, qint64 contentLength)
{
	if ( result != CURLE_OK ) {
		const NetworkError errorCode = networkError(result);
		setError(errorCode, payload);
		emit errorOccurred(errorCode);
		emit finished();
		return;
	}
	if ( directoryRequest )
		setListContent(payload);
	else {
		expectedSize = contentLength >= 0 ? contentLength : content.size();
		setHeader(QNetworkRequest::ContentLengthHeader, expectedSize);
		emit downloadProgress(content.size(), expectedSize);
		emit finished();
	}
}

void FtpReply::setListContent(const QString &listing)
{
	QUrl base = url();
	if ( !base.path().endsWith('/') )
		base.setPath(base.path() + '/');
	const QString displayUrl = url().toString(QUrl::RemovePassword).toHtmlEscaped();
	QString html("<html><head>\n<title>" + displayUrl + "</title>\n"
		"<style type=\"text/css\">\nth { background-color: #aaaaaa; color: black; }\n"
		"table { border: solid 1px #aaaaaa; }\ntr.odd { background-color: #dddddd; color: black; }\n"
		"tr.even { background-color: white; color: black; }\n</style>\n</head><body>\n"
		"<h1>" + tr("FTP directory listing for %1").arg(base.path().toHtmlEscaped()) + "</h1>\n"
		"<table align=\"center\" cellspacing=\"0\" width=\"100%\">\n<tr><th align=\"left\">" + tr("Name")
		+ "</th><th align=\"left\">" + tr("Type") + "</th><th align=\"left\">" + tr("Size") + "</th></tr>\n");
	const QUrl parent = base.resolved(QUrl(".."));
	if ( parent.isParentOf(base) )
		html += "<tr><td><strong><a href=\"" + parent.toString(QUrl::RemovePassword).toHtmlEscaped() + "\">" + tr("Parent directory") + "</a></strong></td><td></td></tr>\n";
	const QRegularExpression unixEntry(QStringLiteral("^([dl-])[^ ]*\\s+\\d+\\s+\\S+\\s+\\S+\\s+(\\d+)\\s+\\S+\\s+\\S+\\s+\\S+\\s+(.+)$"));
	const QRegularExpression dosEntry(QStringLiteral("^\\d{2}-\\d{2}-\\d{2,4}\\s+\\d{2}:\\d{2}(?:AM|PM)\\s+(<DIR>|\\d+)\\s+(.+)$"), QRegularExpression::CaseInsensitiveOption);
	int row = 0;
	for ( const QString &line : listing.split('\n', Qt::SkipEmptyParts) ) {
		const QString entry = line.trimmed();
		const QRegularExpressionMatch unixMatch = unixEntry.match(entry);
		const QRegularExpressionMatch dosMatch = dosEntry.match(entry);
		if ( !unixMatch.hasMatch() && !dosMatch.hasMatch() )
			continue;
		const bool directory = unixMatch.hasMatch() ? unixMatch.captured(1) == "d" : dosMatch.captured(1).compare("<DIR>", Qt::CaseInsensitive) == 0;
		const quint64 size = directory ? 0 : (unixMatch.hasMatch() ? unixMatch.captured(2) : dosMatch.captured(1)).toULongLong();
		const QString name = unixMatch.hasMatch() ? unixMatch.captured(3) : dosMatch.captured(2);
		if ( name == "." || name == ".." )
			continue;
		const QUrl child = base.resolved(QUrl(name));
		html += QString("<tr class=\"%1\"><td><a href=\"%2\">%3</a></td>")
			.arg(row++ % 2 ? "even" : "odd", child.toString(QUrl::RemovePassword).toHtmlEscaped(), name.toHtmlEscaped());
		if ( directory )
			html += "<td>" + tr("Folder") + "<td></td></tr>\n";
		else
			html += "<td>" + tr("File") + "</td><td>" + humanReadableSize(size) + "</td></tr>\n";
	}
	html += "</table>\n</body></html>\n";
	content = html.toUtf8();
	expectedSize = content.size();
	setHeader(QNetworkRequest::ContentTypeHeader, "text/html; charset=UTF-8");
	setHeader(QNetworkRequest::ContentLengthHeader, expectedSize);
	emit readyRead();
	emit finished();
}

QNetworkReply::NetworkError FtpReply::networkError(int result) const
{
	switch ( CURLcode(result) ) {
		case CURLE_ABORTED_BY_CALLBACK: return OperationCanceledError;
		case CURLE_COULDNT_RESOLVE_HOST: return HostNotFoundError;
		case CURLE_COULDNT_CONNECT: return ConnectionRefusedError;
		case CURLE_LOGIN_DENIED: return AuthenticationRequiredError;
		case CURLE_OPERATION_TIMEDOUT: return TimeoutError;
		case CURLE_REMOTE_ACCESS_DENIED:
		case CURLE_REMOTE_FILE_NOT_FOUND: return ContentNotFoundError;
		default: return ProtocolFailure;
	}
}

void FtpReply::abort()
{
	if ( transfer && transfer->isRunning() )
		transfer->cancel();
}

qint64 FtpReply::bytesAvailable() const
{
	return content.size() - offset + QIODevice::bytesAvailable();
}

bool FtpReply::isSequential() const { return true; }

qint64 FtpReply::readData(char *data, qint64 maxSize)
{
	if ( offset >= content.size() )
		return -1;
	const qint64 number = qMin(maxSize, content.size() - offset);
	memcpy(data, content.constData() + offset, size_t(number));
	offset += number;
	if ( offset >= 1024 * 1024 && offset >= content.size() / 2 ) {
		content.remove(0, offset);
		offset = 0;
	}
	return number;
}

qint64 FtpReply::totalSize(QString) { return expectedSize; }
