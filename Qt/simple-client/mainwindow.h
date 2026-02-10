#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QFile>
#include <QByteArray>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

// 消息类型（与服务端保持一致）
enum MsgType {
    MSG_TEXT = 0,          // 文本消息
    MSG_FILE_INFO = 1,     // 文件信息（文件名+大小）
    MSG_FILE_REQUEST = 2,  // 客户端请求文件
    MSG_FILE_DATA = 3      // 文件内容分块
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // 界面按钮槽函数
    void on_lianjieanniu_clicked();     // 连接服务器
    void on_duankailianjie_clicked();   // 断开连接
    void on_fasong_clicked();           // 发送文本
    void on_wenjian_clicked();          // 发送文件
    void on_jieshouwenjian_clicked();   // 接收文件（点击按钮）

    // Socket 信号槽函数
    void onReadyRead();                 // 收到数据
    void onConnected();                 // 连接成功
    void onDisconnected();              // 连接断开

    // 字体样式槽函数
    void onFontChanged(const QFont &font);
    void onFontSizeChanged(const QString &text);
    void on_qingxie_toggled(bool checked);
    void on_jiachu_toggled(bool checked);

private:
    Ui::MainWindow *ui;
    QTcpSocket *tcpSocket;

    // 数据接收缓冲区（处理粘包/半包）
    QByteArray recvBuffer;

    // 文件接收相关变量
    QString pendingFileName;        // 待接收的文件名（来自通知）
    quint64 pendingFileSize = 0;    // 待接收的文件大小
    
    QFile recvFile;                 // 正在写入的文件对象
    quint64 remainingFileSize = 0;  // 剩余未接收字节数
    QString savedFilePath;          // 保存路径

    // 辅助函数：发送数据包
    void sendData(quint8 type, const QByteArray &data);
};
#endif // MAINWINDOW_H
