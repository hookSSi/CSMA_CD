#include "DC.h"
#include<math.h>
#include<stdlib.h>
#include<iostream>
#include<fstream>
#include<algorithm>
#include<mutex>
#include<random>
#include<Windows.h>

using namespace DC;

std::mutex mtx;

inline int Random(int range) {
    static thread_local std::mt19937 generator((unsigned int)time(NULL));
    std::uniform_int_distribution<int> distribution(1, range);

    return distribution(generator);
}

inline int Random() { return Random(100); }

/// <summary>
/// Exponetial Back-off 알고리즘 함수
/// </summary>
/// <param name="transNum"> 지수승 단계 </param>
/// <returns>기다려야하는 시간</returns>
inline int BackoffTimer(int transNum)
{
    int temp;
    temp = fmin(transNum, 10);

    return (Random((int)(pow(2, temp) - 1)) - 1);
}

std::string ZeroPadding(int n, int paddingNum)
{
    std::string old_string = std::to_string(n);
    int n_zero = paddingNum;

    return std::string(n_zero - old_string.length(), '0') + old_string;
}

std::string IntToTime(int t)
{
    std::string result;

    int msec = t % 1000;
    int sec = ((t - msec) / 1000) % 60;
    int min = (t - sec * 1000 - msec) / 60000;
    
    return ZeroPadding(min, 2) + ":" + ZeroPadding(sec, 2) + ":" + ZeroPadding(msec, 3);
}

void AddLogToFile(std::string path, MessageData data)
{
    std::ofstream writeFile;
    writeFile.open(path + ".txt", std::ios::app);    //파일 열기(파일이 없으면 만들어짐)
    if (writeFile.is_open())
    {
        std::string text = IntToTime(data.t) + " " + data.logData + "\n";
        writeFile << text;
        writeFile.flush();
    }

    writeFile.close();
}

void CreateLogFile(std::string path, MessageData data)
{
    std::ofstream writeFile;
    writeFile.open(path + ".txt");    //파일 열기(파일이 없으면 만들어짐)
    std::string text = IntToTime(data.t) + " " + data.logData + "\n";
    writeFile << text;
    writeFile.flush();
    writeFile.close();
}

/// <summary>
/// 각 노드를 구성할 NodeComputer 클래스 구현
/// </summary>
void NodeComputer::Init()
{
    _linkedBus->_nodeArr.push_back(this);
    std::string nodeName = "Node" + std::to_string(this->_id);
    MessageData data(_linkedBus->GetSystemClock(), nodeName + " Start");

    CreateLogFile(nodeName, data);
}

/// <summary>
/// 전송 요청
/// </summary>
void NodeComputer::SendRequest(int dest)
{
    _linkedBus->GetRequest(this->_id, dest);
}

void NodeComputer::Update()
{
    mtx.lock();
    if (!_linkedBus->IsActive())
    {
        _isActive = false;
        return;
    }

    switch (_state)
    {
    case DC::NODE_STATE::WAITING:
        if (_linkedBus->GetSystemClock() >= (_prevTime + _waitTime))
        {
            std::string nodeName = "Node" + std::to_string(this->_id);
            MessageData msg(_prevTime + _waitTime, _msg);
            AddLogToFile(nodeName, msg);
            _state = NODE_STATE::NORMAL;
        }
        break;
    case DC::NODE_STATE::RETRY:
        if (_linkedBus->GetSystemClock() >= (_prevTime + _waitTime)) // 큐로 관리해야하나?
        {
            NodeComputer::SendRequest(_savedDest);
        }
        break;
    case DC::NODE_STATE::NORMAL:
        NodeComputer::RandomSomething(1, &NodeComputer::SendRequest);
        break;
    }
    mtx.unlock();
}

void NodeComputer::RandomSomething(int probability, void (NodeComputer::*func)(int))
{
    int rndom = Random();

    if (rndom <= probability)
    {
        if (_linkedBus->_nodeArr.size() >= 2)
        {
            int dest = Random(_linkedBus->_nodeArr.size());

            // 목적지가 자신의 id일 경우 다시 한번 고름
            while (dest == this->_id || dest == 0)
            {
                dest = Random(_linkedBus->_nodeArr.size());
            }

            (this->*func)(dest);
        }
    }
}

void NodeComputer::Wait(int t, int range, std::string msg)
{
    _prevTime = t;
    _waitTime = range;
    _msg = msg;
    _state = NODE_STATE::WAITING;
}

