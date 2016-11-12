//
// pedsim - A microscopic pedestrian simulation system.
// Copyright (c) by Christian Gloor
//

#include "itemcontainer.h"
#include "item.h"
#include "itemagent.h"
#include "itemobstacle.h"
#include "globals.h"

#include "serverstream.h"
#include "iostream"
#include "sstream"

#include <QObject>
#include <QHostAddress>
#include <QTcpSocket>
#include <QMutex>
#include <QMutexLocker>

#include <QtGlobal>
#include <Qt3DCore/QEntity>
#include <Qt3DRender/QCamera>
#include <Qt3DRender/QCameraLens>

using namespace std;

QString host = "localhost";
int port = 2323;

extern Qt3DCore::QEntity *scene;
extern Qt3DRender::QCamera *camera;

extern ItemContainer agentcontainer;
extern ItemContainer obstaclecontainer;


ServerStream::ServerStream(QObject* parent = 0) : QObject(parent) {

  m_mutex = new QMutex(QMutex::Recursive);
  m_socket = new QTcpSocket();

  connect(m_socket, SIGNAL(connected()), this, SIGNAL(connected()));
  connect(m_socket, SIGNAL(disconnected()), this, SIGNAL(disconnected()));
  connect(m_socket, SIGNAL(readyRead()), this, SLOT(processData()));
  connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(handleError(QAbstractSocket::SocketError)));
  connect(this, SIGNAL(welcomeReceived()), this, SLOT(sendWelcome()));
}


void ServerStream::open() {
  QMutexLocker locker(m_mutex);

  m_socket->connectToHost(host, port);
}


void ServerStream::close () {
  QMutexLocker locker(m_mutex);

  m_socket->disconnectFromHost();
  emit disconnected(); // NOTE: disconnectFromHost() should send this, but somehow it doesn't
}


void ServerStream::processData () {
  while (m_socket->bytesAvailable() > 4096) {
    char buffer[4096];
    int numRead = m_socket->read(buffer, 4096);
    buffer[numRead] = '\0';
    m_xmlReader.addData(buffer);
  }

  while (!m_xmlReader.atEnd()) {
    //    QMutexLocker locker(m_mutex);

    m_xmlReader.readNext();

    if (m_xmlReader.isStartElement()) {
      if (m_xmlReader.name() == "timestep") {
	QString value = m_xmlReader.attributes().value("value").toString();
	cout << "timestep " << value.toStdString() << endl;
	// Quote *q = new Quote();
	// q->setInstrumentId(m_xmlReader.attributes().value("instrument").toString().toInt());
	// q->setBid(m_xmlReader.attributes().value("bid").toString().toDouble());
	// q->setAsk(m_xmlReader.attributes().value("ask").toString().toDouble());
	// q->setLast(m_xmlReader.attributes().value("last").toString().toDouble());
	// q->setBids(m_xmlReader.attributes().value("bids").toString().toInt());
	// q->setAsks(m_xmlReader.attributes().value("asks").toString().toInt());

	// emit quoteReceived(q);

      } else if (m_xmlReader.name() == "welcome") {
	// emit welcomeReceived();

      } else if (m_xmlReader.name() == "position") { // <position id="249" type="agent" x="27.6718" y="-0.552733" z="6.0119"  />
	QString type = m_xmlReader.attributes().value("type").toString();
	QString id = m_xmlReader.attributes().value("id").toString();
	double x = m_xmlReader.attributes().value("x").toString().toDouble();
	double y = m_xmlReader.attributes().value("y").toString().toDouble();
	double z = m_xmlReader.attributes().value("z").toString().toDouble();

	if (type == "agent") {
	  if (!agentcontainer.contains(id)) {
	    Item *agent = new ItemAgent(scene);
	    agentcontainer.addItem(id, agent);
	  }
	  agentcontainer.updatePosition(id, x, y, z);
	}
	
	if (type == "camera") {
	  if (g_option_camera) {
	    if (id == g_option_camera_id) {
	      double rx = m_xmlReader.attributes().value("rx").toString().toDouble();
	      double ry = m_xmlReader.attributes().value("ry").toString().toDouble();
	      double rz = m_xmlReader.attributes().value("rz").toString().toDouble();
	      camera->setPosition(QVector3D(x, z, y));
	      camera->setViewCenter(QVector3D(x + rx, z + rz , y + ry));
	    }
	  }
	}


	
}



    // else if (m_xmlReader.isEndElement()) {
    //   if (m_xmlReader.name() == "position") {
    //  	this->inPosition = false;
    //   }
    // }
    }
  }
}


void ServerStream::handleError (QAbstractSocket::SocketError err) {
  emit socketError(err);
}

void ServerStream::sendWelcome () {
  QMutexLocker locker(m_mutex);

  stringstream ss;
  ss << "<welcome client=\"" << "3dvis\">";
  m_socket->write(ss.str().c_str());
}

void ServerStream::sendGoodBye () {
  QMutexLocker locker(m_mutex);

  stringstream ss;
  ss << "</welcome>";
  m_socket->write(ss.str().c_str());
}

// void ServerStream::sendLogin (QString username, QString password) {
//   QMutexLocker locker(m_mutex);

//   stringstream ss;
//   ss << "<login username=\"" << username.toStdString() << "\" password=\"" << password.toStdString() << "\" />";
//   m_socket->write(ss.str().c_str());
// }
