//
// Created by cc on 8/4/21.
//

#ifndef CC_WEBSERVER_MUDUO_NONCOPYABLE_H
#define CC_WEBSERVER_MUDUO_NONCOPYABLE_H
/* noncopyable的构造有两种方式
1是将拷贝构造函数provate
2是将拷贝构造函数声明为delete
*/
class noncopyable {
protected:
    noncopyable() {}
    ~noncopyable() {}

private:
    noncopyable(const noncopyable&);
    noncopyable& operator=(const noncopyable&);
};



#endif //CC_WEBSERVER_MUDUO_NONCOPYABLE_H
