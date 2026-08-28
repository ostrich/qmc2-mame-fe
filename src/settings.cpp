#include "settings.h"

QStringList Settings::stResolve(const QStringList& qstrl) {
	QString qstr;
	QStringList qstrl2;
	foreach (qstr, qstrl)
		qstrl2 << stResolve(qstr);
	return qstrl2;
}

QString Settings::stResolve(const QString& qstr) {
	QByteArray qbaBuf;
	QString qstrEnv, qstrFinal;

#if defined(QMC2_OS_WIN)
	QRegularExpression qrx("(\\%(.*?)\\%)", QRegularExpression::CaseInsensitiveOption);
#else
	QRegularExpression qrx("(\\$\\{(.*?)\\})");
#endif

	int posLastEnd = -1;
	QRegularExpressionMatchIterator matches = qrx.globalMatch(qstr);
	while (matches.hasNext()) {
		const QRegularExpressionMatch match = matches.next();
		const int pos = match.capturedStart();
		if (pos > posLastEnd)
			qstrFinal += qstr.mid(posLastEnd + 1, pos - (posLastEnd + 1));

		qbaBuf = match.captured(2).toUtf8();
		qbaBuf = qgetenv(qbaBuf.constData());
		if (!qbaBuf.isNull()) {
			qstrFinal += QString::fromLocal8Bit(qbaBuf.constData());
		} else
			qstrFinal += match.captured(1);  // unresolved, so put it back untouched
    
		posLastEnd = match.capturedEnd() - 1;
	}
	if (posLastEnd < qstr.length())
		qstrFinal += qstr.mid(posLastEnd + 1, qstr.length());
	return qstrFinal;
}

QVariant Settings::value(const QString& key, const QVariant& defaultValue) const
{
	QVariant v = QSettings::value(key, defaultValue);
	if (QString(v.typeName()) == QString("QString") && v.toString().contains("${")) {
		v = QVariant(stResolve(v.toString()));
		return v;
	} else
		return v;
}
