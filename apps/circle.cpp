#include "asio.hpp"
#include "dbat/net.h"
#include "dbat/utils.h"
#include "asio/experimental/awaitable_operators.hpp"
#include "asio/experimental/concurrent_channel.hpp"
#include "dbat/db.h"
#include "dbat/saveload.h"

using namespace asio;
using namespace asio::experimental::awaitable_operators;
template<typename T>
    using Channel = asio::experimental::concurrent_channel<void(asio::error_code, T)>;

std::unique_ptr<io_context> ioContext;

namespace codes {
    constexpr uint8_t NUL = 0, BEL = 7, CR = 13, LF = 10, SGA = 3, TELOPT_EOR = 25, NAWS = 31;
    constexpr uint8_t LINEMODE = 34, EOR = 239, SE = 240, NOP = 241, GA = 249, SB = 250;
    constexpr uint8_t WILL = 251, WONT = 252, DO = 253, DONT = 254, IAC = 255, MNES = 39;
    constexpr uint8_t MXP = 91, MSSP = 70, MCCP2 = 86, MCCP3 = 87, GMCP = 201, MSDP = 69;
    constexpr uint8_t MTTS = 24;
}

using TelnetMessage = std::variant<std::vector<uint8_t>, uint8_t, std::pair<uint8_t, uint8_t>, std::pair<uint8_t, std::vector<uint8_t>>>;

using GameMessage = std::variant<std::string, std::pair<std::string, nlohmann::json>>;

class TelnetConnection;

enum class ColorType : uint8_t {
    NoColor = 0,
    Standard = 1,
    Xterm256 = 2,
    TrueColor = 3
};

struct ProtocolCapabilities {
    std::string clientName = "UNKNOWN", clientVersion = "UNKNOWN";
    std::string hostAddress = "UNKNOWN";
    std::string encoding;
    std::vector<std::string> hostNames{};
    ColorType colorType = ColorType::NoColor;
    int16_t hostPort{0};

    bool encryption = false;
    bool utf8 = false;
    int width = 80, height = 52;
    bool gmcp = false, msdp = false, mssp = false, mxp = false;
    bool mccp2 = false, mccp2_active = false, mccp3 = false, mccp3_active = false;
    bool ttype = false, naws = false, sga = false, linemode = false;
    bool force_endline = false, oob = false, tls = false;
    bool screen_reader = false, mouse_tracking = false, vt100 = false;
    bool osc_color_palette = false, proxy = false, mnes = false;

    void deserialize(const nlohmann::json& j);
    nlohmann::json serialize();
};

void ProtocolCapabilities::deserialize(const nlohmann::json& j) {
    if(j.contains("encryption")) encryption = j["encryption"];
    if(j.contains("client_name")) clientName = j["client_name"];
    if(j.contains("client_version")) clientVersion = j["client_version"];
	if(j.contains("host_address")) hostAddress = j["host_address"];
    if(j.contains("host_port")) hostPort = j["host_port"];
    if(j.contains("host_names")) for(auto &hn : j["host_names"]) {
        hostNames.emplace_back(hn.get<std::string>());
    }
    if(j.contains("encoding")) encoding = j["encoding"];
    if(j.contains("utf8")) utf8 = j["utf8"];

    if(j.contains("colorType")) colorType = j["color"].get<ColorType>();

    if(j.contains("width")) width = j["width"];
    if(j.contains("height")) height = j["height"];
    if(j.contains("gmcp")) gmcp = j["gmcp"];
    if(j.contains("msdp")) msdp = j["msdp"];
    if(j.contains("mssp")) mssp = j["mssp"];
    if(j.contains("mxp")) mxp = j["mxp"];
    if(j.contains("mccp2")) mccp2 = j["mccp2"];
    if(j.contains("mccp3")) mccp3 = j["mccp3"];
    if(j.contains("ttype")) ttype = j["ttype"];
    if(j.contains("naws")) naws = j["naws"];
    if(j.contains("sga")) sga = j["sga"];
    if(j.contains("linemode")) linemode = j["linemode"];
    if(j.contains("force_endline")) force_endline = j["force_endline"];
    if(j.contains("oob")) oob = j["oob"];
    if(j.contains("tls")) tls = j["tls"];
    if(j.contains("screen_reader")) screen_reader = j["screen_reader"];
    if(j.contains("mouse_tracking")) mouse_tracking = j["mouse_tracking"];
    if(j.contains("vt100")) vt100 = j["vt100"];
    if(j.contains("osc_color_palette")) osc_color_palette = j["osc_color_palette"];
    if(j.contains("proxy")) proxy = j["proxy"];
    if(j.contains("mnes")) mnes = j["mnes"];
}

