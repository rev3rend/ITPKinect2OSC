#include "resource.h"
#define MAXSOCKETS 256

// UDP stuff!
#define OSCPKT_OSTREAM_OUTPUT
#include "oscpkt/oscpkt.hh"
#include "oscpkt/udp.hh"
using namespace oscpkt;



class KinectQuery
{
public:
	// type: 0 = joint, 1 = trigger, 2 = toggle, 3 = controller, 4 = motion
	int type;
	std::string message;
	int mode; // for joint, position (world or body)
	int omni; // for joint, all joints or just some
	std::vector<int> joints; // for joint... list of joints
	int j1;
	int j2; // for trigger, toggle, controller
	int polarity; // for trigger, toggle
	int axis; // for controller, motion
	int isunsigned; // for controller, motion
	float threshold; // for trigger, toggle
	float debounce; //for trigger
	float min; // for controller
	float max; // for controller
	int valid; // valid query?
	KinectQuery();
	~KinectQuery();

};


class KinectClient
{


public:
	std::string address;
	bool active;
	UdpSocket socket;
	std::vector<KinectQuery> query;
	KinectClient();
	KinectClient(std::string p_address);
	~KinectClient();
	void printFucker(std::string s);


};
