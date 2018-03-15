/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// "liveMedia"
// Copyright (c) 1996-2011 Live Networks, Inc.  All rights reserved.
// A simple UDP source, where every UDP payload is a complete frame
// C++ header

#ifndef _BASIC_TCP_SOURCE_HH
#define _BASIC_TCP_SOURCE_HH

#ifndef _FRAMED_SOURCE_HH
#include "FramedSource.hh"
#endif
#ifndef _GROUPSOCK_HH
#include "Groupsock.hh"
#endif

#define TCP_MAX_READ			50000
#define TCP_MAX_MTU				1900
#define TCP_MAX_READ_COUNT		10	
#define TCP_RTP_HEADER_LEN		12

class BasicTCPSource: public FramedSource {
public:
  static BasicTCPSource* createNew(UsageEnvironment& env, int socket);

  virtual ~BasicTCPSource();

private:
  BasicTCPSource(UsageEnvironment& env, int socket);
      // called only by createNew()

  static void incomingPacketHandler(BasicTCPSource* source, int mask);
  void incomingPacketHandler1();

private: // redefined virtual functions:
  virtual void doGetNextFrame();
  virtual void doStopGettingFrames();

private:
  int readTCPSocket;
  Boolean fHaveStartedReading;
  unsigned char fReadBuffer[TCP_MAX_READ];
  unsigned char fReadLen[2];
  unsigned fReadSize;
  unsigned short fReadCount;
  unsigned short packetLength;
  Boolean haveRemainData;
  unsigned short remainDataLen;
  unsigned char remainData[TCP_MAX_MTU];
};

#endif
