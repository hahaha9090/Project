#include <iostream>
#include "hstring.h"

//默认构造函数，对缓冲区进行初始化
hstring::hstring() : buffer{ nullptr }, buffer_size{ INIT_BUFF_SIZE }, data_length{ 0 }
{
    buffer = new char[buffer_size];
	buffer[0] = '\0';   // 先初始化为空字符串
}


// C字符串构造 —— 利用这一个接收字符串，完成对字符串的初始化
hstring::hstring(const char* str) : buffer{ nullptr }, buffer_size{ INIT_BUFF_SIZE }, data_length{ 0 }
{
    buffer = new char[buffer_size];
    buffer[0] = '\0';

	size_t len = str_len(str);  // 调用str_len()获取字符串长度
	ensure_capacity(len + 1);   // 调用ensure_capacity()，根据输入的字符串长度判断是否需要扩容

	if (len > 0)    //判断输入的字符串是否有长度
    {
		copy_chars(buffer, str, len);   // 复制字符
    }
	buffer[len] = '\0';     // 结尾添加 '\0'，确保是 C 字符串
	data_length = len;      // 更新实际长度
}


// 拷贝构造 —— 利用这一个接收 hstring 对象，完成对字符串的初始化
hstring::hstring(const hstring& other) : buffer{ nullptr }, buffer_size{ INIT_BUFF_SIZE }, data_length{ 0 }
{
    buffer = new char[buffer_size];
    buffer[0] = '\0';

    ensure_capacity(other.data_length + 1);
	if (other.data_length > 0)      //判断被拷贝的字符串是否有长度
    {
		copy_chars(buffer, other.buffer, other.data_length);
    }
    buffer[other.data_length] = '\0';
    data_length = other.data_length;
}

//析构函数，释放内存
hstring::~hstring()
{
    delete[] buffer;
    buffer = nullptr;
    buffer_size = 0;
    data_length = 0;
}

/*--------------------工具函数的实现--------------------*/

// 计算 C 字符串长度
size_t hstring::str_len(const char* str)
{
	if (!str)       // 如果是空指针
    {
        return 0;   // 直接返回
    }
    size_t len = 0;
    while (str[len] != '\0')    //统计输入字符串长度
    {
        ++len;
    }
    return len;
}

// 复制字符
void hstring::copy_chars(char* dest, const char* src, size_t count)
{
    for (size_t i = 0; i < count; ++i)
    {
        dest[i] = src[i];
    }
}

// 移动字符
void hstring::move_chars(char* dest, const char* src, size_t count)
{
    if (dest == src || count == 0)  // 如果源字符串和目标相同，或者需要复制的长度为0，直接返回
    {
        return;
    }

	//判断源字符串和目标字符串的内存区域是否有重叠，来决定是采用正向复制还是反向复制
	if (dest < src)         //如果目标地址小于源地址，正向复制
    {
		for (size_t i = 0; i < count; ++i)  //一点一点往前复制
        {
            dest[i] = src[i];
        }
    }else                  //如果目标地址大于源地址，反向复制
    {
		for (size_t i = count; i > 0; --i)  //一点点往后复制
        {
            dest[i - 1] = src[i - 1];
        }
    }
}

// 确保缓冲区容量（包含结尾 '\0'）
void hstring::ensure_capacity(size_t required)
{
	if (required <= buffer_size) {      //判断当前容量够不够
		return;     //不需要扩容直接返回
    }

	// 需要扩容
	size_t new_size = buffer_size;
	while (new_size < required)     //判断new_size的容量是不是还是不够
    {
        new_size *= 2; // 扩容的时候，按照 buffer_size * 2 扩容
    }

	char* new_buf = new char[new_size];     // 分配新的，能够满足空间大小的缓冲区

	if (data_length > 0)  //如果原缓冲区有数据
    {
		copy_chars(new_buf, buffer, data_length);   // 复制原数据
    }
	new_buf[data_length] = '\0';    // 结尾添加 '\0'

	//更新缓冲区指针和大小
    delete[] buffer;
    buffer = new_buf;
    buffer_size = new_size;
}


