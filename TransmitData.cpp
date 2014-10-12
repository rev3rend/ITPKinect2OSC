

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

			//
			// parse messages
			//

			// CONNECT
			if (msg->match("/connect").isOkNoMoreArgs()) {
				// find empty client - check existing list
				int foundmatch = 0;
				for (int i = 0; i < clients.size(); i++)
				{
					if (clients[i].address == c) // already in list
					{
						printFucker("RECONNECTING TO HOST " + c + ":\n");
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
							printFucker("ADDING CONNECTION TO HOST " + c + ":\n");
							clients[i].address = c;
							clients[i].active = true;
							clients[i].socket.connectTo(c, SENDPORT);
							foundmatch = 1;
							break;
						}
					}
				}
				if (foundmatch == 0) // out of slots
				{
					std::string s = "Out of slots!!!\n";
					printFucker(s);

				}

			}

			// DISCONNECT
			else if (msg->match("/disconnect").isOkNoMoreArgs()) {
				for (int i = 0; i < clients.size(); i++)
				{
					if (clients[i].address == c) // shut it off
					{
						printFucker("DISCONNECTING FROM HOST " + c + ":\n");
						clients[i].active = false;
						break;
					}
				}
			}

			// ADDQUERY
			else if (msg->match("/addQuery")){
				Message::ArgReader arg(msg->arg()); // reader for arguments
				KinectQuery kq; // new query obect

				if (arg.isStr()) // first value should be string
				{
					std::string firstString;
					arg.popStr(firstString);
					// figure out query type
					if (firstString == "joint") kq.type = 0;
					else if (firstString == "trigger") kq.type = 1;
					else if (firstString == "toggle") kq.type = 2;
					else if (firstString == "controller") kq.type = 3;
					else if (firstString == "motion") kq.type = 4;
					printFucker("query type: " + std::to_string(kq.type) + "\n");
				}

				// parse remaining args based on type

				// joint
				if (kq.type == 0)
					// we want either:
					// string... stringn int string (enumerated joints)
					// or
					// int string (all joints)
				{
					// check first arg:
					if (arg.isInt32()) { // first arg is int: omni mode
						kq.omni = 1; // omni: send all joints each frame
						int p; arg.popInt32(p);
						kq.mode = p; // 0 (world) or 1 (body) orientation for output

						// check second arg
						if (arg.isStr()) { // in omni mode, next arg should be a string
							std::string p; arg.popStr(p);
							kq.message = p; // message for OSC argument with joint data inside
							kq.valid = 1; // valid query
						}
					}

					else if (arg.isStr()) { // first arg is string: individual joints
						kq.omni = 0;
						std::string p; arg.popStr(p);
						if (j2i.count(p) == 1) kq.joints.push_back(j2i[p]); // first joint to use
						while (arg.nbArgRemaining()) { // iterate until int
							if (arg.isStr()) {
								std::string f; arg.popStr(f); // another joint
								if(j2i.count(f)==1) kq.joints.push_back(j2i[f]);
							}
							else {
								break; // get outta here
							}
						}
						// check next-to-last arg:
						if (arg.isInt32()) { // first arg is int: omni mode
							int f; arg.popInt32(f);
							kq.mode = f; // 0 (world) or 1 (body) orientation for output

							// check last arg
							if (arg.isStr()) { // in omni mode, next arg should be a string
								std::string f; arg.popStr(f);
								kq.message = f; // message for OSC argument with joint data inside
								kq.valid = 1; // valid query
							}

						}
					}

					if (kq.valid == 1)
					{
						// DEBUG
						printFucker("ADDING JOINT QUERY for host " + c + ":\n");
						printFucker("OMNI: " + std::to_string(kq.omni) + " ");
						if (kq.omni == 0)
						{
							printFucker("JOINTS: ");
							for (int j = 0; j < kq.joints.size(); j++)
							{
								printFucker(std::to_string(kq.joints[j]) + " ");
							}
						}
						printFucker("MODE: " + std::to_string(kq.mode) + " ");
						printFucker("MESSAGE: " + kq.message + "\n");

						// PUSH ONTO STACK
						for (int i = 0; i < clients.size(); i++)
						{
							if (clients[i].address == c && clients[i].active == true) // add here
							{
								// avoid duplicate queries by message
								int ismatched = -1;
								for (int j = 0; j < clients[i].query.size(); j++)
								{
									if (clients[i].query[j].message == kq.message) // match
									{
										ismatched = j;
										break;
									}
								}
								if (ismatched == -1) // new message 
								{
									clients[i].query.push_back(kq);
									printFucker("ADDED QUERY SLOT: " + std::to_string(clients[i].query.size() - 1));
									break;
								}
								else // replace old message in queue with same query name
								{
									clients[i].query[ismatched] = kq;
									printFucker("REPLACED QUERY SLOT: " + std::to_string(ismatched));
									break;
								}
							}
						}

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
			//printFucker("sending to client " + clients[cptr].address + ": " + std::to_string(nBodyCount) + " bodies!\n");

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
