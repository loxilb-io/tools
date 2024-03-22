#include "pfcp_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>

int debug = 0;

int udpReceive(int s, struct sockaddr_in *servaddr, char *buffer) {
    int n, len;
    fd_set fds; // will be checked for being ready to read
    FD_ZERO(&fds);
    FD_SET(s, &fds);

    // sets timeout
    struct timeval tv = { 0 };
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    int ret = select( s + 1, &fds, NULL, NULL, &tv );
    if (ret <= 0) {
        return 0;
    } else if(FD_ISSET( s, &fds )){
        if ((n = recvfrom(s, (char *)buffer, MAXLINE,
                        0, (struct sockaddr *) servaddr,
                        &len)) == -1) {
            return 0;
        }
        if (n < sizeof(struct pfcp_nhdr)) {
            printf("SIZE err\n");
            return 0;
        }
        return n;
    }
}
int udpSend(int sockfd, struct sockaddr_in *servaddr, void *msg, int len) {
    int n = 0;
    if (n = sendto(sockfd, msg, len,
                MSG_CONFIRM, (const struct sockaddr *) servaddr,
                sizeof(struct sockaddr_in)) < 0) {
        printf("udpSend failed: %d\n", n);
        return 0;
    }
    return 1;
}

void asReqResp(int sockfd, struct sockaddr_in *servaddr, 
               int seq, int node_ip, int *sent, int *recv) {
    struct pfcp_asreq  *asreq;
    struct pfcp_asres  *asres;
    struct pfcp_nhdr *nhdr;
    char buffer[MAXLINE];
    int n = 0;

    memset(buffer, 0, MAXLINE);
    nhdr = (struct pfcp_nhdr *)buffer;
    nhdr->ver = 1;
    nhdr->mp = 0;
    asreq = (void*) buffer;
    asreq->hdr.mt = PFCP_ASSOCIATION_SETUP_REQ;
    asreq->hdr.mlen = htons(sizeof(*asreq) - sizeof(struct pfcp_tlv));
    asreq->hdr.seq = htonl(seq << 8);

    /*Node ID*/
    asreq->node_id.tlv.type = htons(60);
    asreq->node_id.tlv.len = htons(sizeof(struct nodeid_ie) - sizeof(struct pfcp_tlv));
    asreq->node_id.type = 0;
    asreq->node_id.node_ip = node_ip;

    asreq->ts.tlv.type = htons(96);
    asreq->ts.tlv.len = htons(sizeof(struct rects_ie) - sizeof(struct pfcp_tlv));
    asreq->ts.tsv = htonl(seq);

    asreq->features.tlv.type = htons(89);
    asreq->features.tlv.len = htons(sizeof(struct feat_ie) - sizeof(struct pfcp_tlv));
    strcpy(asreq->features.data, "Function features");
    n = udpSend(sockfd, servaddr, (void *)asreq, sizeof(struct pfcp_asreq));
    if (!n) {
        printf("AS Req sent failed %u\n", ntohl(asreq->hdr.seq) >> 8);
        return;
    }
    if (debug) printf("AS Req sent seq %u\n", ntohl(asreq->hdr.seq) >> 8);
    (*sent)++;
    n = udpReceive(sockfd, servaddr, buffer);
    if (n) {
        nhdr = (struct pfcp_nhdr *)buffer;
        if (nhdr->mt !=  PFCP_ASSOCIATION_SETUP_RSP) {
            printf("Unexpected msg %u\n", nhdr->mt);
            return;
        }

        asres = (struct pfcp_asres *)buffer;
        if (debug) printf("AS resp recvd - seq %u\n",ntohl(asres->hdr.seq) >> 8);
        (*recv)++;
    }
}

