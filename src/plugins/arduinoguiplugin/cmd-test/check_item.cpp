#include <QPainter>
#include <QPen>
#include <QDebug>

#include "check_item.h"
#include "ui_check_item.h"

CheckItemWidget::CheckItemWidget(QWidget *parent, QString labelText) :
    QWidget(parent),
    ui(new Ui::check_item)
{
    ui->setupUi(this);
    ui->label->setText(labelText);
    qDebug() << labelText;
    qDebug() << this->property("labelText").toString();
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
    QPixmap pm(40, 40);
    pm.fill();

    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    QPen pen(color, 2);
    p.setPen(pen);
    QBrush brush(color);
    p.setBrush(brush);
    p.drawEllipse(5, 5, 35, 35);

    this->ui->label->setPixmap(pm);
}
