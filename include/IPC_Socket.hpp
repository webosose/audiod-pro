// Copyright (c) 2012-2020 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#include "IPC_Socket.h"
#include "log.h"

#include <cerrno>

#define DO_DEBUG_SOCKETS 0

#if DO_DEBUG_SOCKETS
    #define DEBUG_SOCKETS(...) PM_LOG_INFO(MSGID_DEBUG_SOCKETS, INIT_KVCOUNT, ##__VA_ARGS__)
    #define DEBUG_SOCKETS_W(...) PM_LOG_WARNING(MSGID_DEBUG_SOCKETS, INIT_KVCOUNT, ##__VA_ARGS__)
    #define DEBUG_SOCKETS_M(...) PM_LOG_INFO(MSGID_DEBUG_SOCKETS, INIT_KVCOUNT, ##__VA_ARGS__)
#else
    #define DEBUG_SOCKETS(...)
    #define DEBUG_SOCKETS_W(...)
    #define DEBUG_SOCKETS_M(...)
#endif

#define _NAME_STRUCT_OFFSET(struct_type, member) \
                     ((long) ((unsigned char*) &((struct_type*) 0)->member))

const gint32 cMagicSignature = 0x47656d42; // 4 bytes signature to add
                                           // confidence that we are receiving
                                           // the size of a message...

/*
 * header of each message sent, so that we can rebuild each
 * send message with the right amount of data...
 */
struct IPC_Header
{
    IPC_Header(ssize_t size = 0) : mMagic(cMagicSignature), mSize(size) {}

    gint32    mMagic;
    ssize_t    mSize;
};

IPC_Socket::IPC_Socket() : mFD(-1), mChannel(0), mSourceID(0), mCallbacks(0),
                           mName(""), mBuffer(0),
                           mBufferSize(cDefaultPacketSize), mBufferUsed(0),
                           mMessageSize(0),
                           mPacketsSizeMode(eSocketPacketsSize_Free)
{
}

IPC_Socket::~IPC_Socket()
{
    close(false);
}

void IPC_Socket::connectionEstablished()
{
    DEBUG_SOCKETS_M("IPC_Socket::connectionEstablished: %s/%d", mName, mFD);
    mBufferUsed = 0;
    if (mPacketsSizeMode == eSocketPacketsSize_Controlled)
        mMessageSize = 0;
    if (mCallbacks)
        mCallbacks->connectionEstablished(this);
}

void IPC_Socket::setPacketing(ESocketPacketsSize packetsSizeMode,
                              ssize_t packetSize)
{
    if (packetSize <= 0)
        packetSize = cDefaultPacketSize;
    mPacketsSizeMode = packetsSizeMode;
    if (mPacketsSizeMode == eSocketPacketsSize_Controlled)
    {
        mBufferSize = packetSize;
        mMessageSize = 0;
    }
    else
        mMessageSize = mBufferSize = packetSize;
    // reset current buffers
    delete[] mBuffer;
    mBuffer = 0;
    mBufferUsed = 0;
}

bool IPC_Socket::receiveData()
{
    DEBUG_SOCKETS("IPC_Socket::receiveData: %s/%d", mName, mFD);
    if (mPacketsSizeMode == eSocketPacketsSize_Controlled && mMessageSize == 0)
    {
        IPC_Header    header;

        ssize_t bytes = ::recv(mFD, &header, sizeof(IPC_Header), 0);

        if (bytes == 0)    // connection is being closed.
            return true;

        if (bytes < 0)
            return receiveError();

        if (!VERIFY(bytes == sizeof(IPC_Header)) ||
            !VERIFY(header.mSize > 0) ||
            !VERIFY(header.mMagic == cMagicSignature))
        {
            DEBUG_SOCKETS_W("IPC_Socket::receiveData: invalid header  \
                          data on %s/%d. Shutting down socket...", mName, mFD);
            shutdown();
            return false;
        }

        mMessageSize = header.mSize;
        mBufferUsed = 0;
    }
    if (mMessageSize > mBufferSize)
    {
        delete[] mBuffer;
        mBuffer = 0;
        mBufferSize = mMessageSize;
        VERIFY(mBufferUsed == 0);
    }
    if (mBuffer == 0)
        mBuffer = new char[mBufferSize];

    ssize_t bytes = ::recv(mFD, mBuffer + mBufferUsed,
                                mMessageSize - mBufferUsed, 0);
    mBufferUsed += bytes;

    DEBUG_SOCKETS("IPC_Socket::receiveData: %d bytes payload on '%s/%d'.", bytes, mName, mFD);

    if (bytes == 0)    // connection is being closed.
        return true;

    if (bytes < 0 || (mPacketsSizeMode == eSocketPacketsSize_Fixed &&
                      bytes != mMessageSize))
        return receiveError();

    if (!VERIFY(mBufferUsed <= mMessageSize)) // should never happen: we got
                                              // more bytes than we asked for
                                              // an overrun our buffer... :-(
    {
        shutdown();
        return false;
    }
    if (mPacketsSizeMode == eSocketPacketsSize_Free ||
        mBufferUsed == mMessageSize)
    {
        if (mCallbacks)
            mCallbacks->dataReceived(mBuffer, mBufferUsed);
        mBufferUsed = 0;
        if (mPacketsSizeMode == eSocketPacketsSize_Controlled)
            mMessageSize = 0;
    }
    return true;
}