/*--------------------重载运算符的实现--------------------*/

// 重载赋值运算符
hstring& hstring::operator=(const hstring& other)
{

    ensure_capacity(other.data_length + 1);
    if (other.data_length > 0)
    {
        copy_chars(buffer, other.buffer, other.data_length);
    }
    buffer[other.data_length] = '\0';
	data_length = other.data_length;
    return *this;
}

// 重载赋值运算符，把int整数，转换成字符形式，能对输入的正数和负数都能处理。
hstring& hstring::operator=(int value)
{
	//设置一个临时缓冲区，用来存放转换后的字符
	char temp[16];          //int最多是10位 + 符符号，再加上结尾的'\0'，16足够用了
	int index = 0;          //弄个索引，用来存放字符位置
	bool negative = false;  //再看看是不是负数


	if (value == 0)     //如果输入的是0
    {
        temp[index++] = '0';    //直接放进去
    }
    else if (value < 0)  //如果输入的是负数
    {
        negative = true;        //标记是负数
		value = -value;         //转换成正数

        while (value > 0)       //在把转换成正数后的数，一个个取出来放进去
        {
            temp[index++] = '0' + (value % 10);
            value /= 10;
        }
    }
    else                 //否则就是正数
    {
        while (value > 0)       //直接一个个取出来放进去
        {
            temp[index++] = '0' + (value % 10);
            value /= 10;
        }
    }

	if (negative)//如果是负数
    {
        temp[index++] = '-';    //先把负号放进去
    }

    //最后，temp里面存放的数字是倒序的，需要把它们反过来
    for (int i = 0; i < index / 2; ++i)
    {
		std::swap(temp[i], temp[index - 1 - i]);
    }
	temp[index] = '\0';         //在补上\0


	//然后把转换后的字符串，放到缓冲区
    ensure_capacity(index + 1);             // 确保缓冲区足够容纳替换后的字符串
	copy_chars(buffer, temp, index + 1);    // 复制转换后的字符串到缓冲区
	data_length = index;                    // 再把实际长度更新了
    return *this;
}


//重载加法运算符，实现 hstring + hstring
hstring hstring::operator+(const hstring& other) const
{
	hstring result;     // 创建一个hstring对象方便存放结果
	size_t total_len = data_length + other.data_length;     // 计算两个字符串拼接后的总长度

	result.ensure_capacity(total_len + 1);  // 确保缓冲区容量足够存放拼接后的字符串
	if (data_length > 0)    //如果第一个字符串（左操作数）有长度
    {
		copy_chars(result.buffer, buffer, data_length); // 复制第一个字符串
    }
	if (other.data_length > 0)  //如果第二个字符串（右操作数）有长度
	{   // 复制第二个字符串到第一个字符串的后面
		copy_chars(result.buffer + data_length, other.buffer, other.data_length);
        // result.buffer 指向结果字符串的起始位置，
        // 加上第一个字符串的字符长度 data_length，
        // 得到第二个字符串开始拷贝的位置
    }
    result.buffer[total_len] = '\0';
    result.data_length = total_len;
    return result;
}

// 查找子串，返回首次出现位置，未找到返回 -1
int hstring::find(const char* sub) const

{   //用朴素贝叶斯法实现子串查找

    size_t sub_len = str_len(sub);  //先记录字串的长度

	if (sub_len == 0)   //如果子串长度为0
    {
        std::cerr << "[ERROR] 查找子串长度为 0!" << std::endl;
        return -1;
    }
	if (sub_len > data_length)  //如果子串长度大于原串
    {
        std::cerr << "[ERROR] 子串长度大于原串!" << std::endl;
        return -1;
    }

	for (size_t i = 0; i + sub_len <= data_length; ++i) //逐个位置匹配子串
    {
        size_t j = 0;
        while (j < sub_len && buffer[i + j] == sub[j])
        {
            ++j;
        }
        if (j == sub_len)
        {
			return static_cast<int>(i); //返回子串首次出现的位置i
        }
    }

    std::cerr << "[ERROR] 子串\"" << sub << "\"未找到!" << std::endl;
    return -1;
}

