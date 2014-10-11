

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
	clients[0].address = "localhost";
	clients[0].socket.connectTo("localhost", SENDPORT);
	clients[0].active = true;

	Message msg;
	PacketWriter pw;
	msg.init("/connected");
	pw.init();
	pw.startBundle().startBundle().addMessage(msg).endBundle().endBundle();
	clients[0].socket.sendPacket(pw.packetData(), pw.packetSize());

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
				// find empty client - check existing list
				int foundmatch = 0;
				for (int i = 0; i < clients.size(); i++)
				{
					if (clients[i].address == c) // already in list
					{
						clients[i].active = 1;
						foundmatch = 1;
						break;
					}
				}
				if (foundmatch == 0) // add new client to first free slot
				{
					for (int i = 0; i < clients.size(); i++)
					{
						if (clients[i].active == false) // add here
						{
							clients[i].address = c;
							clients[i].active = true;
							clients[i].socket.connectTo(c, SENDPORT);
							foundmatch = 1;
							break;
						}
					}
				}
				if (foundmatch = 0) // out of slots
				{
					std::string s = "Out of slots!!!\n";
					printFucker(s);

				}

			}
			
			else if (msg->match("/disconnect").isOkNoMoreArgs()) {
				for (int i = 0; i < clients.size(); i++)
				{
					if (clients[i].address == c) // shut it off
					{
						clients[i].active = false;
						break;
					}
				}
			}
			
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

	
	for (int cptr = 0; cptr < clients.size(); cptr++)
	{
		if (clients[cptr].active == 1) {
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

}


void CBodyBasics::CreateMap()
{
	j2i["basespine"]		= 0;
	j2i["spinebase"] = 0; // dup
	j2i["midspine"] = 1;
	j2i["spinemid"] = 1; // dup
	j2i["torso"] = 1; // dup
	j2i["neck"] = 2;
	j2i["head"]				= 3;
	j2i["leftshoulder"]		= 4;
	j2i["leftelbow"]		= 5;
	j2i["leftwrist"]		= 6;
	j2i["lefthand"]			= 7;
	j2i["rightshoulder"]	= 8;
	j2i["rightelbow"]		= 9;
	j2i["rightwrist"]		= 10;
	j2i["righthand"]		= 11;
	j2i["lefthip"]			= 12;
	j2i["leftknee"]			= 13;
	j2i["leftankle"]		= 14;
	j2i["leftfoot"]			= 15;
	j2i["righthip"]			= 16;
	j2i["rightknee"]		= 17;
	j2i["rightankle"]		= 18;
	j2i["rightfoot"]		= 19;
	j2i["shoulderspine"]	= 20;
	j2i["spineshoulder"] = 20; // dup
	j2i["lefthandtip"] = 21;
	j2i["leftthumb"]		= 22;
	j2i["righthandtip"]		= 23;
	j2i["rightthumb"]		= 24;
}
