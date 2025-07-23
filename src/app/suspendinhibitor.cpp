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

#include "suspendinhibitor.h"
#include "utilities.h"

#include <QDebug>

// Initialize static members
bool SuspendInhibitor::m_inhibited = false;

#ifdef __linux

#include <QDBusInterface>
#include <QDBusReply>

uint32_t SuspendInhibitor::m_inhibitCookie = 0;

bool SuspendInhibitor::inhibit(const QString &reason)
{
    if (m_inhibited) {
        mDebug() << "Suspend inhibition already active";
        return true;
    }

    // Try systemd-logind first (most common on modern Linux systems)
    QDBusInterface logind("org.freedesktop.login1", "/org/freedesktop/login1", "org.freedesktop.login1.Manager", QDBusConnection::systemBus());
    if (logind.isValid()) {
        QDBusReply<QDBusUnixFileDescriptor> reply = logind.call("Inhibit", 
            "sleep:idle", // what to inhibit
            "Fedora Media Writer", // who
            reason, // why
            "block" // mode
        );
        
        if (reply.isValid()) {
            m_inhibited = true;
            mDebug() << "Successfully inhibited suspend via systemd-logind:" << reason;
            return true;
        } else {
            mWarning() << "Failed to inhibit suspend via systemd-logind:" << reply.error().message();
        }
    }

    // Fallback to GNOME session manager
    QDBusInterface gnomeSession("org.gnome.SessionManager", "/org/gnome/SessionManager", "org.gnome.SessionManager", QDBusConnection::sessionBus());
    if (gnomeSession.isValid()) {
        QDBusReply<uint32_t> reply = gnomeSession.call("Inhibit",
            "org.fedoraproject.MediaWriter", // app_id
            0U, // toplevel_xid
            reason, // reason
            8U // flags: 8 = inhibit suspend
        );
        
        if (reply.isValid()) {
            m_inhibitCookie = reply.value();
            m_inhibited = true;
            mDebug() << "Successfully inhibited suspend via GNOME session manager:" << reason;
            return true;
        } else {
            mWarning() << "Failed to inhibit suspend via GNOME session manager:" << reply.error().message();
        }
    }

    // Fallback to KDE PowerDevil
    QDBusInterface powerDevil("org.kde.Solid.PowerManagement", "/org/kde/Solid/PowerManagement/PolicyAgent", "org.kde.Solid.PowerManagement.PolicyAgent", QDBusConnection::sessionBus());
    if (powerDevil.isValid()) {
        QDBusReply<uint32_t> reply = powerDevil.call("AddInhibition", 
            1, // ChangeProfile
            "org.fedoraproject.MediaWriter",
            reason
        );
        
        if (reply.isValid()) {
            m_inhibitCookie = reply.value();
            m_inhibited = true;
            mDebug() << "Successfully inhibited suspend via KDE PowerDevil:" << reason;
            return true;
        } else {
            mWarning() << "Failed to inhibit suspend via KDE PowerDevil:" << reply.error().message();
        }
    }

    mWarning() << "Could not inhibit suspend - no compatible power management service found";
    return false;
}

bool SuspendInhibitor::release()
{
    if (!m_inhibited) {
        return true;
    }

    bool success = false;

    // Try releasing via GNOME session manager
    if (m_inhibitCookie != 0) {
        QDBusInterface gnomeSession("org.gnome.SessionManager", "/org/gnome/SessionManager", "org.gnome.SessionManager", QDBusConnection::sessionBus());
        if (gnomeSession.isValid()) {
            QDBusReply<void> reply = gnomeSession.call("Uninhibit", m_inhibitCookie);
            if (reply.isValid()) {
                mDebug() << "Successfully released suspend inhibition via GNOME session manager";
                success = true;
            }
        }

        // Try releasing via KDE PowerDevil
        if (!success) {
            QDBusInterface powerDevil("org.kde.Solid.PowerManagement", "/org/kde/Solid/PowerManagement/PolicyAgent", "org.kde.Solid.PowerManagement.PolicyAgent", QDBusConnection::sessionBus());
            if (powerDevil.isValid()) {
                QDBusReply<void> reply = powerDevil.call("ReleaseInhibition", m_inhibitCookie);
                if (reply.isValid()) {
                    mDebug() << "Successfully released suspend inhibition via KDE PowerDevil";
                    success = true;
                }
            }
        }
        
        m_inhibitCookie = 0;
    } else {
        // For systemd-logind, the file descriptor closes automatically when the process exits
        // or when we explicitly close it, so we just mark as released
        success = true;
        mDebug() << "Released suspend inhibition (systemd-logind file descriptor closed)";
    }

    m_inhibited = false;
    return success;
}

#endif // __linux

#ifdef __APPLE__

#include <IOKit/pwr_mgt/IOPMLib.h>

uint32_t SuspendInhibitor::m_assertionId = 0;

bool SuspendInhibitor::inhibit(const QString &reason)
{
    if (m_inhibited) {
        mDebug() << "Suspend inhibition already active";
        return true;
    }

    CFStringRef reasonCF = CFStringCreateWithCString(NULL, reason.toUtf8().constData(), kCFStringEncodingUTF8);
    IOReturn result = IOPMAssertionCreateWithName(kIOPMAssertionTypeNoIdleSleep, 
                                                  kIOPMAssertionLevelOn, 
                                                  reasonCF, 
                                                  &m_assertionId);
    CFRelease(reasonCF);

    if (result == kIOReturnSuccess) {
        m_inhibited = true;
        mDebug() << "Successfully inhibited suspend on macOS:" << reason;
        return true;
    } else {
        mWarning() << "Failed to inhibit suspend on macOS, error code:" << result;
        return false;
    }
}

bool SuspendInhibitor::release()
{
    if (!m_inhibited) {
        return true;
    }

    if (m_assertionId != 0) {
        IOReturn result = IOPMAssertionRelease(m_assertionId);
        if (result == kIOReturnSuccess) {
            mDebug() << "Successfully released suspend inhibition on macOS";
            m_assertionId = 0;
            m_inhibited = false;
            return true;
        } else {
            mWarning() << "Failed to release suspend inhibition on macOS, error code:" << result;
            return false;
        }
    }

    m_inhibited = false;
    return true;
}

#endif // __APPLE__

#ifdef _WIN32

#include <windows.h>

void* SuspendInhibitor::m_threadExecutionState = nullptr;

bool SuspendInhibitor::inhibit(const QString &reason)
{
    if (m_inhibited) {
        mDebug() << "Suspend inhibition already active";
        return true;
    }

    // Set thread execution state to prevent system sleep
    EXECUTION_STATE prevState = SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_AWAYMODE_REQUIRED);
    
    if (prevState != 0) {
        m_inhibited = true;
        mDebug() << "Successfully inhibited suspend on Windows:" << reason;
        return true;
    } else {
        DWORD error = GetLastError();
        mWarning() << "Failed to inhibit suspend on Windows, error code:" << error;
        return false;
    }
}

bool SuspendInhibitor::release()
{
    if (!m_inhibited) {
        return true;
    }

    // Restore normal execution state
    EXECUTION_STATE prevState = SetThreadExecutionState(ES_CONTINUOUS);
    
    if (prevState != 0) {
        mDebug() << "Successfully released suspend inhibition on Windows";
        m_inhibited = false;
        return true;
    } else {
        DWORD error = GetLastError();
        mWarning() << "Failed to release suspend inhibition on Windows, error code:" << error;
        m_inhibited = false; // Reset anyway
        return false;
    }
}

#endif // _WIN32

bool SuspendInhibitor::isInhibited()
{
    return m_inhibited;
} 