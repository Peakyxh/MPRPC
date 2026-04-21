#include "mprpcapplication.h"
#include "unistd.h"
#include <iostream>
#include <string>

MprpcConfig MprpcApplication::m_config;

void ShowArgsHelp(){
    std::cout << "format: command -i <configfile>" << std::endl;
}

// 初始化操作
void MprpcApplication::Init(int argc, char **argv){
    if(argc < 2){
        ShowArgsHelp();
        exit(EXIT_FAILURE);
    }

    int c = 0;
    std::string config_file;
    while((c = getopt(argc, argv, "i:")) != -1){
        switch (c)
        {
        case 'i':
            config_file = optarg;
            break;
        case '?':
            ShowArgsHelp();
            exit(EXIT_FAILURE);
        case ':':
            ShowArgsHelp();
            exit(EXIT_FAILURE);
        default:
            break;
        }
    }

    // 开始加载配置文件
    m_config.LoadConfigFile(config_file.c_str());

    // std::cout << "rpcserviceip: " << m_config.Load("rpcserviceip") << std::endl;
    // std::cout << "rpcserviceport: " << m_config.Load("rpcserviceport") << std::endl;
    // std::cout << "zookeeperip: " << m_config.Load("zookeeperip") << std::endl;
    // std::cout << "zookeeperport: " << m_config.Load("zookeeperport") << std::endl;
}

// 单例模式，得到实例
MprpcApplication& MprpcApplication::GetInstance()
{
    static MprpcApplication app;
    return app;
}

MprpcConfig& MprpcApplication::GetConfig(){
    return m_config;
}