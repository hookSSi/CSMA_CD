#include "DC.h"
#include<math.h>
#include<stdlib.h>
#include<iostream>
#include<fstream>
#include<algorithm>
#include<mutex>
#include<random>
#include<Windows.h>
#include<pthread.h>

using namespace DC;

//FIFO ����
typedef struct ticket_lock {
    pthread_cond_t cond;
    pthread_mutex_t mutex;
    unsigned long queue_head, queue_tail;
} ticket_lock_t;

#define TICKET_LOCK_INITIALIZER { PTHREAD_COND_INITIALIZER, PTHREAD_MUTEX_INITIALIZER }

void ticket_lock(ticket_lock_t* ticket)
{
    unsigned long queue_me;

    pthread_mutex_lock(&ticket->mutex);
    queue_me = ticket->queue_tail++;
    while (queue_me != ticket->queue_head)
    {
        pthread_cond_wait(&ticket->cond, &ticket->mutex);
    }
    pthread_mutex_unlock(&ticket->mutex);
}

void ticket_unlock(ticket_lock_t* ticket)
{
    pthread_mutex_lock(&ticket->mutex);
    ticket->queue_head++;
    pthread_cond_broadcast(&ticket->cond);
    pthread_mutex_unlock(&ticket->mutex);
}

ticket_lock_t mtx = TICKET_LOCK_INITIALIZER;

inline int Random(int range) {
    static thread_local std::mt19937_64 generator((unsigned int)time(NULL));
    std::uniform_int_distribution<int> distribution(1, range);

    return distribution(generator);
}

inline int Random() { return Random(100); }

/// <summary>
/// Exponetial Back-off �˰��� �Լ�
/// </summary>
/// <param name="transNum"> ������ �ܰ� </param>
/// <returns>��ٷ����ϴ� �ð�</returns>
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

void AddLogToFile(std::string path, int sendTime, std::string logData)
{
    std::ofstream writeFile;
    writeFile.open(path + ".txt", std::ios::app);    //���� ����(������ ������ �������)
    if (writeFile.is_open())
    {
        std::string text = IntToTime(sendTime) + " " + logData + "\n";
        writeFile << text;
        writeFile.flush();
    }

    writeFile.close();
}

void CreateLogFile(std::string path, int sendTime, std::string logData)
{
    std::ofstream writeFile;
    writeFile.open(path + ".txt");    //���� ����(������ ������ �������)
    std::string text = IntToTime(sendTime) + " " + logData + "\n";
    writeFile << text;
    writeFile.flush();
    writeFile.close();
}

void Object::Update()
{
    std::queue<Request*> temp;

    while (!this->requestQueue.empty())
    {
        // Result ó��
        Request* request = this->requestQueue.front();
        this->requestQueue.pop();
        
        // ó���� true�� �� ��쿡�� ť���� ����
        if (this->ProcessRequest(request))
        {
            delete(request); // �޸� ����
        }
        else
        {
            temp.push(request);
        }
    }

    // ó�� �ȵ� �͵� ��Ƽ� �ٽ� ����ֱ�
    while (!temp.empty())
    {
        Request* request = temp.front();
        temp.pop();
        this->requestQueue.push(request);
    }
}

NodeComputer::~NodeComputer()
{
    std::string nodeName = "Node" + std::to_string(this->_id);
    AddLogToFile(nodeName, _linkedBus->GetSystemClock(), nodeName + " Finished");
}

/// <summary>
/// �� ��带 ������ NodeComputer Ŭ���� ����
/// </summary>
void NodeComputer::Init()
{
    _linkedBus->_nodeArr.push_back(this);
    std::string nodeName = "Node" + std::to_string(this->_id);

    CreateLogFile(nodeName, _linkedBus->GetSystemClock(), nodeName + " Start");
}

void NodeComputer::Update()
{
    if (_linkedBus->IsActive())
    {
        ticket_lock(&mtx);
        Object::Update();

        if (_state == DC::NODE_STATE::NORMAL)
        {
            int probabilty = 1;
            NodeComputer::RandomSomething(probabilty, &NodeComputer::SendRequest);
        }
        ticket_unlock(&mtx);
    }
    else
    {
        _isActive = false;
    }
}

bool NodeComputer::ProcessRequest(const Request* request)
{
    switch (request->data.state)
    {
    case REQUEST_STATE_TYPE::REQUEST_SUCCESS:
        this->_state = NODE_STATE::TRANSMITING;
        break;
    case REQUEST_STATE_TYPE::TRANSMIT_FINISHED:
        this->_state = NODE_STATE::NORMAL;
        break;
    }

    return true;
}

