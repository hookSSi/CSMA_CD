#include "DC.h"
#include <iostream>
#include <thread>
#include <mutex>
#include <Windows.h>
#include <pthread.h>

void* LinkUpdate(void* data)
{
    DC::LinkBus* linkBus = (DC::LinkBus*)data;
    while (linkBus->IsActive())
    {
        linkBus->Update();
    }

    return data;
}

void* NodeUpdate(void* data)
{
    DC::NodeComputer* node = (DC::NodeComputer*)data;
    while (node->IsActive())
    {
        node->Update();
    }

    return data;
}

int main()
{
    DC::LinkBus linkBus(60000);
    linkBus.Init();

    DC::NodeComputer node1(1, &linkBus);
    node1.Init();
    DC::NodeComputer node2(2, &linkBus);
    node2.Init();
    DC::NodeComputer node3(3, &linkBus);
    node3.Init();
    DC::NodeComputer node4(4, &linkBus);
    node4.Init();

    pthread_t th[5];

    pthread_create(&th[0], NULL, LinkUpdate, &linkBus);
    pthread_create(&th[1], NULL, NodeUpdate, &node1);
    pthread_create(&th[2], NULL, NodeUpdate, &node2);
    pthread_create(&th[3], NULL, NodeUpdate, &node3);
    pthread_create(&th[4], NULL, NodeUpdate, &node4);

    pthread_join(th[0], NULL);
    pthread_join(th[1], NULL);
    pthread_join(th[2], NULL);
    pthread_join(th[3], NULL);
    pthread_join(th[4], NULL);

    //std::thread t1(LinkUpdate, &linkBus);
    //std::thread t2(NodeUpdate, &node1);
    //std::thread t3(NodeUpdate, &node2);
    //std::thread t4(NodeUpdate, &node3);
    //std::thread t5(NodeUpdate, &node4);

    //t1.join();
    //t2.join();
    //t3.join();
    //t4.join();
    //t5.join();

    return 0;
}