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

#include "suspendmanager.h"
#include "suspendinhibitor.h"
#include "utilities.h"

SuspendManager* SuspendManager::m_instance = nullptr;

SuspendManager* SuspendManager::instance()
{
    if (!m_instance) {
        m_instance = new SuspendManager();
    }
    return m_instance;
}

SuspendManager::SuspendManager(QObject *parent) : QObject(parent)
{
}

void SuspendManager::registerOperation(const QString &operationId, const QString &description)
{
    mDebug() << "Registering operation:" << operationId << "-" << description;
    
    bool wasEmpty = m_activeOperations.isEmpty();
    m_activeOperations.insert(operationId);
    
    if (wasEmpty) {
        m_currentReason = description;
        updateSuspendInhibition();
    }
}

void SuspendManager::unregisterOperation(const QString &operationId)
{
    mDebug() << "Unregistering operation:" << operationId;
    
    m_activeOperations.remove(operationId);
    
    if (m_activeOperations.isEmpty()) {
        updateSuspendInhibition();
    }
}

bool SuspendManager::hasActiveOperations() const
{
    return !m_activeOperations.isEmpty();
}

void SuspendManager::updateSuspendInhibition()
{
    if (hasActiveOperations()) {
        if (!SuspendInhibitor::isInhibited()) {
            SuspendInhibitor::inhibit(m_currentReason);
        }
    } else {
        if (SuspendInhibitor::isInhibited()) {
            SuspendInhibitor::release();
        }
    }
} 