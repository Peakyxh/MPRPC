#include "mprpcconfig.h"

#include <iostream>
#include <string>

// 加载配置文件
void MprpcConfig::LoadConfigFile(const char *config_file){
    FILE *pf = fopen(config_file, "r");

    // 文件不存在
    if(nullptr == pf){
        std::cout << config_file << " is not exsit" << std::endl;
        exit(EXIT_FAILURE);
    }

    // 读取文件内容
    while (!feof(pf))
    {
        char buf[512] = {0};
        fgets(buf, 512, pf);

        std::string read_buf(buf);
        // 去掉字符串前后多余的空格
        Trim(read_buf);

        // 去掉注释和空行
        if(read_buf[0] == '#' || read_buf.empty()){
            continue;
        }

        int idx = read_buf.find('=');
        if(-1 == idx){
            continue;
        }
        std::string key = read_buf.substr(0, idx);
        Trim(key);
        int end_idx = read_buf.find('\n', idx);
        std::string value = read_buf.substr(idx+1, end_idx-idx-1);
        Trim(value);
        m_configMap.insert({key, value});
    }
    
}

// 查询配置项信息
std::string MprpcConfig::Load(const std::string &key){
    auto it = m_configMap.find(key);
    if(it == m_configMap.end()){
        return "";
    }

    return it->second;
}

// 去掉字符串前后空格
void MprpcConfig::Trim(std::string &src_buf){
    // 去掉字符串前多余的空格
    int idx = src_buf.find_first_not_of(' ');
    if(-1 != idx){
        src_buf = src_buf.substr(idx, src_buf.size()-idx);
    }
    // 去掉字符串后多余的空格
    idx = src_buf.find_last_not_of(' ');
    if(-1 != idx){
        src_buf = src_buf.substr(0, idx+1);
    }
}