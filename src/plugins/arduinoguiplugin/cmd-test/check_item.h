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
    explicit CheckItemWidget(QWidget *parent = nullptr);
    ~CheckItemWidget();

    void setState(ArduinoPlugin::CheckItemStates state);
    void setLabelText(QString labelText);

private:
    Ui::check_item *ui;

    void drawCircle(Qt::GlobalColor color);
};

#endif // CHECK_ITEM_H