nlohmann::json ProtocolCapabilities::serialize() {
    nlohmann::json j;

    if(encryption) j["encryption"] = encryption;
    j["client_name"] = clientName;
    j["client_version"] = clientVersion;
    j["host_address"] = hostAddress;
    if(hostPort) j["host_port"] = hostPort;
    for(auto &hn : hostNames) {
        j["host_names"].push_back(hn);
    }
    if(!encoding.empty()) j["encoding"] = encoding;
    if(utf8) j["utf8"] = utf8;
    if(colorType != ColorType::NoColor) j["colorType"] = colorType;
    if(width != 80) j["width"] = width;
    if(height != 52) j["height"] = height;
    if(gmcp) j["gmcp"] = gmcp;
    if(msdp) j["msdp"] = msdp;
    if(mssp) j["mssp"] = mssp;
    if(mxp) j["mxp"] = mxp;
    if(mccp2) j["mccp2"] = mccp2;
    if(mccp3) j["mccp3"] = mccp3;
    if(ttype) j["ttype"] = ttype;
    if(naws) j["naws"] = naws;
    if(sga) j["sga"] = sga;
    if(linemode) j["linemode"] = linemode;
    if(force_endline) j["force_endline"] = force_endline;
    if(oob) j["oob"] = oob;
    if(tls) j["tls"] = tls;
    if(screen_reader) j["screen_reader"] = screen_reader;
    if(mouse_tracking) j["mouse_tracking"] = mouse_tracking;
    if(vt100) j["vt100"] = vt100;
    if(osc_color_palette) j["osc_color_palette"] = osc_color_palette;
    if(proxy) j["proxy"] = proxy;
    if(mnes) j["mnes"] = mnes;

    return j;
}

struct TelnetOptionState {
    bool negotiating{false};
    bool enabled{false};
};

struct TelnetOptionPerspective {
    TelnetOptionState local, remote;
};

class TelnetOption {
public:
    TelnetOption(TelnetConnection &conn) : conn(conn) {

    }
    virtual uint8_t getCode() = 0;
    void start();
    void sendNegotiate(uint8_t code);
    void sendSubnegotiate(const std::vector<uint8_t>& data);
    virtual bool supportLocal() { return false; }
    virtual bool supportRemote() { return false; }
    virtual void onLocalEnable() {};
    virtual void onRemoteEnable() {};
    virtual void onRemoteDisable() {};
    virtual void onLocalDisable() {};
    virtual void onLocalReject() {};
    virtual void onRemoteReject() {};
    virtual void onSubnegotiation(const std::vector<uint8_t> &data) {};
    void onNegotiate(uint8_t negotiate);
protected:
    TelnetConnection &conn;
    TelnetOptionPerspective perspective;

};

class TelnetConnection : public net::Connection {
public:
    TelnetConnection(int64_t connId, ip::tcp::socket socket, const any_io_executor& ex);

    awaitable<void> run();

    void sendBytes(const std::vector<uint8_t> &bytes);
    void sendNegotiate(uint8_t negotiate, uint8_t option);
    void sendSubnegotiate(uint8_t option, const std::vector<uint8_t> &data);
    void sendAppData(const std::vector<uint8_t> &data);
    void sendCommand(uint8_t command);

    void changeCapabilities(const nlohmann::json& j);

