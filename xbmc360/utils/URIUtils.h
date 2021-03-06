/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
#pragma once

#include "StdString.h"

//class CURL;

class URIUtils
{
public:
	URIUtils(void) {};
	virtual ~URIUtils(void) {};

	static bool IsDOSPath(const CStdString &path);

	static void AddSlashAtEnd(CStdString& strFolder);
	static bool HasSlashAtEnd(const CStdString& strFile);

	static void AddFileToFolder(const CStdString& strFolder,
                              const CStdString& strFile, CStdString& strResult);

	static CStdString AddFileToFolder(const CStdString &strFolder, 
                                    const CStdString &strFile)
	{
		CStdString result;
		AddFileToFolder(strFolder, strFile, result);
		return result;
	}

};