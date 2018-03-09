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

#ifndef IPC_PROPERTY_H_
#define IPC_PROPERTY_H_

#include <glibconfig.h>

#include <vector>
#include <string>

#include <boost/signals2.hpp>
#include <boost/signals2/signal.hpp>

#include "log.h"

namespace boost {
    namespace interprocess {
        class shared_memory_object;
        class mapped_region;
    }
};

class IPC_PropertyBase;
class IPC_SharedProperties;

template <class T> class IPC_ServerProperty;

/*
 * Properties changes can be controlled by giving them a particular set behavior
 * For instance, a set can be filtered and rejected (for instance,
 * when changes a not allowed in the current context)
 * or a set can be modified to take actions prior to the assignment
 * (mode change?), or the value assigned can be adjusted (bounds?)...
 * To change the property eventually, the property's doSet() method should be
 * used, which is what is called when no set behavior is assigned.
 */
template <class T> class IPC_SetPropertyBehavior
{
public:
    virtual ~IPC_SetPropertyBehavior() {}

    virtual bool    set(IPC_ServerProperty<T> & property, const T & value) = 0;
};

typedef guint16 IPC_PropertyID;

const IPC_PropertyID cInvalidPropertyID = -1;

enum EOperationRequest
{
    eOperationRequest_Set = 0,  // set request sent by clients,
                                // or server to set local value
    eOperationRequest_Add,      // add request sent by clients
    eOperationRequest_Multiply, // multiply request sent by clients
    eOperationRequest_Divide,   // divide request sent by clients
    eOperationRequest_And,      // and request sent by clients
    eOperationRequest_Or,       // or request sent by clients
    eOperationRequest_XOr,      // xor request sent by clients
    eOperationRequest_Invert,   // invert request sent by clients

    eOperationRequest_Message,  // message handling, both directions

    eOperationRequest_HandlesChangeNotifications, // sent by clients
                                                  //requesting change
                                                  // notification notices
    eOperationRequest_ReportIncompatibleClient    // debug message
};

/*
 * Type independent base class of a property.
 * Holds type independent methods.
 */
class IPC_PropertyBase
{
public:

    enum EPropertyFlag
    {
        ePropertyFlag_None = 0,

        ePropertyFlag_UseLocalCopy = 1 << 0, ///< do not use the version in
                                             // shared memory. Use for
                                             // non-atomic values (> 32bits). Slower.
        ePropertyFlag_NotifyClientsEvenIfNoChange = 1 << 1, ///< requesting a
                                                  // change with the current
                                                  // value will always trigger
                                                  // a change notification to clients.
        ePropertyFlag_DontNotifyClients = 1 << 2, ///< requesting a change
                                                  //will never trigger a
                                                  // change notification to clients.
        ePropertyFlag_DontNotifyClientsForOwnChanges = 1 << 3, ///< requesting
                                                     // a change from a client
                                                     // will not trigger a
                                                     // notification back to
                                                     // that particular client.
        ePropertyFlag_NoNotificationOnConnect = 1 << 4,///< suppress initial
                                                       // notification when
                                                       // connection is
                                                       // established.
    };

    IPC_PropertyBase();    // no need for a virtual destructor:
                          // properties should never be deleted...

    IPC_PropertyID            getID() const    { return mID; }

    void setFlag(EPropertyFlag flag)                { mPropertyFlags |= flag; }
    void clearFlag(EPropertyFlag flag)            { mPropertyFlags &= ~flag; }
    bool testFlag(EPropertyFlag flag) const { return (mPropertyFlags & flag) != 0; }

    /// Called when a remote user wants to change the value.
    // For property system implementation only. Don't call directly.
    virtual void requestReceived(EOperationRequest operation,
                                 const void * value, ssize_t size) = 0;

    /// Called when a remote user wants to change the value.
    // For property system implementation only. Don't call directly.
    virtual        void        requestReceived(EOperationRequest operation) = 0;

    /// Called when a slave is told that the master property was changed
    virtual        void        notifyListeners()        {}

