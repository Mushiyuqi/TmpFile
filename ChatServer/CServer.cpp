#include "CServer.h"
#include "IOContextPool.h"
#include <iostream>
#include "UserManager.h"
#include "ConfigManager.h"
#include "RedisManager.h"

CServer::CServer(boost::asio::io_context& io_context, const short port):
    _io_context(io_context), m_acceptor(_io_context, tcp::endpoint(tcp::v4(), port)), m_port(port) {
    std::cout << "CServer::CServer() server is running" << std::endl
        << "ip is   : " << m_acceptor.local_endpoint().address() << std::endl
        << "port is : " << m_acceptor.local_endpoint().port() << std::endl;

    StartAccept();
}

CServer::~CServer() {
    m_acceptor.close();
    std::cerr << "CServer::~CServer()" << std::endl;
}

void CServer::ClearSession(std::string sessionId) {
    if(m_sessions.contains(sessionId)) {
        // 移除用户与session的关联
        UserManager::GetInstance().RemoveUserSession(m_sessions[sessionId]->GetUserId());
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_sessions.erase(sessionId);

        // 连接计数减1
        auto serverName = ConfigManager::GetInstance()["SelfServer"]["Name"];
        std::string tmp;
        RedisManager::GetInstance().HGet(LOGIN_COUNT, serverName, tmp);
        if(tmp.empty()) return;
        int count = std::stoi(tmp);
        if(count <= 0) return;
        auto countStr = std::to_string(--count);
        RedisManager::GetInstance().HSet(LOGIN_COUNT, serverName, countStr);
    }
}

void CServer::HandleAccept(std::shared_ptr<CSession> session, const boost::system::error_code& error) {
    if (!error) {
        session->Start();
        std::lock_guard<std::mutex> lock(m_mutex);
        m_sessions[session->GetSessionId()] = session;
    }
    else {
        std::cerr << "CServer::HandleAccept error: " << error.what() << std::endl;
    }
    // 继续接收连接请求
    StartAccept();
}

void CServer::StartAccept() {
    // 获取一个io_context
    auto& io_context = IOContextPool::GetInstance().GetIOContext();
    auto session = std::make_shared<CSession>(io_context, this);
    m_acceptor.async_accept(session->GetSocket(),
                            std::bind(&CServer::HandleAccept, this, session, std::placeholders::_1));
}

