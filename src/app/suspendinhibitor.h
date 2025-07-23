/*
 * Fedora Media Writer
 * Copyright (C) 2024 Fedora Media Writer Contributors
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef SUSPENDINHIBITOR_H
#define SUSPENDINHIBITOR_H

#include <QObject>

/**
 * @brief The SuspendInhibitor class
 * 
 * Prevents the system from going to sleep or suspend during long-running operations
 * like downloading ISOs or writing to flash drives.
 * 
 * Platform-specific implementation for Windows, macOS, and Linux.
 */
class SuspendInhibitor
{
public:
    /**
     * @brief Inhibit system suspend/sleep
     * @param reason Human-readable reason for inhibiting suspend
     * @return true if inhibition was successfully enabled
     */
    static bool inhibit(const QString &reason);
    
    /**
     * @brief Release suspend inhibition
     * @return true if inhibition was successfully released
     */
    static bool release();
    
    /**
     * @brief Check if suspend is currently inhibited
     * @return true if suspend is currently inhibited
     */
    static bool isInhibited();

private:
    static bool m_inhibited;
    
#ifdef __linux
    static uint32_t m_inhibitCookie;
#endif
#ifdef _WIN32
    static void* m_threadExecutionState;
#endif
#ifdef __APPLE__
    static uint32_t m_assertionId;
#endif
};

#endif // SUSPENDINHIBITOR_H 