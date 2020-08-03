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

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/dynamic_bitset.hpp>

#include "IPC_Property.h"
#include "IPC_Socket.hpp"

#include <map>

struct IPC_MessageHeader        // make this fit in 32 bits
{
    IPC_PropertyID        mID;
    guint16                mOperation;
};

const ssize_t cBasicMessageSize = 16;

struct IPC_MessageWithData
{
    IPC_MessageHeader    mHeader;
    char                mData[cBasicMessageSize];
};

class IPC_MasterLink : public IPC_Link, public IPC_SocketServerCallbacks
{
    class Client : public IPC_SocketCallbacks
    {
    public:
        Client() : mSocket(0), mLink(0), mPropertyBeingChanged(cInvalidPropertyID) {}

        // IPC_SocketCallbacks methods
        void    connectionEstablished(IPC_Socket * socket)
        {
            DEBUG_SOCKETS("Client::connectionEstablished");
            mSocket = socket;
            if (VERIFY(mLink))
            {
                mProperties.resize(mLink->sharedProperties().getPropertyCount(), false);
            }
        }
        void    dataReceived(const void * data, ssize_t size)
        {
            DEBUG_SOCKETS("Client::dataReceived: %d bytes", size);
            if (VERIFY(mLink))
            {
                if (size == sizeof(IPC_MessageHeader))
                {
                    const IPC_MessageHeader *message =
                         reinterpret_cast<const IPC_MessageHeader *>(data);
                    EOperationRequest operation = EOperationRequest(message->mOperation);
                    if (operation == eOperationRequest_HandlesChangeNotifications)
                    {
                        mProperties.set(message->mID);
                        if (!mLink->sharedProperties().testPropertyFlag(message->mID,
                             IPC_PropertyBase::ePropertyFlag_NoNotificationOnConnect))
                            sendChangeNotification(message->mID);
                    }
                    else
                    {
                        mPropertyBeingChanged = message->mID;
                        mLink->sharedProperties().requestReceived(operation,
                                                        mPropertyBeingChanged);
                        mPropertyBeingChanged = cInvalidPropertyID;
                    }
                }
                else if (VERIFY(size > (ssize_t) sizeof(IPC_MessageHeader)))
                {
                    const IPC_MessageWithData * message =
                          reinterpret_cast<const IPC_MessageWithData *>(data);
                    mPropertyBeingChanged = message->mHeader.mID;
                    mLink->sharedProperties().requestReceived(
                                   EOperationRequest(message->mHeader.mOperation),
                                    mPropertyBeingChanged, message->mData,
                                     size - sizeof(IPC_MessageHeader));
                    mPropertyBeingChanged = cInvalidPropertyID;
                }
            }
        }
        void    closed(IPC_Socket * socket)
        {
            if (mLink)
                mLink->socketClosed(socket);
        }
        void    sendChangeNotification(IPC_PropertyID id)
        {
            if (VERIFY(mSocket && mLink) &&        // defensive programming
                    mProperties.test(id) && // client listens to this property
                    (id != mPropertyBeingChanged || // we are not processing a
                                                    // change request on
                                                    // this very property
                            !mLink->sharedProperties().testPropertyFlag(id,
                            IPC_PropertyBase::ePropertyFlag_DontNotifyClientsForOwnChanges)))
                mSocket->send(&id, sizeof(id));
        }

        typedef boost::dynamic_bitset<>    Bitset;

        IPC_Socket *        mSocket;
        IPC_MasterLink *    mLink;
        IPC_PropertyID        mPropertyBeingChanged;
        Bitset                mProperties;
    };

    class ClosedSocketsCollector
    {
    public:
        ClosedSocketsCollector(IPC_MasterLink & link) : mLink(link)
        {
            if (mLink.mClosedSocketCollector == 0)
                mLink.mClosedSocketCollector = this;
        }
        ~ClosedSocketsCollector()
        {
            if (mLink.mClosedSocketCollector == this)
            {
                for (std::vector<IPC_Socket *>::iterator iter = mClosedSockets.begin();
                                          iter != mClosedSockets.end(); ++iter)
                {
                    mLink.mClients.erase(*iter);
                }
                mLink.mClosedSocketCollector = 0;
            }
        }
        void socketClosed(IPC_Socket * socket)
        {
            mClosedSockets.push_back(socket);
        }
    private:
        IPC_MasterLink &            mLink;
        std::vector<IPC_Socket *>    mClosedSockets;
    };

public:
    IPC_MasterLink(IPC_SharedProperties & sharedProperties) :
                         IPC_Link(sharedProperties), mClosedSocketCollector(0)
    {
        sharedProperties.setLink(this);
        mSocketServer.setCallbacks(this);
        mSocketServer.listen(sharedProperties.getName(), eSocketPacketsSize_Controlled);
    }
    ~IPC_MasterLink() {}