    void close() override;
    void onNetworkDisconnected() override;
    void onHeartbeat(double deltaTime) override;
    void sendText(const std::string& messg) override;
    void sendGMCP(const std::string& cmd, const nlohmann::json& j) override;

protected:
    net::ProtocolCapabilities capabilities;
    awaitable<void> runReader();
    awaitable<void> runWriter();
    awaitable<void> runNegotiation();
    awaitable<bool> parseTelnet();

    awaitable<void> handleApplicationData(const std::vector<uint8_t> &d);
    awaitable<void> handleCommand(uint8_t command);
    awaitable<void> handleNegotiation(uint8_t negotiate, uint8_t option);
    awaitable<void> handleSubnegotiation(uint8_t option, const std::vector<uint8_t> &data);

    std::unordered_map<uint8_t, std::unique_ptr<TelnetOption>> options;
    ip::tcp::socket socket;

    streambuf readbuf, writebuf, appbuf;
    Channel<TelnetMessage> outMessage;
    Channel<GameMessage> toGame;

    bool started{false};
};


// Telnet Options
void TelnetOption::sendNegotiate(uint8_t code) {
    conn.sendBytes({codes::IAC, code, getCode()});
}

void TelnetOption::sendSubnegotiate(const std::vector<uint8_t>& data) {
    conn.sendSubnegotiate(getCode(), data);
}

void TelnetOption::start() {
    if(supportLocal()) {
        perspective.local.negotiating = true;
        sendNegotiate(codes::WILL);
    }
    if(supportRemote()) {
        perspective.remote.negotiating = true;
        sendNegotiate(codes::DO);
    }
}

void TelnetOption::onNegotiate(uint8_t negotiate) {
    using namespace codes;
    switch(negotiate) {
        case WILL:
            if(supportRemote()) {
                auto &state = perspective.remote;
                if(!state.enabled) {
                    state.enabled = true;
                    if(!state.negotiating) {
                        sendNegotiate(DO);
                    }
                    onRemoteEnable();
                }
            } else {
                sendNegotiate(DONT);
            }
        break;

        case DO:
            if(supportLocal()) {
                auto &state = perspective.local;
                if(!state.enabled) {
                    state.enabled = true;
                    if(!state.negotiating) {
                        sendNegotiate(WILL);
                    }
                    onLocalEnable();
                }
            } else {
                sendNegotiate(WONT);
            }
        break;

        case WONT:
            if(supportRemote()) {
                auto &state = perspective.remote;
                if(state.enabled) {
                    state.enabled = false;
                    onRemoteDisable();
                }
                if(state.negotiating) {
                    state.negotiating = false;
                    onRemoteReject();
                }
            }
        break;

        case DONT:
            if(supportLocal()) {
                auto &state = perspective.local;
                if(state.enabled) {
                    state.enabled = false;
                    onLocalDisable();
                }
                if(state.negotiating) {
                    state.negotiating = false;
                    onLocalReject();
                }
            }
        break;

        default:
            // Handle other cases or unknown command
                break;
    }
}

// TelnetConnection

TelnetConnection::TelnetConnection(int64_t connId, ip::tcp::socket sock, const any_io_executor &ex) : Connection(connId),
socket(std::move(sock)), outMessage(ex, 200),
    toGame(ex, 200) {

}

awaitable<void> TelnetConnection::runNegotiation() {
    // first let's wait for the connection to negotiate.
    // This should take about 300ms at most. We will need to
    // wait asynchronously using an asio timer.
    auto ex = co_await this_coro::executor;
    //asio::steady_timer timer(ex, std::chrono::milliseconds(300));
    //co_await timer.async_wait(use_awaitable);

    state = net::ConnectionState::Pending;
    co_return;
}

awaitable<void> TelnetConnection::run() {
    auto ex = co_await this_coro::executor;
    co_spawn(ex, runNegotiation(), asio::detached);
    try {
        co_await (runReader() || runWriter());
    }
    catch(...) {

    }

    co_return;
}

