#pragma once
#include<string>
#include<vector>
#include<pthread.h>

const int NODE_NUM = 4;

namespace DC
{
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
        LinkBus* _linkedBus;
        bool _isActive;

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

        void GetRequest(int from, int dest);

        NodeComputer* GetNode(int id);
        bool CollisionCheck(int dest);
        int GetSystemClock() { return _systemClock; }
        bool IsActive() { return _isActive; }
        ~LinkBus();
    };
}
