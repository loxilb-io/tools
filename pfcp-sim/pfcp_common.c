#include "pfcp_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

void *node_msg_func() {
    int sockfd;
	__u8 buffer[MAXLINE];
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
		
}