awaitable<void> TelnetConnection::runWriter() {
    while(true) {
        auto message = co_await outMessage.async_receive(use_awaitable);

        switch(message.index()) {
            case 0: // raw application data: a vector of uint8_t
                sendBytes(std::get<0>(message));
                break;
            case 1: // a single byte, a telnet command. IAC <command>
                sendBytes({codes::IAC, std::get<1>(message)});
                break;
            case 2: // a negotiation pair, IAC <first> <second>
            {
                auto pair = std::get<2>(message);
                sendBytes({codes::IAC, std::get<0>(pair), std::get<1>(pair)});
            }
                break;
            case 3: // iac subnegotiation.
            {
                auto pair = std::get<3>(message);
                // The first element of the pair is the option, the second is a vector of bytes. we need to send...
                // IAC SB <option> <bytes> IAC SE
                std::vector<uint8_t> bytes = {codes::IAC, codes::SB, std::get<0>(pair)};
                bytes.insert(bytes.end(), std::get<1>(pair).begin(), std::get<1>(pair).end());
                bytes.push_back(codes::IAC);
                bytes.push_back(codes::SE);
                sendBytes(bytes);
            }
            break;
        }

        while(writebuf.size()) {
            auto results = co_await socket.async_write_some(writebuf.data(), use_awaitable);
            writebuf.consume(results);
        }

    }

    co_return;
}

void TelnetConnection::sendBytes(const std::vector<uint8_t>& bytes) {
    // flush bytes to writebuf.
    writebuf.commit(asio::buffer_copy(writebuf.prepare(bytes.size()), asio::buffer(bytes)));
}


awaitable<void> TelnetConnection::runReader() {
    while(true) {
        auto results = co_await socket.async_read_some(readbuf.prepare(1024), use_awaitable);
        readbuf.commit(results);

        while(co_await parseTelnet()) {

        }

    }

    co_return;
}

awaitable<bool> TelnetConnection::parseTelnet() {
    using namespace codes;
    auto data = static_cast<const uint8_t *>(readbuf.data().data());
    auto size = readbuf.size();

    if (size == 0) {
        co_return false;
    }

    if (data[0] != IAC) {
        // Handle raw app data
        size_t iac_pos = std::find(data, data + size, IAC) - data;
        std::vector<uint8_t> app_data(data, data + iac_pos);
        readbuf.consume(iac_pos);
        co_await handleApplicationData(app_data);
        co_return true;
    }

    // Handle Telnet commands
    if (size < 2) {
        // Need at least two bytes for any command
        co_return false;
    }

    uint8_t command = data[1];
    switch (command) {
        case IAC:
            readbuf.consume(2);
            co_await handleApplicationData({IAC});
            co_return true;

        case WILL:
        case WONT:
        case DO:
        case DONT:
            // Handle negotiation commands
            if (size < 3) {
                // Need three bytes for negotiation
                co_return false;
            }
            readbuf.consume(3);
            co_await handleNegotiation(command, data[2]);

            co_return true;

        case SB:
            // Handle subnegotiation
            if (size < 5) {
                // Need at least five bytes for subnegotiation
                co_return false;
            }
        // Find IAC SE sequence
            for (size_t i = 2; i < size - 1; ++i) {
                if (data[i] == IAC && data[i + 1] == SE) {
                    // Found end of subnegotiation
                    std::vector<uint8_t> subneg_data(data + 2, data + i); // Exclude IAC SE
                    readbuf.consume(i + 2);
                    co_await handleSubnegotiation(data[1], subneg_data);
                    co_return true;
                }
            }
        // IAC SE not found, need more data
            co_return false;

        default:
            readbuf.consume(2);
            co_await handleCommand(command);
            co_return true;
    }
}

awaitable<void> TelnetConnection::handleCommand(uint8_t command) {
    // this really does nothing at the moment.
    co_return;
}

awaitable<void> TelnetConnection::handleSubnegotiation(uint8_t option, const std::vector<uint8_t>& data) {
    if(const auto found = options.find(option); found != options.end()) {
        found->second->onSubnegotiation(data);
    }
    co_return;
}

