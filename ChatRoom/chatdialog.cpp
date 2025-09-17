#include "chatdialog.h"
#include "ui_chatdialog.h"
#include "chatuseritem.h"
#include "loadingdlg.h"
#include <QAction>
#include <QRandomGenerator>
#include <QMouseEvent>
#include "tcpmanager.h"
#include "usermanager.h"
#include "contactuserlist.h"
#include "contactuseritem.h"

ChatDialog::ChatDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::ChatDialog),
    m_mode(ChatUIMode::ChatMode), m_state(ChatUIMode::ChatMode),
    m_b_loading(false), m_cur_chat_uid(0)
{
    ui->setupUi(this);
    // 设置add按钮样式
    ui->add_btn->SetState("normal", "hover", "press");
    ui->add_btn->setProperty("state", "normal");

    // 设置search_edit
    ui->search_edit->SetMaxLength(15);

    // 默认情况下隐藏搜索列表
    ShowSearch(false);

    // 搜索栏外观行为设置
    QAction* searchAction = new QAction(ui->search_edit);
    searchAction->setIcon(QIcon(":/resource/search.png"));
    ui->search_edit->addAction(searchAction, QLineEdit::LeadingPosition);
    ui->search_edit->setPlaceholderText(QStringLiteral("搜索"));

    QAction* clearAction = new QAction(ui->search_edit);
    clearAction->setIcon(QIcon(":/resource/close_transparent.png"));
    ui->search_edit->addAction(clearAction, QLineEdit::TrailingPosition);
    connect(ui->search_edit, &QLineEdit::textChanged, [this, clearAction](const QString &text){
        if(!text.isEmpty()){
            clearAction->setIcon(QIcon(":/resource/close_search.png"));
            // 按下清除按钮则显示搜索列表
            ShowSearch(true);
        }else{
            clearAction->setIcon(QIcon(":/resource/close_transparent.png"));
            // 按下清除按钮则不显示搜索列表
            ShowSearch(false);
        }
    });
    connect(clearAction, &QAction::triggered, [this](){
        // 清除文本
        ui->search_edit->clear();
        // 清除焦点
        ui->search_edit->clearFocus();
        // 按下清除按钮则不显示搜索列表
        ShowSearch(false);
    });

    // 初始化好友列表
    AddChatUserList();
    connect(ui->chat_user_list, &ChatUserList::sig_loading_chat_user,
            this, &ChatDialog::slot_loading_chat_user);
    // 初始化联系人列表
    AddContactUserList();
    connect(ui->con_user_list, &ContactUserList::sig_loading_contact_user,
            this, &ChatDialog::slot_loading_contact_user);

    // 侧边栏添加头像
    QPixmap pixmap(UserManager::GetInstance().GetUserInfo()->m_icon); // 加载图片
    ui->side_head_lb->setPixmap(pixmap); // 将图片设置到QLabel上
    QPixmap scaledPixmap = pixmap.scaled( ui->side_head_lb->size(), Qt::KeepAspectRatio); // 将图片缩放到label的大小
    ui->side_head_lb->setPixmap(scaledPixmap); // 将缩放后的图片设置到QLabel上
    ui->side_head_lb->setScaledContents(true); // 设置QLabel自动缩放图片内容以适应大小

    // 设置侧边栏行为
    ui->side_chat_lb->SetState("normal","hover","pressed","selected_normal","selected_hover","selected_pressed");
    ui->side_contact_lb->SetState("normal","hover","pressed","selected_normal","selected_hover","selected_pressed");

    // 将侧边栏按钮添加到按钮组中
    AddLBGroup(ui->side_chat_lb);
    AddLBGroup(ui->side_contact_lb);

    // 注册侧边栏事件处理
    connect(ui->side_chat_lb, &StateWidget::clicked, this, &ChatDialog::slot_side_chat);
    connect(ui->side_contact_lb, &StateWidget::clicked, this, &ChatDialog::slot_side_contact);

    //链接搜索框输入变化
    connect(ui->search_edit, &QLineEdit::textChanged, this, &ChatDialog::slot_text_changed);

    //检测鼠标点击位置判断是否要清空搜索框
    this->installEventFilter(this); // 安装事件过滤器

    // 设置聊天按钮为选中状态
    ui->side_chat_lb->SetSelected(true);
    //设置选中条目
    SetSelectChatItem(0);
    //更新聊天界面信息
    SetSelectChatPage(0);

    //为searchlist 设置search edit
    ui->search_list->SetSearchEdit(ui->search_edit);

    // 连接加载联系人的信号和槽函数
    connect(&TcpManager::GetInstance(), &TcpManager::sig_friend_apply, this, &ChatDialog::slot_apply_friend);

    //连接认证添加好友信号
    connect(&TcpManager::GetInstance(), &TcpManager::sig_add_auth_friend, this, &ChatDialog::slot_add_auth_friend);

    //链接自己认证回复信号
    connect(&TcpManager::GetInstance(), &TcpManager::sig_auth_rsp, this, &ChatDialog::slot_auth_rsp);

    // 连接searchlist跳转聊天信号
    connect(ui->search_list, &SearchList::sig_jump_chat_item, this, &ChatDialog::slot_jump_chat_item);



    // 设置中心部件为chatpage
    ui->stackedWidget->setCurrentWidget(ui->chat_page);

    // 连接聊天列表点击信号
    connect(ui->chat_user_list, &QListWidget::itemClicked, this, &ChatDialog::slot_item_clicked);

    connect(ui->chat_page, &ChatPage::sig_append_send_chat_msg, this, &ChatDialog::slot_append_send_chat_msg);

    // 收到好友发送的消息
    connect(&TcpManager::GetInstance(), &TcpManager::sig_text_chat_msg, this, &ChatDialog::slot_text_chat_msg);
}

