#include "DC.h"
#include <iostream>
#include <thread>
#include <mutex>
#include <Windows.h>

void LinkUpdate(DC::LinkBus* linkBus)
{
    while (linkBus->IsActive())
    {
        linkBus->Update();
    }
}

void NodeUpdate(DC::NodeComputer* node)
{
    node->Init();

    while (node->IsActive())
    {
        node->Update();
    }
}

int main()
{
    DC::LinkBus linkBus(600);
    linkBus.Init();
    DC::NodeComputer node1(1, &linkBus);
    DC::NodeComputer node2(2, &linkBus);
    DC::NodeComputer node3(3, &linkBus);
    DC::NodeComputer node4(4, &linkBus);

    std::thread t2(NodeUpdate, &node1);
    std::thread t3(NodeUpdate, &node2);
    std::thread t4(NodeUpdate, &node3);
    std::thread t5(NodeUpdate, &node4);

    std::thread t1(LinkUpdate, &linkBus);

    t1.join();
    t2.join();
    t3.join();
    t4.join();
    t5.join();

    return 0;
}