void TelnetConnection::sendNegotiate(uint8_t negotiate, uint8_t option) {
    TelnetMessage msg(std::make_pair(negotiate, option));
    outMessage.try_send(asio::error_code{}, msg);
}

void TelnetConnection::sendSubnegotiate(uint8_t option, const std::vector<uint8_t> &data) {
    // we need to call sendbytes with a vector of bytes that is IAC SB <code> <data> IAC SE
    TelnetMessage msg(std::make_pair(option, data));
    outMessage.try_send(asio::error_code{}, msg);
}

void TelnetConnection::sendAppData(const std::vector<uint8_t>& data) {
    // This is basically just a byte send except we need to escape IAC bytes with an additional IAC.
    std::vector<uint8_t> bytes;
    bytes.reserve(data.size());
    for(auto byte : data) {
        if(byte == codes::IAC) {
            bytes.push_back(codes::IAC);
        }
        bytes.push_back(byte);
    }
    TelnetMessage msg(bytes);
    outMessage.try_send(asio::error_code{}, msg);
}

void TelnetConnection::sendCommand(uint8_t command) {
    TelnetMessage msg({command});
    outMessage.try_send(asio::error_code{}, msg);
}

awaitable<void> TelnetConnection::handleNegotiation(uint8_t negotiate, uint8_t option) {
    using namespace codes;
    if(const auto found = options.find(option); found != options.end()) {
        found->second->onNegotiate(negotiate);
        co_return;
    }

    switch(negotiate) {
        case WILL:
            sendNegotiate(DONT, option);
            break;
        case DO:
            sendNegotiate(WONT, option);
            break;
    }
    co_return;

}

awaitable<void> TelnetConnection::handleApplicationData(const std::vector<uint8_t>& d) {
    // first, we copy data into appbuf.
    appbuf.commit(asio::buffer_copy(appbuf.prepare(d.size()), asio::buffer(d)));

    auto data = static_cast<const char*>(appbuf.data().data());
    auto size = appbuf.size();

    if (size == 0) {
        co_return;
    }

    size_t start = 0;
    while (start < size) {
        // Find the next occurrence of \r\n in the buffer
        size_t end = start;
        bool found = false;
        while (end < size - 1) {
            if (data[end] == '\r' && data[end + 1] == '\n') {
                found = true;
                break;
            }
            ++end;
        }

        if (!found) {
            // No complete line found, exit the loop
            break;
        }

        // Create a string from the line (excluding \r\n)
        std::string line(data + start, data + end);

        // Send the line for further processing
        co_await toGame.async_send(asio::error_code{}, line, use_awaitable);

        // Move start position past the \r\n
        start = end + 2;
    }

    // Consume processed part of the buffer
    appbuf.consume(start);

    co_return;
}


void TelnetConnection::changeCapabilities(const nlohmann::json& j) {

}

void TelnetConnection::close() {
    // This is called by the game.
    state = net::ConnectionState::Dead;
    // Close the socket to signal that no more data will be sent.
    socket.close();
}

void TelnetConnection::onNetworkDisconnected() {
    // This is called by ASIO. The socket is already closed.
    state = net::ConnectionState::Dead;
}

void TelnetConnection::sendGMCP(const std::string& cmd, const nlohmann::json& j) {
    auto code = codes::GMCP;
    std::vector<uint8_t> out;

    // Copy the cmd string into the output buffer.
    std::copy(cmd.begin(), cmd.end(), std::back_inserter(out));

    if (!j.empty()) {
        // Add a space between the command and the JSON data.
        out.push_back(' ');

        // Convert the JSON to a string and then copy it into the output buffer.
        auto jd = jdump(j);
        std::copy(jd.begin(), jd.end(), std::back_inserter(out));
    }

    // Now 'out' contains the properly formatted command and data.
    outMessage.try_send(asio::error_code{}, std::make_pair(code, out));
}

