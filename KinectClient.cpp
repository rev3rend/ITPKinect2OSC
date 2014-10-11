#include <iostream>   // std::cout
#include <string>     // std::string, std::to_string
#include "stdafx.h"
#include <strsafe.h>
#include "resource.h"

#include "BodyBasics.h"

KinectClient::KinectClient()
{
	address = "localhost";
	active = false;


}


KinectClient::~KinectClient()
{

}


void KinectClient::printFucker(std::string s)
{
	std::wstring stemp = std::wstring(s.begin(), s.end());
	LPCWSTR sw = stemp.c_str();
	OutputDebugString(sw);
}

KinectQuery::KinectQuery()
{

}

KinectQuery::~KinectQuery()
{

}