    /// Called internally when a client property is connected.
    // Gives it a change to notify local listeners or to
    // register for notifications. INTERNAL USE ONLY!
    virtual        void        onConnectedClient()        {}

protected:
    IPC_SharedProperties *    mSharedProperties;
    IPC_PropertyID            mID;
    guint16                    mPropertyFlags;
};

/*
 * Communication channel with the master/slaves
 */
class IPC_Link
{
public:

    IPC_Link(IPC_SharedProperties & sharedProperties) :
                                        mSharedProperties(sharedProperties) {}
    virtual ~IPC_Link() {}

    /// Send an operation request on a property. Can be a request to set
    // the master from a slave, or a request to set
    // local copy from master to a slave.
    virtual bool    sendRequest(EOperationRequest operation,
                                IPC_PropertyID id,
                                const void * value, ssize_t size) = 0;

    /// Server only: Notify clients that a property was changed
    virtual void    sendChangeNotification(IPC_PropertyID id) = 0;

    /// Client only: Send an operation request on a property.
    // Can be a request to set the master from a slave,
    // or a request to set local copy from master to a slave.
    virtual bool    sendRequest(EOperationRequest operation, IPC_PropertyID id) = 0;

protected:
    IPC_SharedProperties &    mSharedProperties;
};

/*
 * Container class for a group of properties
 * that are all member variables of that image
 */
class IPC_SharedProperties
{
public:

    /// static method to construct the shared
    // properties of your type SharedProperties
    /// do: SharedProperties * sharedProperties =
    // IPC_SharedProperties::createSharedProperties<SharedProperties>
    //("name of shared memory segment");
    template <class SP> static SP * createSharedProperties(const std::string & name);

    /// When appropriate (when master-slave communication is established
    // for instance), bind the local copy of the slave with the shared memory
    bool    attachSharedMemory();
    /// One might want to stop reading from shared memory,
    // for instance, when connection is lost
    void    detachSharedMemory();

    const char *    getName() const   { return mSharedMemoryName.c_str(); }

    void    setLink(IPC_Link * link)        { mLink = link; }

    typedef std::vector<IPC_PropertyBase *> IPC_PropertyRegister;

    /// Send an operation request on a property. Can be a request to set
    // from a slave, or a request to set local copy from master to a slave.
    bool    sendRequest(EOperationRequest operation,
                        IPC_PropertyID id,
                        const void * value, ssize_t size)
    {
        if (mLink)
            return mLink->sendRequest(operation, id, value, size);
        return false;
    }

    // Server only
    void    sendChangeNotification(IPC_PropertyID id)
    {
        if (mLink)
            mLink->sendChangeNotification(id);
    }

    /// Client only: Send an operation request on a property.
    // Can be a request to set from a slave,
    // or a request to set local copy from master to a slave.
    bool    sendRequest(EOperationRequest operation, IPC_PropertyID id)
    {
        if (mLink)
            return mLink->sendRequest(operation, id);
        return false;
    }

    /// Notify local listeners that a property was changed
    void    changeNotificationReceived(IPC_PropertyID id)
    {
        if (VERIFY(id < mProperties.size()))
            mProperties[id]->notifyListeners();
    }

    /// Request received: change master value or slave's local value
    void    requestReceived(EOperationRequest operation,
                            IPC_PropertyID id,
                            const void * value, ssize_t size)
    {
        if (VERIFY(id < mProperties.size()))
            mProperties[id]->requestReceived(operation, value, size);
    }

    /// Request received: change master value or slave's local value
    void    requestReceived(EOperationRequest operation, IPC_PropertyID id)
    {
        if (VERIFY(id < mProperties.size()))
            mProperties[id]->requestReceived(operation);
    }

    /// Used by properties to access their value from shared memory
    const void *    getSharedPropertyAddress(const void * propertyAddress) const
    {
        ssize_t valueOffset  = reinterpret_cast<const char *>(propertyAddress)
                                       - reinterpret_cast<const char *>(this);
        return reinterpret_cast<const char *>(mSharedProperties) + valueOffset;
    }