void ChatDialog::AddLBGroup(StateWidget* lb)
{
    m_lb_list.push_back(lb);
}

ChatDialog::~ChatDialog()
{
    delete ui;
}

void ChatDialog::AddChatUserList()
{
    auto friend_list = UserManager::GetInstance().GetChatListPerPage();
    if(!friend_list.empty()){
        for(auto& friend_ele : friend_list){
            auto find_iter = m_chat_items_added.find(friend_ele->m_uid);
            if(find_iter != m_chat_items_added.end()){
                continue;
            }

            auto*chat_user_item = new ChatUserItem();
            auto user_info = std::make_shared<UserInfo>(friend_ele);
            chat_user_item->SetInfo(user_info);
            QListWidgetItem *item = new QListWidgetItem;
            item->setSizeHint(chat_user_item->sizeHint());
            ui->chat_user_list->addItem(item);
            ui->chat_user_list->setItemWidget(item, chat_user_item);
            m_chat_items_added.insert(friend_ele->m_uid, item);
        }
        // 更新已加载条目
        UserManager::GetInstance().UpdateChatLoadedCount();
    }
}

void ChatDialog::AddContactUserList()
{
    //加载后端发送过来的好友列表
    auto con_list = UserManager::GetInstance().GetConListPerPage();
    for(auto & con_ele : con_list){
        auto *con_user_wid = new ContactUserItem();
        con_user_wid->SetInfo(con_ele->m_uid,con_ele->m_name, con_ele->m_icon);
        QListWidgetItem *item = new QListWidgetItem;
        item->setSizeHint(con_user_wid->sizeHint());
        ui->con_user_list->addItem(item);
        ui->con_user_list->setItemWidget(item, con_user_wid);
    }

    UserManager::GetInstance().UpdateContactLoadedCount();
}

void ChatDialog::ClearLabelState(StateWidget *lb)
{
    for(auto & ele: m_lb_list){
        if(ele == lb){
            continue;
        }

        ele->ClearState();
    }
}

bool ChatDialog::eventFilter(QObject* watched, QEvent* event)
{
    if(event->type() == QEvent::MouseButtonPress){
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        HandleGlobalMousePress(mouseEvent);
    }

    return QDialog::eventFilter(watched, event);
}

void ChatDialog::ShowSearch(bool bsearch)
{
    if(bsearch){
        ui->chat_user_list->hide();
        ui->con_user_list->hide();
        ui->search_list->show();
        m_mode = ChatUIMode::SearchMode;
    }else if(m_state == ChatUIMode::ChatMode){
        ui->con_user_list->hide();
        ui->search_list->hide();
        ui->chat_user_list->show();
        m_mode = ChatUIMode::ChatMode;
    }else if(m_state == ChatUIMode::ContactMode){
        ui->chat_user_list->hide();
        ui->search_list->hide();
        ui->con_user_list->show();
        m_mode = ChatUIMode::ContactMode;
    }
}

