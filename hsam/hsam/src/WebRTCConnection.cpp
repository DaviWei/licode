/*
 * WebRTCConnection.cpp
 *
 *  Created on: Mar 14, 2012
 *      Author: pedro
 */

#include "WebRTCConnection.h"
#include "sdpinfo.h"

#include <cstdio>
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <boost/thread.hpp>

WebRTCConnection::WebRTCConnection(): MediaReceiver() {
	// TODO Auto-generated constructor stub

	video_nice = new NiceConnection(VIDEO_TYPE,"video_rtp");
	video_nice_rtcp = new NiceConnection(VIDEO_TYPE,"video_rtcp");
	video_nice->setWebRTCConnection(this);
	video_nice_rtcp->setWebRTCConnection(this);
	video_srtp = new SrtpChannel();
	CryptoInfo crytp;
	crytp.cipher_suite=std::string("AES_CM_128_HMAC_SHA1_80");
	crytp.media_type= VIDEO_TYPE;
	std::string key = SrtpChannel::generateBase64Key();
	crytp.key_params = key;
	local_sdp.addCrypto(crytp);



}

WebRTCConnection::~WebRTCConnection() {
	if (video_nice)
		delete video_nice;
	if (video_nice_rtcp)
		delete video_nice_rtcp;
	if(video_srtp)
		delete video_srtp;
	// TODO Auto-generated destructor stub
}

bool WebRTCConnection::init(){
	video_nice->start();
	video_nice_rtcp->start();
	while (video_nice->state!=NiceConnection::CANDIDATES_GATHERED){
		sleep(1);
	}

	std::vector<CandidateInfo> *cands = video_nice->local_candidates;

	for (unsigned int it = 0; it<cands->size();it++ ){
		CandidateInfo cand = cands->at(it);
		local_sdp.addCandidate(cand);
	}
	while (video_nice_rtcp->state!=NiceConnection::CANDIDATES_GATHERED){
		sleep(1);
	}

	cands = video_nice_rtcp->local_candidates;

	for (unsigned int it = 0; it<cands->size();it++ ){
		CandidateInfo cand = cands->at(it);
		local_sdp.addCandidate(cand);
	}
	return true;
}
bool WebRTCConnection::setRemoteSDP(const std::string &sdp){
	remote_sdp.initWithSDP(sdp);
	video_nice->setRemoteCandidates(remote_sdp.getCandidateInfos());
	video_nice_rtcp->setRemoteCandidates(remote_sdp.getCandidateInfos());

	video_srtp->SetRtpParams((char*)local_sdp.getCryptoInfos().at(0).key_params.c_str(), (char*)remote_sdp.getCryptoInfos().at(0).key_params.c_str());
	video_nice->join();
	return true;
}
std::string WebRTCConnection::getLocalSDP(){
	return local_sdp.getSDP();
}
void WebRTCConnection::setAudioReceiver(MediaReceiver *receiv){
	this->audio_receiver = receiv;
}
void WebRTCConnection::setVideoReceiver(MediaReceiver *receiv){
	this->video_receiver = receiv;
}

int WebRTCConnection::receiveAudioData(char* buf, int len){
	int res=-1;
	int length= len;
	if (audio_srtp){
		length = audio_srtp->ProtectRtp(buf, len);
	}
	if (audio_nice){
		res= audio_nice->sendData(buf, length);
	}
	return res;
}
int WebRTCConnection::receiveVideoData(char* buf, int len){
	int res=-1;
	int length= len;
	if (video_srtp){
		length = video_srtp->ProtectRtp(buf, len);
	}
	if (video_nice){
		res= video_nice->sendData(buf, length);
	}
	return res;
}

int WebRTCConnection::receiveNiceData(char* buf, int len, NiceConnection* nice){
	boost::mutex::scoped_lock lock(write_mutex);
	printf("hola\n");
	int length = len;

	if(nice->media_type == AUDIO_TYPE){
		if (audio_receiver){
			if (audio_srtp){
				length = audio_srtp->UnprotectRtp(buf,len);
			}
			audio_receiver->receiveAudioData(buf, length);
			return length;
		}
	}
	else if(nice->media_type == VIDEO_TYPE){
		if (video_receiver){
			if (video_srtp){
				length = video_srtp->UnprotectRtp(buf,len);
			}
			//video_receiver->receiveVideoData(buf, length);
			return length;
		}
	}
	return -1;
}

/*
std::string WebRTCConnection::getLocalAddress(){
    struct ifaddrs * ifAddrStruct=NULL;
    struct ifaddrs * ifa=NULL;
    void * tmpAddrPtr=NULL;

    getifaddrs(&ifAddrStruct);

    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa ->ifa_addr->sa_family==AF_INET) { // check it is IP4
            // is a valid IP4 Address
            tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
            printf("%s IP Address %s\n", ifa->ifa_name, addressBuffer);
            if (!strcmp(ifa->ifa_name, "eth0")){
            	return std::string(addressBuffer);
            }

        }
    }
    if (ifAddrStruct!=NULL) freeifaddrs(ifAddrStruct);
    return 0;

}
*/



