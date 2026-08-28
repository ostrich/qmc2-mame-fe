#include <QtTest>
#include "xmlmachine.h"
class TestXmlMachine : public QObject
{
	Q_OBJECT
private slots:
	void metadataAndRelationships();
	void bundledQueries();
	void malformedXml();
};
static const QByteArray fixture = R"XML(
<mame><machine name="child" sourcefile="driver.cpp" cloneof="parent" romof="parent" sampleof="samples">
 <description>Child Machine</description><year>1985</year><manufacturer>Example</manufacturer>
 <softwarelist name="cart"/><driver emulation="good" graphic="imperfect" color="good" sound="good" savestate="supported"/>
 <display type="raster" width="320" height="240"/><input buttons="2"><control type="joystick" ways="8"/></input>
 <chip name="cpu" tag="maincpu" type="cpu" clock="4000000"/><chip name="audio" tag="sound" type="audio" clock="1000000"/>
 <rom name="merged.rom" merge="parent.rom"/><disk name="merged.chd" merge="parent.chd"/>
</machine></mame>)XML";
void TestXmlMachine::metadataAndRelationships()
{
	XmlMachine machine(fixture);
	QVERIFY(machine.isValid());
	QCOMPARE(machine.attribute("cloneof"), QString("parent"));
	QCOMPARE(machine.attribute("romof"), QString("parent"));
	QCOMPARE(machine.childText("description"), QString("Child Machine"));
	QCOMPARE(machine.namedChildAttribute("rom", "merged.rom", "merge"), QString("parent.rom"));
	QCOMPARE(machine.namedChildAttribute("disk", "merged.chd", "merge"), QString("parent.chd"));
}
void TestXmlMachine::bundledQueries()
{
	XmlMachine machine(fixture);
	QStringList result;
	QVERIFY(machine.evaluateBundledTemplateQuery("doc($xmlDocument)//machine/softwarelist/@name/string()", "machine", &result));
	QCOMPARE(result, QStringList({ "cart" }));
	QVERIFY(machine.evaluateBundledTemplateQuery("doc($xmlDocument)//machine/chip/@name/string()", "machine", &result));
	QCOMPARE(result, QStringList({ "cpu", "audio" }));
	QVERIFY(machine.evaluateBundledTemplateQuery("doc($xmlDocument)//machine/input/control/@ways/string()", "machine", &result));
	QCOMPARE(result, QStringList({ "8" }));
	QVERIFY(!machine.evaluateBundledTemplateQuery("doc($xmlDocument)//machine/rom/@name/string()", "machine", &result));
}
void TestXmlMachine::malformedXml()
{
	XmlMachine machine("<mame><machine>");
	QVERIFY(!machine.isValid());
}
QTEST_MAIN(TestXmlMachine)
#include "tst_xmlmachine.moc"
