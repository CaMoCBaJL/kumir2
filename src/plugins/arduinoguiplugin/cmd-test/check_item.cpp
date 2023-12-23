#include <QPainter>
#include <QPen>
#include <QDebug>

#include "check_item.h"
#include "ui_check_item.h"

CheckItemWidget::CheckItemWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::check_item)
{
    ui->setupUi(this);
}

void CheckItemWidget::setLabelText(QString labelText) {
    ui ->label->setText(labelText);
}

CheckItemWidget::~CheckItemWidget()
{
    delete ui;
}

void CheckItemWidget::setState(ArduinoPlugin::CheckItemStates state){
    switch(state){
        case ArduinoPlugin::Error:
            drawCircle(Qt::red);
            return;
        case ArduinoPlugin::Success:
            drawCircle(Qt::green);
            return;
        case ArduinoPlugin::Warning:
            drawCircle(Qt::yellow);
            return;
    }
}

void CheckItemWidget::drawCircle(Qt::GlobalColor color) {
    QPixmap pm(20, 20);
    pm.fill();

    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    QPen pen(color, 2);
    p.setPen(pen);
    QBrush brush(color);
    p.setBrush(brush);
    p.drawEllipse(3, 3, 15, 15);

    this->ui->statusImage->setPixmap(pm);
}