    bool    testPropertyFlag(IPC_PropertyID id,
                             IPC_PropertyBase::EPropertyFlag flag) const
    {
        if (VERIFY(id < mProperties.size()))
            return mProperties[id]->testFlag(flag);
        return false;
    }

    size_t    getPropertyCount() const    { return mProperties.size(); }

    bool    isServer() const            { return mIsPropertyServer; }

protected:
    IPC_SharedProperties(const std::string & name, ssize_t sharedPropertiesSize);

    friend class IPC_Link;
    friend class IPC_PropertyBase;

    std::string                                    mSharedMemoryName;
    bool                                        mIsPropertyServer;
    boost::interprocess::shared_memory_object *    mSharedMemory;
    boost::interprocess::mapped_region *         mRegion;
    IPC_PropertyRegister                        mProperties;
    IPC_SharedProperties *                        mSharedProperties;
    ssize_t                                        mSharedPropertiesSize;
    size_t                                        mSharedPropertiesCount;
    IPC_Link *                                    mLink;

    /// for internal use, and only while properties are being created...
    static IPC_SharedProperties *        sSharedPropertiesBeingBuilt;
};

template <class T> class IPC_PropertyBaseT : public IPC_PropertyBase
{
public:
    typedef boost::signals2::signal<void (const T &)>         TSignal;
    typedef boost::signals2::slot<void (const T &)>            TSlot;
    typedef boost::signals2::signal<void (const char *)>     TMessageSignal;
    typedef boost::signals2::slot<void (const char *)>        TMessageSlot;

    // Be notified when a property is changed
    boost::signals2::connection sendChanges(const TSlot & slot)
    {
        if (mSignal.empty() && !mSharedProperties->isServer())
            mSharedProperties->sendRequest(
                   eOperationRequest_HandlesChangeNotifications, this->getID());
        return this->mSignal.connect(slot);
    }

    // Be notified when a property receives a message
    boost::signals2::connection sendMessages(const TMessageSlot & slot)
    {
        return mMessageSignal.connect(slot);
    }

    /// Initial value of the property. Will NEVER make an IPC request
    void init(const T & newValue)        { setLocalValue(newValue); }

    const T &        getLocalValue() const                    { return mValue; }

    /// Call listeners' message reception callbacks
    void            messageReceived(const char * message)
    {
        mMessageSignal(message);
    }

    /// Send a text message
    bool            sendMessage(const char * message)
    {
        return mSharedProperties->sendRequest(eOperationRequest_Message,
                                              getID(),
                                               message, ::strlen(message) + 1);
    }
    bool            sendMessage(std::string message)
    {
        return mSharedProperties->sendRequest(eOperationRequest_Message,
                                              getID(),
                                              message.c_str(),
                                              message.size() + 1);
    }

protected:
    IPC_PropertyBaseT() : mSetPropertyBehavior(0) {}
    IPC_PropertyBaseT(T initValue) : mValue(initValue), mSetPropertyBehavior(0) {}

    /// Last level setter of the property. Should not be used directly,
    // but may well be overridden! (save as init, but for code clarity...)
    virtual void    setLocalValue(const T & newValue)        { mValue = newValue; }

    /// The value of the property for master properties
    // and properties using local copies.
    T                                mValue;

    // declare these both in master & slave to have a consistent memory
    // layout. Only master will use mSetPropertyBehavior
    IPC_SetPropertyBehavior<T> *    mSetPropertyBehavior;

    TSignal                            mSignal;
    TMessageSignal                    mMessageSignal;
};

/*
 * Master version of the property: the one that's really manipulated.
 */

