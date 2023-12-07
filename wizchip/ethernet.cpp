#include "Ethernet/socket.h"
#include "etl/thread.h"
#include "etl/string_view.h"
#include "wizchip/ethernet.h"

using namespace Project;
using namespace Project::etl::literals;
using namespace Project::wizchip;

Ethernet* Ethernet::self = nullptr;
uint8_t Ethernet::rxData[WIZCHIP_BUFFER_LENGTH] = {};

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
        debug("wizchip_init fail\r\n");
        return;
    }

    while (wizphy_getphylink() == PHY_LINK_OFF) {
        debug("wizphy_getphylink PHY_LINK_OFF\r\n");
    }

    wizchip_setnetinfo(&netInfo);

    isRunning = true;

    static etl::Thread<512> thd;
    thd.init({
        .function=etl::bind<&Ethernet::execute>(this),
    });
}

void Ethernet::execute() {
    while (isRunning) {
        for (auto socket_number : etl::range(_WIZCHIP_SOCK_NUM_)) {
            auto socket = sockets[socket_number];
            if (socket == nullptr) {
                continue;
            }

            switch (getSn_SR(socket_number)) {
                case SOCK_ESTABLISHED: {
                    // Interrupt clear
                    if(getSn_IR(socket_number) & Sn_IR_CON) {
                        setSn_IR(socket_number, Sn_IR_CON);
                    }

                    size_t len = getSn_RX_RSR(socket_number);
                    if (len == 0) 
                        break;
                    
                    if (len > WIZCHIP_BUFFER_LENGTH) 
                        len = WIZCHIP_BUFFER_LENGTH;

                    len = ::recv(socket_number, rxData, len);
                    rxData[len] = '\0';

                    socket->process(socket_number, rxData, len);

                    // Check the TX socket buffer for End of HTTP response sends
                    while (getSn_TX_FSR(socket_number) != (getSn_TxMAX(socket_number))) {}
                }
                break;

                case SOCK_CLOSE_WAIT:
                    ::disconnect(socket_number);
                    break;

                case SOCK_CLOSED:
                    if (socket->keep_alive)
                        ::socket(socket_number, Sn_MR_TCP, socket->port, 0x00);
                    else 
                        unregisterSocket(socket);
                    break;

                case SOCK_INIT:
                    ::listen(socket_number);
                    break;

                case SOCK_LISTEN:
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

void Ethernet::registerSocket(Socket* socket, int numberOfSocket) {
    int i = 0;
    for (auto &socket_instance : sockets) if (socket_instance == nullptr) {
        if (i == numberOfSocket) return;
        socket_instance = socket;
        ++i;
    }
}

void Ethernet::unregisterSocket(Socket* socket) {
    for (auto &socket_instance : sockets) if (socket_instance == socket) {
        socket_instance = nullptr;
    }
}

uint8_t Ethernet::readWriteByte(uint8_t data) {
    while ((hspi.Instance->SR & SPI_FLAG_TXE) != SPI_FLAG_TXE); // wait until FIFO has a free slot
    *(__IO uint8_t*) &hspi.Instance->DR = data; // write data
    while ((hspi.Instance->SR & SPI_FLAG_RXNE) != SPI_FLAG_RXNE); // wait until data arrives
    return *(__IO uint8_t*) &hspi.Instance->DR; // read data
}