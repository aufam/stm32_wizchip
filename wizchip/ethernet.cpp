#include "Ethernet/socket.h"
#include "wizchip/ethernet.h"
#include "etl/async.h"
#include "etl/heap.h"
#include "etl/keywords.h"

using namespace Project;
using namespace Project::etl::literals;
using namespace Project::wizchip;

Ethernet* Ethernet::self = nullptr;
auto static f = etl::string<64>();

void Ethernet::init() {
    self = this;

    cs.init({.mode=GPIO_MODE_OUTPUT_OD});
    rst.init({.mode=GPIO_MODE_OUTPUT_OD});

    cs.write(true);
    rst.write(true);

    reg_wizchip_cs_cbfunc(
        [] { Ethernet::self->cs.write(false); }, 
        [] { Ethernet::self->cs.write(true); }
    );
    reg_wizchip_spi_cbfunc(
        [] {
            uint8_t byte; 
            HAL_SPI_Receive(&Ethernet::self->hspi, &byte, 1, HAL_MAX_DELAY); 
            return byte;
        }, 
        [] (uint8_t byte) { 
            HAL_SPI_Transmit(&Ethernet::self->hspi, &byte, 1, HAL_MAX_DELAY); 
        }
    );
    reg_wizchip_spiburst_cbfunc(
        [] (uint8_t* buf, uint16_t len) { 
            HAL_SPI_Receive(&Ethernet::self->hspi, buf, len, HAL_MAX_DELAY); 
        }, 
        [] (uint8_t* buf, uint16_t len) { 
            HAL_SPI_Transmit(&Ethernet::self->hspi, buf, len, HAL_MAX_DELAY); 
        }
    );

    mutex.init();
    etl::async(etl::bind<&Ethernet::execute>(this));
}

void Ethernet::execute() {
    rst.write(0);
    etl::this_thread::sleep(100ms);
    rst.write(1);
    etl::this_thread::sleep(100ms);
    
    logger << "ethernet start\n";

    uint8_t memsize[2][8] = { {2,2,2,2,2,2,2,2}, {2,2,2,2,2,2,2,2} };
    if (wizchip_init(memsize[0], memsize[1]) == -1) {
        logger << "wizchip_init fail\n";
        return;
    }

    auto check_phy_link = [this] {
        while (wizphy_getphylink() == PHY_LINK_OFF) {
            logger << "PHY_LINK_OFF\n";
            etl::this_thread::sleep(50ms);
        }
    };

    check_phy_link();
    _is_running = true;
    setNetInfo(netInfo);

    while (_is_running) {
        etl::this_thread::sleep(1ms);
        
        auto lock = mutex.lock().await();
        for (auto socket_number : etl::range(_WIZCHIP_SOCK_NUM_)) {
            auto &si = socket_handlers[socket_number].socket_interface;
            if (si == nullptr)
                continue;

            switch (int res; getSn_SR(socket_number)) {
                case SOCK_INIT:
                    res = si->on_init(socket_number);
                    logger << f("%d, %d: %s init\n", socket_number, res, si->kind());
                    break;

                case SOCK_LISTEN:
                    res = si->on_listen(socket_number);
                    logger << f("%d, %d: %s listen\n", socket_number, res, si->kind());
                    break;

                case SOCK_ESTABLISHED: 
                    // Interrupt clear
                    if (getSn_IR(socket_number) & Sn_IR_CON)
                        setSn_IR(socket_number, Sn_IR_CON);
                    res = si->on_established(socket_number);
                    logger << f("%d, %d: %s established\n", socket_number, res, si->kind());
                    break;

                case SOCK_CLOSE_WAIT:
                    res = si->on_close_wait(socket_number);
                    logger << f("%d, %d: %s close wait\n", socket_number, res, si->kind());
                    break;

                case SOCK_FIN_WAIT:
                case SOCK_CLOSED:
                    res = si->on_closed(socket_number);
                    logger << f("%d, %d: %s closed\n", socket_number, res, si->kind());
                    break;

                default:
                    break;
            }
        }

        check_phy_link();
    }
}

void Ethernet::deinit() {
    _is_running = false;
}

bool Ethernet::isRunning() const {
    return _is_running;
}

void Ethernet::setNetInfo(const wiz_NetInfo& netInfo_) {
    netInfo = netInfo_;
    if (not _is_running) 
        return;

    auto lock = mutex.lock().await();

	wiz_PhyConf phyConf;
	wizphy_getphystat(&phyConf);

    wiz_NetTimeout tout = {.retry_cnt=10, .time_100us=100};

	wizchip_setnetinfo(&netInfo);
	wizchip_getnetinfo(&netInfo);

	ctlnetwork(CN_SET_TIMEOUT,(void*)&tout);
	ctlnetwork(CN_GET_TIMEOUT, (void*)&tout);

    logger << f("config: %d %d %d %d\n", phyConf.by, phyConf.mode, phyConf.speed, phyConf.duplex);
    logger << f("retry: %d\n", tout.retry_cnt);
    logger << f("timeout: %d ms\n", tout.time_100us * 10);
    logger << f("dhcp: %d\n", netInfo.dhcp == NETINFO_STATIC ? "static" : "dynamic");
    logger << f("dns: %d.%d.%d.%d\n", netInfo.dns[0], netInfo.dns[1], netInfo.dns[2], netInfo.dns[3]);
    logger << f("mac: %02x:%02x:%02x:%02x:%02x:%02x\n", netInfo.mac[0], netInfo.mac[1], netInfo.mac[2], netInfo.mac[3], netInfo.mac[4], netInfo.mac[5]);
    logger << f("gw: %d.%d.%d.%d\n", netInfo.gw[0], netInfo.gw[1], netInfo.gw[2], netInfo.gw[3]);
    logger << f("ip: %d.%d.%d.%d\n", netInfo.ip[0], netInfo.ip[1], netInfo.ip[2], netInfo.ip[3]);
}