void arReqResp(int sockfd, struct sockaddr_in *servaddr, 
               int seq, int node_ip, int *sent, int *recv) {
    struct pfcp_arreq  *arreq;
    struct pfcp_arres  *arres;
    struct pfcp_nhdr *nhdr;
    char buffer[MAXLINE];
    int n = 0;

    memset(buffer, 0, MAXLINE);
    nhdr = (struct pfcp_nhdr *)buffer;
    nhdr->ver = 3;
    nhdr->mp = 0;
    arreq = (void*) buffer;
    arreq->hdr.mt = PFCP_ASSOCIATION_REL_REQ;
    arreq->hdr.mlen = htons(sizeof(*arreq) - sizeof(struct pfcp_tlv));
    arreq->hdr.seq = htonl(seq << 8);

    /*Node ID*/
    arreq->node_id.tlv.type = htons(60);
    arreq->node_id.tlv.len = htons(sizeof(struct nodeid_ie) - sizeof(struct pfcp_tlv));
    arreq->node_id.type = 0;
    arreq->node_id.node_ip = node_ip;

    n = udpSend(sockfd, servaddr, (void *)arreq, sizeof(struct pfcp_arreq));
    if (!n) {
        printf("AR Req sent failed %u\n", ntohl(arreq->hdr.seq) >> 8);
        return;
    }
    if (debug) printf("AR Req sent seq %u\n", ntohl(arreq->hdr.seq) >> 8);
    (*sent)++;
    n = udpReceive(sockfd, servaddr, buffer);
    if (n) {
        nhdr = (struct pfcp_nhdr *)buffer;
        if (nhdr->mt !=  PFCP_ASSOCIATION_REL_RSP) {
            printf("Unexpected msg %u\n", nhdr->mt);
            return;
        }

        arres = (struct pfcp_arres *)buffer;
        if (debug) printf("AR resp recvd - seq %u\n",ntohl(arres->hdr.seq) >> 8);
        (*recv)++;
    }

}

void hbReqResp(int sockfd, struct sockaddr_in *servaddr, 
        int seq, int *sent, int *recv) {
    struct pfcp_hbreq  *hbreq;
    struct pfcp_hbres  *hbres;
    struct pfcp_nhdr *nhdr;
    char buffer[MAXLINE];
    int n = 0;
    memset(buffer, 0, MAXLINE);

    nhdr = (struct pfcp_nhdr *)buffer;
    nhdr->ver = 3;
    nhdr->mp = 0;

    hbreq = (void *)buffer;
    hbreq->hdr.mt = PFCP_HEARTBEAT_REQ;
    hbreq->hdr.mlen = htons(sizeof(*hbreq) - sizeof(struct pfcp_tlv));
    hbreq->hdr.seq = htonl(seq << 8);

    hbreq->ts.tlv.type = htons(96);
    hbreq->ts.tlv.len = htons(sizeof(struct rects_ie) - sizeof(struct pfcp_tlv));
    hbreq->ts.tsv = htonl(seq);

    n = udpSend(sockfd, servaddr, (void *)hbreq, sizeof(struct pfcp_arreq));
    if (!n) {
        printf("HB Req sent failed %u\n", ntohl(hbreq->hdr.seq) >> 8);
        return;
    }
    if (debug) printf("HB Req sent seq %u\n", ntohl(hbreq->hdr.seq) >> 8);
    (*sent)++;
    n = udpReceive(sockfd, servaddr, buffer);
    if (n) {
        nhdr = (struct pfcp_nhdr *)buffer;
        if (nhdr->mt !=  PFCP_HEARTBEAT_RSP) {
            printf("Unexpected msg %u\n", nhdr->mt);
            return;
        }

        hbres = (struct pfcp_hbres *)buffer;
        if (debug) printf("HB resp recvd - seq %u\n",ntohl(hbres->hdr.seq) >> 8);
        (*recv)++;
    }
}

void seReqResp(int sockfd, struct sockaddr_in *servaddr, 
        int seq, int node_ip, uint64_t seid, int *sent, int *recv) {
    struct pfcp_sereq *sereq;
    struct pfcp_seres *seres;
    struct pfcp_shdr *shdr;
    char buffer[MAXLINE];
    int n = 0;
    memset(buffer, 0, MAXLINE);

    shdr = (struct pfcp_shdr *)buffer;
    shdr->ver = 3;
    shdr->mp = 0;
    shdr->sbit = 0;

    sereq = (void *)buffer;
    sereq->hdr.seq = htonl(seq << 8);

    sereq->hdr.mt = PFCP_SESSION_EST_REQ;
    sereq->hdr.mlen = htons(sizeof(*sereq) - sizeof(struct pfcp_tlv));

    /*Node ID*/
    sereq->node_id.tlv.type = htons(60);
    sereq->node_id.tlv.len = htons(sizeof(struct nodeid_ie) - sizeof(struct pfcp_tlv));
    sereq->node_id.type = 0;
    sereq->node_id.node_ip = node_ip;

    /*F-SEID*/
    sereq->fseid.tlv.type = htons(57);
    sereq->fseid.tlv.len = htons(sizeof(struct fseid_ie) - sizeof(struct pfcp_tlv));
    sereq->fseid.v4 = 1;
    sereq->fseid.seid = htobe64(seid);
    sereq->fseid.node_ip = node_ip;

    n = udpSend(sockfd, servaddr, (void *)sereq, sizeof(struct pfcp_sereq));
    if (!n) {
        printf("SE Req sent failed %u\n", ntohl(sereq->hdr.seq) >> 8);
        return;
    }
    if (debug) printf("SE sent seq %u f-seid %lu.\n", ntohl(sereq->hdr.seq) >> 8, be64toh(sereq->fseid.seid));
    (*sent)++;
    n = udpReceive(sockfd, servaddr, buffer);
    if (n) {
        shdr = (struct pfcp_shdr *)buffer;
        if (shdr->mt !=  PFCP_SESSION_EST_RSP) {
            printf("Unexpected msg %u\n", shdr->mt);
            return;
        }

        seres = (struct pfcp_seres *)buffer;
        if (debug)
            printf("SE resp recvd - seq %u seid %lu f-seid %lu \n", ntohl(seres->hdr.seq) >> 8, 
            be64toh(seres->hdr.seid), be64toh(seres->fseid.seid));
        (*recv)++;
    }
}

