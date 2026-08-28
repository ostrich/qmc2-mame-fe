#include <QtTest>
#include <QTcpServer>
#include <QTcpSocket>

#include "ftpreply.h"

class FakeFtpServer : public QObject
{
	Q_OBJECT

public:
	explicit FakeFtpServer(QObject *parent = nullptr) : QObject(parent)
	{
		connect(&controlServer, &QTcpServer::newConnection, this, [this]() {
			control = controlServer.nextPendingConnection();
			connect(control, &QTcpSocket::readyRead, this, &FakeFtpServer::readCommands);
			writeControl("220 qmc2 test server ready\r\n");
		});
		QVERIFY(controlServer.listen(QHostAddress::LocalHost));
	}

	QUrl url(const QString &path) const
	{
		QUrl result;
		result.setScheme("ftp");
		result.setHost("127.0.0.1");
		result.setPort(controlServer.serverPort());
		result.setPath(path);
		return result;
	}

	QString userName;

private:
	void writeControl(const QByteArray &line)
	{
		control->write(line);
		control->flush();
	}

	void openPassiveServer()
	{
		dataServer.close();
		QVERIFY(dataServer.listen(QHostAddress::LocalHost));
		const quint16 port = dataServer.serverPort();
		writeControl(QByteArray("227 Entering Passive Mode (127,0,0,1,")
			+ QByteArray::number(port / 256) + ',' + QByteArray::number(port % 256) + ").\r\n");
	}

	void sendData(const QByteArray &payload)
	{
		auto finish = [this, payload]() {
			QTcpSocket *data = dataServer.nextPendingConnection();
			data->write(payload);
			data->disconnectFromHost();
			connect(data, &QTcpSocket::disconnected, data, &QObject::deleteLater);
			writeControl("226 Transfer complete\r\n");
		};
		if ( dataServer.hasPendingConnections() )
			finish();
		else
			connect(&dataServer, &QTcpServer::newConnection, this, finish, Qt::SingleShotConnection);
	}

	void readCommands()
	{
		commandBuffer += control->readAll();
		while ( commandBuffer.contains("\r\n") ) {
			const int end = commandBuffer.indexOf("\r\n");
			const QByteArray command = commandBuffer.left(end);
			commandBuffer.remove(0, end + 2);
			const int separator = command.indexOf(' ');
			const QByteArray verb = (separator < 0 ? command : command.left(separator)).toUpper();
			const QByteArray argument = separator < 0 ? QByteArray() : command.mid(separator + 1);
			if ( verb == "USER" ) {
				userName = QString::fromUtf8(argument);
				writeControl("331 Password required\r\n");
			} else if ( verb == "PASS" ) {
				writeControl("230 Logged in\r\n");
			} else if ( verb == "TYPE" ) {
				writeControl("200 Type set\r\n");
			} else if ( verb == "PASV" ) {
				openPassiveServer();
			} else if ( verb == "LIST" ) {
				if ( argument == "/missing.bin" ) {
					writeControl("550 Not found\r\n");
				} else {
					writeControl("150 Opening data connection\r\n");
					if ( argument.endsWith("/") )
						sendData("-rw-r--r-- 1 owner group 12 Jan 01 2026 file.bin\r\n"
							"drwxr-xr-x 1 owner group 0 Jan 01 2026 subdir\r\n");
					else
						sendData("-rw-r--r-- 1 owner group 12 Jan 01 2026 file.bin\r\n");
				}
			} else if ( verb == "RETR" ) {
				writeControl("150 Opening data connection\r\n");
				sendData("hello ftp!\n");
			} else if ( verb == "QUIT" ) {
				writeControl("221 Goodbye\r\n");
				control->disconnectFromHost();
			} else {
				writeControl("200 OK\r\n");
			}
		}
	}

	QTcpServer controlServer;
	QTcpServer dataServer;
	QTcpSocket *control = nullptr;
	QByteArray commandBuffer;
};

class TestFtpReply : public QObject
{
	Q_OBJECT

private slots:
	void downloadsFile()
	{
		FakeFtpServer server;
		FtpReply reply(server.url("/pub/file.bin"));
		QSignalSpy finished(&reply, &QNetworkReply::finished);
		QVERIFY(finished.wait());
		QCOMPARE(reply.error(), QNetworkReply::NoError);
		QCOMPARE(reply.readAll(), QByteArray("hello ftp!\n"));
		QCOMPARE(reply.header(QNetworkRequest::ContentLengthHeader).toLongLong(), 11);
		QCOMPARE(server.userName, QString("anonymous"));
	}

	void rendersDirectoryListing()
	{
		FakeFtpServer server;
		FtpReply reply(server.url("/pub/"));
		QSignalSpy finished(&reply, &QNetworkReply::finished);
		QVERIFY(finished.wait());
		const QByteArray html = reply.readAll();
		QVERIFY(html.contains("file.bin"));
		QVERIFY(html.contains("subdir"));
		QCOMPARE(reply.header(QNetworkRequest::ContentTypeHeader).toString(), QString("text/html; charset=UTF-8"));
	}

	void reportsMissingFile()
	{
		FakeFtpServer server;
		FtpReply reply(server.url("/missing.bin"));
		QSignalSpy error(&reply, &QNetworkReply::errorOccurred);
		QVERIFY(error.wait());
		QCOMPARE(reply.error(), QNetworkReply::ContentNotFoundError);
	}
};

QTEST_MAIN(TestFtpReply)
#include "tst_ftp.moc"
