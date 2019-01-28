
#include "ProcessTask.h"
#include <vector>

using namespace std;

namespace storagemanager
{

ProcessTask::ProcessTask(int _sock, uint _length) : sock(_sock), length(_length)
{
    assert(length > 0);
}

ProcessTask::~ProcessTask()
{
}

void ProcessTask::handleError(int saved_errno)
{
    // return sock to SessionManager
    char buf[80];
    cout << "ProcessTask: got an error during a socket read: " << strerror_r(saved_errno, buf, 80) << endl;
}

void ProcessTask::operator()()
{
    /*
        Read the command from the socket
        Create the appropriate PosixTask
        Run it
    */
    vector<uint8_t> msg;
    int err;
    uint8_t opcode;
    
    /* D'oh, a 1-byte read....   TODO! */
    err = ::read(sock, &opcode, 1);
    if (err <= 0)
    {
        handleError(errno);
        return;
    }
        
    PosixTask *task;
    switch(opcode)
    {
        case OPEN:
            task = new OpenTask(sock, length);
            break;
        case READ:
            task = new ReadTask(sock, length);
            break;
        case WRITE:
            task = new WriteTask(sock, length);
            break;
        case STAT:
            task = new StatTask(sock, length);
            break;
        case UNLINK:
            task  = new UnlinkTask(sock, length);
            break;
        case APPEND:
            task = new AppendTask(sock, length);
            break;
        case TRUNCATE:
            task = new TruncateTask(sock, length);
            break;
        case LIST_DIRECTORY:
            task = new ListDirectoryTask(sock, length);
            break;
        case PING:
            task = new PingTask(sock, length);
            break;
        default:
            throw runtime_error("ProcessTask: got an unknown opcode");
    }
    task->run();
}


}