    // IPC_Link methods
    bool    sendRequest(EOperationRequest operation, IPC_PropertyID id,
                                             const void * value, ssize_t size)
    {
        if (VERIFY(size > 0))
        {
            if (size <= cBasicMessageSize)
            {    // send in one transaction
                IPC_MessageWithData    message;
                message.mHeader.mID = id;
                message.mHeader.mOperation = operation;
                ::memcpy(message.mData, value, size);
                for (Clients::iterator iter = mClients.begin();
                                                 iter != mClients.end(); ++iter)
                {
                    iter->first->send(&message,
                        sizeof(IPC_MessageWithData) - cBasicMessageSize + size);
                }
            }
            else
            {    // send in two transactions
                IPC_MessageHeader    message;
                message.mID = id;
                message.mOperation = operation;
                for (Clients::iterator iter = mClients.begin();
                                                iter != mClients.end(); ++iter)
                {
                    iter->first->send(&message,
                                       sizeof(IPC_MessageHeader), value, size);
                }
            }
        }
        return false;
    }

    bool    sendRequest(EOperationRequest operation, IPC_PropertyID id)
    {
        SHOULD_NOT_REACH_HERE; // This method should be called only on IPC_SlaveLink
        return false;
    }

    void    sendChangeNotification(IPC_PropertyID id)
    {
        for (Clients::iterator iter = mClients.begin();
                                         iter != mClients.end(); ++iter)
        {
            iter->second.sendChangeNotification(id);
        }
    }

    IPC_SharedProperties & sharedProperties()    { return mSharedProperties; }

    void    socketClosed(IPC_Socket * socket)
    {
        if (mClosedSocketCollector)
            mClosedSocketCollector->socketClosed(socket);
        else
            mClients.erase(socket);
    }

    // IPC_SocketServerCallbacks methods
    void    newConnection(IPC_Socket & connection)
    {
        Client & client = mClients[&connection];
        client.mLink = this;
        connection.setCallbacks(&client);
        connection.setPacketing(eSocketPacketsSize_Controlled, 512);
    }

private:

    friend class ClosedSocketsCollector;
    typedef std::map<IPC_Socket *, Client>    Clients;

    IPC_SocketServer            mSocketServer;
    Clients                        mClients;
    ClosedSocketsCollector *    mClosedSocketCollector;
};

class IPC_SlaveLink : public IPC_Link, public IPC_SocketCallbacks
{
public:
    IPC_SlaveLink(IPC_SharedProperties & sharedProperties) :
                                      IPC_Link(sharedProperties), mSocket(0)
    {
        sharedProperties.setLink(this);
        mSocketClient.setCallbacks(this);
        mSocketClient.connect(sharedProperties.getName(),
                                      true,
                                      eSocketPacketsSize_Controlled);
    }
    ~IPC_SlaveLink() {}

    // IPC_Link methods

    bool    sendRequest(EOperationRequest operation,
                                     IPC_PropertyID id,
                                     const void * value,
                                     ssize_t size)
    {
        if (mSocket && VERIFY(size > 0))
        {
            if (size <= cBasicMessageSize)
            {    // send in one transaction
                IPC_MessageWithData    message;
                message.mHeader.mID = id;
                message.mHeader.mOperation = operation;
                ::memcpy(message.mData, value, size);
                return mSocket->send(&message, size + sizeof(IPC_MessageHeader));
            }
            else
            {    // send in two transactions
                return sendRequest(operation, id) && mSocket && mSocket->send(value, size);
            }
        }
        return false;
    }

    bool    sendRequest(EOperationRequest operation, IPC_PropertyID id)
    {
        if (mSocket)
        {
            IPC_MessageHeader    message;
            message.mID = id;
            message.mOperation = operation;
            return mSocket->send(&message, sizeof(IPC_MessageHeader));
        }
        return false;
    }

