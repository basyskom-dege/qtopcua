/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt OPC UA module.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "opcuamethodnode.h"
#include "opcuanodeidtype.h"
#include <QOpcUaNode>
#include <QLoggingCategory>

QT_BEGIN_NAMESPACE

/*!
    \qmltype MethodNode
    \inqmlmodule QtOpcUa
    \brief Calls a method on the server.
    \since QtOpcUa 5.12
    \inherits Node

    This QML element supports calling method nodes on a server.
    The target object node ID has to be specified by the \l objectNodeId property.

    \code
    import QtOpcUa 5.13 as QtOpcUa

    QtOpcUa.MethodNode {
        nodeId : QtOpcUa.NodeId {
            identifier: "s=Example.Method"
            ns: "Example Namespace"
        }
        objectNodeId : QtOpcUa.NodeId {
            identifier: "s=Example.Object"
            ns: "Example Namespace"
        }
        connection: myConnection
     }
     \endcode

     The actual function call can be triggered by a signal.

     \code
     Button {
           text: "Start"
           onClicked: myNode.callMethod
     }
     \endcode

     or by JavaScript

     \code
          myNode.callMethod()
     \endcode
*/

/*!
    \qmlmethod MethodNode::callMethod

    Calls the method on the connected server.
*/

/*!
    \qmlproperty OpcUaNode MethodNode::objectNodeId

    Determines the actual node on which the method is called.
    It can be a relative or absolute node Id.
*/

/*!
    \qmlproperty QOpcUa::UaStatusCode MethodNode::resultStatusCode
    \readonly

    Status of the last method call. This property has to be checked
    to determine if the method call was successful.
    On success, the value is \c QtOpcua.Constants.Good.

    \sa QOpcUa::UaStatusCode
*/

/*!
    \qmlproperty list<MethodArgument> MethodNode::inputArguments

    Arguments to be used when calling the method on the server.

    This example shows how to call a method with two double arguments.
    \code
    QtOpcUa.MethodNode {
        ...
        inputArguments: [
            QtOpcUa.MethodArgument {
                value: 3
                type: QtOpcUa.Constants.Double
            },
            QtOpcUa.MethodArgument {
                value: 4
                type: QtOpcUa.Constants.Double
            }
        ]
    }
    \endcode

    \sa callMethod
*/

/*!
    \qmlproperty list<var> MethodNode::outputArguments
    \readonly

    Returns values from the method call. Depending on the output arguments,
    this list may contain zero or more values. The \l resultStatusCode has to be checked
    separately. In case the method call failed, the list will be empty.

    \code
    if (node.statusCode == QtOpcUa.Constants.Good) {
        // print two arguments
        console.log("Number of return values:", node.outputArguments.length)
        console.log("Return value #1:", node.outputArguments[0])
        console.log("Return value #2:", node.outputArguments[1])
    }
    \endcode

    \sa callMethod, resultStatusCode
*/

Q_DECLARE_LOGGING_CATEGORY(QT_OPCUA_PLUGINS_QML)

OpcUaMethodNode::OpcUaMethodNode(QObject *parent):
    OpcUaNode(parent)
{
}

OpcUaNodeIdType *OpcUaMethodNode::objectNodeId() const
{
    return m_objectNodeId;
}

QQmlListProperty<OpcUaMethodArgument> OpcUaMethodNode::inputArguments()
{
    return QQmlListProperty<OpcUaMethodArgument>(this,
        &m_inputArguments,
        &OpcUaMethodNode::appendArgument,
        &OpcUaMethodNode::argumentCount,
        &OpcUaMethodNode::argument,
        &OpcUaMethodNode::clearArguments);
}

QVariantList OpcUaMethodNode::outputArguments()
{
    return m_outputArguments;
}

void OpcUaMethodNode::setObjectNodeId(OpcUaNodeIdType *node)
{
    if (m_objectNodeId)
        disconnect(m_objectNodeId);

    m_objectNodeId = node;
    connect(m_objectNodeId, &OpcUaNodeIdType::nodeChanged, this, &OpcUaMethodNode::handleObjectNodeIdChanged);
    handleObjectNodeIdChanged();
}

