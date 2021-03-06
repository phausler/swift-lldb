#include "lldb/Host/ProcessRunLock.h"
#include "lldb/Host/windows/windows.h"

namespace {
#if defined(__MINGW32__)
// Taken from WinNT.h
typedef struct _RTL_SRWLOCK { PVOID Ptr; } RTL_SRWLOCK, *PRTL_SRWLOCK;

// Taken from WinBase.h
typedef RTL_SRWLOCK SRWLOCK, *PSRWLOCK;
#endif
}

static PSRWLOCK GetLock(lldb::rwlock_t lock) {
  return static_cast<PSRWLOCK>(lock);
}

static bool ReadLock(lldb::rwlock_t rwlock) {
  ::AcquireSRWLockShared(GetLock(rwlock));
  return true;
}

static bool ReadUnlock(lldb::rwlock_t rwlock) {
  ::ReleaseSRWLockShared(GetLock(rwlock));
  return true;
}

static bool WriteLock(lldb::rwlock_t rwlock) {
  ::AcquireSRWLockExclusive(GetLock(rwlock));
  return true;
}

static bool WriteTryLock(lldb::rwlock_t rwlock) {
  return !!::TryAcquireSRWLockExclusive(GetLock(rwlock));
}

static bool WriteUnlock(lldb::rwlock_t rwlock) {
  ::ReleaseSRWLockExclusive(GetLock(rwlock));
  return true;
}

using namespace lldb_private;

ProcessRunLock::ProcessRunLock() : m_running(false) {
  m_rwlock = new SRWLOCK;
  InitializeSRWLock(GetLock(m_rwlock));
}

ProcessRunLock::~ProcessRunLock() { delete static_cast<SRWLOCK *>(m_rwlock); }

bool ProcessRunLock::ReadTryLock() {
  ::ReadLock(m_rwlock);
  if (m_running == false)
    return true;
  ::ReadUnlock(m_rwlock);
  return false;
}

bool ProcessRunLock::ReadUnlock() { return ::ReadUnlock(m_rwlock); }

bool ProcessRunLock::SetRunning() {
  WriteLock(m_rwlock);
  m_running = true;
  WriteUnlock(m_rwlock);
  return true;
}

bool ProcessRunLock::TrySetRunning() {
  if (WriteTryLock(m_rwlock)) {
    bool was_running = m_running;
    m_running = true;
    WriteUnlock(m_rwlock);
    return !was_running;
  }
  return false;
}

bool ProcessRunLock::SetStopped() {
  WriteLock(m_rwlock);
  m_running = false;
  WriteUnlock(m_rwlock);
  return true;
}
