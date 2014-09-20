

#include <iostream>   // std::cout
#include <string>     // std::string, std::to_string
#include "stdafx.h"
#include <strsafe.h>
#include "resource.h"

#include "BodyBasics.h"


void CBodyBasics::StartOSC()
{
	// UDP stuff
	// RECEIVER
	recvsock.bindTo(RECVPORT);
	if (!recvsock.isOk()) {
		std::string s = "Error opening port " + std::to_string(RECVPORT) + ": " + recvsock.errorMessage() + "\n";
		printFucker(s);
	}
	else {
		std::string s = "Server started, will listen to packets on port " + std::to_string(RECVPORT) + "\n";
		printFucker(s);
	}


	// INITAL SENDER

	//sock.connectTo("localhost", 8000);
	kptr = 0;
	clients[kptr].address = "localhost";
	clients[kptr].socket.connectTo("localhost", SENDPORT);

	Message msg;
	PacketWriter pw;
	msg.init("/baz");
	pw.init();
	pw.startBundle().startBundle().addMessage(msg).endBundle().endBundle();
	clients[kptr].socket.sendPacket(pw.packetData(), pw.packetSize());
	kptr++;

}

void CBodyBasics::ParseOSC()
{

	// UDP stuff
	PacketReader pr;
	PacketWriter pw;

	if (recvsock.isOk() && recvsock.receiveNextPacket(1 /* timeout, in ms */)) {
		pr.init(recvsock.packetData(), recvsock.packetSize());
		oscpkt::Message *msg;
		while (pr.isOk() && (msg = pr.popMessage()) != 0) {
			int iarg;

			// parse IP address
			std::string c = recvsock.packetOrigin().asString();
			std::string delim = ":";
			size_t pos = 0;
			std::string token;
			while ((pos = c.find(delim)) != std::string::npos) {
				token = c.substr(0, pos);
				c = token;
			}
			std::string s = "Server: received message from " + c + "\n";
			printFucker(s);

			if (msg->match("/connect").isOkNoMoreArgs()) {
				// reconnect on new IP address
				clients[kptr].address = c;
				clients[kptr].socket.connectTo(c, SENDPORT);
				kptr++;

			}
			/*
			else if (msg->match("/disconnect").isOkNoMoreArgs()) {
				for (int i = 0; i < kptr; i++)
				{
					if (clients[i].address == c)
					{
						clients.erase(clients.begin() + i);
						break;
					}
				}
			}*/
			
			else {
				std::string s = "Server: unhandled message! \n";
				printFucker(s);
			}
		}
	}
}

void CBodyBasics::TransmitBody(INT64 nTime, int nBodyCount, IBody** ppBodies)
{
	// UDP
	Message msg;
	PacketWriter pw;
	bool ok;
	HRESULT hr;

	
	for (int cptr = 0; cptr < kptr; cptr++)
	{
		printFucker("sending to client " + clients[cptr].address + ": " + std::to_string(nBodyCount) + " bodies!\n");

		// SEND FRAME START OVER UDP
		msg.init("/beginFrame");
		msg.pushInt32(nBodyCount);
		pw.init();
		pw.startBundle().startBundle().addMessage(msg).endBundle().endBundle();
		ok = clients[cptr].socket.sendPacket(pw.packetData(), pw.packetSize());


		for (int i = 0; i < nBodyCount; ++i)
		{
			IBody* pBody = ppBodies[i];
			if (pBody)
			{
				BOOLEAN bTracked = false;
				hr = pBody->get_IsTracked(&bTracked);

				if (SUCCEEDED(hr) && bTracked)
				{
					Joint joints[JointType_Count];
					D2D1_POINT_2F jointPoints[JointType_Count];
					HandState leftHandState = HandState_Unknown;
					HandState rightHandState = HandState_Unknown;

					pBody->get_HandLeftState(&leftHandState);
					pBody->get_HandRightState(&rightHandState);

					hr = pBody->GetJoints(_countof(joints), joints);
					if (SUCCEEDED(hr))
					{
						msg.init("/beginBody");
						msg.pushInt32(i);
						pw.init();
						pw.startBundle().startBundle().addMessage(msg).endBundle().endBundle();
						ok = clients[cptr].socket.sendPacket(pw.packetData(), pw.packetSize());

						for (int j = 0; j < _countof(joints); ++j)
						{
							// /kinect body joint x y z
							msg.init("/bodyJoint");
							msg.pushInt32(i);
							msg.pushInt32(j);
							// body relative - joints[1] is spineMid which maps to Torso in OpenNI
							msg.pushFloat(joints[j].Position.X - joints[1].Position.X);
							msg.pushFloat(joints[j].Position.Y - joints[1].Position.Y);
							msg.pushFloat(joints[j].Position.Z - joints[1].Position.Z);
							// send message
							pw.init();
							pw.startBundle().startBundle().addMessage(msg).endBundle().endBundle();
							ok = clients[cptr].socket.sendPacket(pw.packetData(), pw.packetSize());

						}

						msg.init("/endBody");
						msg.pushInt32(i);
						pw.init();
						pw.startBundle().startBundle().addMessage(msg).endBundle().endBundle();
						ok = clients[cptr].socket.sendPacket(pw.packetData(), pw.packetSize());
					}


				}
			}
		}


		// SEND FRAME END OVER UDP
		msg.init("/endFrame");
		pw.init();
		pw.startBundle().startBundle().addMessage(msg).endBundle().endBundle();
		ok = clients[cptr].socket.sendPacket(pw.packetData(), pw.packetSize());


	}

}


