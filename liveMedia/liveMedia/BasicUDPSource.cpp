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

#include "BasicUDPSource.hh"
#include <GroupsockHelper.hh>

#include <time.h>
#define USER_DELEY 150
#if 1
//For qian's new request: printf direct
typedef struct
{
	int sequence_num;
	unsigned long timestamp_in_header;
	double local_time;
}REC_DATA_T;
REC_DATA_T *prev_receive_time=NULL;
REC_DATA_T *curr_receive_time=NULL;
//For qian's new request: printf direct end
#endif
bool fHavepacketlost = False;
bool b_transporting_UDP = False;
bool b_data_finish_UDP = True;

BasicUDPSource* BasicUDPSource::createNew(UsageEnvironment& env,
					Groupsock* inputGS) {
  return new BasicUDPSource(env, inputGS);
}

BasicUDPSource::BasicUDPSource(UsageEnvironment& env, Groupsock* inputGS)
  : FramedSource(env), fInputGS(inputGS), fHaveStartedReading(False), fHaveSeenFirstPacket(False){
  // Try to use a large receive buffer (in the OS):
  increaseReceiveBufferTo(env, inputGS->socketNum(), 50*1024);

  // Make the socket non-blocking, even though it will be read from only asynchronously, when packets arrive.
  // The reason for this is that, in some OSs, reads on a blocking socket can (allegedly) sometimes block,
  // even if the socket was previously reported (e.g., by "select()") as having data available.
  // (This can supposedly happen if the UDP checksum fails, for example.)
  makeSocketNonBlocking(fInputGS->socketNum());
  b_transporting_UDP = False;
  b_data_finish_UDP = True;
}

BasicUDPSource::~BasicUDPSource(){
  envir().taskScheduler().turnOffBackgroundReadHandling(fInputGS->socketNum());
  b_transporting_UDP = False;
  b_data_finish_UDP = True;
}

void BasicUDPSource::doGetNextFrame() {
  if (!fHaveStartedReading) {
    // Await incoming packets:
    envir().taskScheduler().turnOnBackgroundReadHandling(fInputGS->socketNum(),
	 (TaskScheduler::BackgroundHandlerProc*)&incomingPacketHandler, this);
    fHaveStartedReading = True;
  }
}

void BasicUDPSource::doStopGettingFrames() {
  envir().taskScheduler().turnOffBackgroundReadHandling(fInputGS->socketNum());
  b_transporting_UDP = True;
  b_data_finish_UDP = True;
  fHaveStartedReading = False;
}


void BasicUDPSource::incomingPacketHandler(BasicUDPSource* source, int /*mask*/){
  b_data_finish_UDP = False;
  source->incomingPacketHandler1();
}

