#include "Ethernet/socket.h"
#include "etl/async.h"
#include "etl/string.h"
#include "wizchip/ethernet.h"

using namespace Project;
using namespace Project::etl::literals;
using namespace Project::wizchip;

Ethernet* Ethernet::self = nullptr;
uint8_t Ethernet::rxData[WIZCHIP_BUFFER_LENGTH] = {};
uint8_t Ethernet::txData[WIZCHIP_BUFFER_LENGTH] = {};

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

    isRunning = true;
    etl::async(etl::bind<&Ethernet::execute>(this));
}

void Ethernet::execute() {
    debug << "ethernet start\n";

    uint8_t memsize[2][8] = { {2,2,2,2,2,2,2,2}, {2,2,2,2,2,2,2,2} };
    if (wizchip_init(memsize[0], memsize[1]) == -1) {
        debug << "wizchip_init fail\n";
        return;
    }

    while (wizphy_getphylink() == PHY_LINK_OFF) {
        debug << "PHY_LINK_OFF\n";
        etl::this_thread::sleep(50ms);
    }

    wiz_NetTimeout tout = {.retry_cnt=10, .time_100us=100};
    wiz_PhyConf phyConf = {1, 0, 0, 0};

    wizchip_setnetinfo(&netInfo);
    wizchip_settimeout(&tout);

    wizphy_setphyconf(&phyConf);

    wizphy_getphyconf(&phyConf);
    wizphy_getphystat(&phyConf);

    wizchip_gettimeout(&tout);
    wizchip_getnetinfo(&netInfo);

    auto static f = etl::string<256>();
    debug << f(
        "\nconf %d %d %d %d"
        "\nretry_cnt: %d"
        "\ntimeout: %d ms"
        "\nip: %d.%d.%d.%d" 
        "\ngw: %d.%d.%d.%d" 
        "\ndns: %d.%d.%d.%d" 
        "\ndhcp: %d\n", 
        phyConf.by, phyConf.mode, phyConf.speed, phyConf.duplex,
        tout.retry_cnt,
        tout.time_100us * 10,
        netInfo.ip[0], netInfo.ip[1], netInfo.ip[2], netInfo.ip[3],
        netInfo.gw[0], netInfo.gw[1], netInfo.gw[2], netInfo.gw[3],
        netInfo.dns[0], netInfo.dns[1], netInfo.dns[2], netInfo.dns[3],
        netInfo.dhcp
    );

    while (isRunning) {
        for (auto socket_number : etl::range(_WIZCHIP_SOCK_NUM_)) {
            auto &socket = sockets[socket_number];
            if (socket == nullptr) {
                etl::this_thread::sleep(1ms);
                continue;
            }

            switch (int res; getSn_SR(socket_number)) {
                case SOCK_INIT:
                    res = socket->on_init(socket_number);
                    debug << f("%d, %d: socket init\n", socket_number, res);
                    break;

                case SOCK_LISTEN:
                    res = socket->on_listen(socket_number);
                    debug << f("%d, %d: socket listen\n", socket_number, res);
                    break;

                case SOCK_ESTABLISHED: 
                    res = socket->on_established(socket_number);
                    debug << f("%d, %d: socket established\n", socket_number, res);
                    break;

                case SOCK_CLOSE_WAIT:
                    res = socket->on_close_wait(socket_number);
                    debug << f("%d, %d: socket close wait\n", socket_number, res);
                    break;

                case SOCK_CLOSED:
                    res = socket->on_closed(socket_number);
                    debug << f("%d, %d: socket closed\n", socket_number, res);
                    break;

                default:
                    break;
            }

            if (socket == nullptr) // socket is closed
                etl::this_thread::sleep(1ms);
        }
        etl::this_thread::sleep(1ms);
    }
}

void Ethernet::deinit() {
    isRunning = false;
}

void Ethernet::setNetInfo(const wiz_NetInfo& netInfo_) {
    netInfo = netInfo_;
    wizchip_setnetinfo(&netInfo);
}

auto Ethernet::getNetInfo() -> const wiz_NetInfo& {
    wizchip_getnetinfo(&netInfo);
    return netInfo;
}

int Socket::allocate(int number_of_socket) {
    int i = 0;
    for (auto &socket_instance : ethernet.sockets) if (socket_instance == nullptr or socket_instance == this) {
        if (i == number_of_socket) return i;
        socket_instance = this;
        ++i;
    }
    return i;
}

void Socket::deallocate(int socket_number) {
    if (ethernet.sockets[socket_number] == this) {
        ethernet.sockets[socket_number] = nullptr;
    }
}
