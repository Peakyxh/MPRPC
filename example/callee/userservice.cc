#include <string>
#include <iostream>
#include "user.pb.h"
#include "mprpcapplication.h"
#include "mprpcprovider.h"
#include "logger.h"

class UserService: public fixbug::UserService
{
public:
    bool Login(std::string name, std::string pwd){
        std::cout << "do Login service" << std::endl;
        std::cout << "name: " << name << std::endl;
        std::cout << "pwd: " << pwd << std::endl;
        return true;
    }

    bool Register(uint32_t id, std::string name, std::string pwd){
        std::cout << "do Register service" << std::endl;
        std::cout << "id: " << id << std::endl;
        std::cout << "name: " << name << std::endl;
        std::cout << "pwd: " << pwd << std::endl;
        return true;
    }

    // 重写UserService基类的虚函数
    void Login(::google::protobuf::RpcController* controller,
                       const ::fixbug::LoginRequest* request,
                       ::fixbug::LoginResponse* response,
                       ::google::protobuf::Closure* done)
    {   
        // 框架给业务报了请求参数LoginRqueset，应用获取相应数据做本地业务
        std::string name = request->name();
        std::string pwd = request->pwd();

        // 做本地业务
        bool login_result = Login(name, pwd);

        // 把响应写入
        response->set_success(login_result);
        fixbug::ResultCode *result_code = response->mutable_result();
        // result_code->set_errcode(0);
        // result_code->set_errmsg("");
        result_code->set_errcode(1);
        result_code->set_errmsg("do login error");

        // 执行回调操作，执行响应数据的序列化和网络发送（由框架完成）
        done->Run();
    }

    void Register(::google::protobuf::RpcController* controller,
                       const ::fixbug::RegisterRequest* request,
                       ::fixbug::RegisterResponse* response,
                       ::google::protobuf::Closure* done)
    {   
        uint32_t id = request->id();
        std::string name = request->name();
        std::string pwd = request->pwd();

        // 做本地业务
        bool register_result = Register(id, name, pwd);

        // 把响应写入
        response->set_success(register_result);
        fixbug::ResultCode *result_code = response->mutable_result();
        result_code->set_errcode(0);
        result_code->set_errmsg("");

        done->Run();
    }
};

int main(int argc, char **argv) {

    // 调用框架的初始化操作
    MprpcApplication::Init(argc, argv);

    // provider是一个rpc网络服务对象。 把UserService对象发布到rpc节点上
    RpcProvider provider;
    provider.NotifyService(new UserService());

    // 启动一个rpc服务发布节点
    provider.Run();

    return 0;
}