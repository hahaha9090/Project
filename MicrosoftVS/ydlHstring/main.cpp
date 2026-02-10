#include <iostream>
#include "hstring.h"

int main()
{

    std::cout << "----------------------子串拼接测试---------------------" << std::endl;
    hstring s1("123456789");
    hstring s2("abc");
    hstring add1 = s1 + s2;             // hstring + hstring
    std::cout << "字符串\"123456789\"+字符串\"abc\"的结果为：  ";
    std::cout << add1.getvalue_cstr() << std::endl;
    std::cout << std::endl;


    std::cout << "----------------------子串删除测试----------------------" << std::endl;
    hstring s3("123456789");
    hstring sub1 = s3 - "456";          // 结果应为 "123789"
    std::cout << "从字符串\"123456789\"中，删除字串\"456\"的结果为：";
    std::cout << sub1.getvalue_cstr() << std::endl;
    std::cout << std::endl;


    std::cout << "----------------------子串替换测试----------------------" << std::endl;
    hstring s4("123456789");
    s4.replace(2, "34", "abc");
    std::cout << "将字符串\"123456789\"中的\"34\"替换为“abc”的结果为：";
    std::cout << s4.getvalue_cstr() << std::endl;
    std::cout << std::endl;


    std::cout << "--------------------子串位置查找测试--------------------" << std::endl;
    hstring s5("123456");
    int pos = s5.find("34");
    std::cout << "字符串\"123456\"中，子串\"34\"的位置为：";
    std::cout << pos << std::endl;
    std::cout << std::endl;


    std::cout << "--------------int类型转换为hstring类型测试--------------" << std::endl;
    hstring s6;
    s6 = 12345;
    std::cout << "int型\"12345\"转换为hstring型\"12345\"的结果为:";
    std::cout << s6.getvalue_cstr() << std::endl;
    std::cout << std::endl;

    hstring s7;
    s7 = -9870;
    std::cout << "int型\"-9870\"转换为hstring型\"-9870\"的结果为:";
    std::cout << s7.getvalue_cstr() << std::endl;
    std::cout << std::endl;


    std::cout << "----------------------边界情况测试----------------------" << std::endl;
    std::cout << "在字符串\"abcdef\"中替换\"xyz\"的结果为: ";
    hstring s8("abcdef");
    hstring sub2 = s8 - "xyz";         // 会输出错误信息
    std::cout << std::endl;

    // 替换不存在的子串
    std::cout << "在字符串\"abcdef\"中寻找\"xyz\"的结果为: ";
    int pos2 = s8.find("xyz");

    return 0;
}
