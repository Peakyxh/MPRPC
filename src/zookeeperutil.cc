#include "zookeeperutil.h"
#include "mprpcapplication.h"
#include "logger.h"
#include <semaphore.h>
#include <iostream>

/*
当会话成功连接时，从句柄中取出之前设置的上下文（一个信号量指针），
然后调用 sem_post 增加信号量，唤醒正在等待连接完成的主线程。
*/
void global_watcher(zhandle_t *zh, int type, 
        int state, const char *path,void *watcherCtx){
    if(type == ZOO_SESSION_EVENT){
        if(state == ZOO_CONNECTED_STATE){
            sem_t *sem = (sem_t*)zoo_get_context(zh);
            sem_post(sem);
        }
    }
}

ZkClient::ZkClient() : m_zhandle(nullptr) {

}

ZkClient::~ZkClient(){
    if(m_zhandle != nullptr){
        zookeeper_close(m_zhandle);
    }
}

// zkClient启动连接zkserver
void ZkClient::Start(){
    std::string host = MprpcApplication::GetInstance().GetConfig().Load("zookeeperip");
    std::string port = MprpcApplication::GetInstance().GetConfig().Load("zookeeperport");
    std::string connstr = host + ":" + port;

    // 初始化 ZooKeeper 句柄
    m_zhandle = zookeeper_init(connstr.c_str(), global_watcher, 3000, nullptr, nullptr, 0);
    if(nullptr == m_zhandle){
        LOG_ERR("zookeeper_init error!");
        exit(EXIT_FAILURE);
    }

    // 创建信号量并设置为句柄的上下文
    sem_t sem;
    sem_init(&sem, 0, 0);
    zoo_set_context(m_zhandle, &sem);

    // 等待全局回调中信号量 post
    sem_wait(&sem);
    std::cout << "zookeeper_init success!" << std::endl;
}

// 在zkserver上根据指定的path创建znode节点
void ZkClient::Create(const char *path, const char *data, int datalen, int state){
    char path_buffer[128];
    int bufferlen = sizeof(path_buffer);
    int flag;
    // 检查节点是否存在
    flag = zoo_exists(m_zhandle, path, 0, nullptr);
    if(ZNONODE == flag){
        // 节点不存在，则创建节点
        flag = zoo_create(m_zhandle, path, data, datalen, 
            &ZOO_OPEN_ACL_UNSAFE, state, path_buffer, bufferlen);
        if(flag == ZOK){
            std::cout << "znode create success, path:" << path << std::endl;
        }else{
            LOG_ERR("znode create error, path:%s", path);
            exit(EXIT_FAILURE);
        }
    }
}

// 根据参数指定的znode节点路径，获取znode节点上存储的数据
std::string ZkClient::GetData(const char *path){
    char buffer[64];
    int bufferlen = sizeof(buffer);
    int flag = zoo_get(m_zhandle, path, 0, buffer, &bufferlen, nullptr);
    if(flag != ZOK){
        LOG_ERR("get znode error, path:%s", path);
        return "";
    }else{
        return buffer;
    }
}