void ChatDialog::HandleGlobalMousePress(QMouseEvent *event)
{
    // 实现点击位置的判断和处理逻辑
    // 先判断是否处于搜索模式，如果不处于搜索模式则直接返回
    if( m_mode != ChatUIMode::SearchMode){
        return;
    }

    // 将鼠标点击位置转换为搜索列表坐标系中的位置
    QPoint posInSearchList = ui->search_list->mapFromGlobal(event->globalPos());
    // 判断点击位置是否在聊天列表的范围内
    if (!ui->search_list->rect().contains(posInSearchList)) {
        // 如果不在聊天列表内，清空输入框
        ui->search_edit->clear();
        // search_edit会监听text的变化，从而不显示搜索列表
        // ShowSearch(false);
    }

}

void ChatDialog::slot_loading_chat_user()
{
    // 判断当前是否是正在加载数据
    if(m_b_loading){
        return;
    }

    m_b_loading = true;
    LoadingDlg *loadingDialog = new LoadingDlg(this);
    loadingDialog->setModal(true);
    loadingDialog->show();
    qDebug() << "add new data to list.....";
    AddChatUserList();
    // 加载完成后关闭对话框
    loadingDialog->deleteLater();

    m_b_loading = false;

}

void ChatDialog::slot_side_chat()
{
    qDebug()<< "receive side chat clicked";
    ClearLabelState(ui->side_chat_lb);
    ui->stackedWidget->setCurrentWidget(ui->chat_page);
    m_state = ChatUIMode::ChatMode;
    ShowSearch(false);
}

void ChatDialog::slot_side_contact(){
    qDebug()<< "receive side contact clicked";
    ClearLabelState(ui->side_contact_lb);
    //设置
    ui->stackedWidget->setCurrentWidget(ui->friend_apply_page);
    m_state = ChatUIMode::ContactMode;
    ShowSearch(false);
}

void ChatDialog::slot_text_changed(const QString &str)
{
    if (!str.isEmpty()) {
        ShowSearch(true);
    }
}

void ChatDialog::slot_apply_friend(std::shared_ptr<AddFriendApply> apply)
{
    qDebug() << "ChatDialog::slot_apply_friend, from_uid is " << apply->m_from_uid;

    bool already = UserManager::GetInstance().AlreadyApply(apply->m_from_uid);
    if(already){
        // 已经有申请了退出
        return;
    }

    // 添加好友申请到列表与用户管理
    UserManager::GetInstance().AddApplyList(std::make_shared<ApplyInfo>(apply));
    ui->side_contact_lb->ShowRedPoint(true);
    ui->con_user_list->ShowRedPoint(true);
    ui->friend_apply_page->AddNewApply(apply);
}

void ChatDialog::slot_add_auth_friend(std::shared_ptr<AuthInfo> auth_info)
{
    auto isFriend = UserManager::GetInstance().CheckFriendById(auth_info->m_uid);
    if(isFriend) return;

    UserManager::GetInstance().AddFriend(auth_info);

    auto* chat_user_wid = new ChatUserItem();
    auto user_info = std::make_shared<UserInfo>(auth_info);
    chat_user_wid->SetInfo(user_info);
    QListWidgetItem* item = new QListWidgetItem();
    item->setSizeHint(chat_user_wid->sizeHint());
    ui->chat_user_list->insertItem(0, item);
    ui->chat_user_list->setItemWidget(item, chat_user_wid);
    m_chat_items_added.insert(auth_info->m_uid, item);
}

void ChatDialog::slot_auth_rsp(std::shared_ptr<AuthRsp> auth_rsp)
{
    auto isFriend = UserManager::GetInstance().CheckFriendById(auth_rsp->m_uid);
    if(isFriend) return;

    UserManager::GetInstance().AddFriend(auth_rsp);

    auto* chat_user_wid = new ChatUserItem();
    auto user_info = std::make_shared<UserInfo>(auth_rsp);
    chat_user_wid->SetInfo(user_info);
    QListWidgetItem* item = new QListWidgetItem();
    item->setSizeHint(chat_user_wid->sizeHint());
    ui->chat_user_list->insertItem(0, item);
    ui->chat_user_list->setItemWidget(item, chat_user_wid);
    m_chat_items_added.insert(auth_rsp->m_uid, item);
}

