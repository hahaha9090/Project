#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>

#include <QMenu>
#include <QAction>

#include <QMouseEvent>
#include <QPoint>

QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

private:
    Ui::Widget *ui;


protected:
    void contextMenuEvent(QContextMenuEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);


private slots:
    void slot_exitAPP();   // 槽函数必须是类成员

private:

    QMenu *menu;           // 菜单作为成员
    QAction *exitAction;   // 退出动作作为成员

    QPoint mPos;
};
#endif // WIDGET_H