void smReqResp(int sockfd, struct sockaddr_in *servaddr, 
        int seq, uint64_t seid, uint64_t gap, int *sent, int *recv) {
    struct pfcp_semreq *semreq;
    struct pfcp_semres *semres;
    struct pfcp_shdr *shdr;
    char buffer[MAXLINE];
    int n = 0;
    memset(buffer, 0, MAXLINE);
    shdr = (struct pfcp_shdr *)buffer;
    shdr->ver = 3;
    shdr->mp = 0;
    shdr->sbit = 1;
    shdr->seq = htonl(seq << 8);
    semreq = (void *)buffer;

    semreq->hdr.mt = PFCP_SESSION_MOD_REQ;
    semreq->hdr.mlen = htons(sizeof(*semreq) - sizeof(struct pfcp_tlv));
    semreq->hdr.seid = htobe64(seid + gap);

    n = udpSend(sockfd, servaddr, (void *)semreq, sizeof(struct pfcp_semreq));
    if (!n) {
        printf("SM Req sent failed %u\n", ntohl(semreq->hdr.seq) >> 8);
        return;
    }
    if (debug) 
        printf("SM req sent seq %u u-seid %lu.\n", ntohl(semreq->hdr.seq) >> 8, be64toh(semreq->hdr.seid));
    (*sent)++;
    n = udpReceive(sockfd, servaddr, buffer);
    if (n) {
        shdr = (struct pfcp_shdr *)buffer;
        if (shdr->mt !=  PFCP_SESSION_MOD_RSP) {
            printf("Unexpected msg %u\n", shdr->mt);
            return;
        }

        semres = (struct pfcp_semres *)buffer;
        if (debug)
            printf("SM resp recvd - seq %u c-seid %lu\n", ntohl(shdr->seq) >> 8, be64toh(semres->hdr.seid));
        (*recv)++;
    }
}

void sdReqResp(int sockfd, struct sockaddr_in *servaddr, 
        int seq, uint64_t seid, uint64_t gap, int *sent, int *recv) {
    struct pfcp_sedreq *sedreq;
    struct pfcp_sedres *sedres;
    struct pfcp_shdr *shdr;
    char buffer[MAXLINE];
    int n = 0;
    memset(buffer, 0, MAXLINE);

    shdr = (struct pfcp_shdr *)buffer;
    shdr->ver = 3;
    shdr->mp = 0;
    shdr->sbit = 1;
    shdr->seq = htonl(seq << 8);
    sedreq = (void *)buffer;

    sedreq->hdr.mt = PFCP_SESSION_DEL_REQ;
    sedreq->hdr.mlen = htons(sizeof(*sedreq) - sizeof(struct pfcp_tlv));
    sedreq->hdr.seid = htobe64(seid + gap);

    n = udpSend(sockfd, servaddr, (void *)sedreq, sizeof(struct pfcp_sedreq));
    if (!n) {
        printf("SD Req sent failed %u\n", ntohl(sedreq->hdr.seq) >> 8);
        return;
    }
    if (debug) 
        printf("SD req sent seq %u u-seid %lu.\n", ntohl(sedreq->hdr.seq) >> 8, be64toh(sedreq->hdr.seid));
    (*sent)++;
    n = udpReceive(sockfd, servaddr, buffer);
    if (n) {
        shdr = (struct pfcp_shdr *)buffer;
        if (shdr->mt !=  PFCP_SESSION_DEL_RSP) {
            printf("Unexpected msg %u\n", shdr->mt);
            return;
        }

        sedres = (struct pfcp_sedres *)buffer;
        if (debug)
            printf("SD resp recvd - seq %u c-seid %lu\n", ntohl(shdr->seq) >> 8, be64toh(sedres->hdr.seid));
        (*recv)++;
    }
}

