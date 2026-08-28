#include "clickablelabel.h"

ClickableLabel::ClickableLabel(QWidget *parent, Qt::WindowFlags) :
	QLabel(parent)
{
	// NOP
}

void ClickableLabel::mousePressEvent(QMouseEvent *)
{
	emit clicked();
}