bool IPC_Socket::receiveError()
{
    if (errno == ECONNRESET)
    {
        mBufferUsed = 0;    // don't close the connection...
        if (mPacketsSizeMode == eSocketPacketsSize_Controlled)
            mMessageSize = 0;
    }
    else
    {
        DEBUG_SOCKETS_W("IPC_Socket::receiveError: error on '%s/%d'.  \
                                        Shutting down socket...", mName, mFD);
        shutdown();
    }

    return false;
}

bool IPC_Socket::send(const void * data,
                      ssize_t size,
                      const void * data2,
                      ssize_t size2)
{
    if (!isConnected())
    {
        PM_LOG_INFO(MSGID_DEBUG_SOCKETS, INIT_KVCOUNT,
            "IPC_SocketServer::send: socket '%s/%d' not connected.",mName, mFD);
        return false;
    }

    if (mPacketsSizeMode == eSocketPacketsSize_Controlled)
    {
        IPC_Header    header(size + size2);
        if (::send(mFD, &header, sizeof(header), MSG_DONTWAIT) != sizeof(header))
        {
            PM_LOG_INFO(MSGID_DEBUG_SOCKETS, INIT_KVCOUNT,
                "IPC_SocketServer::send: error '%s' on socket '%s/%d'.",
                strerror(errno), mName, mFD);
            return false;
        }
    }
    else if (mPacketsSizeMode == eSocketPacketsSize_Fixed)
    {
        if (size != mMessageSize)
        {
            PM_LOG_INFO(MSGID_DEBUG_SOCKETS, INIT_KVCOUNT,
                "IPC_SocketServer::send: incorrect packet size  \
                (%d instead of %d) on socket '%s/%d'.",
                size, mMessageSize, mName, mFD);
            return false;
        }
    }
    if (::send(mFD, data, size, MSG_DONTWAIT) != size ||
              (size2 > 0 && data2 &&
              ::send(mFD, data2, size2, MSG_DONTWAIT) != size2))
    {
        PM_LOG_INFO(MSGID_DEBUG_SOCKETS, INIT_KVCOUNT,
            "IPC_SocketServer::send: error '%s' on socket '%s/%d'.",
            strerror(errno), mName, mFD);
        return false;
    }
    DEBUG_SOCKETS("IPC_Socket::send: sent %d bytes on '%s/%d'",
                                                   size + size2, mName, mFD);
    return true;
}

void IPC_Socket::shutdown()
{
    if (mFD != -1)
    {
        ::shutdown(mFD, SHUT_RDWR);
        ::close(mFD);
        mFD = -1;
    }
}

void IPC_Socket::close(bool flush)
{
    if (mChannel)
    {
        g_io_channel_shutdown(mChannel, flush ? TRUE : FALSE, NULL);
        g_source_remove(mSourceID);
        g_io_channel_unref(mChannel);
        mChannel = 0;
        mSourceID = 0;
        if (mCallbacks)
            mCallbacks->closed(this);
    }
    shutdown();
    delete[] mBuffer;
    mBuffer = 0;
    mBufferUsed = 0;
}

gboolean IPC_SocketClient::socketStatusCallback(GIOChannel * ch,
                                                GIOCondition condition,
                                                gpointer user_data)
{
    IPC_SocketClient * socket = reinterpret_cast<IPC_SocketClient *> (user_data);

    if (VERIFY(socket))
        socket->statusCallback(ch, condition);

    return TRUE;
}

gboolean IPC_SocketClient::socketConnectTimerCallback(gpointer user_data)
{
    IPC_SocketClient * socket = reinterpret_cast<IPC_SocketClient *> (user_data);

    if (VERIFY(socket))
        socket->tryAndRetryToConnect();

    return FALSE;
}