void TelnetConnection::sendText(const std::string &text) {
    std::vector<uint8_t> out;

    auto colored = processColors(text, true, nullptr);

    std::copy(colored.begin(), colored.end(), std::back_inserter(out));
    outMessage.try_send(asio::error_code{}, out);
}

void TelnetConnection::onHeartbeat(double deltaTime) {
    asio::error_code ec;
    GameMessage msg;
    while (toGame.ready()) {
        if(!toGame.try_receive([&](asio::error_code ec2, const GameMessage& value2) {
            ec = ec2;
            msg = value2;
        })) {
            break;
        }
        if (ec) {
            // TODO: Handle any errors..
        } else {
            lastActivity = std::chrono::steady_clock::now();
            switch(msg.index()) {
                case 0:
                    executeCommand(std::get<std::string>(msg));
                    break;
                case 1: {
                    auto &[cmd, data] = std::get<std::pair<std::string, nlohmann::json>>(msg);
                    executeGMCP(cmd, data);
                }
                default:
                    break;
            }
        }
    }
}


static awaitable<void> handleConnection(ip::tcp::socket socket) {
    auto socketID = socket.native_handle();
    auto ex = co_await asio::this_coro::executor;

    auto conn = std::make_shared<TelnetConnection>(socketID, std::move(socket), ex);

    {
        std::lock_guard guard(net::connectionMutex);
        net::connections[socketID] = conn;
    }

    co_await conn->run();

    co_return;
}

static awaitable<void> listen(ip::tcp::acceptor& acceptor) {
    while(true) {
        auto socket = co_await acceptor.async_accept(use_awaitable);
        co_spawn(make_strand(*ioContext), handleConnection(std::move(socket)), detached);
    }

    co_return;
}

awaitable<void> manage_loop() {

    double deltaTime = 0.1;
    // The frequency in seconds. so, a tenth of a second. 100ms.
    double frequency = 0.1;
    double saveTimer = 60.0 * 5.0;

    auto lastTime = std::chrono::steady_clock::now();
    auto ex = co_await this_coro::executor;
    asio::steady_timer timer(ex, std::chrono::milliseconds(100));

    while(true) {
        auto start = std::chrono::steady_clock::now();
        game::run_loop_once(deltaTime);
        auto end = std::chrono::steady_clock::now();

        saveTimer -= deltaTime;
        if(saveTimer <= 0.0) {
            // TODO : Run save operation.
            co_await runSave();
            saveTimer = 60.0 * 5.0;
        }

        auto runDuration = end - start;
        // We want to sleep for the remainder of the frequency.
        auto sleepDuration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::duration<double>(frequency) - runDuration);
        if(sleepDuration.count() > 0) {
            timer.expires_after(sleepDuration);
            co_await timer.async_wait(use_awaitable);
        }
        // Now we need to calculate the delta time.
        auto now = std::chrono::steady_clock::now();
        deltaTime = std::chrono::duration<double>(now - start).count();

    }

    co_return;
}

awaitable<void> run_game() {
    game::init_locale();
    game::init_log();
    if(!game::init_sodium()) {
        logger->error("Failed to initialize libsodium");
        shutdown_game(EXIT_FAILURE);
    }
    load_config();
    chdir("lib");
    game::init_database();
    game::init_zones();

    co_await manage_loop();

    co_return;
}

int main(int argc, char* argv[]) {

    ioContext = std::make_unique<io_context>();

    ip::tcp::acceptor acceptor(*ioContext, {ip::tcp::v4(), 7999});
    co_spawn(*ioContext, listen(acceptor), asio::detached);
    co_spawn(*ioContext, run_game(), asio::detached);


    // Use as many threads as there are CPUs
    const auto thread_count = std::max<int>(1, std::thread::hardware_concurrency() - 1);
    std::vector<std::thread> threads;
    if(thread_count) {
        threads.reserve(thread_count);
        for (auto i = 0u; i < thread_count; ++i) {
            threads.emplace_back([&] { ioContext->run(); });
        }
    }

    ioContext->run();

    // Wait for all threads to complete
    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }
    threads.clear();

    return 0;
}