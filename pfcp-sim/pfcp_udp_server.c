// Server side implementation of UDP client-server model
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
	
#define PORT	 8805
#define MAXLINE   12000

#define PFCP_HEARTBEAT_REQ                  1
#define PFCP_HEARTBEAT_RSP                  2
#define PFCP_ASSOCIATION_SETUP_REQ          5
#define PFCP_ASSOCIATION_SETUP_RSP          6
#define PFCP_ASSOCIATION_REL_REQ            9
#define PFCP_ASSOCIATION_REL_RSP            10

#define PFCP_SESSION_SET_DEL_REQ          14
#define PFCP_SESSION_SET_DEL_RSP          15
#define PFCP_SESSION_EST_REQ              50
#define PFCP_SESSION_EST_RSP              51
#define PFCP_SESSION_MOD_REQ              52
#define PFCP_SESSION_MOD_RSP              53
#define PFCP_SESSION_DEL_REQ              54
#define PFCP_SESSION_DEL_RSP              55
#define PFCP_SESSION_REPORT_REQ           56
#define PFCP_SESSION_REPORT_RSP           57

typedef unsigned char __u8;
typedef unsigned int __u32;
typedef unsigned short __be16;
typedef unsigned long long __u64;

/*
 * struct pfcp_shdr - PFCP common session header
 */

struct pfcp_shdr {
  __u8    sbit:1;
  __u8    mp:1;
  __u8    s3:1;
  __u8    s2:1;
  __u8    s1:1;
  __u8    ver:3;
  __u8    mt;
  __be16  mlen;
  __u64   seid;
  __u32   seq;
}__attribute__((packed, aligned(1)));

/*
 * struct pfcp_nhdr - PFCP common node header
 */

struct pfcp_nhdr {
  __u8    sbit:1;
  __u8    mp:1;
  __u8    s3:1;
  __u8    s2:1;
  __u8    s1:1;
  __u8    ver:3;
  __u8    mt;
  __be16  mlen;
  __u32   seq;
}__attribute__((packed, aligned(1)));

struct pfcp_tlv {
  __be16 type;
  __be16 len;
}__attribute__((packed, aligned(1)));

struct fseid_ie {
  struct pfcp_tlv tlv;
  __u8  v6:1;
  __u8  v4:1;
  __u8  spare:6;
  __u64 seid;
  __u32 node_ip;
}__attribute__((packed, aligned(1)));

struct cause_ie {
  struct pfcp_tlv tlv;
  __u8 cause;
}__attribute__((packed, aligned(1)));

struct nodeid_ie {
  struct pfcp_tlv tlv;
  __u8  spare:4;
  __u8  type:4;
  __u32 node_ip;
}__attribute__((packed, aligned(1)));

struct pfcp_sereq {
  struct pfcp_shdr hdr;
  struct nodeid_ie node_id;
  struct fseid_ie  fseid; 
}__attribute__((packed, aligned(1)));

struct pfcp_seres {
  struct pfcp_shdr hdr;
  struct nodeid_ie node_id;
  struct fseid_ie  fseid;
  struct cause_ie cause;
}__attribute__((packed, aligned(1)));

struct pfcp_semreq {
  struct pfcp_shdr hdr;
}__attribute__((packed, aligned(1)));

struct pfcp_sedreq {
  struct pfcp_shdr hdr;
}__attribute__((packed, aligned(1)));

struct pfcp_semres {
  struct pfcp_shdr hdr;
}__attribute__((packed, aligned(1)));

struct pfcp_sedres {
  struct pfcp_shdr hdr;
}__attribute__((packed, aligned(1)));

struct feat_ie {
  struct pfcp_tlv tlv;
  __u8   data[10000]; /* large data to check fragmented packets*/
}__attribute__((packed, aligned(1)));

struct rects_ie {
  struct pfcp_tlv tlv;
  __u32 tsv;
}__attribute__((packed, aligned(1)));;

struct pfcp_asreq {
  struct pfcp_nhdr hdr;
  struct nodeid_ie node_id;
  struct rects_ie ts;
  struct feat_ie features;
}__attribute__((packed, aligned(1)));

struct pfcp_asres {
  struct pfcp_nhdr hdr;
  struct nodeid_ie node_id;
  struct rects_ie ts;
  struct feat_ie features;
  struct cause_ie cause;
}__attribute__((packed, aligned(1)));

struct pfcp_arreq {
  struct pfcp_nhdr hdr;
  struct nodeid_ie node_id;
}__attribute__((packed, aligned(1)));

struct pfcp_arres {
  struct pfcp_nhdr hdr;
  struct nodeid_ie node_id;
  struct cause_ie cause;
}__attribute__((packed, aligned(1)));

struct pfcp_hbreq {
  struct pfcp_nhdr hdr;
  struct rects_ie ts;
}__attribute__((packed, aligned(1)));

struct pfcp_hbres {
  struct pfcp_nhdr hdr;
  struct rects_ie ts;
}__attribute__((packed, aligned(1)));

// Driver code
int main(int argc, char **argv) {
	int sockfd;
	__u8 buffer[MAXLINE];
	char *hello = "Hello from server";
	struct sockaddr_in servaddr, cliaddr;
    struct pfcp_shdr *shdr;
    struct pfcp_nhdr *nhdr, *pfcp;
    struct pfcp_seres *seres;
    struct pfcp_hbres *hbres;
    struct pfcp_asres *asres;
    struct pfcp_arres *arres;
		
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
  	printf("PFCP message sent.\n");
  }
		
	return 0;
}