IPC_SocketClient::IPC_SocketClient() : mTimeout(10),
                                       mConnectAttempt(0),
                                       mAutoConnect(false)
{
    *socketName() = 0;
    mSocket.mName = socketName();
}

bool IPC_SocketClient::connect(const char * name,
                               bool autoConnect,
                               ESocketPacketsSize packetsSizeMode,
                               ssize_t packetSize)
{
    mSocket.setPacketing(packetsSizeMode, packetSize);
    mAutoConnect = autoConnect;
    ::memset(&mAddressName, 0, sizeof(mAddressName));
    mAddressName.sun_family = AF_UNIX;
    mAddressName.sun_path[0] = 0; /* this is what says "use abstract" */

    size_t length = ::strlen(name) + 1;
    size_t maxLength = G_N_ELEMENTS(mAddressName.sun_path) - 1;

    if (length > maxLength)
    {
        PM_LOG_INFO(MSGID_DEBUG_SOCKETS, INIT_KVCOUNT,
            "IPC_SocketClient::IPC_SocketClient: socket name '%s'  \
            is too long (%i > %i),", name, length, maxLength);
        return false;
    }

    ::strncpy(&mAddressName.sun_path[1], name, length);

    return mAutoConnect ? tryAndRetryToConnect() : tryToConnectOnce();
}

bool IPC_SocketClient::tryAndRetryToConnect()
{
    if (*socketName() == 0)
        return false;

    bool connected = tryToConnectOnce();
    if (!connected)
    {
        guint timeout = mTimeout + guint(rand()) % (4 * mTimeout); // random time between mTimeout & 5x mTimeout
        g_timeout_add(timeout, IPC_SocketClient::socketConnectTimerCallback, this);
        const guint cMaxTimeout = 1000;
        if (mTimeout < cMaxTimeout)
            mTimeout *= 3;
        if (mTimeout > cMaxTimeout)
            mTimeout = cMaxTimeout;
    }
    return connected;
}

bool IPC_SocketClient::tryToConnectOnce()
{
    if (isConnected())
        return true;

    ++mConnectAttempt;

    if (-1 == (mSocket.mFD = ::socket(AF_UNIX, SOCK_STREAM, 0)))
    {
        PM_LOG_INFO(MSGID_DEBUG_SOCKETS, INIT_KVCOUNT,
            "IPC_SocketClient::connect: error '%s' on socket '%s/%d'.",
            strerror(errno), socketName(), mSocket.mFD);
        return false;
    }

    if (-1 == ::connect(mSocket.mFD,
                       (struct sockaddr *) &mAddressName,
                        _NAME_STRUCT_OFFSET (struct sockaddr_un, sun_path) +
                        ::strlen(socketName()) + 1))
    {
        DEBUG_SOCKETS("IPC_SocketClient::connect: connect error '%s'  \
                                on socket '%s/%d'.",
                                  strerror(errno), socketName(), mSocket.mFD);
        if (::close(mSocket.mFD))
        {
            PM_LOG_INFO(MSGID_DEBUG_SOCKETS, INIT_KVCOUNT,
                "IPC_SocketClient::connect: close error '%s'   \
                on socket '%s/%d'.",
                strerror(errno), socketName(), mSocket.mFD);
        }
        mSocket.mFD = -1;
        return false;
    }

    mSocket.mChannel = g_io_channel_unix_new(mSocket.mFD);
    mSocket.mSourceID = g_io_add_watch(mSocket.mChannel,
                                       GIOCondition(G_IO_ERR |
                                                    G_IO_HUP |
                                                    G_IO_IN),
                                       IPC_SocketClient::socketStatusCallback,
                                       this);

    PM_LOG_INFO(MSGID_DEBUG_SOCKETS, INIT_KVCOUNT,
        "IPC_SocketClient::connect: successfully connected socket  \
        '%s/%d' on attempt #%i.",
        socketName(), mSocket.mFD, mConnectAttempt);
    mSocket.connectionEstablished();
    mConnectAttempt = 0;

    return true;
}

bool IPC_SocketClient::send(const void * data,
                            ssize_t size,
                            const void * data2,
                            ssize_t size2)
{
    bool sent = mSocket.send(data, size, data2, size2);
    return sent;
}

void IPC_SocketClient::close(bool flush)
{
    mSocket.close(flush);
    mTimeout = 10;
}

