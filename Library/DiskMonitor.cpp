/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "DiskMonitor.h"

DiskMonitor::DiskMonitor(const std::wstring& driveLetter)
    : m_DriveLetter(driveLetter)
{
    // Ensure drive letter ends with backslash
    if (!m_DriveLetter.empty() && m_DriveLetter.back() != L'\\')
    {
        m_DriveLetter += L'\\';
    }
}

DiskMonitor::~DiskMonitor()
{
}

DiskMonitor::Stats DiskMonitor::GetStats()
{
    Stats stats = { 0, 0, 0, 0 };

    // Get disk space information
    ULARGE_INTEGER freeBytesAvailable, totalBytes, totalFreeBytes;
    if (GetDiskFreeSpaceEx(m_DriveLetter.c_str(), &freeBytesAvailable, &totalBytes, &totalFreeBytes))
    {
        stats.totalSpace = totalBytes.QuadPart;
        stats.freeSpace = totalFreeBytes.QuadPart;
        stats.usedSpace = totalBytes.QuadPart - totalFreeBytes.QuadPart;
        
        if (totalBytes.QuadPart > 0)
        {
            stats.percentUsed = (int)((stats.usedSpace * 100) / totalBytes.QuadPart);
        }
    }

    return stats;
}
