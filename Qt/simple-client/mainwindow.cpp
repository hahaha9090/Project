#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>
#include <QScrollBar>
#include <QFileInfo>
#include <cstring>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 字体相关信号
    connect(ui->ziti, &QFontComboBox::currentFontChanged, this, &MainWindow::onFontChanged);
    ui->zitidaxiao->setCurrentText("14");
    connect(ui->zitidaxiao, &QComboBox::currentTextChanged, this, &MainWindow::onFontSizeChanged);


    // 初始化 Socket
    tcpSocket = new QTcpSocket(this);

    // 连接 Socket 信号
    connect(tcpSocket, &QTcpSocket::readyRead, this, &MainWindow::onReadyRead);
    connect(tcpSocket, &QTcpSocket::connected, this, &MainWindow::onConnected);         // 连接成功槽函数
    connect(tcpSocket, &QTcpSocket::disconnected, this, &MainWindow::onDisconnected);   // 接收到断开连接信号槽函数

    // 初始化变量
    pendingFileSize = 0;
    remainingFileSize = 0;
}

MainWindow::~MainWindow()
{
    delete ui;
}

// ================== 网络通信 ====================

// 连接服务器
void MainWindow::on_lianjieanniu_clicked()
{
    QString ip = ui->IPshuru->text().trimmed();
    quint16 port = ui->DuanKoushuru->text().toUShort();

    if (ip.isEmpty() || port == 0) {
        QMessageBox::warning(this, "提示", "请输入有效的 IP 和端口");
        return;
    }

    if (tcpSocket->state() == QAbstractSocket::ConnectedState) {
        tcpSocket->disconnectFromHost();
    }

    tcpSocket->connectToHost(ip, port);
    ui->liaotiankuang->append("正在连接服务器...");
}
// 连接成功槽函数
void MainWindow::onConnected()
{
    ui->liaotiankuang->append("<font color='green'>已连接到服务器</font>");
}

// 手动断开连接
void MainWindow::on_duankailianjie_clicked()
{
    tcpSocket->disconnectFromHost();
}

// 接收到断开连接信号槽函数
void MainWindow::onDisconnected()
{
    ui->liaotiankuang->append("<font color='red'>与服务器断开连接</font>");
}

//  发送数据包
void MainWindow::sendData(quint8 type, const QByteArray &data)
{
    if (tcpSocket->state() != QAbstractSocket::ConnectedState) return;

    // 1. 构造包头
    char header[5];
    header[0] = type;
    quint32 len = data.size();
    memcpy(header + 1, &len, 4); // 将长度写入头部的后4位 (小端序)

    // 2. 发送头和数据
    tcpSocket->write(header, 5);
    tcpSocket->write(data);
}

// 发送文本消息
void MainWindow::on_fasong_clicked()
{
    QString text = ui->shuruxiaoxi->toPlainText();
    if (text.isEmpty()) return;

    if (tcpSocket->state() != QAbstractSocket::ConnectedState) {
        QMessageBox::warning(this, "错误", "未连接到服务器");
        return;
    }

    sendData(MSG_TEXT, text.toUtf8());
    ui->shuruxiaoxi->clear();
}

// 发送文件 (上传)
void MainWindow::on_wenjian_clicked()
{
    if (tcpSocket->state() != QAbstractSocket::ConnectedState) {
        QMessageBox::warning(this, "错误", "未连接到服务器");
        return;
    }

    QString filePath = QFileDialog::getOpenFileName(this, "选择要发送的文件");
    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "错误", "无法打开文件");
        return;
    }

    // 1. 发送文件信息：文件名\n大小
    QString fileName = QFileInfo(filePath).fileName();
    quint64 fileSize = file.size();

    QByteArray fileInfo;
    fileInfo.append(fileName.toUtf8());
    fileInfo.append('\n');
    fileInfo.append(QString::number(fileSize).toUtf8());

    sendData(MSG_FILE_INFO, fileInfo);

    // 2. 发送文件内容 (分块发送)
    const qint64 chunkSize = 4096;
    qint64 totalSent = 0;

    while (totalSent < fileSize) {
        QByteArray chunk = file.read(chunkSize);
        if (chunk.isEmpty()) break;
        sendData(MSG_FILE_DATA, chunk);
        totalSent += chunk.size();
        tcpSocket->waitForBytesWritten(100);    // 限流，避免发送过快撑爆内存
    }
    file.close();
    ui->liaotiankuang->append(QString("已发送文件: %1").arg(fileName));
}