void IPC_SocketClient::statusCallback(GIOChannel *ch, GIOCondition condition)
{
    DEBUG_SOCKETS("IPC_SocketClient::statusCallback: %d", int(condition));
    if (!VERIFY(ch == mSocket.mChannel))
        return;

    if (condition & G_IO_IN)
    {
        if (!mSocket.receiveData())
        {
            PM_LOG_INFO(MSGID_DEBUG_SOCKETS, INIT_KVCOUNT,
                "IPC_Socket::statusCallback: receive error on socket  \
                '%s/%d' (%s).", socketName(), mSocket.mFD, strerror(errno));
        }
    }

    if (condition & G_IO_ERR)
    {
        PM_LOG_INFO(MSGID_DEBUG_SOCKETS, INIT_KVCOUNT,
            "IPC_Socket::statusCallback: error condition on socket  \
            '%s/%d'.", socketName(), mSocket.mFD);
    }

    if (condition & G_IO_HUP)
    {
        PM_LOG_INFO(MSGID_DEBUG_SOCKETS, INIT_KVCOUNT,
            "IPC_Socket::statusCallback: socket '%s/%d'  \
            hung up.", socketName(), mSocket.mFD);
        mSocket.close(false);
        if (mAutoConnect)
            tryAndRetryToConnect();
    }
}


IPC_SocketServer::IPC_SocketServer() : mCallbacks(0),
                                       mSocketCallbacks(0),
                                       mMaxConnectionsCount(64)
{
    *socketName() = 0;
}

IPC_SocketServer::~IPC_SocketServer()
{
    closeAll();
}

void IPC_SocketServer::setMaxConnectionsCount(size_t maxConnectionsCount)
{
    mMaxConnectionsCount = maxConnectionsCount;
}

void IPC_SocketServer::closeAll()
{
    mConnections.clear();
    mSocket.close(false);
    *socketName() = 0;
}

bool IPC_SocketServer::listen(const char * name,
                              ESocketPacketsSize packetsSizeMode,
                              ssize_t packetSize)
{
    mSocket.setPacketing(packetsSizeMode, packetSize);// only used to remember
                                                     // settings for
                                                     // new connections

    /* create a socket for ipc with audiod the policy manager */
    ::memset(&mAddressName, 0, sizeof(mAddressName));
    mAddressName.sun_family = AF_UNIX;
    mAddressName.sun_path[0] = 0; /* this is what says "use abstract" */

    size_t nameLength = ::strlen(name) + 1;
    size_t maxLength = G_N_ELEMENTS(mAddressName.sun_path) - 1;

    if (nameLength > maxLength)
    {
        PM_LOG_INFO(MSGID_DEBUG_SOCKETS, INIT_KVCOUNT,
            "IPC_SocketServer::create: name too long '%s'.", name);
        return false;
    }

    ::strncpy(socketName(), name, nameLength);

    /* build the socket */
    if (-1 == (mSocket.mFD = socket(AF_UNIX, SOCK_STREAM, 0)))
    {
        PM_LOG_INFO(MSGID_DEBUG_SOCKETS, INIT_KVCOUNT,
            "IPC_SocketServer::create: error creating socket   \
            '%s': %s ", name, strerror(errno));
        return false;
    }

    /* bind it to a name */
    if (-1 == ::bind(mSocket.mFD, (struct sockaddr*) &(mAddressName),
                                    _NAME_STRUCT_OFFSET (struct sockaddr_un,
                                                      sun_path) + nameLength))
    {
        PM_LOG_INFO(MSGID_DEBUG_SOCKETS, INIT_KVCOUNT,
            "IPC_SocketServer::create: error binding socket   \
            '%s/%d': %s ", name, mSocket.mFD, strerror(errno));
        ::close(mSocket.mFD);
        mSocket.mFD = -1;
        return false;
    }

    if (-1 == ::listen(mSocket.mFD, 5))
    {
        PM_LOG_INFO(MSGID_DEBUG_SOCKETS, INIT_KVCOUNT,
            "IPC_SocketServer::create: error listening to socket   \
            '%s/%d': %s ", name, mSocket.mFD, strerror(errno));
        ::close(mSocket.mFD);
        mSocket.mFD = -1;
        return false;
    }

    mSocket.mChannel = g_io_channel_unix_new(mSocket.mFD);
    mSocket.mSourceID = g_io_add_watch(mSocket.mChannel,
                                       GIOCondition(G_IO_ERR |
                                                    G_IO_HUP |
                                                    G_IO_IN),
                                       IPC_SocketServer::socketListenCallback,
                                       this);

    return true;
}

gboolean IPC_SocketServer::socketListenCallback(GIOChannel * ch,
                                                GIOCondition condition,
                                                gpointer user_data)
{
    IPC_SocketServer * socket = reinterpret_cast<IPC_SocketServer *> (user_data);

    if (VERIFY(socket))
        socket->listenCallback(ch, condition);

    return TRUE;
}

