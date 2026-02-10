// VSMySQL.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <mysql.h>   // 引入 MySQL C API 库，用于进行 MySQL 数据库操作。


int main()
{
    /* ==================== 1. 创建并初始化 MYSQL 句柄 ==================== */

    // 声明一个 MYSQL 指针，用来表示数据库连接句柄
    MYSQL* conn = nullptr;
    //如果使用 MYSQL* conn = new MYSQL(); 后面必须要有 delete conn;

    // 初始化 MYSQL 结构体，传 NULL 表示由 MySQL 库内部申请 MYSQL 对象
    conn = mysql_init(nullptr);

    if (conn == nullptr)
    {
        std::cout << "mysql_init failed" << std::endl;
        return -1;      // 初始化失败，直接退出
    }

    /* ==================== 2. 连接数据库 ==================== */

    // 返回值仍然是 MYSQL*，失败返回 NULL
    if (mysql_real_connect(
        conn,               // 已初始化的 MYSQL 句柄
        "localhost",        // 数据库主机地址
        "root",             // 用户名
        "123123",           // 密码
        "mysql",            // 要使用的数据库名
        3306,               // MySQL 端口（3306为数据库默认端口）
        nullptr,            // Unix socket（一般为 NULL）
        0                   // 连接选项（通常为 0）
    ) == nullptr)
    {
        std::cout << "connect failed: " << mysql_error(conn) << std::endl;
        mysql_close(conn);	// 连接失败也要释放 MYSQL 资源
        return -1;
    }

    /* ==================== 3. 执行 SQL 语句（查询） ==================== */

    // 定义一条查询 SQL 语句
    const char* sql = "SELECT id, name, score FROM student";

    // 执行 SQL 语句
    // 成功返回 0，失败返回非 0
    if (mysql_query(conn, sql) != 0)
    {
        std::cout << "sql execute failed: "
            << mysql_error(conn) << std::endl;

        mysql_close(conn);
        return -1;
    }

    /* ==================== 4. 获取并处理查询结果 ==================== */

    // 将查询结果从服务器读取到客户端
    MYSQL_RES* result = mysql_store_result(conn);

    // 判断是否成功获取结果集
    if (result == nullptr)
    {
        std::cout << "get result failed: "
            << mysql_error(conn) << std::endl;

        mysql_close(conn);
        return -1;
    }

    // 定义一行数据指针
    MYSQL_ROW row;

    // 逐行读取结果集
    while ((row = mysql_fetch_row(result)) != nullptr)
    {
        // row[i] 是 char*，对应第 i 列的数据
        // 注意：字段值可能为 NULL，实际项目要判断
        std::cout << "id: " << row[0]
            << ", name: " << row[1]
            << ", score: " << row[2]
            << std::endl;
    }

    // 使用完结果集后，必须释放
    mysql_free_result(result);

    /* ==================== 5. 关闭数据库连接 ==================== */

    // 关闭数据库连接并释放 MYSQL 相关资源
    mysql_close(conn);

    return 0;   // 程序正常结束
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
