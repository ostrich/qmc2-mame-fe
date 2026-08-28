#ifndef QMC2_XMLMACHINE_H
#define QMC2_XMLMACHINE_H
#include <QDomDocument>
#include <QStringList>
class XmlMachine
{
public:
	explicit XmlMachine(const QByteArray &xml);
	bool isValid() const { return !m_machine.isNull(); }
	QString attribute(const QString &name) const;
	QString childText(const QString &name) const;
	QString namedChildAttribute(const QString &element, const QString &name, const QString &attribute) const;
	bool evaluateBundledTemplateQuery(const QString &query, const QString &entityName, QStringList *result) const;
private:
	QDomDocument m_document;
	QDomElement m_machine;
};
#endif
