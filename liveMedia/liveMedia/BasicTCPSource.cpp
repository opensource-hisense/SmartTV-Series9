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
// Implementation

#include <unistd.h>
//#include <stropts.h>
//#include <termios.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include "BasicTCPSource.hh"
#include <GroupsockHelper.hh>
#include <time.h>

static int dataNo = 0 ;
struct timeval local_time_print;

BasicTCPSource* BasicTCPSource::createNew(UsageEnvironment& env,
					int socket) {
  return new BasicTCPSource(env, socket);
}

BasicTCPSource::BasicTCPSource(UsageEnvironment& env, int socket)
  : FramedSource(env), readTCPSocket(socket), fHaveStartedReading(False),
  fReadSize(0), packetLength(0), haveRemainData(False), remainDataLen(0) {
  // Try to use a large receive buffer (in the OS):
  increaseReceiveBufferTo(env, readTCPSocket, 104*1024);

  // Make the socket non-blocking, even though it will be read from only asynchronously, when packets arrive.
  // The reason for this is that, in some OSs, reads on a blocking socket can (allegedly) sometimes block,
  // even if the socket was previously reported (e.g., by "select()") as having data available.
  // (This can supposedly happen if the UDP checksum fails, for example.)

  //makeSocketNonBlocking(readTCPSocket);
}

BasicTCPSource::~BasicTCPSource(){
  envir().taskScheduler().turnOffBackgroundReadHandling(readTCPSocket);
  //::close(readTCPSocket);  //ruicheng.he modify for close tcp later  --> transportThread() in wfdrtspclient.cpp
}

void BasicTCPSource::doGetNextFrame() {
  if (!fHaveStartedReading) {
  	//fprintf(stderr, "Enter into %s-->%d\n", __FUNCTION__, __LINE__);
    // Await incoming packets:
    envir().taskScheduler().turnOnBackgroundReadHandling(readTCPSocket,
	 (TaskScheduler::BackgroundHandlerProc*)&incomingPacketHandler, this);
    fHaveStartedReading = True;
  }
}

void BasicTCPSource::doStopGettingFrames() {
  envir().taskScheduler().turnOffBackgroundReadHandling(readTCPSocket);
  fHaveStartedReading = False;
}


void BasicTCPSource::incomingPacketHandler(BasicTCPSource* source, int /*mask*/){
	//fprintf(stderr, "Enter into %s-->%d\n", __FUNCTION__, __LINE__);
  source->incomingPacketHandler1();
}

void BasicTCPSource::incomingPacketHandler1() {
	//fprintf(stderr, "Enter into %s-->%d\n", __FUNCTION__, __LINE__);
  if (!isCurrentlyAwaitingData()) return; // we're not ready for the data yet

  // Read the packet into our desired destination:
  struct sockaddr_in fromAddress;
  int unReadSize;
  unsigned char* bufferPtr;
  fFrameSize = fReadCount = 0;
  //Read length part
  ioctl(readTCPSocket, FIONREAD, &unReadSize);
  if (unReadSize >= 2)
  {
	fReadSize = readSocket(envir(), readTCPSocket, fReadBuffer, TCP_MAX_READ, fromAddress);

	if (fReadSize <= 0)
	{
		printf("%s --> %d, read size = %d \n", __FUNCTION__, __LINE__, fReadSize);
		return;
	}
  }
		
  bufferPtr = fReadBuffer;

  if (haveRemainData)
  {
    if (fReadSize < (packetLength - remainDataLen))
    {
		memcpy(remainData + remainDataLen, bufferPtr, fReadSize);
		bufferPtr += fReadSize;
		remainDataLen += fReadSize;
    }
    else
    {
		//Copy remain data
		if (remainDataLen >= TCP_RTP_HEADER_LEN)
		{
			memcpy(fTo + fFrameSize, remainData + TCP_RTP_HEADER_LEN, remainDataLen - TCP_RTP_HEADER_LEN);
			fFrameSize += (remainDataLen - TCP_RTP_HEADER_LEN);
		}
		else
		{
			bufferPtr += (TCP_RTP_HEADER_LEN - remainDataLen);
			remainDataLen = TCP_RTP_HEADER_LEN;
		}
		//Copy unread data
		memcpy(fTo + fFrameSize, bufferPtr, packetLength - remainDataLen);
		fFrameSize += (packetLength - remainDataLen);
		bufferPtr += (packetLength - remainDataLen);
		remainDataLen = 0;
		haveRemainData = False;
	}
  }

  while (bufferPtr < (fReadBuffer + fReadSize))
  {
  	fReadLen[0] = *(bufferPtr++);
	fReadLen[1] = *(bufferPtr++);
    packetLength = ntohs(*(uint16_t*)(fReadLen));
	//printf(" packetLength = %u\n", packetLength);

	if ((bufferPtr + packetLength) < (fReadBuffer + fReadSize))
	{
		memcpy(fTo + fFrameSize, bufferPtr + TCP_RTP_HEADER_LEN, packetLength - TCP_RTP_HEADER_LEN);
		fFrameSize += (packetLength - TCP_RTP_HEADER_LEN);
		bufferPtr += packetLength;
	}
	else
	{
		memcpy(remainData, bufferPtr, fReadBuffer + fReadSize - bufferPtr);
		remainDataLen = fReadBuffer + fReadSize - bufferPtr;
		haveRemainData = True;
		bufferPtr = fReadBuffer + fReadSize;
	}
  }

  if (fFrameSize == 0)
  	return;

  if (fFrameSize < 0) {
      envir().setResultMsg("Groupsock read failed: ",
			 envir().getResultMsg());
	  //return False;
    }

  // Tell our client that we have new data:
  if( (dataNo++%1000) == 0) 
  {
      gettimeofday(&local_time_print,NULL);
      fprintf(stderr, "[DBG]BasicTCPSource....No = %d .\n",(dataNo-1));
  }
 // fprintf(stderr, "[DBG]BasicTCPSource....fFrameSize = %d ,fReadSize=%d.packetLength=%d \n",fFrameSize, fReadSize, packetLength);
  afterGetting(this); // we're preceded by a net read; no infinite recursion
}