/// <summary>
/// ���� ��û
/// </summary>
/// /// <param name="receiver">���� ��� id</param>
void NodeComputer::SendRequest(int receiver)
{
    RequestData requestData(_linkedBus->GetSystemClock(), REQUEST_STATE_TYPE::SEND_REQUEST);
	Request* request = new Request(this->_id, receiver, requestData);
	_linkedBus->GetRequest(request);
	this->_state = NODE_STATE::WAITING;
}

/// <summary>
/// ������ ��û
/// </summary>
/// <param name="request"></param>
void NodeComputer::RetrySendRequest(const Request* request)
{
    RequestData requestData(_linkedBus->GetSystemClock(), REQUEST_STATE_TYPE::RETRY_SEND_REQUEST, 0, request->data.count);
    Request* newRequest = new Request(request->sender, request->receiver, requestData);
    _linkedBus->GetRequest(newRequest);
    this->_state = NODE_STATE::WAITING;
}

void NodeComputer::RandomSomething(int probability, void (NodeComputer::*func)(int))
{
    int rndom = Random();

    if (rndom <= probability)
    {
        if (_linkedBus->_nodeArr.size() >= 2)
        {
            int dest = Random(_linkedBus->_nodeArr.size());

            // �������� �ڽ��� id�� ��� �ٽ� �ѹ� ��
            while (dest == this->_id || dest == 0)
            {
                dest = Random(_linkedBus->_nodeArr.size());
            }

            (this->*func)(dest);
        }
    }
}

/// <summary>
/// Link Bus Ŭ���� ����
/// </summary>
void LinkBus::Init()
{
    CreateLogFile("Link", _systemClock, "Link Start");
    AddLogToFile("Link", _systemClock, "System Clock Start");
}

void LinkBus::Update()
{
    ticket_lock(&mtx);
    if (_isActive)
    {
        Object::Update();

        this->_systemClock += 1;

        if (_timeLimit <= this->_systemClock)
        {
            _isActive = false;
            ticket_unlock(&mtx);
            return;
        }
    }
    ticket_unlock(&mtx);
}

bool LinkBus::ProcessRequest(const Request* request)
{
    REQUEST_STATE_TYPE state = request->data.state;

    switch (state)
    {
    case DC::REQUEST_STATE_TYPE::SEND_REQUEST:
    case DC::REQUEST_STATE_TYPE::RETRY_SEND_REQUEST:
        ProcessSenderRequest(request);
        return true;
        break;
    case DC::REQUEST_STATE_TYPE::WAIT_REQUEST:
    case DC::REQUEST_STATE_TYPE::REQUEST_SUCCESS:
        bool result = ProcessWait(request);
        if (result)
        {
            return true;
        }
        else
        {
            return false;
        }
        break;
    }

    return false;
}

