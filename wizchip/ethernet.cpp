#include "Ethernet/socket.h"
#include "etl/thread.h"
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

    cs.write(true);
    reg_wizchip_cs_cbfunc(
        [] { Ethernet::self->cs.write(false); }, 
        [] { Ethernet::self->cs.write(true); }
    );
    reg_wizchip_spi_cbfunc(
        [] { return Ethernet::self->readWriteByte(0); }, 
        [] (uint8_t byte) { Ethernet::self->readWriteByte(byte); }
    );
    reg_wizchip_spiburst_cbfunc(
        [] (uint8_t* data, uint16_t len) { 
            for (uint16_t i = 0; i < len; i++) data[i] = Ethernet::self->readWriteByte(0); 
        }, 
        [] (uint8_t* data, uint16_t len) { 
            for (uint16_t i = 0; i < len; i++) Ethernet::self->readWriteByte(data[i]); 
        }
    );

    uint8_t memsize[2][8] = { {2,2,2,2,2,2,2,2}, {2,2,2,2,2,2,2,2} };
    if (wizchip_init(memsize[0], memsize[1]) == -1) {
        debug << "wizchip_init fail\r\n";
        return;
    }

    while (wizphy_getphylink() == PHY_LINK_OFF) {
        debug << "wizphy_getphylink PHY_LINK_OFF\r\n";
    }

    wizchip_setnetinfo(&netInfo);

    isRunning = true;

    static etl::Thread<512> thd;
    thd.init({
        .function=etl::bind<&Ethernet::execute>(this),
    });
}

void Ethernet::execute() {
    auto static f = etl::string();
    while (isRunning) {
        for (auto socket_number : etl::range(_WIZCHIP_SOCK_NUM_)) {
            auto socket = sockets[socket_number];
            if (socket == nullptr) {
                etl::this_thread::sleep(1ms);
                continue;
            }

            switch (int res; getSn_SR(socket_number)) {
                case SOCK_INIT:
                    socket->on_init(socket_number);
                    debug << f("%d: socket init\r\n", socket_number);
                    break;

                case SOCK_LISTEN:
                    socket->on_listen(socket_number);
                    debug << f("%d: socket listen\r\n", socket_number);
                    break;

                case SOCK_ESTABLISHED: 
                    res = socket->on_established(socket_number);
                    debug << f("%d, %d: socket established\r\n", socket_number, res);
                    break;

                case SOCK_CLOSE_WAIT:
                    res = socket->on_close_wait(socket_number);
                    debug << f("%d, %d: socket close wait\r\n", socket_number, res);
                    break;

                case SOCK_CLOSED:
                    socket->on_closed(socket_number);
                    debug << f("%d: socket closed\r\n", socket_number);
                    break;

                default:
                    break;
            }
            etl::this_thread::sleep(1ms);
        }
    }
}

void Ethernet::deinit() {
    isRunning = false;
}

void Ethernet::setNetInfo(const wiz_NetInfo& netInfo_) {
    netInfo = netInfo_;
    wizchip_setnetinfo(&netInfo);
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

uint8_t Ethernet::readWriteByte(uint8_t data) {
    while ((hspi.Instance->SR & SPI_FLAG_TXE) != SPI_FLAG_TXE); // wait until FIFO has a free slot
    *(__IO uint8_t*) &hspi.Instance->DR = data; // write data
    while ((hspi.Instance->SR & SPI_FLAG_RXNE) != SPI_FLAG_RXNE); // wait until data arrives
    return *(__IO uint8_t*) &hspi.Instance->DR; // read data
}