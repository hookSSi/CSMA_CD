#pragma once
#include<string>
#include<vector>
#include<queue>

const int NODE_NUM = 4;

namespace DC
{
	enum class REQUEST_TYPE
	{
		SEND_REQUEST,
		RETRY_REQUEST
	};

	struct Request
	{
		int from;
		int dest;
		REQUEST_TYPE type;
		int count;

		Request(int p_from, int p_dest, REQUEST_TYPE p_type, int p_count) : from(p_from), dest(p_dest), type(p_type), count(p_count) {}
		Request(int p_from, int p_dest) { Request(p_from, p_dest, REQUEST_TYPE::SEND_REQUEST, 0); }
	};

	enum class RESULT_TYPE
	{
		SUCCESS,
		FAILED
	};

	struct Result
	{
		int from;
		int dest;
		REQUEST_TYPE requestType;
		int count;
		int waitTime;
		RESULT_TYPE result;

		Result(int p_from, int p_dest, REQUEST_TYPE p_requestType, int p_count, RESULT_TYPE p_result) : from(p_from), dest(p_dest), requestType(p_requestType), count(p_count), result(p_result) {}
		void SetWaitTime(int p_waitTime) { waitTime = p_waitTime; }
	};

    struct MessageData
    {
        int t;
        std::string logData;

        MessageData(int p_t, std::string p_logData) : t(p_t), logData(p_logData) {}
        void Set(int p_t, std::string p_logData) { t = p_t; logData = p_logData; }
    };

    class NodeComputer;
    class LinkBus;

    enum class NODE_STATE
    {
        WAITING,
        NORMAL,
        RETRY
    };

    class NodeComputer
    {
    private:
        int _id;
		int _transmitTime = 5;
        LinkBus* _linkedBus;
        bool _isActive;

		std::queue<Result*> resultQueue; // 메모리 해제해야함?

        int _prevTime;
        int _waitTime;
        int _savedDest;
        std::string _msg;
    public:
        NODE_STATE _state;

        NodeComputer(int p_id, LinkBus* p_linkBus) : _id(p_id), _linkedBus(p_linkBus)
        {
            _isActive = true;
            _savedDest = 0;
            _prevTime = 0;
            _waitTime = 0;
            _state = NODE_STATE::NORMAL;
        }
        void Init();
        void Update();
        void Wait(int t, int range, std::string msg);
        void Wait(int t, int range, int dest);

        void RandomSomething(int probability, void (NodeComputer::*func)(int));
        void SendRequest(int dest);
		void GetResult(Result* result);
		void ProcessResult(const Result* result);
        bool IsActive() { return _isActive; }
        int GetId() { return _id; }
        ~NodeComputer();
    };

    class LinkBus
    {
    private:
        int _systemClock;
        int _timeLimit;
        bool _isActive;
        bool _isValid;

		std::queue<Request*> requestQueue;

        int _prevTime;
        int _waitTime;
        std::string _msg;
    public:
        std::vector<NodeComputer*> _nodeArr;

        LinkBus(int p_timeLimit) : _timeLimit(p_timeLimit) 
        { 
            _prevTime = 0;
            _waitTime = 0;
            _systemClock = 0; 
            _isActive = true; 
            _isValid = true;
        }
        void Init();
        void Update();

        void GetRequest(Request* request);
		Result* ProcessRequest(const Request* request);
		void SendResult(Result* result);

        NodeComputer* GetNode(int id);
        bool CollisionCheck(int dest);
        int GetSystemClock() { return _systemClock; }
        bool IsActive() { return _isActive; }
        ~LinkBus();
    };
}
