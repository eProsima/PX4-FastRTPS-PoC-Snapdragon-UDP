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
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <poll.h>
#include <string.h>
#include <termios.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <arpa/inet.h>

#include "UDP_node.h"
#include <fastcdr/Cdr.h>
#include <fastcdr/FastCdr.h>
#include <fastcdr/exceptions/Exception.h>
#include <fastrtps/utils/eClock.h>
#include <fastrtps/Domain.h>

#include "../../micrortps_agent_udp/apps_proc/sensor_combined_Publisher.h"
#include "../../micrortps_agent_udp/apps_proc/sensor_combined_Subscriber.h"

#define BUFFER_SIZE 1024
#define USLEEP_MS 0

using namespace eprosima;
using namespace eprosima::fastrtps;


UDP_node udp_node;
int sent = 0;

void* t_send(void* data)
{

    // Create subscribers
    sensor_combined_Subscriber sensor_combined_sub;
    if (true == sensor_combined_sub.init()) std::cout << "sensor_combined subscriber created" << std::endl;
    else std::cout << "ERROR creating sensor_combined subscriber" << std::endl;

    char data_buffer[BUFFER_SIZE] = {};

    while (true)
    {
        // Send subscribed topics over UART
        if (sensor_combined_sub.hasMsg())
        {
            sensor_combined_ msg = sensor_combined_sub.getMsg();
            eprosima::fastcdr::FastBuffer cdrbuffer(data_buffer, sizeof(data_buffer));
            eprosima::fastcdr::Cdr scdr(cdrbuffer);
            msg.serialize(scdr);
            ssize_t len = scdr.getSerializedDataLength();
            printf("[%d]>> %f\n", 58, (double)msg.baro_temp_celcius());
            udp_node.write((char) 58, scdr.getBufferPointer(), len);
            ++sent;
        }
        usleep(1);
    }
}

int main(int argc, char *argv[])
{

    if (0 != udp_node.init(2019, 2020))
    {
        printf("ERROR UDP INIT, EXITING...\n");
        return -1;
    }

    // create a thread for sending data to the simulator
    pthread_t sender_thread;
    // initialize threads
    pthread_attr_t sender_thread_attr;
    pthread_attr_init(&sender_thread_attr);
    //pthread_attr_setstacksize(&sender_thread_attr, PX4_STACK_ADJUSTED(4000));
    struct sched_param param;
    (void)pthread_attr_getschedparam(&sender_thread_attr, &param);
    /* low priority */
    param.sched_priority = 125;
    (void)pthread_attr_setschedparam(&sender_thread_attr, &param);
    // got data from simulator, now activate the sending thread
    pthread_create(&sender_thread, &sender_thread_attr, t_send, NULL);
    pthread_attr_destroy(&sender_thread_attr);

    // Create publishers
    sensor_combined_Publisher sensor_combined_pub;
    sensor_combined_pub.init();

    char data_buffer[BUFFER_SIZE] = {};
    char topic_ID = 255;
    while (true)
    {
        // Blocking call
        if (0 < udp_node.read(&topic_ID, data_buffer))
        {
            switch (topic_ID)
            {
                case 58:
                {
                    sensor_combined_ st;
                    eprosima::fastcdr::FastBuffer cdrbuffer(data_buffer, sizeof(data_buffer));
                    eprosima::fastcdr::Cdr cdr_des(cdrbuffer);
                    st.deserialize(cdr_des);
                    sensor_combined_pub.publish(&st);
                    printf("                >>[%d] %f\n", 58, (double)st.baro_temp_celcius());
                }
                break;
            }
        }
    }
    pthread_join(sender_thread, NULL);

    printf("exiting\n");
    fflush(stdout);
    return 0;
}