    void    sendChangeNotification(IPC_PropertyID id)
    {
        SHOULD_NOT_REACH_HERE;// This method should be called only on IPC_MasterLink
    }

    // IPC_SocketCallbacks methods
    void    connectionEstablished(IPC_Socket * socket)
    {
        DEBUG_SOCKETS("IPC_SlaveLink::connectionEstablished");
        mSocket = socket;
        if (mSharedProperties.attachSharedMemory())
        {
            if (mSocket)
                mSocket->setCallbacks(this);
        }
        else
            mSocket->shutdown();
    }
    void    dataReceived(const void * data, ssize_t size)
    {
        DEBUG_SOCKETS("IPC_SlaveLink::dataReceived: %d bytes", size);
        if (size == sizeof(IPC_PropertyID))
        {
            IPC_PropertyID id = *reinterpret_cast<const IPC_PropertyID *>(data);
            mSharedProperties.changeNotificationReceived(id);
        }
        else if (size == sizeof(IPC_MessageHeader))
        {
            const IPC_MessageHeader * message = reinterpret_cast<const IPC_MessageHeader *>(data);
            mSharedProperties.requestReceived(EOperationRequest(message->mOperation), message->mID);
        }
        else if (VERIFY(size > (ssize_t) sizeof(IPC_MessageHeader)))
        {
            const IPC_MessageWithData *    message = reinterpret_cast<const IPC_MessageWithData *>(data);
            mSharedProperties.requestReceived(EOperationRequest
                                             (message->mHeader.mOperation),
                                              message->mHeader.mID,
                                              message->mData,
                                              size - sizeof(IPC_MessageHeader));
        }
    }
    void    closed(IPC_Socket * socket)
    {
        if (VERIFY(socket == mSocket))
            mSocket = 0;
    }

private:
    IPC_SocketClient    mSocketClient;
    IPC_Socket *        mSocket;
};

IPC_PropertyBase::IPC_PropertyBase() : mSharedProperties(
                                          IPC_SharedProperties::sSharedPropertiesBeingBuilt),
                                          mID(-1), mPropertyFlags(0)
{
    if (VERIFY(mSharedProperties != 0))
    {
        mID = mSharedProperties->mProperties.size();
        mSharedProperties->mProperties.push_back(this);
    }
}

IPC_SharedProperties * IPC_SharedProperties::sSharedPropertiesBeingBuilt = 0;

IPC_SharedProperties::IPC_SharedProperties(const std::string & name,
                                               ssize_t sharedPropertiesSize) :
        mSharedMemoryName(name), mIsPropertyServer(true), mSharedMemory(0),
        mRegion(0), mSharedProperties(this),
        mSharedPropertiesSize(sharedPropertiesSize),
        mSharedPropertiesCount(0), mLink(0)
{
    // reserve enough to avoid multiple reallocatio
    mProperties.reserve(sharedPropertiesSize / sizeof(IPC_PropertyBaseT<bool>));

    this->sSharedPropertiesBeingBuilt = this;
}

template <class SharedProperties> SharedProperties * IPC_SharedProperties::
                               createSharedProperties(const std::string & name)
{
    using namespace boost::interprocess;

    SharedProperties *    sharedProperties = 0;
    try
    {
        if (SharedProperties::cIsServerNotClient)
        {
            // A server object is created in shared memory
            // by the placement new operator
            boost::interprocess::shared_memory_object * sharedMemory =
              new shared_memory_object(open_or_create, name.c_str(), read_write);
            sharedMemory->truncate(sizeof(SharedProperties));
            boost::interprocess::mapped_region * region =
              new mapped_region(*sharedMemory, read_write, 0, sizeof(SharedProperties));

            // Big deal! this is where we create
            // the shared properties in shared memory!
            sharedProperties = new (region->get_address()) SharedProperties(name);
            sharedProperties->mIsPropertyServer = true;
            sharedProperties->mSharedPropertiesCount =
                                          sharedProperties->mProperties.size();
            sharedProperties->mSharedMemory = sharedMemory;
            sharedProperties->mRegion = region;

            new IPC_MasterLink(*sharedProperties);
        }
        else
        {
            // A client is created on the heap and later
            //attached to the shared memory. A standard constructor is used.
            sharedProperties = new SharedProperties(name);
            sharedProperties->mIsPropertyServer = false;
            sharedProperties->mSharedPropertiesCount =
                                           sharedProperties->mProperties.size();

            new IPC_SlaveLink(*sharedProperties);
        }

        IPC_SharedProperties::sSharedPropertiesBeingBuilt = 0;
    }
    catch(interprocess_exception & error)
    {
        PM_LOG_ERROR(MSGID_DEBUG_IPC, INIT_KVCOUNT,\
            "attachSharedMemory: Exception trying to use  \
            the shared memory: %s", error.what());
        delete sharedProperties->mRegion;
        sharedProperties->mRegion = 0;
        delete sharedProperties->mSharedMemory;
        sharedProperties->mSharedMemory = 0;
    }

    return sharedProperties;
}