template <class T> class IPC_ServerProperty : public IPC_PropertyBaseT<T>
{
public:
    static const bool cIsServerNotClient = true;

    IPC_ServerProperty() {}
    IPC_ServerProperty(T initValue) : IPC_PropertyBaseT<T>(initValue) {}

    // Override set handling, with pre-conditions,
    // pre-operation, request filtering, etc.
    void setSetPropertyBehavior(IPC_SetPropertyBehavior<T> * setPropertyBehavior)
    {
        if (VERIFY(this->mSetPropertyBehavior == 0))
            this->mSetPropertyBehavior = setPropertyBehavior;
    }

    /// Normal way to get the value of the property. Will use the shared
    // memory value if allowed, or cached value.
    const T &        get() const
    {
        return this->mValue;
    }

    /// Normal way to set the value.
    bool            set(const T & newValue)
    {
        if (this->mSetPropertyBehavior)
            return this->mSetPropertyBehavior->set(*this, newValue);

        return doSet(newValue);
    }

    // specialized version must be explicitly provided
    bool add(const T & value) { return VERIFY(false); }
    bool multiply(const T & value) { return VERIFY(false); }
    bool divide(const T & value){ return VERIFY(false); }
    bool andValue(const T & value){ return VERIFY(false); }
    bool orValue(const T & value){ return VERIFY(false); }
    bool xorValue(const T & value){ return VERIFY(false); }
    bool invert(){ return VERIFY(false); }

    /// Method to set the value of the property that bypasses a possible
    // setPropertyBehavior.
    /// This is also how a setPropertyBehavior can
    // change the value of the property
    bool            doSet(const T & newValue)
    {
        if (newValue != this->mValue ||
            this->testFlag(IPC_PropertyBase::ePropertyFlag_NotifyClientsEvenIfNoChange))
        {
            this->mValue = newValue;
            if (!this->testFlag(IPC_PropertyBase::ePropertyFlag_DontNotifyClients))
            {
                if (this->testFlag(IPC_PropertyBase::ePropertyFlag_UseLocalCopy))
                {
                    this->mSharedProperties->sendRequest(eOperationRequest_Set,
                                      this->getID(), &this->mValue, sizeof(T));
                }
                else
                {
                    this->mSharedProperties->sendChangeNotification(this->getID());
                }
            }
            IPC_ServerProperty<T>::notifyListeners();
            return true;
        }
        return false;
    }

    /// Used internally to execute the request made in by a slave
    void requestReceived(EOperationRequest operation,
                         const void * valuePtr, ssize_t size)
    {
        if (operation == eOperationRequest_Message)
        {
            if (VERIFY(size > 0))
            {
                const char * message = reinterpret_cast<const char *>(valuePtr);
                if (VERIFY(message[size - 1] ==  0))
                {
                    IPC_PropertyBaseT<T>::messageReceived(message);
                }
            }
        }
        else if (VERIFY(size == sizeof(T)))
        {
            const T * value = reinterpret_cast<const T *>(valuePtr);
            switch (operation)
            {
            case eOperationRequest_Set:            set(*value);        break;
            case eOperationRequest_Add:            add(*value);        break;
            case eOperationRequest_Multiply:    multiply(*value);    break;
            case eOperationRequest_Divide:        divide(*value)    ;    break;
            case eOperationRequest_And:            andValue(*value);    break;
            case eOperationRequest_Or:            orValue(*value);    break;
            case eOperationRequest_XOr:            xorValue(*value);    break;
            default: FAILURE("IPC_ServerProperty::requestReceived: Invalid operation requested");
            }
        }
    }

    /// Used internally to execute the request made in by a slave
    void            requestReceived(EOperationRequest operation)
    {
        if (operation == eOperationRequest_Invert)
            invert();
        else if (operation == eOperationRequest_ReportIncompatibleClient)
        {
            FAILURE("IPC_ServerProperty::requestReceived: Incompatible client encountered...");
        }
        else
            FAILURE("IPC_ServerProperty::requestReceived: Invalid operation requested");
    }

    /// Call listeners' callback method
    void notifyListeners()
    {
        this->mSignal(this->mValue);
    }
};

/*
 * Slave version of the property: the one that remote users use to access
 * the (remote) master object.
 */