// 重载减法运算符（删除首次出现的子串）
hstring hstring::operator-(const char* sub) const
{
    hstring result(*this); //先拷贝一份到result，尽量不修改原字符串

    size_t sub_len = str_len(sub);
    if (sub_len == 0)
    {
        std::cerr << "[ERROR] 子串长度为 0，无法删除!" << std::endl;
        return result;
    }

	int pos = result.find(sub); //利用find()函数查找子串位置
    if (pos < 0)
    {
        return result;
    }

	size_t start = static_cast<size_t>(pos);    //记录子串起始位置
	size_t tail_start = start + sub_len;        //记录子串后面部分的起始位置
	size_t tail_len = result.data_length - tail_start;      //记录子串后面部分的长度

	// 将匹配到的子串后面的部分整体左移，覆盖掉子串。
	move_chars(result.buffer + start, result.buffer + tail_start, tail_len);
    result.data_length -= sub_len;  // 更新实际长度
	result.buffer[result.data_length] = '\0';   // 结尾添加 '\0'

    return result;
}


// 修改操作：从指定位置开始，把目标字串，替换成替换字串
bool hstring::replace(size_t pos, const char* target, const char* replacement)
{
	size_t target_len = str_len(target);        // 目标子串长度
	size_t repl_len = str_len(replacement);     // 替换子串长度

	if (target_len == 0)    //目标子串长度不能为0
    {
        std::cerr << "[ERROR] 目标子串长度为 0!" << std::endl;
        return false;
    }
	if (pos > data_length)  //要替换的位置不能越界
    {
        std::cerr << "[ERROR] 起始位置越界!" << std::endl;
        return false;
    }

    // 从指定位置检查是否匹配目标子串
    if (pos + target_len > data_length)
    {
        std::cerr << "[ERROR] 从指定位置开始，目标子串超出原串范围!" << std::endl;
        return false;
    }

	for (size_t j = 0; j < target_len; ++j) //逐个字符比较，先确保目标子串匹配
    {
        if (buffer[pos + j] != target[j])
        {
            std::cerr << "[ERROR] 子串" << target << "未在指定位置找到!" << std::endl;
			return false;   //返回 false
        }
    }

	size_t new_length = data_length - target_len + repl_len;    //先计算替换后的新长度

	if (new_length + 1 > buffer_size)   //判断是否需要扩容
    {
        ensure_capacity(new_length + 1);
    }

	size_t tail_start = pos + target_len;           //记录目标子串后面部分的起始位置
	size_t tail_len = data_length - tail_start;     //记录目标子串后面部分的长度

	if (repl_len > target_len)      // 如果替换子串更长
    {
		size_t shift = repl_len - target_len;   //计算需要右移的位数

		for (size_t i = tail_len; i > 0; --i)   //尾部整体右移
        {
            buffer[tail_start + shift + i - 1] = buffer[tail_start + i - 1];
        }
    }
    else if (repl_len < target_len) //如果替换子串更短
    {
        size_t shift = target_len - repl_len;   //计算需要左移的位数

		for (size_t i = 0; i < tail_len; ++i)   //尾部整体左移
        {
            buffer[tail_start - shift + i] = buffer[tail_start + i];
        }
    }

 
    for (size_t i = 0; i < repl_len; ++i)   //把替换子串复制到目标位置
    {
        buffer[pos + i] = replacement[i];
    }

	data_length = new_length;       //更新实际长度
    buffer[data_length] = '\0';     // 结尾添加 '\0'
    return true;
}