void *pfcp_msg_responder() {
    int sockfd;
	__u8 buffer[MAXLINE];
	struct sockaddr_in servaddr, cliaddr;
    struct pfcp_shdr *shdr;
    struct pfcp_nhdr *nhdr, *pfcp;
    struct pfcp_seres *seres;
    struct pfcp_hbres *hbres;
    struct pfcp_asres *asres;
    struct pfcp_arres *arres;
    memset(buffer, 0, MAXLINE);
		
	// Creating socket file descriptor
	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}
		
	memset(&servaddr, 0, sizeof(servaddr));
	memset(&cliaddr, 0, sizeof(cliaddr));
		
	// Filling server information
	servaddr.sin_family = AF_INET; // IPv4
	servaddr.sin_addr.s_addr = INADDR_ANY;
	servaddr.sin_port = htons(PORT);
		
	// Bind the socket with the server address
	if ( bind(sockfd, (const struct sockaddr *)&servaddr,
			sizeof(servaddr)) < 0 )
	{
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
		
	int len, n, mlen;
	
	len = sizeof(cliaddr); //len is value/result
	
  
    uint64_t seid, gap = 1000, rseid;
  
    while (1) {
    memset(buffer, 0, MAXLINE);
  	n = recvfrom(sockfd, (char *)buffer, MAXLINE,
				MSG_WAITALL, ( struct sockaddr *) &cliaddr,
				&len);
    
    if (n < sizeof(*nhdr)) {
      printf("SIZE err\n");
      continue;
    }

    pfcp = (struct pfcp_nhdr *)buffer;
    switch (pfcp->mt) {
    case PFCP_SESSION_EST_REQ:

      shdr = (struct pfcp_shdr *)buffer;
      shdr->mt = PFCP_SESSION_EST_RSP;
      seres = (struct pfcp_seres*) buffer;
      rseid = be64toh(seres->fseid.seid) + gap;
      seres->hdr.seid = seres->fseid.seid;
      seres->fseid.seid = htobe64(rseid);
      seres->cause.tlv.type = htons(19);
      seres->cause.tlv.len = htons(1);
      seres->cause.cause = 1;
      mlen = sizeof(struct pfcp_seres); 
      seres->hdr.mlen = htons(mlen - sizeof(struct pfcp_tlv));
      break;
    case PFCP_SESSION_DEL_REQ:
      shdr = (struct pfcp_shdr *)buffer;
      shdr->mt = PFCP_SESSION_DEL_RSP;
      rseid = be64toh(shdr->seid) - gap;
      shdr->seid = htobe64(rseid);
      mlen = sizeof(struct pfcp_sedres); 
     
      break;
    case PFCP_SESSION_MOD_REQ:
      shdr = (struct pfcp_shdr *)buffer;
      shdr->mt = PFCP_SESSION_MOD_RSP;
      rseid = be64toh(shdr->seid) - gap;
      shdr->seid = htobe64(rseid);
      mlen = sizeof(struct pfcp_semres); 
      break;
    case PFCP_HEARTBEAT_REQ:
      nhdr = (struct pfcp_nhdr *)buffer;
      nhdr->mt = PFCP_HEARTBEAT_RSP;
      hbres = (struct pfcp_hbres*) buffer;
      hbres->ts.tsv = hbres->ts.tsv + htonl(1);
      mlen = sizeof(struct pfcp_hbres); 
      break;
    case PFCP_ASSOCIATION_SETUP_REQ:
      nhdr = (struct pfcp_nhdr *)buffer;
      nhdr->mt = PFCP_ASSOCIATION_SETUP_RSP;
      asres = (struct pfcp_asres*) buffer;
      asres->features.tlv.type = htons(43);
      asres->cause.tlv.type = htons(19);
      asres->cause.tlv.len = htons(1);
      asres->cause.cause = 1;
      mlen = sizeof(struct pfcp_asres); 
      asres->hdr.mlen = htons(mlen - sizeof(struct pfcp_tlv));
      break;
    case PFCP_ASSOCIATION_REL_REQ:
      nhdr = (struct pfcp_nhdr *)buffer;
      nhdr->mt = PFCP_ASSOCIATION_REL_RSP;
      arres = (struct pfcp_arres*) buffer;
      arres->cause.tlv.type = htons(19);
      arres->cause.tlv.len = htons(1);
      arres->cause.cause = 1;
      mlen = sizeof(struct pfcp_arres); 
      arres->hdr.mlen = htons(mlen - sizeof(struct pfcp_tlv));
      break;


    default:
      printf("Unexpected msg %u\n", pfcp->mt);
    }
	  sendto(sockfd, (void *)buffer, mlen,
		  MSG_CONFIRM, (const struct sockaddr *) &cliaddr,
			len);
  	if(debug) printf("PFCP message sent.\n");
  }
		
}