void NodeComputer::Wait(int t, int range, int dest)
{
    _prevTime = t;
    _waitTime = range;
    _msg = "";
    _savedDest = dest;
    _state = NODE_STATE::RETRY;
}

NodeComputer::~NodeComputer()
{
    std::string nodeName = "Node" + std::to_string(this->_id);
    MessageData msg(_linkedBus->GetSystemClock(), nodeName + " Finished");
    AddLogToFile(nodeName, msg);
}

/// <summary>
/// Link Bus 클래스 구현
/// </summary>
void LinkBus::Init()
{
    MessageData msg(_systemClock, "Link Start");
    CreateLogFile("Link", msg);

    msg.Set(_systemClock, "System Clock Start");
    AddLogToFile("Link", msg);
}

void LinkBus::Update()
{
    if (_isActive)
    {
        mtx.lock();
        if (_timeLimit <= this->_systemClock)
        {
            _isActive = false;
            mtx.unlock();
            return;
        }
        this->_systemClock += 1;

        if (_systemClock >= (_prevTime + _waitTime) && !_isValid)
        {
            MessageData msg(_systemClock, _msg);
            AddLogToFile("Link", msg);
            this->_isValid = true;
            this->_waitTime = 0;
            this->_msg = "";
        }
        mtx.unlock();

        if (_isValid)
        {
            std::string text = IntToTime(_systemClock) + " == 진행상황 ==";
            std::cout << text << std::endl;
        }
    }
}

void LinkBus::GetRequest(int from, int dest)
{
    MessageData msg(_systemClock, "Data Send Request To Node" + std::to_string(dest));
    std::string path = "Node" + std::to_string(from);
    AddLogToFile(path, msg);

    if (LinkBus::CollisionCheck(dest))
    {
        MessageData msg(_systemClock, "Accept: Node" + std::to_string(from) + " Data Send Request To Node" + std::to_string(dest));
        AddLogToFile("Link", msg);

        // from 에게 알림
        msg.Set(_systemClock, "Data Send Request Accept from Link");
        path = "Node" + std::to_string(from);
        AddLogToFile(path, msg);

        // dest 에게 알림
        msg.Set(_systemClock, "Data Receive Start from Node" + std::to_string(from));
        path = "Node" + std::to_string(dest);
        AddLogToFile(path, msg);

        NodeComputer* fromNode = GetNode(from);
        NodeComputer* destNode = GetNode(dest);

        // 전송시간 5 msec
        fromNode->Wait(_systemClock, 5, "Data Send Finished To Node" + std::to_string(dest));
        destNode->Wait(_systemClock, 5, "Data Receive Finished From Node" + std::to_string(from));

        _waitTime = 5;
        _prevTime = _systemClock;
        _msg = "Node" + std::to_string(from) + " Data Send Finished To Node" + std::to_string(dest);
    }
    else
    {
        // Link에게 알림
        MessageData msg(_systemClock, "Reject: Node" + std::to_string(from) + " Data Send Request To Node" + std::to_string(dest));
        AddLogToFile("Link", msg);

        // from 에게 알림
        msg.Set(_systemClock, "Data Send Request Reject from Link");
        path = "Node" + std::to_string(from);
        AddLogToFile(path, msg);

        NodeComputer* fromNode = GetNode(from);
        int waitTime = BackoffTimer(_nodeArr.size());

        std::string nodeName = "Node" + std::to_string(from);
        msg.Set(_systemClock, "Exponential Back-off Time: " + std::to_string(waitTime) + " msec");
        AddLogToFile(nodeName, msg);

        fromNode->Wait(_systemClock, waitTime, dest);
    }
}

bool LinkBus::CollisionCheck(int dest)
{
    // 두 노드를 연결
    NodeComputer* destNode = GetNode(dest);

    if (_isValid && destNode->_state != NODE_STATE::WAITING)
    {
        _isValid = false;
        return true;
    }
    else
    {
        return false;
    }
}

NodeComputer* LinkBus::GetNode(int id)
{
    for (int i = 0; i < _nodeArr.size(); i++)
    {
        if (_nodeArr[i]->GetId() == id)
        {
            return _nodeArr[i];
        }
    }

    std::cerr << "노드 인덱스 에러 : 해당하는 id를 가진 노드가 없습니다 - " << id << std::endl;
    return nullptr;
}


LinkBus::~LinkBus()
{
    MessageData msg(_systemClock, "System Clock Finished");
    AddLogToFile("Link", msg);

    msg.Set(_systemClock, "Link Finished");
    AddLogToFile("Link", msg);
}