#include "resource.h"
#define MAXSOCKETS 256

// UDP stuff!
#define OSCPKT_OSTREAM_OUTPUT
#include "oscpkt/oscpkt.hh"
#include "oscpkt/udp.hh"
using namespace oscpkt;


class KinectClient
{


public:
	std::string address;
	bool active;
	UdpSocket socket;
	KinectClient();
	KinectClient(std::string p_address);
	~KinectClient();
	void printFucker(std::string s);


};