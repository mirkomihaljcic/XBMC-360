// SingleLock.h: interface for the CSingleLock class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SINGLELOCK_H__50A43114_6A71_4FBD_BF51_D1F2DD3A60FA__INCLUDED_)
#define AFX_SINGLELOCK_H__50A43114_6A71_4FBD_BF51_D1F2DD3A60FA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "criticalsection.h"

class CSingleLock
{
public:
	void Unlock();
	void Lock();

	CSingleLock(CCriticalSection& cs);
	CSingleLock(const CCriticalSection& cs);
	virtual ~CSingleLock();

	bool IsOwner() const;
	bool Enter();
	void Leave();

private:
	CSingleLock(const CSingleLock& src);
	CSingleLock& operator=(const CSingleLock& src);

	// Reference to critical section object
	CCriticalSection& m_cs;
	// Ownership flag
	bool m_bIsOwner;
};

class CSingleExit
{
public:
	CSingleExit(CCriticalSection& cs);
	CSingleExit(const CCriticalSection& cs);
	virtual ~CSingleExit();

	void Exit();
	void Restore();

	CCriticalSection& m_cs;
	unsigned int      m_count;
};


#endif // !defined(AFX_SINGLELOCK_H__50A43114_6A71_4FBD_BF51_D1F2DD3A60FA__INCLUDED_)
