#include "test.pb.h"
#include <istream>
#include <string>

// int main()
// {
//     fixbug::LoginRequest req;
//     req.set_name("zhangsan");
//     req.set_pwd("123456");

//     std::string send_str;
//     if(req.SerializeToString(&send_str)){
//         std::cout << send_str << std::endl;
//     }

//     fixbug::LoginRequest reqB;
//     if(reqB.ParseFromString(send_str)){
//         std::cout << reqB.name() << std::endl;
//         std::cout << reqB.pwd() << std::endl;
//     }

//     return 0;
// }

int main()
{
    fixbug::LoginResponse resp;
    fixbug::ResultCode *rc = resp.mutable_result();
    rc->set_errcode(1);
    rc->set_errmsg("错误");

    fixbug::GetFirendListResponse fresp;
    fixbug::ResultCode *frc = fresp.mutable_result();
    frc->set_errcode(0);

    fixbug::User *user1 = fresp.add_friend_list();
    user1->set_name("zhangsan");
    user1->set_age(12);
    user1->set_sex(fixbug::User::MAN);

    fixbug::User *user2 = fresp.add_friend_list();
    user2->set_name("lisi");
    user2->set_age(13);
    user2->set_sex(fixbug::User::MAN);

    std::cout << fresp.friend_list_size() << std::endl;

    return 0;
}