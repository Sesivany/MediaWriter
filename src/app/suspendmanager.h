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

#ifndef SUSPENDMANAGER_H
#define SUSPENDMANAGER_H

#include <QObject>
#include <QSet>

/**
 * @brief The SuspendManager class
 * 
 * Centralized manager for tracking long-running operations and managing
 * suspend inhibition. This ensures suspend is only inhibited when needed
 * and properly released when all operations complete.
 */
class SuspendManager : public QObject
{
    Q_OBJECT
public:
    static SuspendManager* instance();
    
    /**
     * @brief Register a long-running operation
     * @param operationId Unique identifier for the operation
     * @param description Human-readable description of the operation
     */
    void registerOperation(const QString &operationId, const QString &description);
    
    /**
     * @brief Unregister a completed operation
     * @param operationId Unique identifier for the operation
     */
    void unregisterOperation(const QString &operationId);
    
    /**
     * @brief Check if any operations are currently active
     * @return true if any operations are registered
     */
    bool hasActiveOperations() const;

private:
    SuspendManager(QObject *parent = nullptr);
    void updateSuspendInhibition();
    
    static SuspendManager *m_instance;
    QSet<QString> m_activeOperations;
    QString m_currentReason;
};

#endif // SUSPENDMANAGER_H 