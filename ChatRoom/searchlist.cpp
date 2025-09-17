#include "searchlist.h"
#include "tcpmanager.h"
#include "adduseritem.h"
#include <tcpmanager.h>
#include <findsuccessdlg.h>
#include "customizeedit.h"
#include "usermanager.h"
#include "findfaildlg.h"

SearchList::SearchList(QWidget *parent)
    :QListWidget(parent),m_find_dlg(nullptr), m_search_edit(nullptr), m_send_pending(false)
{
    Q_UNUSED(parent);
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    // 安装事件过滤器
    this->viewport()->installEventFilter(this);
    //连接点击的信号和槽
    connect(this, &QListWidget::itemClicked, this, &SearchList::slot_item_clicked);
    //添加条目
    AddTipItem();
    //连接搜索条目
    connect(&TcpManager::GetInstance(), &TcpManager::sig_user_search,
            this, &SearchList::slot_user_search);
}

void SearchList::CloseFindDlg()
{
    if(m_find_dlg){
        m_find_dlg->hide();
        m_find_dlg = nullptr;
    }
}

void SearchList::SetSearchEdit(QWidget *edit)
{
    m_search_edit = edit;
}


bool SearchList::eventFilter(QObject *watched, QEvent *event)
{
    // 检查事件是否是鼠标悬浮进入或离开
    if (watched == this->viewport()) {
        if (event->type() == QEvent::Enter) {
            // 鼠标悬浮，显示滚动条
            this->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        } else if (event->type() == QEvent::Leave) {
            // 鼠标离开，隐藏滚动条
            this->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        }
    }

    // 检查事件是否是鼠标滚轮事件
    if (watched == this->viewport() && event->type() == QEvent::Wheel) {
        QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event);
        int numDegrees = wheelEvent->angleDelta().y() / 8;
        int numSteps = numDegrees / 15; // 计算滚动步数

        // 设置滚动幅度
        this->verticalScrollBar()->setValue(this->verticalScrollBar()->value() - numSteps);

        return true; // 停止事件传递
    }

    return QListWidget::eventFilter(watched, event);
}

void SearchList::WaitPending(bool pending)
{
    if(pending){
        m_loadingDialog = new LoadingDlg(this);
        m_loadingDialog->setModal(true);
        m_loadingDialog->show();
        m_send_pending = pending;
    }else{
        m_loadingDialog->hide();
        m_loadingDialog->deleteLater();
        m_send_pending = pending;
    }
}

void SearchList::AddTipItem()
{
    auto *invalid_item = new QWidget();
    QListWidgetItem *item_tmp = new QListWidgetItem;
    //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
    item_tmp->setSizeHint(QSize(250,10));
    this->addItem(item_tmp);
    invalid_item->setObjectName("invalid_item");
    this->setItemWidget(item_tmp, invalid_item);
    item_tmp->setFlags(item_tmp->flags() & ~Qt::ItemIsSelectable);


    auto *add_user_item = new AddUserItem();
    QListWidgetItem *item = new QListWidgetItem;
    //qDebug()<<"chat_user_wid sizeHint is " << chat_user_wid->sizeHint();
    item->setSizeHint(add_user_item->sizeHint());
    this->addItem(item);
    this->setItemWidget(item, add_user_item);

}

void SearchList::slot_item_clicked(QListWidgetItem *item)
{
    QWidget* widget = this->itemWidget(item); // 获取自定义widget对象
    if(!widget){
        qDebug()<< "slot item clicked widget is nullptr";
        return;
    }

    // 对自定义widget进行操作， 将item 转化为基类ListItemBase
    ListItemBase *customItem = qobject_cast<ListItemBase*>(widget);
    if(!customItem){
        qDebug()<< "slot item clicked widget is nullptr";
        return;
    }

    auto itemType = customItem->GetItemType();
    if(itemType == ListItemType::Invalid_Item){
        qDebug()<< "slot invalid item clicked ";
        return;
    }

    if(itemType == ListItemType::AddUser_Tip_Item){
        if(m_send_pending) return;

        if(m_search_edit == nullptr) return;

        // 等待发送请求, 添加蒙板
        WaitPending(true);

        auto search_edit = dynamic_cast<CustomizeEdit*>(m_search_edit);
        auto uid_str = search_edit->text(); //用户输入信息可能是uid或者name
        //此处发送请求给server
        QJsonObject jsonObj;
        jsonObj["uid"] = uid_str;
        QJsonDocument doc(jsonObj);
        QByteArray jsonStr = doc.toJson(QJsonDocument::Compact);

        //发送tcp请求给chat server
        emit TcpManager::GetInstance().sig_send_data(ReqId::ID_SEARCH_USER, jsonStr);
        return;
    }

    //清除弹出框
    CloseFindDlg();
}

void SearchList::slot_user_search(std::shared_ptr<SearchInfo> si)
{
    WaitPending(false);
    if (si == nullptr) {
        m_find_dlg = std::make_shared<FindFailDlg>(this);
    }else{
        // 判断是否是自己
        if(si->m_uid == UserManager::GetInstance().GetUid())
            return;
        // 此处分两种情况，一种是搜多到已经是自己的朋友了，一种是未添加好友
        bool bExist = UserManager::GetInstance().CheckFriendById(si->m_uid);
        if(bExist){
            emit sig_jump_chat_item(si);
            return;
        }
        m_find_dlg = std::make_shared<FindSuccessDlg>(this);
        std::dynamic_pointer_cast<FindSuccessDlg>(m_find_dlg)->SetSearchInfo(si);
    }
    m_find_dlg->show();
}
