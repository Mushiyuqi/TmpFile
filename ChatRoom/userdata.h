#pragma once
#include <QString>
#include <memory>
#include <QJsonArray>

class TextChatData;
class SearchInfo {
public:
    SearchInfo(int uid, QString name, QString nick, QString desc, int sex, QString icon = "")
        :m_uid(uid),m_name(name), m_nick(nick),m_desc(desc),m_sex(sex), m_icon(icon)
    {}
    int m_uid;
    QString m_name;
    QString m_nick;
    QString m_desc;
    int m_sex;
    QString m_icon;
};

class AddFriendApply {
public:
    AddFriendApply(int from_uid, QString name, QString desc, QString icon, QString nick, int sex)
        :m_from_uid(from_uid),m_name(name),m_desc(desc),m_icon(icon), m_nick(nick), m_sex(sex)
    {}
    int m_from_uid;
    QString m_name;
    QString m_desc;
    QString m_icon;
    QString m_nick;
    int m_sex;
};

struct ApplyInfo {
    ApplyInfo(int uid, QString name, QString desc, QString icon, QString nick, int sex, int status)
    :m_uid(uid),m_name(name),m_desc(desc), m_icon(icon),m_nick(nick),m_sex(sex),m_status(status){}

    ApplyInfo(std::shared_ptr<AddFriendApply> addinfo)
        :m_uid(addinfo->m_from_uid),m_name(addinfo->m_name),
        m_desc(addinfo->m_desc),m_icon(addinfo->m_icon),
        m_nick(addinfo->m_nick),m_sex(addinfo->m_sex),
        m_status(0)
    {}

    void SetIcon(QString head){
        m_icon = head;
    }
    int m_uid;
    QString m_name;
    QString m_desc;
    QString m_icon;
    QString m_nick;
    int m_sex;
    int m_status;
};

struct AuthInfo {
    AuthInfo(int uid, QString name,
             QString nick, QString icon, int sex):
        m_uid(uid), m_name(name), m_nick(nick), m_icon(icon),
        m_sex(sex){}
    int m_uid;
    QString m_name;
    QString m_nick;
    QString m_icon;
    int m_sex;
};

struct AuthRsp {
    AuthRsp(int peer_uid, QString peer_name,
            QString peer_nick, QString peer_icon, int peer_sex)
        :m_uid(peer_uid),m_name(peer_name),m_nick(peer_nick),
        m_icon(peer_icon),m_sex(peer_sex)
    {}

    int m_uid;
    QString m_name;
    QString m_nick;
    QString m_icon;
    int m_sex;
};

struct FriendInfo {
    FriendInfo(int uid, QString name, QString nick, QString icon,
               int sex, QString desc, QString back, QString last_msg=""):m_uid(uid),
        m_name(name),m_nick(nick),m_icon(icon),m_sex(sex),m_desc(desc),
        m_back(back),m_last_msg(last_msg){}

    FriendInfo(std::shared_ptr<AuthInfo> auth_info):m_uid(auth_info->m_uid),
        m_nick(auth_info->m_nick),m_icon(auth_info->m_icon),m_name(auth_info->m_name),
        m_sex(auth_info->m_sex){}

    FriendInfo(std::shared_ptr<AuthRsp> auth_rsp):m_uid(auth_rsp->m_uid),
        m_nick(auth_rsp->m_nick),m_icon(auth_rsp->m_icon),m_name(auth_rsp->m_name),
        m_sex(auth_rsp->m_sex){}

    void AppendChatMsgs(const std::vector<std::shared_ptr<TextChatData>> text_vec){
        for(const auto & text: text_vec){
            m_chat_msgs.push_back(text);
        }
    }

    int m_uid;
    QString m_name;
    QString m_nick;
    QString m_icon;
    int m_sex;
    QString m_desc;
    QString m_back;
    QString m_last_msg;
    std::vector<std::shared_ptr<TextChatData>> m_chat_msgs;
};

struct UserInfo {
    UserInfo(int uid, QString name, QString nick, QString icon, int sex, QString last_msg = ""):
        m_uid(uid),m_name(name),m_nick(nick),m_icon(icon),m_sex(sex),m_last_msg(last_msg){}

    UserInfo(std::shared_ptr<AuthInfo> auth):
        m_uid(auth->m_uid),m_name(auth->m_name),m_nick(auth->m_nick),
        m_icon(auth->m_icon),m_sex(auth->m_sex),m_last_msg(""){}

    UserInfo(int uid, QString name, QString icon):
        m_uid(uid), m_name(name), m_icon(icon),m_nick(m_name),
        m_sex(0),m_last_msg(""){

    }

    UserInfo(std::shared_ptr<AuthRsp> auth):
        m_uid(auth->m_uid),m_name(auth->m_name),m_nick(auth->m_nick),
        m_icon(auth->m_icon),m_sex(auth->m_sex),m_last_msg(""){}

    UserInfo(std::shared_ptr<SearchInfo> search_info):
        m_uid(search_info->m_uid),m_name(search_info->m_name),m_nick(search_info->m_nick),
        m_icon(search_info->m_icon),m_sex(search_info->m_sex),m_last_msg(""){

    }

    UserInfo(std::shared_ptr<FriendInfo> friend_info):
        m_uid(friend_info->m_uid),m_name(friend_info->m_name),m_nick(friend_info->m_nick),
        m_icon(friend_info->m_icon),m_sex(friend_info->m_sex),m_last_msg(friend_info->m_last_msg){
        m_chat_msgs = friend_info->m_chat_msgs;
    }

    int m_uid;
    QString m_name;
    QString m_nick;
    QString m_icon;
    int m_sex;
    QString m_last_msg;
    std::vector<std::shared_ptr<TextChatData>> m_chat_msgs;
};

struct TextChatData{
    TextChatData(QString msg_id, QString msg_content, int fromuid, int touid)
        :m_msg_id(msg_id),m_msg_content(msg_content),m_from_uid(fromuid),m_to_uid(touid){

    }
    QString m_msg_id;
    QString m_msg_content;
    int m_from_uid;
    int m_to_uid;
};

struct TextChatMsg{
    TextChatMsg(int fromuid, int touid, QJsonArray arrays):
        m_from_uid(fromuid), m_to_uid(touid){
        for(const QJsonValue& msg_data : arrays){
            auto content = msg_data["content"].toString();
            auto msgid = msg_data["msgid"].toString();
            auto msg_ptr = std::make_shared<TextChatData>(msgid, content,fromuid, touid);
            m_chat_msgs.push_back(msg_ptr);
        }
    }
    int m_to_uid;
    int m_from_uid;
    std::vector<std::shared_ptr<TextChatData>> m_chat_msgs;
};
