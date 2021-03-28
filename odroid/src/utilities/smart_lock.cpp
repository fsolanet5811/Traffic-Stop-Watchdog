#include "utilities.hpp"
#include <iostream>
#include <sstream>

using namespace tsw::utilities;
using namespace std;

SmartLock::SmartLock() : SmartLock("Lock") { }

SmartLock::SmartLock(string name)
{
    Name = name;
}

void SmartLock::Lock(string description)
{
    // Tell them that we want to enter a lock with this guy.
    string descriptor =  BuildDescriptor(description);
    Log("Lock Entering  " + descriptor, Locking);

    while(!_lock.try_lock()) { }

    // Now tell them that we locked ok.
    Log("Lock Entered   " + descriptor, Locking);
}

void SmartLock::Unlock(string description)
{
    // Even though this is guaranteed to not get deadlocked, we want to log it in case another thread's message beats the next one.
    // If that happened, the log would be confusing.
    string descriptor =  BuildDescriptor(description);
    Log("Lock Releasing " + descriptor, Locking);
    
    _lock.unlock();

    // Alert that we released the lock.
    Log("Lock Released  " + descriptor, Locking);
}

string SmartLock::BuildDescriptor(string description)
{
    stringstream ss;
    ss << Name << " | " << this_thread::get_id() << " | " << description;
    return ss.str();
}
