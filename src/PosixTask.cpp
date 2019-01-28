
#include "PosixTask.h"
#include <sys/types.h>
#include <sys/socket.h>

namespace storagemanager
{

PosixTask::PosixTask(int _sock, uint _length) : 
    sock(_sock),
    totalLength(_length),
    remainingLengthInStream(_length),
    remainingLengthForCaller(_length),
    bufferPos(0),
    bufferLen(0),
    socketReturned(false)
{
}

PosixTask::~PosixTask()
{
    if (!socketReturned)
        returnSocket();
}

void PosixTask::handleError(const char *name, int errCode)
{
    char buf[80];
    
    // send an error response if possible
    int32_t *buf32;
    buf32[0] = SM_MSG_START;
    buf32[1] = 8;
    buf32[2] = -1;
    buf32[3] = errCode;
    write(buf32, 16);
    
    // TODO: construct and log a message
    cout << name << " caught an error reading from a socket: " << strerror_r(errCode, buf, 80) << endl;
    socketError();
}

void PosixTask::returnSocket()
{
    socketReturned = true;
}

void PosixTask::socketError()
{
    socketReturned = true;
}

uint PosixTask::getRemainingLength()
{
    return remainingLengthForCaller;
}

uint PosixTask::getLength()
{
    return totalLength;
}

bool PosixTask::read(uint8_t *buf, uint length)
{
    if (length > remainingLengthForCaller)
        length = remainingLengthForCaller;
    
    uint count = 0;
    int err;
    
    // copy data from the local buffer first.
    uint dataInBuffer = bufferLen - bufferPos;
    if (length <= dataInBuffer)
    {
        memcpy(buf, &localBuffer[bufferPos], length);
        count = length;
        bufferPos += length;
        remainingLengthForCaller -= length;
    }
    else if (dataInBuffer > 0)
    {
        memcpy(buf, &localBuffer[bufferPos], dataInBuffer);
        count = dataInBuffer;
        bufferPos += dataInBuffer;
        remainingLengthForCaller -= dataInBuffer;
    }
    
    // read the remaining requested amount from the stream.
    // ideally, combine the recv here with the recv below that refills the local
    // buffer.
    while (count < length)
    {
        err = ::recv(sock, &buf[count], length - count, 0);
        if (err <= 0)
            return false;

        count += err;
        remainingLengthInStream -= err;
        remainingLengthForCaller -= err;
    }
    
    /* The caller's request has been satisfied here.  If there is remaining data in the stream
    get what's available. */
    if (remainingLengthInStream > 0)
    {
        // Reset the buffer to allow a larger read.
        if (bufferLen == bufferPos)
        {
            bufferLen = 0;
            bufferPos = 0;
        }
        else if (bufferLen - bufferPos < 1024)  // if < 1024 in the buffer, move data to the front
        {
            memmove(buffer, &buffer[bufferPos], bufferLen - bufferPos);
            bufferLen -= bufferPos;
            bufferPos = 0;
        }
        
        uint toRead = min(remainingLengthInStream, bufferSize - bufferLen);
        err = ::recv(sock, &localBuffer[bufferLen], toRead, MSG_NOBLOCK);
        // ignoring errors here since the request has been satisfied successfully.
        // errors will be caught by the next read
        if (err > 0) 
        {
            bufferLen += err;
            remainingLengthInStream -= err;
        }
    }
    return true;
}

bool PosixTask::write(uint8_t *buf, uint len)
{
    int err;
    uint count = 0;
    
    while (count < len)
    {
        err = ::write(sock, &buf[count], len - count);
        if (err < 0)
            return false;
        count += err;
    }
    return true;
}

bool PosixTask::write(const vector<uint8_t> &buf)
{
    return write(&buf[0], buf.size());
}

}
