#ifndef CHECK_ITEM_H
#define CHECK_ITEM_H

#include <QWidget>
#include "check_item_states.h"

namespace Ui {
class check_item;
}

class CheckItemWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CheckItemWidget(QWidget *parent = nullptr, QString labelText = QString());
    ~CheckItemWidget();

    void setState(ArduinoPlugin::CheckItemStates state);

private:
    Ui::check_item *ui;

    void drawCircle(Qt::GlobalColor color);
};

#endif // CHECK_ITEM_H
