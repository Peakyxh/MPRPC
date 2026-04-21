#include "mprpcchannel.h"
#include <string>
#include "rpcheader.pb.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include "mprpcapplication.h"
#include "zookeeperutil.h"

// 统一做rpc方法调用数据，进行数据序列化和网络发送
void MprpcChannel::CallMethod(const google::protobuf::MethodDescriptor* method,
                            google::protobuf::RpcController* controller, 
                            const google::protobuf::Message* request,
                            google::protobuf::Message* response, 
                            google::protobuf::Closure* done)
{
    // 获取service_name
    const google::protobuf::ServiceDescriptor* sd = method->service();
    std::string service_name = sd->name();
    // 获取method_name
    std::string method_name = method->name();

    // 获取参数长度args_size
    uint32_t args_size = 0;
    std::string args_str;
    if(request->SerializeToString(&args_str)){
        args_size = args_str.size();
    }else{
        controller->SetFailed("Serialize args_str error!");
        return;
    }

    // 定义rpcHeader
    mprpc::RpcHeader rpcHeader;
    rpcHeader.set_service_name(service_name);
    rpcHeader.set_method_name(method_name);
    rpcHeader.set_args_size(args_size);

    // 得到rpcHeader size
    uint32_t rpc_header_size = 0;
    std::string rpc_header_str;
    if(rpcHeader.SerializeToString(&rpc_header_str)){
        rpc_header_size = rpc_header_str.size();
    }else{
        controller->SetFailed("Serialize rpc_header error!");
        return;
    }

    // 组织发送的rpc请求的字符串
    std::string send_rpc_str;
    send_rpc_str.insert(0, std::string((char *)&rpc_header_size, 4));
    send_rpc_str += rpc_header_str;
    send_rpc_str += args_str;

    // 打印
    std::cout << "============================" << std::endl;
    std::cout << "service_name: " << service_name << std::endl;
    std::cout << "method_name: " << method_name << std::endl;
    std::cout << "args_str: " << args_str << std::endl;
    std::cout << "============================" << std::endl;

    // 使用tcp编程，完成rpc方法的远程调用
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if(-1 == clientfd){
        char errtxt[512] = {0};
        sprintf(errtxt, "create socket error, errnum: %d", errno);
        controller->SetFailed(errtxt);
        return;
    }

    // 从配置文件获取ip和port
    // std::string ip = MprpcApplication::GetInstance().GetConfig().Load("rpcserviceip");
    // uint16_t port = atoi(MprpcApplication::GetInstance().GetConfig().Load("rpcserviceport").c_str());
    // 改为查询zk上该服务所在的host信息
    ZkClient zkClient;
    zkClient.Start();
    std::string method_path = "/" + service_name + "/" + method_name;
    std::string host_data = zkClient.GetData(method_path.c_str());
    if("" == host_data){
        controller->SetFailed(method_path + " is not exist!");
        return;
    }
    int idx = host_data.find(":");
    if(-1 == idx){
        controller->SetFailed(method_path + " address is invalid!");
        return;
    }
    std::string ip = host_data.substr(0, idx);
    uint16_t port = atoi(host_data.substr(idx+1, host_data.size()-idx).c_str());

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip.c_str());

    // 连接rpc服务节点
    if(-1 == connect(clientfd, (struct sockaddr*)&server_addr, sizeof(server_addr))){
        close(clientfd);
        char errtxt[512] = {0};
        sprintf(errtxt, "connect socket error, errnum: %d", errno);
        controller->SetFailed(errtxt);
        return;
    }

    // 发送rpc请求
    if(-1 == send(clientfd, send_rpc_str.c_str(), send_rpc_str.size(), 0)){
        close(clientfd);
        char errtxt[512] = {0};
        sprintf(errtxt, "send error, errnum: %d", errno);
        controller->SetFailed(errtxt);
        return;
    }

    // 接收rpc的响应值
    char recv_buf[1024] = {0};
    int recv_size = recv(clientfd, recv_buf, 1024, 0);
    if(-1 == recv_size){
        close(clientfd);
        char errtxt[512] = {0};
        sprintf(errtxt, "recv error, errnum: %d", errno);
        controller->SetFailed(errtxt);
        return;
    }

    // 反序列化rpc调用的响应值
    // std::string response_str(recv_buf, 0, recv_size);
    // if(!response->ParseFromString(response_str)){
    if(!response->ParseFromArray(recv_buf, recv_size)){
        close(clientfd);
        char errtxt[2048] = {0};
        sprintf(errtxt, "Parse response_str error, recv_buf: %s", recv_buf);
        controller->SetFailed(errtxt);
        return;
    }

    close(clientfd);
}