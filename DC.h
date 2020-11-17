#pragma once
#include<string>
#include<vector>
#include<queue>

namespace DC
{
	enum class REQUEST_STATE_TYPE
	{
		SEND_REQUEST, // ��ũ���� ���� ��û
		RETRY_SEND_REQUEST, // ��ũ���� ������ ��û
        WAIT_REQUEST, // ��ũ�� ��ٷ����ϴ� ���
        REQUEST_SUCCESS, // ����� ��û�� ��������
        REQUEST_FAILED, // ����� ��û�� ����
        TRANSMIT_FINISHED // ����� ������ ����
	};

    struct RequestData
    {
        int sendTime; // ������ �ð�
        REQUEST_STATE_TYPE state;
        int waitTime; // ��ٷ����Ѵٸ� ��ٸ��� �ð�
        int count; // back-off �˰����� ������ Ƚ��

        RequestData(int p_sendTime, REQUEST_STATE_TYPE p_state, int p_watiTIme, int p_count) : sendTime(p_sendTime), state(p_state), waitTime(p_watiTIme), count(p_count) {}
        RequestData(int p_sendTime, REQUEST_STATE_TYPE p_state) : RequestData(p_sendTime, p_state, 0, 0) {}
        RequestData(const RequestData& other) : RequestData(other.sendTime, other.state, other.waitTime, other.count) {}
        RequestData() : RequestData(0, REQUEST_STATE_TYPE::WAIT_REQUEST, 0, 0) {}
        void Set(int p_sendTime, REQUEST_STATE_TYPE p_state, int p_watiTIme, int p_count)
        {
            sendTime = p_sendTime;
            state = p_state;
            waitTime = p_watiTIme;
            count = p_count;
        }
        void Set(int p_sendTime, REQUEST_STATE_TYPE p_state)
        {
            this->Set(p_sendTime, p_state, 0, 0);
        }
        void Set(const RequestData& other)
        {
            sendTime = other.sendTime;
            state = other.state;
            waitTime = other.waitTime;
            count = other.count;
        }
    };

    struct Request
    {
        int sender;
        int receiver;
        RequestData data;

        Request(int p_sender, int p_receiver, RequestData p_data) : sender(p_sender), receiver(p_receiver), data(p_data){}
        Request(const Request& other) : Request(other.sender, other.receiver, other.data) {}
        Request() : Request(-1, -1, RequestData(0, REQUEST_STATE_TYPE::WAIT_REQUEST, 0, 0)) {}
    };

    class Object
    {
    protected:
        bool _isActive;

        std::queue<Request*> requestQueue;
        virtual void Update();
        virtual bool ProcessRequest(const Request* request) = 0;
        Object() { _isActive = true; }
        virtual ~Object()
        {
            while (!this->requestQueue.empty())
            {
                // Result ó��
                Request* request = this->requestQueue.front();
                this->requestQueue.pop();
                delete(request); // �޸� ����
            }
        }
    public:
        virtual void GetRequest(Request* request) 
        { 
            if (request != nullptr) 
                this->requestQueue.push(request); 
        }
    };

    class NodeComputer;
    class LinkBus;

    enum class NODE_STATE
    {
        WAITING,
        TRANSMITING,
        NORMAL
    };

    class NodeComputer : public Object
    {
    private:
        int _id;
		int _transmitTime = 5;
        LinkBus* _linkedBus;

        std::queue<Request*> requestQueue;
    public:
        NODE_STATE _state;

        NodeComputer(int p_id, LinkBus* p_linkBus) : _id(p_id), _linkedBus(p_linkBus), Object()
        {
            _state = NODE_STATE::NORMAL;
        }
        void Init();

        virtual void Update();
		virtual bool ProcessRequest(const Request* request);

        void SendRequest(int receiver);
        void RandomSomething(int probability, void (NodeComputer::*func)(int));
        bool IsQueueEmpty(){ return requestQueue.empty(); }
        bool IsActive() { return _isActive; }
        int GetId() { return _id; }
        int GetTransmitTime() { return _transmitTime; }
        ~NodeComputer();
    };

    class LinkBus : public Object
    {
    private:
        int _systemClock;
        int _timeLimit;
        bool _isValid;
    public:
        std::vector<NodeComputer*> _nodeArr;

        LinkBus(int p_timeLimit) : _timeLimit(p_timeLimit) , Object()
        { 
            _systemClock = 0;  
            _isValid = true;
        }
        void Init();

        virtual void Update();
        virtual bool ProcessRequest(const Request* request);

        void ProcessSenderRequest(const Request* request);
        bool ProcessWait(const Request* request);
        bool CollisionCheck(int dest);
        bool AlramWaitFinished(const Request* request);

        NodeComputer* GetNode(int id);
        int GetSystemClock() { return _systemClock; }
        bool IsActive() { return _isActive; }
        virtual ~LinkBus();
    };
}