template <class T> class IPC_ClientProperty : public IPC_PropertyBaseT<T>
{
public:
    static const bool cIsServerNotClient = false;

    IPC_ClientProperty() {}
    IPC_ClientProperty(T initValue) : IPC_PropertyBaseT<T>(initValue) {}

    /// Normal way to get the value of the property. Will use the shared
    // memory value if allowed, or cached value.
    const T &        get() const
    {
        if (this->testFlag(IPC_PropertyBase::ePropertyFlag_UseLocalCopy))
            return this->mValue;

        const IPC_ClientProperty<T> * propertyInSharedMemory =
                               reinterpret_cast<const IPC_ClientProperty<T> *>
                               (this->mSharedProperties->getSharedPropertyAddress(this));

        return propertyInSharedMemory->mValue;
    }

    /// Normal way to set the value. Will forward the request to master.
    bool            set(const T & newValue)
    {
        return this->mSharedProperties->sendRequest(eOperationRequest_Set,
                                                    this->getID(),
                                                    &newValue, sizeof(T));
    }
    /// Normal way to add to the value. Will forward the request to master.
    bool            add(const T & value)
    {
        return this->mSharedProperties->sendRequest(eOperationRequest_Add,
                                                    this->getID(),
                                                    &value, sizeof(T));
    }
    /// Normal way to multiply to the value. Will forward the request to master.
    bool            multiply(const T & value)
    {
        return this->mSharedProperties->sendRequest(eOperationRequest_Multiply,
                                                     this->getID(),
                                                     &value, sizeof(T));
    }
    /// Normal way to divide to the value. Will forward the request to master.
    bool            divide(const T & value)
    {
        return this->mSharedProperties->sendRequest(eOperationRequest_Divide,
                                                     this->getID(),
                                                     &value, sizeof(T));
    }
    /// Normal way to "and" the value. Will forward the request to master.
    bool            andValue(const T & value)
    {
        return this->mSharedProperties->sendRequest(eOperationRequest_And,
                                                    this->getID(),
                                                    &value, sizeof(T));
    }
    /// Normal way to "or" the value. Will forward the request to master.
    bool            orValue(const T & value)
    {
        return this->mSharedProperties->sendRequest(eOperationRequest_Or,
                                                      this->getID(),
                                                      &value, sizeof(T));
    }
    /// Normal way to "xor" the value. Will forward the request to master.
    bool            xorValueValue(const T & value)
    {
        return this->mSharedProperties->sendRequest(eOperationRequest_XOr,
                                                     this->getID(),
                                                     &value, sizeof(T));
    }
    /// Normal way to invert the value. Will forward the request to master.
    bool            invert()
    {
        return this->mSharedProperties->sendRequest(eOperationRequest_Invert,
                                                        this->getID());
    }

    /// Used internally to execute the set request made in by the master.
    // In slaves, only set requests & messages are legal...
    void requestReceived(EOperationRequest operation, const void * valuePtr,
                                                       ssize_t size)
    {
        if (operation == eOperationRequest_Set)
        {
            if (VERIFY(size == sizeof(T)))
            {
                this->mValue = *reinterpret_cast<const T *>(valuePtr);
                IPC_PropertyBase::notifyListeners();
            }
        }
        else if (VERIFY(operation == eOperationRequest_Message))
        {
            const char * message = reinterpret_cast<const char *>(valuePtr);
            if (VERIFY(size > 0) && VERIFY(message[size - 1] == 0))
            {
                IPC_PropertyBaseT<T>::messageReceived(message);
            }
        }
    }

    /// Used internally, but only for server properties.
    void            requestReceived(EOperationRequest operation)
    {    /// requests to clients should include only set operations
        SHOULD_NOT_REACH_HERE;
    }

    /// Call listeners' callback method
    void            notifyListeners()
    {
        mSignal(get());
    }

    /// Called when property is connected. Don't call directly.
    void            onConnectedClient()
    {
        if (!this->mSignal.empty())
            this->mSharedProperties->sendRequest
                (eOperationRequest_HandlesChangeNotifications, this->getID());
    }
};

#endif /* IPC_PROPERTY_H_ */