void ChatDialog::slot_jump_chat_item(std::shared_ptr<SearchInfo> si)
{
    auto find_iter = m_chat_items_added.find(si->m_uid);
    if(find_iter != m_chat_items_added.end()){
        ui->chat_user_list->scrollToItem(find_iter.value());
        ui->side_chat_lb->SetSelected(true);
        SetSelectChatItem(si->m_uid);
        // 更新聊天界面信息
        SetSelectChatPage(si->m_uid);
        slot_side_chat();
        return;
    }

    // 如果没有找到，则创建新的插入listwidget
    auto* chat_user_wid = new ChatUserItem();
    auto user_info = std::make_shared<UserInfo>(si);
    chat_user_wid->SetInfo(user_info);
    QListWidgetItem* item = new QListWidgetItem();

    item->setSizeHint(chat_user_wid->sizeHint());
    ui->chat_user_list->insertItem(0, item);
    ui->chat_user_list->setItemWidget(item, chat_user_wid);

    m_chat_items_added.insert(si->m_uid, item);

    ui->side_chat_lb->SetSelected(true);
    SetSelectChatItem(si->m_uid);
    // 更新聊天界面信息
    SetSelectChatPage(si->m_uid);
    slot_side_chat();
}

void ChatDialog::slot_jump_chat_item_from_infopage(std::shared_ptr<UserInfo> ui)
{

}

void ChatDialog::slot_loading_contact_user()
{
    LoadingDlg *loadingDialog = new LoadingDlg(this);
    loadingDialog->setModal(true);
    loadingDialog->show();
    AddContactUserList();
    // 加载完成后关闭对话框
    loadingDialog->deleteLater();
}

void ChatDialog::slot_item_clicked(QListWidgetItem *item)
{
    QWidget *widget = ui->chat_user_list->itemWidget(item); // 获取自定义widget对象
    if(!widget){
        return;
    }

    // 对自定义widget进行操作， 将item 转化为基类ListItemBase
    ListItemBase *customItem = qobject_cast<ListItemBase*>(widget);
    if(!customItem){
        return;
    }

    auto itemType = customItem->GetItemType();

    if(itemType == ListItemType::Invalid_Item
        || itemType == ListItemType::Group_Tip_Item){
        return;
    }

    if(itemType == ListItemType::Chat_User_Item){
        // 创建对话框，提示用户
        qDebug()<< "contact user item clicked ";

        auto chat_wid = qobject_cast<ChatUserItem*>(customItem);
        auto user_info = chat_wid->GetUserInfo();
        //跳转到聊天界面
        ui->chat_page->SetUserInfo(user_info);
        m_cur_chat_uid = user_info->m_uid;
        return;
    }

}

void ChatDialog::slot_append_send_chat_msg(std::shared_ptr<TextChatData> msgdata)
{
    if (m_cur_chat_uid == 0) {
        return;
    }

    auto find_iter = m_chat_items_added.find(m_cur_chat_uid);
    if (find_iter == m_chat_items_added.end()) {
        return;
    }

    //转为widget
    QWidget* widget = ui->chat_user_list->itemWidget(find_iter.value());
    if (!widget) {
        return;
    }

    //判断转化为自定义的widget
    // 对自定义widget进行操作， 将item 转化为基类ListItemBase
    ListItemBase* customItem = qobject_cast<ListItemBase*>(widget);
    if (!customItem) {
        return;
    }

    auto itemType = customItem->GetItemType();
    if (itemType == ListItemType::Chat_User_Item) {
        auto con_item = qobject_cast<ChatUserItem*>(customItem);
        if (!con_item) {
            return;
        }

        //设置信息
        auto user_info = con_item->GetUserInfo();
        user_info->m_chat_msgs.push_back(msgdata);
        std::vector<std::shared_ptr<TextChatData>> msg_vec;
        msg_vec.push_back(msgdata);
        UserManager::GetInstance().AppendFriendChatMsg(m_cur_chat_uid, msg_vec);
        return;
    }

}

