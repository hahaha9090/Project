#pragma once
#include <cstddef>

class hstring
{
private:
	//缓冲区相关成员
    char* buffer;                // 动态缓冲区
    size_t buffer_size;           // 缓冲区总容量
    size_t data_length;           // 实际存储字符串长度
    static const size_t INIT_BUFF_SIZE = 128; // 初始缓冲区大小

    // 构造工具函数：不能使用 strlen/strcpy/strcat
    static size_t str_len(const char* str);         // 计算 C 字符串长度
    static void copy_chars(char* dest, const char* src, size_t count);  // 复制字符
    static void move_chars(char* dest, const char* src, size_t count);  // 移动字符，处理重叠区域

    // 确保缓冲区容量（包含结尾 '\0'）
    void ensure_capacity(size_t required);

public:

    hstring();                       // 默认构造函数，分配 INIT_BUFF_SIZE 大小缓冲区
    hstring(const char* str);        // C字符串构造
    hstring(const hstring& other);   // 拷贝构造
    ~hstring();                      // 析构

    hstring& operator=(const hstring& other);   // 赋值运算符（hstring = hstring）
    hstring& operator=(int value);              // 赋值运算符（hstring = int）

    // 加法运算符重载
    hstring operator+(const hstring& other) const;   // 实现hstring + hstring

    // 减法运算符重载（删除首次出现的子串）
	hstring operator-(const char* sub) const;       //实现 hstring - C字符串

    // 查找子串，返回首次出现位置，未找到返回 -1
	int find(const char* sub) const;                //实现子串查找

    // 未找到子串的时候输出错误提示并返回 false
    bool replace(size_t pos, const char* target, const char* replacement);

    size_t length() const { return data_length; }

    // 获取内部 C 字符串，仅用于只读输出
    const char* getvalue_cstr() const { return buffer; }
};