void IPC_SocketServer::listenCallback(GIOChannel * ch, GIOCondition condition)
{
    if (!VERIFY(ch == mSocket.mChannel))
        return;

    if (condition & G_IO_IN)
    {
        if (mConnections.size() < mMaxConnectionsCount)
        {
            int    fd = -1;
            size_t nameLength = ::strlen(socketName()) + 1;
            socklen_t len = _NAME_STRUCT_OFFSET
                                   (struct sockaddr_un, sun_path) + nameLength;
            if (-1 == (fd = ::accept(mSocket.mFD,
                                   (struct sockaddr*) &mAddressName, &len)))
            {
                PM_LOG_INFO(MSGID_DEBUG_SOCKETS, INIT_KVCOUNT,
                    "IPC_SocketServer::listenCallback: could not   \
                    create new connection on socket: '%s/%d' (%s)",
                    socketName(), mSocket.mFD, strerror(errno));
            }
            else
            {
                PM_LOG_INFO(MSGID_DEBUG_SOCKETS, INIT_KVCOUNT,
                    "IPC_SocketServer::listenCallback: created   \
                    connection '%s/%d' on socket '%s/%d'",
                    socketName(), fd, socketName(), mSocket.mFD);
                GIOChannel * channel = g_io_channel_unix_new(fd);
                IPC_Socket & socket = mConnections[channel];
                socket.mName = socketName();
                socket.mFD = fd;
                socket.mChannel = channel;
                socket.mSourceID = g_io_add_watch(channel,
                                                  GIOCondition(G_IO_ERR |
                                                               G_IO_HUP |
                                                               G_IO_IN),
                                                 IPC_SocketServer::socketConnectionCallback,
                                                  this);
                socket.setPacketing(mSocket.mPacketsSizeMode,
                                    mSocket.mBufferSize);
                if (mSocketCallbacks)
                    socket.setCallbacks(mSocketCallbacks);
                if (mCallbacks)
                    mCallbacks->newConnection(socket);
                socket.connectionEstablished();
            }
        }
        else
        {
            PM_LOG_INFO(MSGID_DEBUG_SOCKETS, INIT_KVCOUNT,
                "IPC_SocketServer::listenCallback: declined new   \
                connection on '%s/%d': too many connections (%u)!...",
                socketName(), mSocket.mFD, mConnections.size());
        }
    }

    if (condition & G_IO_ERR)
    {
        PM_LOG_INFO(MSGID_DEBUG_SOCKETS, INIT_KVCOUNT,
            "IPC_SocketServer::listenCallback: error condition   \
            on socket '%s/%d'.", socketName(), mSocket.mFD);
    }

    if (condition & G_IO_HUP)
    {
        PM_LOG_INFO(MSGID_DEBUG_SOCKETS, INIT_KVCOUNT,
            "IPC_SocketServer::listenCallback: socket   \
            '%s/%d' hung up.", socketName(), mSocket.mFD);
        this->closeAll();
    }
}

gboolean IPC_SocketServer::socketConnectionCallback(GIOChannel * ch,
                                                    GIOCondition condition,
                                                    gpointer user_data)
{
    IPC_SocketServer * socket = reinterpret_cast<IPC_SocketServer *> (user_data);

    if (VERIFY(socket))
        socket->connectionCallback(ch, condition);

    return TRUE;
}

void IPC_SocketServer::connectionCallback(GIOChannel * ch,
                                          GIOCondition condition)
{
    SocketMap::iterator iter = mConnections.find(ch);
    if (!VERIFY(iter != mConnections.end()))
        return;

    IPC_Socket & connection = iter->second;

    if (condition & G_IO_IN)
    {
        if (!connection.receiveData())
            PM_LOG_INFO(MSGID_DEBUG_SOCKETS, INIT_KVCOUNT,
                "IPC_SocketServer::socketConnectionCallback:   \
                receive error on socket '%s/%d' (%s).",
                connection.mName, connection.mFD,
                strerror(errno));
    }

    if (condition & G_IO_HUP)
    {
        PM_LOG_INFO(MSGID_DEBUG_SOCKETS, INIT_KVCOUNT,
            "IPC_SocketServer::socketConnectionCallback:   \
            socket '%s/%d' hung up.", connection.mName, connection.mFD);
        mConnections.erase(iter);
    }
    else if (condition & G_IO_ERR)
    {
        PM_LOG_INFO(MSGID_DEBUG_SOCKETS, INIT_KVCOUNT,
            "IPC_SocketServer::socketConnectionCallback: error   \
            condition on socket '%s/%d'. Closing it...",
            connection.mName, connection.mFD);
        mConnections.erase(iter);
    }
}