void ChatDialog::slot_text_chat_msg(std::shared_ptr<TextChatMsg> msg)
{
    auto find_iter = m_chat_items_added.find(msg->m_from_uid);
    if(find_iter != m_chat_items_added.end()){
        QWidget *widget = ui->chat_user_list->itemWidget(find_iter.value());
        auto chat_wid = qobject_cast<ChatUserItem*>(widget);
        if(!chat_wid){
            return;
        }
        chat_wid->UpdateLastMsg(msg->m_chat_msgs);
        //更新当前聊天页面记录
        UpdateChatMsg(msg->m_chat_msgs);
        UserManager::GetInstance().AppendFriendChatMsg(msg->m_from_uid, msg->m_chat_msgs);
        return;
    }

    //如果没找到，则创建新的插入listwidget

    auto* chat_user_wid = new ChatUserItem();
    //查询好友信息
    auto fi_ptr = UserManager::GetInstance().GetFriendById(msg->m_from_uid);
    chat_user_wid->SetInfo(fi_ptr);
    QListWidgetItem* item = new QListWidgetItem;
    //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
    item->setSizeHint(chat_user_wid->sizeHint());
    chat_user_wid->UpdateLastMsg(msg->m_chat_msgs);
    UserManager::GetInstance().AppendFriendChatMsg(msg->m_from_uid,msg->m_chat_msgs);
    ui->chat_user_list->insertItem(0, item);
    ui->chat_user_list->setItemWidget(item, chat_user_wid);
    m_chat_items_added.insert(msg->m_from_uid, item);
}

void ChatDialog::SetSelectChatItem(int uid)
{
    if(ui->chat_user_list->count() <= 0){
        return;
    }

    if(uid == 0){
        ui->chat_user_list->setCurrentRow(0);
        QListWidgetItem *firstItem = ui->chat_user_list->item(0);
        if(!firstItem){
            return;
        }

        //转为widget
        QWidget *widget = ui->chat_user_list->itemWidget(firstItem);
        if(!widget){
            return;
        }

        auto con_item = qobject_cast<ChatUserItem*>(widget);
        if(!con_item){
            return;
        }

        m_cur_chat_uid = con_item->GetUserInfo()->m_uid;

        return;
    }

    auto find_iter = m_chat_items_added.find(uid);
    if(find_iter == m_chat_items_added.end()){
        qDebug() << "uid " <<uid<< " not found, set curent row 0";
        ui->chat_user_list->setCurrentRow(0);
        return;
    }

    ui->chat_user_list->setCurrentItem(find_iter.value());

    m_cur_chat_uid = uid;
}

void ChatDialog::SetSelectChatPage(int uid)
{
    if( ui->chat_user_list->count() <= 0){
        return;
    }

    if (uid == 0) {
        auto item = ui->chat_user_list->item(0);
        //转为widget
        QWidget* widget = ui->chat_user_list->itemWidget(item);
        if (!widget) {
            return;
        }

        auto con_item = qobject_cast<ChatUserItem*>(widget);
        if (!con_item) {
            return;
        }

        //设置信息
        auto user_info = con_item->GetUserInfo();
        ui->chat_page->SetUserInfo(user_info);
        return;
    }

    auto find_iter = m_chat_items_added.find(uid);
    if(find_iter == m_chat_items_added.end()){
        return;
    }

    //转为widget
    QWidget *widget = ui->chat_user_list->itemWidget(find_iter.value());
    if(!widget){
        return;
    }

    //判断转化为自定义的widget
    // 对自定义widget进行操作， 将item 转化为基类ListItemBase
    ListItemBase *customItem = qobject_cast<ListItemBase*>(widget);
    if(!customItem){
        qDebug()<< "qobject_cast<ListItemBase*>(widget) is nullptr";
        return;
    }

    auto itemType = customItem->GetItemType();
    if(itemType == ListItemType::Chat_User_Item){
        auto con_item = qobject_cast<ChatUserItem*>(customItem);
        if(!con_item){
            return;
        }

        //设置信息
        auto user_info = con_item->GetUserInfo();
        ui->chat_page->SetUserInfo(user_info);

        return;
    }
}

void ChatDialog::UpdateChatMsg(std::vector<std::shared_ptr<TextChatData> > msgdata)
{
    for(auto & msg : msgdata){
        if(msg->m_from_uid != m_cur_chat_uid){
            break;
        }

        ui->chat_page->AppendChatMsg(msg);
    }
}

