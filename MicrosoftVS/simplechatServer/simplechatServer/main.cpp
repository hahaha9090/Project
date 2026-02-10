#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/epoll.h>

#define PORT 8888           // 端口
#define MAX_CLIENTS 100     // 客户端数组大小
#define BUF_SIZE 4096       // 缓冲区大小
#define MAX_EVENTS 20       // epoll每次处理的最大事件数

// 消息类型
enum MsgType {
    MSG_TEXT = 0,           // 文本消息
    MSG_FILE_INFO = 1,      // 文件上传信息
    MSG_FILE_REQUEST = 2,   // 文件下载请求
    MSG_FILE_DATA = 3       // 文件数据
};

// 客户端结构体
struct ClientContext {
    int fd;                 // Socket描述符，-1表示位置空闲
    char ip[32];            // 客户端IP地址
    FILE* fp;               // 文件指针
    char fileName[256];     // 存文件名
    long long totalSize;    // 记录文件的总大小
    long long currentSize;  // 记录当前接收大小
};
struct ClientContext clients[MAX_CLIENTS];          // 客户端结构体数组

// 从路径提取文件名
void getFileName(const char* path, char* dest) {
    const char* lastSlash = strrchr(path, '/');
    const char* lastBackSlash = strrchr(path, '\\');
    const char* filename = path;

    if (lastSlash > filename) filename = lastSlash + 1; // 如果有斜杠，更新文件名指针
    if (lastBackSlash > filename) filename = lastBackSlash + 1; // 如果有反斜杠，更新文件名指针

    strcpy(dest, filename);
}

// 发送所有数据
int sendAll(int fd, const char* data, int len) {
    int totalSent = 0;
    while (totalSent < len) {
        int sent = write(fd, data + totalSent, len - totalSent);
        if (sent <= 0) return 0; // 失败
        totalSent += sent;
    }
    return 1;
}

// 接收所有数据
int recvAll(int fd, char* buf, int len) {
    int totalRecv = 0;
    while (totalRecv < len) {
        int n = read(fd, buf + totalRecv, len - totalRecv);
        if (n <= 0) return 0; // 失败
        totalRecv += n;
    }
    return 1;
}

// 查找客户端上下文
struct ClientContext* findClient(int fd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd == fd) {
            return &clients[i];
        }
    }
    return NULL;
}


