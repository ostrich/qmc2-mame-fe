#ifndef FTPREPLY_H
#define FTPREPLY_H

#include <QNetworkReply>
#include <QThread>

class CurlFtpTransfer;

class FtpReply : public QNetworkReply
{
	Q_OBJECT

	public:
		FtpReply(const QUrl &);
		~FtpReply() override;
		void abort();
		qint64 bytesAvailable() const;
		bool isSequential() const;

		qint64 totalSize(QString);

	protected:
		qint64 readData(char *, qint64);

	private slots:
		void receiveData(const QByteArray &);
		void receiveProgress(qint64, qint64);
		void receiveFinished(int, const QString &, qint64);

	private:
		void setListContent(const QString &);
		QNetworkReply::NetworkError networkError(int) const;

		friend class CurlFtpTransfer;
		CurlFtpTransfer *transfer;
		QByteArray content;
		qint64 offset;
		qint64 expectedSize;
		bool directoryRequest;
};

#endif