auto Ethernet::getNetInfo() -> const wiz_NetInfo& {
    wizchip_getnetinfo(&netInfo);
    return netInfo;
}

void SocketServer::start(int number_of_socket) {
    auto lock = Ethernet::self->mutex.lock().await();
    reserved_sockets.reserve(number_of_socket);

    int cnt = 0;
    for (auto i in etl::range(_WIZCHIP_SOCK_NUM_)) if (not Ethernet::self->socket_handlers[i].is_busy()) {
        Ethernet::self->socket_handlers[i].socket_interface = this;
        reserved_sockets.append(i);

        cnt++;
        if (cnt == number_of_socket) break;
    }
}

void SocketServer::stop() {
    auto lock = Ethernet::self->mutex.lock().await();
    for (auto sn in reserved_sockets) {
        Ethernet::self->socket_handlers[sn].socket_interface = nullptr;
        ::close(sn);
    }
    reserved_sockets.clear();
}

bool SocketServer::isRunning() const {
    return Ethernet::self->isRunning() && reserved_sockets.len() > 0;
}

static uint16_t port_session = 50000;

SocketSession::SocketSession(uint8_t protocol, uint8_t flag, etl::Vector<uint8_t> host, int port) : host(etl::move(host)), port(port), socket_number(-1) {
    auto lock = Ethernet::self->mutex.lock().await();
    for (auto i in etl::range(_WIZCHIP_SOCK_NUM_)) if (not Ethernet::self->socket_handlers[i].is_busy()) {
        Ethernet::self->socket_handlers[i].socket_session = true;
        if (port_session == 0xFFFF) port_session = 50000; 
        ::close(i);
        ::socket(i, protocol, port_session++, flag);
        socket_number = i;
        return;
    }
}

SocketSession::~SocketSession() {
    auto lock = Ethernet::self->mutex.lock().await();
    Ethernet::self->socket_handlers[socket_number].socket_session = false;
    ::close(socket_number);
}

auto detail::ipv4_to_bytes(const char* ip) -> etl::Vector<uint8_t> {
    auto res = etl::vector_reserve<uint8_t>(4);
    auto sv = etl::string_view(ip);
    
    auto nums = sv.split<4>(".");
    if (nums.len() != 4) return res;

    for (auto num in nums) {
        res += uint8_t(num.to_int());
    }

    return res;
}

auto detail::ipv4_port_to_pair(const char* ip_port) -> etl::Pair<etl::Vector<uint8_t>, uint16_t> {
    auto ip = ipv4_to_bytes(ip_port);
    if (ip.len() != 4) return {};

    auto sv = etl::string_view(ip_port);
    auto sv_port = sv.split<2>(":")[1];

    auto port = (uint16_t) sv_port.to_int_or(80);
    return {etl::move(ip), port};
}

auto detail::tcp_transmit(int socket_number, etl::Vector<uint8_t> data) -> etl::Result<void, osStatus_t> {
    auto res = ::send(socket_number, data.data(), data.size());
    if (res < 0) return etl::Err(osStatus_t(res));

    // Check the TX socket buffer for End of tcp response sends
    int retry = 0;
    while (getSn_TX_FSR(socket_number) != (getSn_TxMAX(socket_number))) {
        etl::this_thread::sleep(1ms);
        retry++;
        if (retry == 100) break;
    }

    return etl::Ok();
}

auto detail::tcp_receive(int socket_number) -> etl::Result<etl::Vector<uint8_t>, osStatus_t> {
    auto res = etl::vector<uint8_t>();
    while (true) {
        size_t len = getSn_RX_RSR(socket_number);
        if (len == 0) break;

        if (etl::heap::freeSize < len + res.len()) {
            return etl::Err(osErrorNoMemory);
        }

        auto res_new = etl::vector_allocate<uint8_t>(len + res.len());
        ::memcpy(res_new.data(), res.data(), res.len());
        
        ::recv(socket_number, res_new.data() + res.len(), len);
        res = etl::move(res_new);
    }

    if (res.len() == 0) return etl::Err(osError);
    else return etl::Ok(etl::move(res));
}

auto detail::udp_transmit(int socket_number, etl::Vector<uint8_t> ip, int port, etl::Vector<uint8_t> data) -> etl::Result<void, osStatus_t> {
    auto res = ::sendto(socket_number, data.data(), data.len(), ip.data(), port);
    if (res > 0) {
        return etl::Ok();
    } else {
        return etl::Err(osStatus_t(res));
    }
}

auto detail::udp_receive(int socket_number, etl::Vector<uint8_t> ip) -> etl::Result<Project::etl::Vector<uint8_t>, osStatus_t> {
    auto res = etl::vector<uint8_t>();
    while (true) {
        size_t len = getSn_RX_RSR(socket_number);
        if (len == 0) break;

        if (etl::heap::freeSize < len + res.len()) {
            return etl::Err(osErrorNoMemory);
        }

        auto res_new = etl::vector_allocate<uint8_t>(len + res.len());
        ::memcpy(res_new.data(), res.data(), res.len());
        
        uint16_t port_dummy;
        ::recvfrom(socket_number, res_new.data() + res.len(), len, ip.data(), &port_dummy);
        res = etl::move(res_new);
    }

    if (res.len() == 0) return etl::Err(osError);
    else return etl::Ok(etl::move(res));
}