void LinkBus::ProcessSenderRequest(const Request* request)
{
    int sender = request->sender;
    int receiver = request->receiver;

    std::string path = "Node" + std::to_string(sender);
    AddLogToFile(path, _systemClock, "Data Send Request To Node" + std::to_string(receiver));

    if (LinkBus::CollisionCheck(receiver))
    {
        AddLogToFile("Link", _systemClock, "Accept: Node" + std::to_string(sender) + " Data Send Request To Node" + std::to_string(receiver));

        // sender ���� �˸�
        path = "Node" + std::to_string(sender);
        AddLogToFile(path, _systemClock, "Data Send Request Accept from Link");

        // receiver ���� �˸�
        path = "Node" + std::to_string(receiver);
        AddLogToFile(path, _systemClock, "Data Receive Start from Node" + std::to_string(sender));

        NodeComputer* senderNode = GetNode(sender);
        NodeComputer* receiverNode = GetNode(receiver);

        RequestData requestData(_systemClock, REQUEST_STATE_TYPE::REQUEST_SUCCESS, 0, 0);
        Request* requestToSender = new Request(0, sender, requestData);
        Request* requestToReceiver = new Request(0, receiver, requestData);

        senderNode->GetRequest(requestToSender);
        receiverNode->GetRequest(requestToReceiver);

        // ��ũ���� �󸶳� ��ٷ����ϴ��� �˸�
        RequestData transmitData(_systemClock, REQUEST_STATE_TYPE::REQUEST_SUCCESS, senderNode->GetTransmitTime(), 0);
        Request* requestToLink = new Request(sender, receiver, transmitData);
        GetRequest(requestToLink);
    }
    else
    {
        // Link���� �˸�
        AddLogToFile("Link", _systemClock, "Reject: Node" + std::to_string(sender) + " Data Send Request To Node" + std::to_string(receiver));

        // sender ���� �˸�
        path = "Node" + std::to_string(sender);
        AddLogToFile(path, _systemClock, "Data Send Request Reject from Link");


        RequestData requestData;
        int waitTime = 0;

        if (request->data.state == REQUEST_STATE_TYPE::SEND_REQUEST)
        {
            // back-off �˰���
            waitTime = BackoffTimer(_nodeArr.size());
            requestData.Set(_systemClock, REQUEST_STATE_TYPE::REQUEST_FAILED, waitTime, 1);
        }
        else if (request->data.state == REQUEST_STATE_TYPE::RETRY_SEND_REQUEST)
        {
            // back-off �˰���
            waitTime = BackoffTimer(_nodeArr.size() + request->data.count);
            requestData.Set(_systemClock, REQUEST_STATE_TYPE::REQUEST_FAILED, waitTime, request->data.count + 1);
        }

        std::string nodeName = "Node" + std::to_string(sender);
        AddLogToFile(nodeName, _systemClock, "Exponential Back-off Time: " + std::to_string(waitTime) + " msec");

        Request* requestToSender = new Request(0, sender, requestData);
        NodeComputer* senderNode = GetNode(sender);
        senderNode->GetRequest(requestToSender);

        // ��ũ���� �󸶳� ��ٷ����ϴ��� �˸�
        RequestData transmitData(_systemClock, REQUEST_STATE_TYPE::WAIT_REQUEST, waitTime, 0);
        Request* requestToLink = new Request(sender, receiver, transmitData);
        GetRequest(requestToLink);
    }
}
bool LinkBus::ProcessWait(const Request* request)
{
    int destTime = (request->data.sendTime + request->data.waitTime);
    if (_systemClock >= destTime)
    {
        if (AlramWaitFinished(request))
            return true;
        else
            return false;
    }
    else
    {
        return false;
    }
}
bool LinkBus::CollisionCheck(int receiver)
{
    // �� ��带 ����
    NodeComputer* receiverNode = GetNode(receiver);

    if (_isValid && receiverNode->_state != NODE_STATE::TRANSMITING)
    {
        _isValid = false;
        return true;
    }
    else
    {
        return false;
    }
}

bool LinkBus::AlramWaitFinished(const Request* request)
{
    int sender = request->sender;
    int receiver = request->receiver;

    std::string path;

    NodeComputer* senderNode = GetNode(sender);
    NodeComputer* receiverNode = GetNode(receiver);

    RequestData requestData;

    switch (request->data.state)
    {
        case REQUEST_STATE_TYPE::REQUEST_SUCCESS: // ���� �Ϸ�
        {
            // from ���� �˸�
            path = "Node" + std::to_string(sender);
            AddLogToFile(path, _systemClock, "Data Send Finished To Node" + std::to_string(receiver));

            // dest ���� �˸�
            path = "Node" + std::to_string(receiver);
            AddLogToFile(path, _systemClock, "Data Receive Finished From Node" + std::to_string(sender));

            requestData.Set(_systemClock, REQUEST_STATE_TYPE::TRANSMIT_FINISHED);
            Request* requestToSender = new Request(0, sender, requestData);
            Request* requestToReceiver = new Request(0, receiver, requestData);

            senderNode->GetRequest(requestToSender);
            receiverNode->GetRequest(requestToReceiver);

            AddLogToFile("Link", _systemClock, "Node" + std::to_string(sender) + " Data Send Finished To Node" + std::to_string(receiver));
            this->_isValid = true;
            break;
        }
        case REQUEST_STATE_TYPE::WAIT_REQUEST: // back-off ��ٸ� �Ϸ�
        {

            // �ٽ� ���� �� �ֵ��� ������ �ٽ� ��
            if (senderNode->_state != NODE_STATE::TRANSMITING)
            {
                requestData.Set(request->data.sendTime, REQUEST_STATE_TYPE::RETRY_SEND_REQUEST, request->data.waitTime, request->data.count);
                Request* newRequest = new Request(sender, receiver, requestData);
                this->GetRequest(newRequest);
            }
            else
            {
                return false;
            }
            break;
        }
    }

    return true;
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

    std::cerr << "��� �ε��� ���� : �ش��ϴ� id�� ���� ��尡 �����ϴ� - " << id << std::endl;
    return nullptr;
}

LinkBus::~LinkBus()
{
    AddLogToFile("Link", _systemClock, "System Clock Finished");
    AddLogToFile("Link", _systemClock, "Link Finished");
}