// 接收文件 (请求下载)
void MainWindow::on_jieshouwenjian_clicked()
{
    if (tcpSocket->state() != QAbstractSocket::ConnectedState) {
        QMessageBox::warning(this, "错误", "未连接到服务器");
        return;
    }

    if (pendingFileName.isEmpty()) {
        QMessageBox::information(this, "提示", "当前没有可接收的文件通知");
        return;
    }

    QString savePath = QFileDialog::getSaveFileName(this, "保存文件", pendingFileName);
    if (savePath.isEmpty()) return;

    // 准备接收文件
    recvFile.setFileName(savePath);
    if (!recvFile.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, "错误", "无法写入文件，请检查权限");
        return;
    }
    savedFilePath = savePath;
    remainingFileSize = pendingFileSize; // 需要接收的总大小

    // 发送下载请求，服务端收到请求后，会先发 MSG_FILE_INFO，然后发 MSG_FILE_DATA
    sendData(MSG_FILE_REQUEST, pendingFileName.toUtf8());
    ui->liaotiankuang->append(QString("开始接收文件: %1...").arg(pendingFileName));
    // 清空 pending ，直到收到新的通知，为了防止重复点击
    pendingFileName.clear();
    pendingFileSize = 0;
}

// 处理接收到的数据
void MainWindow::onReadyRead()
{
    // 将新数据追加到缓冲区
    recvBuffer.append(tcpSocket->readAll());

    // 循环解析完整的数据包
    while (recvBuffer.size() >= 5) {
        // 1. 解析头部
        quint8 type = recvBuffer[0];
        quint32 len;
        memcpy(&len, recvBuffer.constData() + 1, 4);

        // 2. 检查数据是否完整
        if (recvBuffer.size() < 5 + len) {
            break; // 数据不够，等待下次
        }

        // 3. 提取数据体
        QByteArray content = recvBuffer.mid(5, len);
        recvBuffer.remove(0, 5 + len); // 从缓冲区移除已处理的数据

        // 4. 根据类型处理
        switch (type) {
        case MSG_TEXT: {
            QString msg = QString::fromUtf8(content);
            ui->liaotiankuang->append(msg);
            
            // 如果收到服务端的错误消息，并且正在接收文件，则中断接收
            if (recvFile.isOpen() && msg.startsWith("Error:")) {
                recvFile.close();
                recvFile.remove(); // 删除创建的空文件
                remainingFileSize = 0;
                ui->liaotiankuang->append("<font color='red'>已取消文件接收（服务端文件不存在）</font>");
            }
            break;
        }
        case MSG_FILE_INFO: {
            // 格式: IP\n文件名\n大小 (广播) 或 文件名\n大小 (请求回复)
            QList<QByteArray> parts = content.split('\n');
            if (parts.size() >= 2) {
                // 取最后两个部分作为文件名和大小，兼容两种格式
                QString fName = QString::fromUtf8(parts[parts.size() - 2]);
                quint64 fSize = parts[parts.size() - 1].toULongLong();
                
                QString sender = (parts.size() >= 3) ? QString::fromUtf8(parts[0]) : "服务端";

                // 更新待接收信息
                pendingFileName = fName;
                pendingFileSize = fSize;

                ui->liaotiankuang->append(
                    QString("<div style='color:blue'>[文件通知] 来自 %1<br>文件名: %2<br>大小: %3 字节<br>请点击“接收文件”按钮下载</div>")
                    .arg(sender, fName).arg(fSize)
                );
            }
            break;
        }
        case MSG_FILE_DATA: {
            if (recvFile.isOpen()) {
                recvFile.write(content);
                remainingFileSize -= content.size();

                if (remainingFileSize <= 0) {
                    recvFile.close();
                    ui->liaotiankuang->append(
                        QString("<div style='color:green'>文件接收成功！<br>保存位置: %1</div>")
                        .arg(savedFilePath)
                    );
                }
            }
            break;
        }
        }
    }
}

// ================== 字体样式部分 =========================

void MainWindow::onFontChanged(const QFont &font)
{
    QTextCharFormat fmt;
    fmt.setFontFamilies({ font.family() });
    QTextCursor cursor(ui->shuruxiaoxi->document());
    cursor.select(QTextCursor::Document);
    cursor.mergeCharFormat(fmt);
}

void MainWindow::onFontSizeChanged(const QString &text)
{
    bool ok = false;
    double size = text.toDouble(&ok);
    if (!ok || size <= 0) return;

    QTextCharFormat fmt;
    fmt.setFontPointSize(size);
    QTextCursor cursor(ui->shuruxiaoxi->document());
    cursor.select(QTextCursor::Document);
    cursor.mergeCharFormat(fmt);
    ui->shuruxiaoxi->setCurrentFont(QFont(ui->shuruxiaoxi->currentFont().family(), size));
}

void MainWindow::on_qingxie_toggled(bool checked)
{
    QTextCharFormat fmt;
    fmt.setFontItalic(checked);
    QTextCursor cursor = ui->shuruxiaoxi->textCursor();
    if (cursor.hasSelection()) cursor.mergeCharFormat(fmt);
    else ui->shuruxiaoxi->mergeCurrentCharFormat(fmt);
}

void MainWindow::on_jiachu_toggled(bool checked)
{
    QTextCharFormat fmt;
    fmt.setFontWeight(checked ? QFont::Bold : QFont::Normal);
    QTextCursor cursor = ui->shuruxiaoxi->textCursor();
    if (cursor.hasSelection()) cursor.mergeCharFormat(fmt);
    else ui->shuruxiaoxi->mergeCurrentCharFormat(fmt);
}
