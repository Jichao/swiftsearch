#pragma once

#ifndef CCOM_CRIT_SEC_LOCK_HPP
#define CCOM_CRIT_SEC_LOCK_HPP

#include <ComDef.h>

namespace ATL
{
	__if_not_exists(CComCritSecLock)
	{
		template< class TLock >
		class CComCritSecLock
		{
		public:
			CComCritSecLock( TLock& cs, bool bInitialLock = true );
			~CComCritSecLock() throw();

			HRESULT Lock() throw();
			void Unlock() throw();

			// Implementation
		private:
			TLock& m_cs;
			bool m_bLocked;

			// Private to avoid accidental use
			CComCritSecLock( const CComCritSecLock& ) throw();
			CComCritSecLock& operator=( const CComCritSecLock& ) throw();
		};

		template< class TLock >
		inline CComCritSecLock< TLock >::CComCritSecLock( TLock& cs, bool bInitialLock ) :
		m_cs( cs ),
		m_bLocked( false )
		{
			if( bInitialLock )
			{
				HRESULT hr;

				hr = Lock();
				if( FAILED( hr ) )
				{
					throw hr;
				}
			}
		}

		template< class TLock >
		inline CComCritSecLock< TLock >::~CComCritSecLock() throw()
		{
			if( m_bLocked )
			{
				Unlock();
			}
		}

		template< class TLock >
		inline HRESULT CComCritSecLock< TLock >::Lock() throw()
		{
			HRESULT hr;

			ATLASSERT( !m_bLocked );
			hr = m_cs.Lock();
			if( FAILED( hr ) )
			{
				return( hr );
			}
			m_bLocked = true;

			return( S_OK );
		}

		template< class TLock >
		inline void CComCritSecLock< TLock >::Unlock() throw()
		{
			assert( m_bLocked );
			m_cs.Unlock();
			m_bLocked = false;
		}
	}
}
#endif