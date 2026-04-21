#include "mprpcprovider.h"
#include "mprpcapplication.h"
#include "rpcheader.pb.h"
#include "logger.h"
#include "zookeeperutil.h"

// 框架提供给外部使用的，可以发布rpc方法的函数接口
void RpcProvider::NotifyService(google::protobuf::Service *service){
    ServiceInfo service_info;

    // 获取服务对象的描述信息
    const google::protobuf::ServiceDescriptor *pserviceDesc = service->GetDescriptor();
    // 获取服务的名字
    std::string service_name = pserviceDesc->name();
    // 获取服务service方法数量
    int method_count = pserviceDesc->method_count();

    std::cout << "service_name: " << service_name << std::endl;
    // LOG_INFO("service_name: %s", service_name);

    // 获取方法名称
    for(int i=0; i<method_count; i++){
        const google::protobuf::MethodDescriptor *pmethodDesc = pserviceDesc->method(i);
        // 方法名字
        std::string method_name = pmethodDesc->name();
        service_info.m_methodMap.insert({method_name, pmethodDesc});

        std::cout << "method_name: " << method_name << std::endl;
        // LOG_INFO("method_name: %s", method_name);
    }
    service_info.m_service = service;
    m_serviceMap.insert({service_name, service_info});
}

// 启动rpc服务节点，提供rpc远程网络调用服务
void RpcProvider::Run(){
    // 需要连接网络
    std::string ip = MprpcApplication::GetInstance().GetConfig().Load("rpcserviceip");
    uint16_t port = atoi(MprpcApplication::GetInstance().GetConfig().Load("rpcserviceport").c_str());

    muduo::net::InetAddress address(ip, port);

    // 创建TcpService对象
    muduo::net::TcpServer server(&m_eventLoop, address, "RpcProvider");
    // 进行连接回调和消息读写回调
    server.setConnectionCallback(std::bind(&RpcProvider::onConnection, this, std::placeholders::_1));
    server.setMessageCallback(std::bind(&RpcProvider::onMessage, this, std::placeholders::_1, 
        std::placeholders::_2, std::placeholders::_3));
    
    // 设置muduo线程数量
    server.setThreadNum(4);

    // 把当然rpc节点上要发布的服务全部注册到zk上面，让rpc client可以从zk上发现服务
    ZkClient zkClient;
    zkClient.Start();
    // service_name为永久节点，method_name为临时节点
    for(auto &sp: m_serviceMap){
        std::string service_path = "/" + sp.first;
        zkClient.Create(service_path.c_str(), nullptr, 0);
        for(auto &mp: sp.second.m_methodMap){
            std::string method_path = service_path + "/" + mp.first;
            char method_path_data[128] = {0};
            sprintf(method_path_data, "%s:%d", ip.c_str(), port);
            // ZOO_EPHEMERAL表示临时节点
            zkClient.Create(method_path.c_str(), method_path_data, strlen(method_path_data), ZOO_EPHEMERAL);
        }
    }

    std::cout << "RpcProvider start service at ip: " << ip << ", port: " << port << std::endl;
    // LOG_INFO("RpcProvider start service at ip: %s, port: %s", ip, port);

    // 启动网络服务
    server.start();
    m_eventLoop.loop();
}

// 连接回调
void RpcProvider::onConnection(const muduo::net::TcpConnectionPtr &conn){
    if(!conn->connected()){
        conn->shutdown();
    }
}

// 消息读写回调
void RpcProvider::onMessage(const muduo::net::TcpConnectionPtr &conn, 
        muduo::net::Buffer *buffer, muduo::Timestamp timestamp){
    // 网络上接收的远程rpc调用请求的字符流
    std::string recv_buf = buffer->retrieveAllAsString();

    // 从字符流中读取前4个字节的内容
    uint32_t header_size = 0;
    recv_buf.copy((char*)&header_size, 4, 0);

    // 根据header_size读取数据头的原始字符流，反序列化数据，得到rpc请求的详细信息
    std::string rpc_header_str = recv_buf.substr(4, header_size);
    mprpc::RpcHeader rpcHeader;
    std::string service_name;
    std::string method_name;
    uint32_t args_size;
    if(rpcHeader.ParseFromString(rpc_header_str)){
        // 数据头反序列化成功
        service_name = rpcHeader.service_name();
        method_name = rpcHeader.method_name();
        args_size = rpcHeader.args_size();
    }else{
        // 数据头反序列化失败
        // std::cout << "rpc_header_str: " << rpc_header_str << " parse error!" << std::endl;
        LOG_ERR("rpc_header_str: %s parse error!", rpc_header_str.c_str());
        return;
    }

    // 获取rpc方法参数的字节流
    std::string args_str = recv_buf.substr(4+header_size, args_size);

    // 打印
    std::cout << "============================" << std::endl;
    std::cout << "service_name: " << service_name << std::endl;
    std::cout << "method_name: " << method_name << std::endl;
    std::cout << "args_str: " << args_str << std::endl;
    std::cout << "============================" << std::endl;

    // 获取service和method对象
    auto it = m_serviceMap.find(service_name);
    if(it == m_serviceMap.end()){
        std::cout << service_name << " is not exist" << std::endl;
        return;
    }

    auto mit = it->second.m_methodMap.find(method_name);
    if(mit == it->second.m_methodMap.end()){
        std::cout << service_name << ":" << method_name << " is not exist" << std::endl;
        return;
    }

    google::protobuf::Service *service = it->second.m_service;  // 获取service
    const google::protobuf::MethodDescriptor *method = mit->second;  //获取method

    // 生成rpc方法调用的请求request和响应response参数
    google::protobuf::Message *request = service->GetRequestPrototype(method).New();
    if(!request->ParseFromString(args_str)){
        std::cout << args_str << " parse error!" << std::endl;
        return;
    }
    google::protobuf::Message *response = service->GetResponsePrototype(method).New();

    // 给下面的method方法的调用，绑定一个Closure回调操作
    google::protobuf::Closure *done = google::protobuf::NewCallback<RpcProvider, 
        const muduo::net::TcpConnectionPtr&, google::protobuf::Message*>
        (this, &RpcProvider::sendRpcResponse, conn, response);

    // 在框架上根据远程rpc请求，调用当前rpc节点发布的方法
    service->CallMethod(method, nullptr, request, response, done);
}

// Closure的回调操作，用于序列化rpc的响应和网络发送
void RpcProvider::sendRpcResponse(const muduo::net::TcpConnectionPtr &conn, google::protobuf::Message *response){
    std::string response_str;
    // 进行序列化
    if(response->SerializeToString(&response_str)){
        // 通过网络把rpc方法执行结果发送给rpc调用方
        conn->send(response_str);
    }else{
        std::cout << "Serialize response_str error!" << std::endl;
    }

    // 执行完，由rpcprovider主动断开连接
    conn->shutdown();
}