void OpcUaMethodNode::callMethod()
{
    if (!m_objectNode) {
        qCWarning(QT_OPCUA_PLUGINS_QML) << "No object node";
        setStatus(Status::InvalidObjectNode);
        return;
    }
    if (!m_objectNode->node()) {
        qCWarning(QT_OPCUA_PLUGINS_QML) << "Invalid object node";
        setStatus(Status::InvalidObjectNode);
        return;
    }
    if (!m_node) {
        qCWarning(QT_OPCUA_PLUGINS_QML) << "Invalid node Id";
        setStatus(Status::InvalidNodeId);
        return;
    }

    QVector<QOpcUa::TypedVariant> arguments;
    for (const auto item : qAsConst(m_inputArguments))
        arguments.push_back(QOpcUa::TypedVariant(item->value(), item->type()));
    m_objectNode->node()->callMethod(m_node->nodeId(), arguments);
}

void OpcUaMethodNode::handleObjectNodeIdChanged()
{
    m_objectNode->deleteLater();
    m_objectNode = new OpcUaNode(this);
    m_objectNode->setNodeId(m_objectNodeId);
    connect(m_objectNode, &OpcUaNode::readyToUseChanged, this, [this](){
        connect(m_objectNode->node(), &QOpcUaNode::methodCallFinished, this, &OpcUaMethodNode::handleMethodCallFinished, Qt::UniqueConnection);
    });

    emit objectNodeIdChanged();
}

void OpcUaMethodNode::handleMethodCallFinished(QString methodNodeId, QVariant result, QOpcUa::UaStatusCode statusCode)
{
    Q_UNUSED(methodNodeId);
    m_resultStatusCode = statusCode;

    m_outputArguments.clear();
    if (result.canConvert<QVariantList>())
        m_outputArguments = result.value<QVariantList>();
    else
        m_outputArguments.append(result);
    emit resultStatusCodeChanged(m_resultStatusCode);
    emit outputArgumentsChanged();
}

void OpcUaMethodNode::setupNode(const QString &absolutePath)
{
    OpcUaNode::setupNode(absolutePath);
}

bool OpcUaMethodNode::checkValidity()
{
    if (m_node->attribute(QOpcUa::NodeAttribute::NodeClass).value<QOpcUa::NodeClass>() != QOpcUa::NodeClass::Method) {
        setStatus(Status::InvalidNodeType);
        return false;
    }
    if (!m_objectNode || !m_objectNode->node()) {
        setStatus(Status::InvalidObjectNode);
        return false;
    }

    const auto objectNodeClass = m_objectNode->node()->attribute(QOpcUa::NodeAttribute::NodeClass).value<QOpcUa::NodeClass>();
    if (objectNodeClass != QOpcUa::NodeClass::Object && objectNodeClass != QOpcUa::NodeClass::ObjectType) {
        setStatus(Status::InvalidObjectNode, tr("Object node is not of type `Object' or `ObjectType'"));
        return false;
    }
    return true;
}

QOpcUa::UaStatusCode OpcUaMethodNode::resultStatusCode() const
{
    return m_resultStatusCode;
}

void OpcUaMethodNode::appendArgument(QQmlListProperty<OpcUaMethodArgument>* list, OpcUaMethodArgument* p) {
    reinterpret_cast< QVector<OpcUaMethodArgument*>* >(list->data)->append(p);
}

void OpcUaMethodNode::clearArguments(QQmlListProperty<OpcUaMethodArgument>* list) {
    reinterpret_cast< QVector<OpcUaMethodArgument*>* >(list->data)->clear();
}

OpcUaMethodArgument* OpcUaMethodNode::argument(QQmlListProperty<OpcUaMethodArgument>* list, int i) {
    return reinterpret_cast< QVector<OpcUaMethodArgument*>* >(list->data)->at(i);
}

int OpcUaMethodNode::argumentCount(QQmlListProperty<OpcUaMethodArgument>* list) {
    return reinterpret_cast< QVector<OpcUaMethodArgument*>* >(list->data)->count();
}


QT_END_NAMESPACE
