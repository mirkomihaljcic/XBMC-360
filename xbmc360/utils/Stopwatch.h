#ifndef H_CSTOPWATCH
#define H_CSTOPWATCH

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "Stdafx.h"

class CStopWatch
{
public:
	CStopWatch();
	~CStopWatch();

	bool IsRunning() const;
	void StartZero();          ///< Resets clock to zero and starts running
	void Start();              ///< Sets clock to zero if not running and starts running.
	void Stop();               ///< Stops clock and sets to zero if running.
	void Reset();              ///< Resets clock to zero - does not alter running state.

	float GetElapsedSeconds() const;
	float GetElapsedMilliseconds() const;

private:
	__int64 GetTicks() const;
	float m_timerPeriod;        // to save division in GetElapsed...()
	__int64 m_startTick;
	bool m_isRunning;
};

#endif //H_CSTOPWATCH