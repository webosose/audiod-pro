// Copyright (c) 2012-2018 LG Electronics, Inc.
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

#ifndef IPC_SOCKET_H_
#define IPC_SOCKET_H_

#include <sys/socket.h>
#include <sys/un.h>
#include <glib.h>

#include <map>

class IPC_Socket;

/*
 * Interface to hook connection events to behaviors
 */
class IPC_SocketCallbacks
{
public:
    virtual void    connectionEstablished(IPC_Socket * socket) = 0;
    virtual void    dataReceived(const void * data, ssize_t size) = 0;

    // Called when socket is closed. In the case of a server (which can have
    // multiple simultaneous connections), after this is called,
    // the socket object is deleted.
    virtual void    closed(IPC_Socket * socket) = 0;
};

enum ESocketPacketsSize
{
    eSocketPacketsSize_Free, // Variable size. Will pass the data as it comes
                             // in without buffering.
    eSocketPacketsSize_Fixed, // Sent packets always have the same size.
                              // Will be read using that same fixed size.
    eSocketPacketsSize_Controlled // Variable size, but ensure that each
                                  // message sent is received individually
                                  // once it has all been received.
};

/*
 * Heart of socket code shared by IPC_SocketClient & IPC_SocketServer
 */
struct IPC_Socket
{
    enum { cDefaultPacketSize = 256 };

    IPC_Socket(); // would be private, if the class wasn't used in an
                  //STL map... Do not create/delete directly!
    ~IPC_Socket();        // Let IPC_SocketClient & IPC_SocketServer manage
                         // these objects for you.

    /// Setup methods. Make sure packet mode &
    // packet size are the same on both sides!
    void setPacketing(ESocketPacketsSize packetsSizeMode, ssize_t packetSize);
    void setCallbacks(IPC_SocketCallbacks * callbacks) { mCallbacks = callbacks; }

    /// Is the socket connected?
    bool                isConnected() const         { return mFD != -1; }

    /// Send data. Will not disconnect/reconnect in case of error.
    bool send(const void * data,
              ssize_t size,
              const void * data2 = 0,
              ssize_t size2 = 0);

    /// Name of the server or client that created it.
    const char *        getName() const                { return mName; }

    /// Shutdown file descriptor, which will close the socket cleanly eventually
    void                shutdown();

protected:

    friend class IPC_SocketClient;
    friend class IPC_SocketServer;

    void                connectionEstablished();
    bool                receiveData();
    bool                receiveError();
    void           close(bool flush);        ///< direct close of everything

    int                 mFD;                 ///< file descriptor for socket
    GIOChannel *        mChannel;            ///< socket event handler
    guint               mSourceID;            ///< source ID for that channel
    IPC_SocketCallbacks * mCallbacks;         ///< callbacks for this socket

    const char *            mName;

    char *                    mBuffer;        ///< receive buffer allocated
    ssize_t                    mBufferSize;   ///< size allocated
    ssize_t                    mBufferUsed;   ///< size used during partial receives
    ssize_t                    mMessageSize;  ///< total message size expected.
                                              /// Will fill-up buffer until
                                              ///the size is reached
    ESocketPacketsSize        mPacketsSizeMode;    ///< How are packets
                                                   ///received & sent size-wise?
};

/*
 * Class to connect to an IPC_SocketServer. The client is responsible for
 * initiating the connection. Can be set to keep trying to connect/reconnect.
 */
class IPC_SocketClient
{
public:
    IPC_SocketClient();

    virtual ~IPC_SocketClient()        { close(true); }

    void setCallbacks(IPC_SocketCallbacks * callbacks)
                                         { mSocket.setCallbacks(callbacks); }

    bool            connect(const char * name,
                            bool autoConnect = true,
                            ESocketPacketsSize packetsSizeMode =
                                                eSocketPacketsSize_Controlled,
                             ssize_t packetSize = -1);
    bool            tryAndRetryToConnect();  ///< only allowed once you've
                                             // called connect once before!
    bool            isConnected() const   { return mSocket.isConnected(); }
    bool            send(const void * data,
                         ssize_t size,
                         const void * data2 = 0,
                         ssize_t size2 = 0);
    void            close(bool flush = true);

    const char *    getName() const     { return &mAddressName.sun_path[1]; }

private:
    static gboolean socketConnectTimerCallback(gpointer user_data);
    bool            tryToConnectOnce();
    char *            socketName()  { return &mAddressName.sun_path[1]; }

    static gboolean socketStatusCallback(GIOChannel * ch,
                                         GIOCondition condition,
                                         gpointer user_data);///< static that
                                                             // delegates to
                                                             // the next method
    void            statusCallback(GIOChannel * ch, GIOCondition condition);

    struct sockaddr_un        mAddressName;
    IPC_Socket                mSocket;
    guint                    mTimeout;
    int                        mConnectAttempt;
    bool                    mAutoConnect;
};

/*
 * Callback to know when a connection has been established
 */
class IPC_SocketServerCallbacks
{
public:
    virtual    void    newConnection(IPC_Socket & connection) = 0;
};

class IPC_SocketServer
{
    typedef std::map<GIOChannel *, IPC_Socket>    SocketMap;

public:
    IPC_SocketServer();
    virtual ~IPC_SocketServer();

    void            setMaxConnectionsCount(size_t maxConnectionsCount);
    void            setCallbacks(IPC_SocketServerCallbacks * callbacks)
                                                    { mCallbacks = callbacks; }
    void            setSocketCallbacks(IPC_SocketCallbacks * callbacks)
                                              { mSocketCallbacks = callbacks; }

    bool            listen(const char * name,
                           ESocketPacketsSize packetsSizeMode =
                                                 eSocketPacketsSize_Controlled,
                            ssize_t packetSize = -1);
    void            closeAll();

    const char *    getName() const      { return &mAddressName.sun_path[1]; }

private:
    char *            socketName()       { return &mAddressName.sun_path[1]; }

    static gboolean socketListenCallback(GIOChannel * ch,
                                         GIOCondition condition,
                                         gpointer user_data); ///< static that
                                                              // delegates to
                                                              // the next method
    void            listenCallback(GIOChannel * ch, GIOCondition condition);

    static gboolean socketConnectionCallback(GIOChannel * ch,
                                             GIOCondition condition,
                                             gpointer user_data);///< static
                                                                //that delegates
                                                                //to the next method
    void            connectionCallback(GIOChannel * ch, GIOCondition condition);

    struct sockaddr_un            mAddressName;

    IPC_SocketServerCallbacks *    mCallbacks;
    IPC_SocketCallbacks *        mSocketCallbacks;
    IPC_Socket  mSocket;    ///< the socket we use to
                            // listen for incoming connections
    SocketMap                    mConnections;
    size_t                        mMaxConnectionsCount;
};

#endif /* IPC_SOCKET_H_ */
