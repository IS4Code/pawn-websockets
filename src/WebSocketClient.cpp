#include "PAWN.hpp"
#include "WebSocketClient.hpp"
#include "SynchronizationCall.hpp"

#include <boost/bind.hpp>

WebSocketClient::WebSocketClient(
        const std::string& connectName,
        const std::string& failName,
        const std::string& disconnectName,
        const std::string& messageName)
    : m_functionConnectName(connectName),
      m_functionFailName(failName),
      m_functionDisconnectName(disconnectName),
      m_functionMessageName(messageName) {

    // Disable logging
    m_client.set_access_channels(websocketpp::log::alevel::none);
    m_client.set_error_channels(websocketpp::log::alevel::none);

    // Set handlers
    m_client.set_open_handler([&](WebsocketConnection) {
        m_connected = true;

        int idx = getID();
        std::string func = m_functionConnectName;
        SynchronizationCall::sharedSynronizationCall() += [idx, func]() {
            int funcIDX = 0;
            if(!amx_FindPublic(PAWN::GetAMX(), func.c_str(), &funcIDX)) {
                amx_Push(PAWN::GetAMX(), idx);
                amx_Exec(PAWN::GetAMX(), NULL, funcIDX);
            }
        };
    });

    m_client.set_fail_handler([&](WebsocketConnection) {
        m_connected = false;

        int idx = getID();
        std::string func = m_functionFailName;
        SynchronizationCall::sharedSynronizationCall() += [idx, func]() {
            int funcIDX = 0;
            if(!amx_FindPublic(PAWN::GetAMX(), func.c_str(), &funcIDX)) {
                amx_Push(PAWN::GetAMX(), idx);
                amx_Exec(PAWN::GetAMX(), NULL, funcIDX);
            }
        };
    });

    m_client.set_close_handler([&](WebsocketConnection) {
        m_connected = false;

        int idx = getID();
        std::string func = m_functionDisconnectName;
        SynchronizationCall::sharedSynronizationCall() += [idx, func]() {
            int funcIDX = 0;
            if(!amx_FindPublic(PAWN::GetAMX(), func.c_str(), &funcIDX)) {
                amx_Push(PAWN::GetAMX(), idx);
                amx_Exec(PAWN::GetAMX(), NULL, funcIDX);
            }
        };
    });

    m_client.set_message_handler([&](WebsocketConnection, WebsocketMessage msg) {
        try {
            std::string data = msg->get_payload();

            int idx = getID();
            std::string func = m_functionMessageName;
            SynchronizationCall::sharedSynronizationCall() += [idx, func, data]() {
                int funcIDX = 0;
                if(!amx_FindPublic(PAWN::GetAMX(), func.c_str(), &funcIDX)) {
                    cell addr = 0;

                    amx_PushString(PAWN::GetAMX(), &addr, NULL, data.c_str(), NULL, NULL);
                    amx_Push(PAWN::GetAMX(), idx);
                    amx_Exec(PAWN::GetAMX(), NULL, funcIDX);
                    amx_Release(PAWN::GetAMX(), addr);
                }
            };
        }
        catch(...) { }
    });

    // Initialize ASIO
    m_client.init_asio();
}

WebSocketClient::~WebSocketClient() {
    disconnect();
}

bool WebSocketClient::connect(const std::string &ip) {
    if(isConnected())
        return false;

    try {
        websocketpp::lib::error_code ec;
        auto ptr = m_client.get_connection(ip, ec);

        m_connectionPtr = ptr->get_handle();
        m_client.connect(ptr);

        m_asioThread = boost::thread(boost::bind(&WebsocketClient::run, &m_client));
        return true;
    } catch(...) { return false; }
}

bool WebSocketClient::disconnect() {
    if(!isConnected())
        return false;

    try {
        m_client.stop();
        if(m_asioThread.joinable())
            m_asioThread.join();

        m_connected = false;
        return m_client.stopped();
    }
    catch(...) { return false; }
}

bool WebSocketClient::isConnected() const {
    return m_connected;
}

bool WebSocketClient::send(const std::string &text) {
    if(!isConnected())
        return false;

    try {
        if(m_client.get_con_from_hdl(m_connectionPtr)->send(text, websocketpp::frame::opcode::text))
            return false;

        return true;
    }
    catch(...) { return false; }
}

int WebSocketClient::getID() const {
    return m_id;
}

void WebSocketClient::setID(int index) {
    m_id = index;
}
