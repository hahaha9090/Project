#include "widget.h"
#include "ui_widget.h"

#include <QContextMenuEvent>
#include <QMenu>
#include <QApplication>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    //设置边框属性为没有边框的窗口
    setWindowFlag(Qt::FramelessWindowHint); // 去掉边框
    setFixedSize(width(), height());        // 固定窗口大小


    menu = new QMenu(this);     // 设置菜单
    exitAction = new QAction(u8"退出", this);   // 设置“退出”菜单项
    exitAction->setIcon(QIcon(":/weatherIco/close.ico"));   // 设置“退出”菜单项图标
    menu->addAction(exitAction);    // 绑定菜单和菜单项

    // 连接信号槽
    connect(exitAction, &QAction::triggered, this, &Widget::slot_exitAPP);

}

Widget::~Widget()
{
    delete ui;
}

void Widget::contextMenuEvent(QContextMenuEvent *event)
{
    // 在鼠标当前位置弹出菜单
    menu->exec(QCursor::pos());
    event->accept();
}


void Widget::mousePressEvent(QMouseEvent *event)    // 记录鼠标点击点  与窗口左上角的偏移量
{
    // mPos = event->globalPos() - this->pos();

    if (event->button() == Qt::LeftButton)  // 只处理左键拖动
        mPos = event->globalPos() - frameGeometry().topLeft();
}

void Widget::mouseMoveEvent(QMouseEvent *event)     // 移动窗口到鼠标当前位置减去偏移量
{
    // this->move(event->globalPos()- mPos);

    if (event->buttons() & Qt::LeftButton) // 左键按下拖动
        move(event->globalPos() - mPos);   // 改变窗口位置
}


void Widget::slot_exitAPP()
{
    qApp->quit();  // 推荐用 quit() 而不是 exit(0)
}