void BasicUDPSource::incomingPacketHandler1() {
  if (!isCurrentlyAwaitingData()) return; // we're not ready for the data yet
  
  if (b_transporting_UDP) {
		return; // reject UDP data when in UDP --> TCP processing
	}

  // Read the packet into our desired destination:
  struct sockaddr_in fromAddress;
#ifndef NEED_BUFFER_POOL
  unsigned char arHeader[RTP_HEADER_LEN] = {0,};
  unsigned char* fTmpBuffer = fTo;
  struct timeval	local_time_print;
  fFrameSize = fReadCount = 0;

  static unsigned short dataNo = 0 ;
  do
  {

		if (!fInputGS->handleRead(fTmpBuffer, fMaxSize - fFrameSize, fReadSize, fromAddress))
			{
              // printf("BasicUDPSource....handleRead fail return.....\n");
			   return;
			}

		//printf("fReadSize = %u\n", fReadSize);

		if (fReadCount == 0 && fReadSize == 0)
			{
			 // printf("BasicUDPSource....fReadCount == 0 && fReadSize == 0 return.....\n");
			  return;
			}

		if (fReadSize == 0)
		{
			//printf("read 0, fReadCount = %u, fFrameSize = %u\n", fReadCount, fFrameSize);
			break;
		}

		unsigned short fSeqNo = ntohs(*(uint16_t*)(fTmpBuffer + 2));
		
		//CLI

	    //For qian's new request: printf direct
#if 1
		
		#ifdef NEED_BUFFER_POOL	
			unsigned long fTimeStamp32 = ntohl(*(uint32_t*)(fReadBuffer + 4));
		#else
			unsigned long fTimeStamp32 = ntohl(*(uint32_t*)(fTmpBuffer + 4));
		#endif	
		
		//struct timeval	local_time_print;
		gettimeofday(&local_time_print,NULL);		
		double stardard_time= (double)USER_DELEY/1000.0;
		
		if(NULL==prev_receive_time)
	 	{
#if 0
	 		printf("[mtk94104] TV_WFD [prev_ts:curr_ts]  [prev_sq:curr_sq]  [delay( curr_time-prev_time) > Stardand(%f)]\n",stardard_time);
#endif
            fprintf(stderr, "[mtk94104] TV_WFD [prev_ts:curr_ts]  [prev_sq:curr_sq]  [delay( curr_time-prev_time) > Stardand(%f)]\n",stardard_time);
            fflush(stderr);
			prev_receive_time = (REC_DATA_T *)malloc(sizeof(REC_DATA_T));
			curr_receive_time = (REC_DATA_T *)malloc(sizeof(REC_DATA_T));
			prev_receive_time->local_time=local_time_print.tv_sec + (double)local_time_print.tv_usec/1000000.0;;
			prev_receive_time->sequence_num=fSeqNo;
			prev_receive_time->timestamp_in_header=fTimeStamp32;			
		}

		curr_receive_time->local_time=local_time_print.tv_sec + (double)local_time_print.tv_usec/1000000.0;;
		curr_receive_time->sequence_num=fSeqNo;
		curr_receive_time->timestamp_in_header=fTimeStamp32;	
		
		double diff_time=curr_receive_time->local_time - prev_receive_time->local_time;
		
		if(diff_time > stardard_time)
#if 0
			printf("[mtk94104]TV_WFD [%ld:%ld][%d:%d][%f(%f-%f)]\n",
				prev_receive_time->timestamp_in_header,
				curr_receive_time->timestamp_in_header,
				prev_receive_time->sequence_num,
				curr_receive_time->sequence_num,
				diff_time,
				curr_receive_time->local_time,
				prev_receive_time->local_time);
#endif 		
		fprintf(stderr, "[mtk94104]TV_WFD [%ld:%ld][%d:%d][%f(%f-%f)]\n",
				prev_receive_time->timestamp_in_header,
				curr_receive_time->timestamp_in_header,
				prev_receive_time->sequence_num,
				curr_receive_time->sequence_num,
				diff_time,
				curr_receive_time->local_time,
				prev_receive_time->local_time);
        fflush(stderr);

		prev_receive_time->local_time          =curr_receive_time->local_time;
		prev_receive_time->sequence_num        =curr_receive_time->sequence_num;
		prev_receive_time->timestamp_in_header =curr_receive_time->timestamp_in_header;	
#endif
	//For qian's new request END
		if (!fHaveSeenFirstPacket)
	    {
			fNextExpectedSeqNo = fSeqNo + 1;
			fHaveSeenFirstPacket = True;
	    }
		else
		{
			if (fNextExpectedSeqNo != fSeqNo)
			{
				int diff = (fSeqNo - fNextExpectedSeqNo + USHORT_MAX - 1) % (USHORT_MAX -1);
#if 0
				printf("UDP Recv SeqNo = %u, Expect SeqNo = %u, Packet lost = %d\n",
						  fSeqNo, fNextExpectedSeqNo, diff);
#endif
				fprintf(stderr, "UDP Recv SeqNo = %u, Expect SeqNo = %u, Packet lost = %d\n",
						  fSeqNo, fNextExpectedSeqNo, diff);
				fflush(stderr);
				fHavepacketlost = True;
			}
			fNextExpectedSeqNo = fSeqNo + 1;
		}
			  
		*(rtp_header_len_t*)fTmpBuffer = *(rtp_header_len_t*)arHeader;
		fTmpBuffer += (fReadSize - RTP_HEADER_LEN);
		fFrameSize += (fReadSize - RTP_HEADER_LEN);
		*(rtp_header_len_t*)arHeader = *(rtp_header_len_t*)fTmpBuffer;

		
		
		if((fReadCount == 0)&& ((dataNo%1000) == 0) )
		{
		    fprintf(stderr, "[First]BasicUDPSource....send data..No = %d ,fReadCount = %d.Time:(%lu.%03lu)s.\n",dataNo,fReadCount,local_time_print.tv_sec, local_time_print.tv_usec/1000);
			fflush(stderr);
		}
		if (++fReadCount > MAX_READ_COUNT)
		{
			//printf("read 10ts, fReadCount = %u, fFrameSize = %u\n", fReadCount, fFrameSize);		
			break;
		}
				
	}while(1);
#else

  if (!fInputGS->handleRead(fTo, fMaxSize, fFrameSize, fromAddress)) return;
  printf("BasicUDPSource....read 1ts.....\n");
#endif		
  //if (!fInputGS->handleRead(fTo, fMaxSize, fFrameSize, fromAddress)) return;
 // printf("BasicUDPSource read fFrameSize = %u\n", fFrameSize);
  // Tell our client that we have new data:
  if( (dataNo++%1000) == 0) 
  {
      gettimeofday(&local_time_print,NULL);
      fprintf(stderr, "[DBG]BasicUDPSource....No = %d .Time:(%lu.%03lu)s.\n",(dataNo-1),local_time_print.tv_sec, local_time_print.tv_usec/1000);
  }
  afterGetting(this); // we're preceded by a net read; no infinite recursion
}