bool IPC_SharedProperties::attachSharedMemory()
{
    // Server only method
    if (!VERIFY(mIsPropertyServer == false))
        return false;

    using namespace boost::interprocess;

    detachSharedMemory();

    try {
        mSharedMemory = new shared_memory_object(open_only,
                                                 this->mSharedMemoryName.c_str(),
                                                 read_only);
        mRegion = new mapped_region(*mSharedMemory,
                                    read_only,
                                    0,
                                    sizeof(this->mSharedPropertiesSize));
        IPC_SharedProperties * sharedProperties =
                 reinterpret_cast<IPC_SharedProperties *>(mRegion->get_address());
        if (sharedProperties->mSharedPropertiesSize == this->mSharedPropertiesSize &&
           sharedProperties->mSharedPropertiesCount == this->mSharedPropertiesCount)
        {
            this->mSharedProperties = sharedProperties;
            for (IPC_PropertyRegister::iterator iter = mProperties.begin();
                                            iter != mProperties.end(); ++iter)
            {
                IPC_PropertyBase * property = *iter;
                if (VERIFY(property))
                    property->onConnectedClient();
            }
        }
        else
        {
            std::string msg = "Shared properties mismatch for '";
            msg += this->mSharedMemoryName;
            msg += "': this client is incompatible with the server found!";
            FAILURE(msg.c_str());
            detachSharedMemory();
            if (mLink)
                mLink->sendRequest(eOperationRequest_ReportIncompatibleClient, 0);
            return false;
        }
    }
    catch(interprocess_exception & ex)
    {
        PM_LOG_ERROR(MSGID_DEBUG_IPC, INIT_KVCOUNT,\
            "attachSharedMemory: Exception trying to use  \
            the shared memory: %s", ex.what());
        detachSharedMemory();
        return false;
    }
    return true;
}

void IPC_SharedProperties::detachSharedMemory()
{
    // Server only method
    if (!VERIFY(mIsPropertyServer == false))
        return;

    // yes, it is legal to delete 0, so we can directly do this:
    delete mRegion;
    mRegion = 0;
    delete mSharedMemory;
    mSharedMemory = 0;
    this->mSharedProperties = this;
}

/// Specialized operations for int
template <> bool IPC_ServerProperty<int>::add(const int & value)
                                             { return set(get() + value); }
template <> bool IPC_ServerProperty<int>::multiply(const int & value)
                                             { return set(get() * value); }
template <> bool IPC_ServerProperty<int>::divide(const int & value)
                                             { return set(get() / value); }
template <> bool IPC_ServerProperty<int>::andValue(const int & value)
                                             { return set(get() & value); }
template <> bool IPC_ServerProperty<int>::orValue(const int & value)
                                             { return set(get() | value); }
template <> bool IPC_ServerProperty<int>::xorValue(const int & value)
                                             { return set(get() ^ value); }
template <> bool IPC_ServerProperty<int>::invert()
                                             { return set(-get()); }

/// Specialized operations for bool
template <> bool IPC_ServerProperty<bool>::andValue(const bool & value)
                                               { return set(get() && value); }
template <> bool IPC_ServerProperty<bool>::orValue(const bool & value)
                                               { return set(get() || value); }
template <> bool IPC_ServerProperty<bool>::xorValue(const bool & value)
                                               { return set(get() != value); }
template <> bool IPC_ServerProperty<bool>::invert()
                                               { return set(!get()); }
