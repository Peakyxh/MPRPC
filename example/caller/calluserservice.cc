#include "user.pb.h"
#include "mprpcapplication.h"

int main(int argc, char **argv){

    MprpcApplication::Init(argc, argv);

    // 调用远程发布的rpc方法
    fixbug::UserService_Stub stub(new MprpcChannel());

    // Login的rpc请求
    fixbug::LoginRequest request;
    request.set_name("zhang san");
    request.set_pwd("123456");

    // rpc响应
    fixbug::LoginResponse response;

    MprpcController controller;

    stub.Login(&controller, &request, &response, nullptr);

    if(controller.Failed()){
        std::cout << controller.ErrorText() << std::endl;
    }else{
        if(0 == response.result().errcode()){
            std::cout << "rpc login response success: " << response.success() << std::endl;
        }else{
            std::cout << "rpc login response error, " << response.result().errmsg() << std::endl;
        }
    }

    // Register的rpc请求
    fixbug::RegisterRequest regt;
    regt.set_id(100);
    regt.set_name("mprpc");
    regt.set_pwd("666666");

    // rpc响应
    fixbug::RegisterResponse regs;

    stub.Register(nullptr, &regt, &regs, nullptr);

    if(0 == regs.result().errcode()){
        std::cout << "rpc Register response success: " << regs.success() << std::endl;
    }else{
        std::cout << "rpc Register response error, " << regs.result().errmsg() << std::endl;
    }

    return 0;
}