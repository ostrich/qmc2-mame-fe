#include "xmlmachine.h"
#include <QHash>

XmlMachine::XmlMachine(const QByteArray &xml)
{
	if ( !m_document.setContent(xml) ) return;
	m_machine = m_document.documentElement();
	if ( m_machine.tagName() != "machine" && m_machine.tagName() != "game" )
		m_machine = m_document.elementsByTagName("machine").at(0).toElement();
}

QString XmlMachine::attribute(const QString &name) const { return m_machine.attribute(name).trimmed(); }
QString XmlMachine::childText(const QString &name) const { return m_machine.firstChildElement(name).text().trimmed(); }

QString XmlMachine::namedChildAttribute(const QString &element, const QString &name, const QString &attribute) const
{
	for (QDomElement child = m_machine.firstChildElement(element); !child.isNull(); child = child.nextSiblingElement(element))
		if ( child.attribute("name") == name ) return child.attribute(attribute).trimmed();
	return QString();
}

bool XmlMachine::evaluateBundledTemplateQuery(const QString &query, const QString &entityName, QStringList *result) const
{
	if ( !result || !isValid() ) return false;
	result->clear();
	const QString entity = entityName.isEmpty() ? QStringLiteral("machine") : entityName;
	const QString prefix = QStringLiteral("doc($xmlDocument)//") + entity + '/';
	const QString rootPrefix = QStringLiteral("doc($xmlDocument)//") + entity + "/@";
	if ( query.startsWith(rootPrefix) && query.endsWith("/string()") ) {
		const QString attributeName = query.mid(rootPrefix.size(), query.size() - rootPrefix.size() - 9);
		if ( attributeName != "sampleof" ) return false;
		const QString value = attribute(attributeName);
		if ( !value.isEmpty() ) result->append(value);
		return true;
	}
	if ( !query.startsWith(prefix) || !query.endsWith("/string()") ) return false;
	const QString body = query.mid(prefix.size(), query.size() - prefix.size() - 9);
	const int marker = body.lastIndexOf("/@");
	if ( marker < 1 ) return false;
	const QString elementPath = body.left(marker);
	const QString attributeName = body.mid(marker + 2);
	static const QHash<QString, QStringList> allowed = {
		{ "softwarelist", { "name" } },
		{ "driver", { "emulation", "graphic", "color", "sound", "savestate" } },
		{ "display", { "type", "width", "height" } },
		{ "input", { "buttons" } },
		{ "input/control", { "type", "ways" } },
		{ "chip", { "name", "tag", "type", "clock" } }
	};
	if ( !allowed.contains(elementPath) || !allowed.value(elementPath).contains(attributeName) ) return false;
	QList<QDomElement> elements = { m_machine };
	for (const QString &part : elementPath.split('/')) {
		QList<QDomElement> next;
		for (const QDomElement &parent : elements)
			for (QDomElement child = parent.firstChildElement(part); !child.isNull(); child = child.nextSiblingElement(part)) next.append(child);
		elements = next;
	}
	for (const QDomElement &element : elements) {
		const QString value = element.attribute(attributeName).trimmed();
		if ( !value.isEmpty() ) result->append(value);
	}
	return true;
}
