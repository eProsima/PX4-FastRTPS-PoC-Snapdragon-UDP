// Copyright 2017 Proyectos y Sistemas de Mantenimiento SL (eProsima).
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <string>
#include <errno.h>

#include "UDP_node.h"


/** CRC table for the CRC-16. The poly is 0x8005 (x^16 + x^15 + x^2 + 1) */
uint16_t const crc16_table[256] = {
	0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
	0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
	0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
	0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
	0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
	0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
	0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
	0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
	0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
	0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
	0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
	0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
	0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
	0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
	0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
	0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
	0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
	0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
	0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
	0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
	0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
	0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
	0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
	0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
	0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
	0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
	0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
	0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
	0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
	0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
	0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
	0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
};

UDP_node::UDP_node(): sender_fd(-1), receiver_fd(-1)
{

}

UDP_node::~UDP_node()
{
    close();
}

int UDP_node::init(uint16_t udp_port_recv, uint16_t udp_port_send)
{
    if (0 > init_receiver(udp_port_recv) || 0 > init_sender(udp_port_send))
        return -1;
    return 0;
}

int UDP_node::init_receiver(uint16_t udp_port)
{
    // udp socket data
    memset((char *)&receiver_inaddr, 0, sizeof(receiver_inaddr));
    receiver_inaddr.sin_family = AF_INET;
    receiver_inaddr.sin_port = htons(udp_port);
    receiver_inaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if ((receiver_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        printf("create socket failed\n");
        return -1;
    }

    printf("Trying to connect...\n");

    if (bind(receiver_fd, (struct sockaddr *)&receiver_inaddr, sizeof(receiver_inaddr)) < 0)
    {
        printf("bind failed\n");
        return -1;
    }

    printf("connected to server!\n");

    return 0;

}

int UDP_node::init_sender(uint16_t udp_port)
{
    if ((sender_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        printf("create socket failed\n");
        return -1;
    }

    memset((char *) &sender_outaddr, 0, sizeof(sender_outaddr));
    sender_outaddr.sin_family = AF_INET;
    sender_outaddr.sin_port = htons(udp_port);

    if (inet_aton("127.0.0.1" , &sender_outaddr.sin_addr) == 0)
    {
        printf("inet_aton() failed\n");
        return -1;
    }

    return 0;
}

uint8_t UDP_node::close()
{
    printf("Close sockets\n");
    ::close(sender_fd);
    ::close(receiver_fd);
    return 0;
}

int16_t UDP_node::read(char* topic_ID, char out_buffer[])
{
    if (-1 == receiver_fd || nullptr == out_buffer || nullptr == topic_ID)
        return -1;

    *topic_ID = 255;

    static socklen_t addrlen = sizeof(receiver_outaddr);

    // Blocking call
    ssize_t len = recvfrom(receiver_fd, (void*)(rx_buffer + rx_buff_pos), sizeof(rx_buffer) - rx_buff_pos, 0,
                           (struct sockaddr*) &receiver_outaddr, &addrlen);
    if (len <= 0)
    {
        printf("Failed receiving UDP message: %ld\n", (long)len);
        return -1;
    }

    //printf("received: %d bytes\n", len);

    rx_buff_pos += len;

    //printf("read %d\n", len);

    /*printf(">>> |");
    for (int i = 0; i < rx_buff_pos + len; ++i)printf(" %hhu", rx_buffer[i]);
    printf("\n");*/

    // We read some
    size_t header_size = sizeof(struct Header);
    if (rx_buff_pos < header_size)
    {
        return 0; //but not enough
    }

    uint32_t msg_start_pos = 0;
    for (msg_start_pos = 0; msg_start_pos <= rx_buff_pos - header_size; ++msg_start_pos)
    {
        if ('>' == rx_buffer[msg_start_pos] && memcmp(rx_buffer + msg_start_pos, ">>>", 3) == 0)
        {
            //printf("start found at %u\n", msg_start_pos);
            break;
        }
    }

    // Start not found
    if (msg_start_pos > rx_buff_pos - header_size)
    {
        //printf("start not found, pos %u\n", last_valid_pos);
        printf("                                 (↓↓ %lu)\n", (unsigned long)(rx_buff_pos - header_size + 1));
        // We don't know nothing about last bytes read, we keep them.
        memmove(rx_buffer, rx_buffer + rx_buff_pos - (header_size - 1), header_size - 1);
        rx_buff_pos = header_size - 1;
        return -1;
    }

    /*
     * [>,>,>,topic_ID,seq,payload_length_H,payload_length_L,CRCHigh,CRCLow,payloadStart, ... ,payloadEnd]
     */

    struct Header *header = (struct Header *)&rx_buffer[msg_start_pos];
    uint32_t payload_len = ((uint32_t)header->payload_len_h << 8) | header->payload_len_l;
    // We do not have a complete message yet
    if(msg_start_pos + header_size + payload_len > rx_buff_pos)
    {
        // If there's garbage at the beginning, drop it
        if (msg_start_pos > 0)
        {
            printf("                                 (↓ %u)\n", msg_start_pos);
            memmove(rx_buffer, rx_buffer + msg_start_pos, rx_buff_pos - msg_start_pos);
            rx_buff_pos -= msg_start_pos;
        }
        //printf("do not have a complete message, pos %u\n", rx_buff_pos);
        return 0;
    }

    // We have the whole message
    int ret;
    uint16_t read_crc = ((uint16_t)header->crc_h << 8) | header->crc_l;
    uint16_t calc_crc = crc16((uint8_t*)rx_buffer + msg_start_pos + header_size, payload_len);
    if (read_crc != calc_crc)
    {
        printf("BAD CRC %u != %u\n", read_crc, calc_crc);
        printf("                                 (↓ %lu)\n", (unsigned long)(header_size + payload_len));
        ret = -1;
    }
    else
    {
        //printf("GOOD CRC %u == %u\n", read_crc, calc_crc);
        // copy message to outbuffer and set other return values
        memmove(out_buffer, rx_buffer + msg_start_pos + header_size, payload_len);
        *topic_ID = header->topic_ID;
        ret = payload_len;
    }

    // discard message from rx_buffer
    rx_buff_pos -= header_size + payload_len;
    memmove(rx_buffer, rx_buffer + msg_start_pos + header_size + payload_len, rx_buff_pos);

    return ret;
}

int16_t UDP_node::write(const char topic_ID, char buffer[], uint32_t length)
{
    if (sender_fd == -1) return 2;

    static struct Header header {
        .marker = {'>','>','>'},
        .topic_ID = 0u,
        .seq = 0u,
        .payload_len_h = 0u,
        .payload_len_l = 0u,
        .crc_h = 0u,
        .crc_l = 0u

    };
    static uint8_t seq = 0;

    // [>,>,>,topic_ID,seq,payload_length,CRCHigh,CRCLow,payload_start, ... ,payload_end]

    uint16_t crc = crc16((uint8_t*)buffer, length);

    header.topic_ID = topic_ID;
    header.seq = seq++;
    header.payload_len_h = (length >> 8) & 0xff;
    header.payload_len_l = length & 0xff;
    header.crc_h = (crc >> 8) & 0xff;
    header.crc_l = crc & 0xff;

    ssize_t len = sendto(sender_fd, &header, sizeof(header), 0, (struct sockaddr *)&sender_outaddr, sizeof(sender_outaddr));

    if (len < 0 || len != sizeof(header))
        goto err;

    len = sendto(sender_fd, buffer, length, 0, (struct sockaddr *)&sender_outaddr, sizeof(sender_outaddr));

    if (len < 0 || (unsigned)len != length)
        goto err;

    //printf("sent!\n");

    /*printf(">>>%hhd %c %c|", topic_ID, seq, (char)length);
    for (int i = 0; i < length; ++i)printf(" %hhu", buffer[i]);
    printf("\n");*/

    return length;

err:
    int errsv = errno;
    if (len == -1 )
    {
        printf("                               => Writing error '%d'\n", errsv);
    }
    else
    {
        printf("                               => Wrote '%ld' != length(%lu) error '%d'\n", (long)len, (unsigned long)length, errsv);
    }
    return len;
}

uint16_t UDP_node::crc16_byte(uint16_t crc, const uint8_t data)
{
	return (crc >> 8) ^ crc16_table[(crc ^ data) & 0xff];
}

uint16_t UDP_node::crc16(uint8_t const *buffer, size_t len)
{
    uint16_t crc = 0;
    while (len--) crc = crc16_byte(crc, *buffer++);
    return crc;
}
