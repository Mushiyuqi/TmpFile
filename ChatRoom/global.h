#pragma once

#include <memory>
#include <iostream>
#include <mutex>
#include <QWidget>
#include <functional>
#include <QStyle>
#include <QRegularExpression>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QFile>
#include <QDir>
#include <QSettings>
#include <cryptopp/sha.h>
#include <cryptopp/hex.h>
#include <unordered_map>
#include <QPainter>
#include <QPainterPath>

/**
 * @brief repolish 刷新qss
 */
extern std::function<void(QWidget*)> repolish;
extern std::function<QString(QString)> Sha256Hash;  // 加密函数
extern QString gate_url_prefix;

enum ReqId{
    ID_GET_VERIFY_CODE = 1001, // 获取验证码
    ID_REG_USER = 1002, // 注册用户
    ID_RESET_PWD = 1003, //重置密码
    ID_LOGIN_USER = 1004, //用户登录
    ID_CHAT_LOGIN = 1005, //登陆聊天服务器
    ID_SEARCH_USER = 1006, //用户搜索请求
    ID_ADD_FRIEND = 1007,  //添加好友申请
    ID_NOTIFY_ADD_FRIEND = 1008,  //通知用户添加好友申请
    ID_AUTH_FRIEND = 1009,  //认证好友请求
    ID_NOTIFY_AUTH_FRIEND = 1010, //通知用户认证好友申请
    ID_TEXT_CHAT_MSG  = 1011,  //文本聊天信息请求
    ID_NOTIFY_TEXT_CHAT_MSG = 1012, //通知用户文本聊天信息
};

enum Modules{
    REGISTER_MOD = 0, // 注册模块
    RESET_MOD = 1,  // 重置密码模块
    LOGINMOD = 2,   // 登陆模块
};

enum ErrorCodes{
    SUCCESS = 0,
    ERR_JSON = 1, // json解析失败
    ERR_NETWORK = 2, // 网络错误
};

struct ServerInfo{
    QString Host;
    QString Port;
    QString Token;
    int Uid;
};

// 聊天界面几种模式
enum ChatUIMode{
    SearchMode, // 搜索模式
    ChatMode,   // 聊天模式
    ContactMode,// 联系人模式
};

//自定义QListWidgetItem的几种类型
enum ListItemType{
    Chat_User_Item, //聊天用户
    Contact_User_Item, //联系人用户
    Search_User_Item, //搜索到的用户
    AddUser_Tip_Item, //提示添加用户
    Invalid_Item,  //不可点击条目
    Group_Tip_Item,
    Line_Item,
    Apply_Friend_Item,
};


enum class ChatRole
{
    Self,
    Other
};

struct MsgInfo{
    QString msgFlag;//"text,image,file"
    QString content;//表示文件和图像的url,文本信息
    QPixmap pixmap;//文件和图片的缩略图
};

enum ClickState{
    Normal = 0,
    Selected = 1
};

//申请好友标签输入框最低长度
const int MIN_APPLY_LABEL_ED_LEN = 40;

const QString add_prefix = "添加标签 ";

const int  tip_offset = 5;


// 测试数据
const std::vector<QString>  strs ={"hello world !",
                                   "nice to meet u",
                                   "New year，new life",
                                   "You have to love yourself",
                                   "My love is written in the wind ever since the whole world is you"};

const std::vector<QString> heads = {
    ":/resource/head_1.jpg",
    ":/resource/head_2.jpg",
    ":/resource/head_3.jpg",
    ":/resource/head_4.jpg",
    ":/resource/head_5.jpg"
};

const std::vector<QString> names = {
    "HanMeiMei",
    "Lily",
    "Ben",
    "Androw",
    "Max",
    "Summer",
    "Candy",
    "Hunter"
};

const int CHAT_COUNT_PER_PAGE = 12;