int main() {
    for (int i = 0; i < MAX_CLIENTS; i++) { clients[i].fd = -1; }   // 先把客户端数组置为 -1

    // 1. 创建监听 Socket
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) { perror("socket"); return 1; }
    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));  // 端口复用

    // 2. 绑定
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(PORT);
    if (bind(listenfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) { perror("bind"); return 1; }
    // 3. 监听
    if (listen(listenfd, 10) < 0) { perror("listen"); return 1; }

    // 创建 epoll 实例
    int epollfd = epoll_create1(0);  if (epollfd < 0) { perror("epoll_create1"); return 1; }

    // 添加 listenfd 到 epoll
    struct epoll_event ev, events[MAX_EVENTS];
    ev.events = EPOLLIN; // 监听读事件
    ev.data.fd = listenfd;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &ev) < 0) { perror("epoll_ctl: listenfd"); return 1; }
    printf("[开始] 服务器在端口 %d 上运行\n", PORT);

    while (1) {
        // 等待事件
        int nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (nfds < 0) { perror("epoll_wait"); break; }

        for (int n = 0; n < nfds; ++n) {
            // 情况一：如果是新的连接
            if (events[n].data.fd == listenfd) {
                struct sockaddr_in cliAddr;
                socklen_t cliLen = sizeof(cliAddr);
                int connfd = accept(listenfd, (struct sockaddr*)&cliAddr, &cliLen);

                if (connfd >= 0) {
                    char ip[32];
                    strcpy(ip, inet_ntoa(cliAddr.sin_addr));
                    printf("[连接] %s\n", ip);

                    // 存放客户端信息
                    int index = -1;
                    for (int i = 0; i < MAX_CLIENTS; i++) {
                        if (clients[i].fd == -1) {
                            clients[i].fd = connfd;
                            strcpy(clients[i].ip, ip);
                            clients[i].fp = NULL;
                            clients[i].totalSize = 0;
                            clients[i].currentSize = 0;
                            index = i;
                            break;
                        }
                    }

                    if (index != -1) {
                        // 将新连接添加到 epoll
                        ev.events = EPOLLIN;    // 水平触发
                        ev.data.fd = connfd;
                        if (epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &ev) < 0) {
                            perror("epoll_ctl: connfd");
                            close(connfd);
                            clients[index].fd = -1;
                        }
                    }
                    else {
                        printf("服务器已满!\n");
                        close(connfd);
                    }
                }
                continue;
            }
            // 情况二：否则就是客户端消息，处理客户端消息
            struct ClientContext* client = findClient(events[n].data.fd);   // 找到对应客户端的指针

            // 读头
            char header[5];
            if (!recvAll(events[n].data.fd, header, 5)) {
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (clients[i].fd == events[n].data.fd) {   // 找到对应客户端，断开连接
                        printf("[断开连接] %s\n", clients[i].ip);
                        if (clients[i].fp) {    // 如果正在接收文件，先关闭文件
                            fclose(clients[i].fp);
                            clients[i].fp = NULL;
                        }
                        clients[i].fd = -1;
                        close(events[n].data.fd);
                        break;
                    }
                }
                continue;
            }
            int type = (unsigned char)header[0];
            unsigned int msgLen = 0;
            memcpy(&msgLen, header + 1, 4);

            // 读取体
            char* body = (char*)malloc(msgLen + 1); // +1 为了字符串结束符安全
            if (!body) {
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (clients[i].fd == events[n].data.fd) {
                        printf("[断开连接] %s\n", clients[i].ip);
                        if (clients[i].fp) {
                            fclose(clients[i].fp);
                            clients[i].fp = NULL;
                        }
                        clients[i].fd = -1;
                        close(events[n].data.fd);
                        break;
                    }
                }
                continue;
            }

            if (msgLen > 0) {
                if (!recvAll(events[n].data.fd, body, msgLen)) {
                    free(body);
                    for (int i = 0; i < MAX_CLIENTS; i++) {
                        if (clients[i].fd == events[n].data.fd) {
                            printf("[断开连接] %s\n", clients[i].ip);
                            if (clients[i].fp) {
                                fclose(clients[i].fp);
                                clients[i].fp = NULL;
                            }
                            clients[i].fd = -1;
                            close(events[n].data.fd);
                            break;
                        }
                    }
                    continue;
                }
            }
            body[msgLen] = '\0'; // 确保是字符串

            // 处理消息
            if (type == MSG_TEXT) {
                // 构造: "IP: 消息"
                char payload[BUF_SIZE];
                snprintf(payload, sizeof(payload), "%s: %s", client->ip, body);
                printf("[接收文本] %s\n", payload);

                // 转发
                char newHeader[5];
                newHeader[0] = MSG_TEXT;
                unsigned int newLen = strlen(payload);
                memcpy(newHeader + 1, &newLen, 4);


                // 广播消息
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (clients[i].fd != -1 ) {
                        sendAll(clients[i].fd, newHeader, 5);
                        sendAll(clients[i].fd, payload, newLen);
                    }
                }
            }
            else if (type == MSG_FILE_INFO) {
                // 格式: "文件名\n大小"
                char* split = strchr(body, '\n');   // 找到 \n 的位置
                if (split) {
                    *split = '\0';  // '\n' 替换为字符串结束符 '\0' ，分割文件名和大小
                    char* sizeStr = split + 1;

                    char safeName[256];
                    getFileName(body, safeName);
                    long long fileSize = atoll(sizeStr);    // 把文件大小字符串转换成数字

                    // 更新客户端上下文
                    strcpy(client->fileName, safeName);
                    client->totalSize = fileSize;
                    client->currentSize = 0;
                    client->fp = fopen(safeName, "wb");

                    if (client->fp) {
                        printf("[上传开始] %s (%lld 字节)\n", safeName, fileSize);
                    }
                    else {
                        printf("[ERROR] 文件创建失败: %s\n", safeName);
                    }
                }
            }
            else if (type == MSG_FILE_DATA) {
                if (client->fp) {
                    fwrite(body, 1, msgLen, client->fp);
                    client->currentSize += msgLen;

                    if (client->currentSize >= client->totalSize) {
                        fclose(client->fp);
                        client->fp = NULL;
                        printf("[上传完毕] %s\n", client->fileName);

                        // 广播完成消息: "IP\n文件名\n大小"
                        char payload[BUF_SIZE];
                        snprintf(payload, sizeof(payload), "%s\n%s\n%lld", client->ip, client->fileName, client->totalSize);
                        char newHeader[5];
                        newHeader[0] = MSG_FILE_INFO;
                        unsigned int newLen = strlen(payload);
                        memcpy(newHeader + 1, &newLen, 4);

                        // 广播消息
                        for (int i = 0; i < MAX_CLIENTS; i++) {
                            if ( clients[i].fd != -1 ) { // 
                                sendAll(clients[i].fd, newHeader, 5);
                                sendAll(clients[i].fd, payload, newLen);
                            }
                        }
                    }
                }
            }
            else if (type == MSG_FILE_REQUEST) {
                char safeName[256];
                getFileName(body, safeName);
                printf("[文件下载请求] %s\n", safeName);

                FILE* f = fopen(safeName, "rb");    // 只读打开文件，确保文件存在且可读
                if (f) {
                    fseek(f, 0, SEEK_END);
                    long long fileSize = ftell(f);  // 获取文件大小
                    fseek(f, 0, SEEK_SET);

                    // 发送 MSG_FILE_INFO
                    char payload[BUF_SIZE];
                    snprintf(payload, sizeof(payload), "%s\n%lld", safeName, fileSize);
                    char infoHeader[5];
                    infoHeader[0] = MSG_FILE_INFO;
                    unsigned int infoLen = strlen(payload);
                    memcpy(infoHeader + 1, &infoLen, 4);

                    sendAll(events[n].data.fd, infoHeader, 5);
                    sendAll(events[n].data.fd, payload, infoLen);

                    // 发送 MSG_FILE_DATA
                    char fileBuf[BUF_SIZE];
                    int bytesRead;
                    while ((bytesRead = fread(fileBuf, 1, BUF_SIZE, f)) > 0) {
                        char dataHeader[5];
                        dataHeader[0] = MSG_FILE_DATA;
                        unsigned int chunkLen = bytesRead;
                        memcpy(dataHeader + 1, &chunkLen, 4);

                        sendAll(events[n].data.fd, dataHeader, 5);
                        sendAll(events[n].data.fd, fileBuf, bytesRead);
                    }
                    fclose(f);
                    printf("[FILE SENT] %s\n", safeName);
                }
            }
            free(body);
        }
    }
    close(listenfd);
    close(epollfd);

    return 0;
}