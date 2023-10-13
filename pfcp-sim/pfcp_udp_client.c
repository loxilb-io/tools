// Client side implementation of UDP client-server model
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
	
#define PORT	  8805 
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
  struct cause_ie cause;
  struct feat_ie features;
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
	char buffer[MAXLINE];
    struct pfcp_shdr *shdr;
    struct pfcp_nhdr *nhdr;
    struct pfcp_sereq *sereq;
    struct pfcp_seres *seres;
    struct pfcp_semreq *semreq;
    struct pfcp_semres *semres;
    struct pfcp_sedreq *sedreq;
    struct pfcp_sedres *sedres;
    struct pfcp_asreq  *asreq;
    struct pfcp_asres  *asres;
    struct pfcp_arreq  *arreq;
    struct pfcp_arres  *arres;
    struct pfcp_hbreq  *hbreq;
    struct pfcp_hbres  *hbres;

	char *hello = "Hello from client";
	struct sockaddr_in	 servaddr, cliaddr;
	
	// Creating socket file descriptor
	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}
	
	memset(&servaddr, 0, sizeof(servaddr));
		
	// Filling server information
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(PORT);
	servaddr.sin_addr.s_addr = inet_addr("20.20.20.1");

    cliaddr.sin_family = AF_INET;
    //cliaddr.sin_addr.s_addr= htonl(INADDR_ANY);
    cliaddr.sin_port=htons(30000); //source port for outgoing packets
    bind(sockfd,(struct sockaddr *)&cliaddr,sizeof(cliaddr));
		
	int n, len, ch;
    srandom(atoi(argv[1]));
    uint32_t seq = random(), num;
    uint64_t seid, gap = 1000;
    bool all = false;
    do {
       printf("\n\n1 for assoc setup req\n");
       printf("2 for assoc rel req\n");
       printf("3 for heartbeat \n");
       printf("4 for session est req\n");
       printf("5 for session mod req\n");
       printf("6 for session del req\n");
       printf("7 for session est-mod-del req\n");
       printf("8 for exit\n");
       scanf("%d", &ch);
       if (ch == 8) {
        exit(0);
       }
       printf("Number of msgs: ");
       scanf("%d", &num);
       seid = atoi(argv[1]);
       for(int i = 1; i <= num; i++) {
            memset(buffer, 0, sizeof(buffer));

       switch(ch) {
        case 1:
            nhdr = (struct pfcp_nhdr *)buffer;
            nhdr->ver = 3;
            nhdr->mp = 0;
            asreq = (void*) buffer;
            asreq->hdr.mt = PFCP_ASSOCIATION_SETUP_REQ;
            asreq->hdr.mlen = htons(sizeof(*asreq) - sizeof(struct pfcp_tlv));
            asreq->hdr.seq = htonl(seq << 8);

            /*Node ID*/
            asreq->node_id.tlv.type = htons(60);
            asreq->node_id.tlv.len = htons(sizeof(struct nodeid_ie) - sizeof(struct pfcp_tlv));
            asreq->node_id.type = 0;
            asreq->node_id.node_ip = 0x01010101;

            asreq->ts.tlv.type = htons(96);
            asreq->ts.tlv.len = htons(sizeof(struct rects_ie) - sizeof(struct pfcp_tlv));
            asreq->ts.tsv = htonl(seq);

            asreq->features.tlv.type = htons(89);
            asreq->features.tlv.len = htons(sizeof(struct feat_ie) - sizeof(struct pfcp_tlv));
            strcpy(asreq->features.data, "Function features");

            sendto(sockfd, (void *)hbreq, sizeof(struct pfcp_asreq),
                            MSG_CONFIRM, (const struct sockaddr *) &servaddr,
                            sizeof(servaddr));
            printf("AS Req sent seq %u\n", ntohl(asreq->hdr.seq) >> 8);

            n = recvfrom(sockfd, (char *)buffer, MAXLINE,
                            MSG_WAITALL, (struct sockaddr *) &servaddr,
                            &len);
            if (n < sizeof(*nhdr)) {
                    printf("SIZE err\n");
                    exit(1);
            }

            nhdr = (struct pfcp_nhdr *)buffer;
            if (nhdr->mt !=  PFCP_ASSOCIATION_SETUP_RSP) {
                    printf("Unexpected msg %u\n", nhdr->mt);
                    exit(1);
            }
            
            asres = (struct pfcp_asres *)buffer;
            printf("AS resp recvd - seq %u\n",ntohl(asres->hdr.seq) >> 8);
            seq++;
            break;

        case 2:
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
            arreq->node_id.node_ip = 0x01010101;

            sendto(sockfd, (void *)hbreq, sizeof(struct pfcp_arreq),
                            MSG_CONFIRM, (const struct sockaddr *) &servaddr,
                            sizeof(servaddr));
            printf("AR Req sent seq %u\n", ntohl(arreq->hdr.seq) >> 8);

            n = recvfrom(sockfd, (char *)buffer, MAXLINE,
                            MSG_WAITALL, (struct sockaddr *) &servaddr,
                            &len);
            if (n < sizeof(*nhdr)) {
                    printf("SIZE err\n");
                    exit(1);
            }

            nhdr = (struct pfcp_nhdr *)buffer;
            if (nhdr->mt !=  PFCP_ASSOCIATION_REL_RSP) {
                    printf("Unexpected msg %u\n", nhdr->mt);
                    exit(1);
            }
            
            arres = (struct pfcp_arres *)buffer;
            printf("AS resp recvd - seq %u\n",ntohl(arres->hdr.seq) >> 8);
            seq++;
            break;

        case 3:
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

            sendto(sockfd, (void *)hbreq, sizeof(struct pfcp_hbreq),
                            MSG_CONFIRM, (const struct sockaddr *) &servaddr,
                            sizeof(servaddr));
            printf("HB sent seq %u ts %u\n", ntohl(hbreq->hdr.seq) >> 8, ntohl(hbreq->ts.tsv));

            n = recvfrom(sockfd, (char *)buffer, MAXLINE,
                            MSG_WAITALL, (struct sockaddr *) &servaddr,
                            &len);
            if (n < sizeof(*nhdr)) {
                    printf("SIZE err\n");
                    exit(1);
            }

            nhdr = (struct pfcp_nhdr *)buffer;
            if (nhdr->mt !=  PFCP_HEARTBEAT_RSP) {
                    printf("Unexpected msg %u\n", nhdr->mt);
                    exit(1);
            }
            
            hbres = (struct pfcp_hbres *)buffer;
            printf("HB resp recvd - seq %u rts %u \n",ntohl(hbres->hdr.seq) >> 8, ntohl(hbres->ts.tsv));
            seq++;
            break;

        case 7: all = true;
        case 4:

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
            sereq->node_id.node_ip = 0x01010101;

            /*F-SEID*/
            sereq->fseid.tlv.type = htons(57);
            sereq->fseid.tlv.len = htons(sizeof(struct fseid_ie) - sizeof(struct pfcp_tlv));
            sereq->fseid.v4 = 1;
            sereq->fseid.seid = htobe64(seid);
            sereq->fseid.node_ip = 0x01010101;

            sendto(sockfd, (void *)sereq, sizeof(struct pfcp_sereq),
                            MSG_CONFIRM, (const struct sockaddr *) &servaddr,
                            sizeof(servaddr));
            printf("est sent seq %u f-seid %lu.\n", ntohl(sereq->hdr.seq) >> 8, be64toh(sereq->fseid.seid));

            n = recvfrom(sockfd, (char *)buffer, MAXLINE,
                            MSG_WAITALL, (struct sockaddr *) &servaddr,
                            &len);
            if (n < sizeof(*shdr)) {
                    printf("SIZE err\n");
                    exit(1);
            }

            shdr = (struct pfcp_shdr *)buffer;
            if (shdr->mt !=  PFCP_SESSION_EST_RSP) {
                    printf("Unexpected msg %u\n", shdr->mt);
                    exit(1);
            }
            
            if(shdr->seq != htonl(seq << 8)) {
                    printf("Unexpected seq no. Sent %u, Recvd %d\n", seq, ntohl(shdr->seq) >> 8);
                    exit(1);
            }
            seres = (struct pfcp_seres *)buffer;
            printf("est resp recvd - seq %u seid %lu f-seid %lu \n", ntohl(seres->hdr.seq) >> 8, 
                be64toh(seres->hdr.seid), be64toh(seres->fseid.seid));
            seq++;
            if (!all)
            break;

         case 5:

            shdr = (struct pfcp_shdr *)buffer;
            shdr->ver = 3;
            shdr->mp = 0;
            shdr->sbit = 1;
            shdr->seq = htonl(seq << 8);
            semreq = (void *)buffer;

            semreq->hdr.mt = PFCP_SESSION_MOD_REQ;
            semreq->hdr.mlen = htons(sizeof(*semreq) - sizeof(struct pfcp_tlv));
            semreq->hdr.seid = htobe64(seid + gap);

            sendto(sockfd, (void *)semreq, sizeof(struct pfcp_semreq),
                            MSG_CONFIRM, (const struct sockaddr *) &servaddr,
                            sizeof(servaddr));
            printf("mod req sent seq %u u-seid %lu.\n", ntohl(semreq->hdr.seq) >> 8, be64toh(semreq->hdr.seid));

            n = recvfrom(sockfd, (char *)buffer, MAXLINE,
                            MSG_WAITALL, (struct sockaddr *) &servaddr,
                            &len);
            if (n < sizeof(*shdr)) {
                    printf("SIZE err\n");
                    exit(1);
            }

            shdr = (struct pfcp_shdr *)buffer;
            if (shdr->mt !=  PFCP_SESSION_MOD_RSP) {
                    printf("Unexpected err\n");
                    exit(1);
            }
            if(shdr->seq != htonl(seq << 8)) {
                    printf("Unexpected seq no. Sent %u, Recvd %d\n", seq >> 8, ntohl(shdr->seq) >> 8);
                    exit(1);
            }
            semres = (struct pfcp_semres *)buffer;
            printf("mod resp recvd - seq %u c-seid %lu\n", ntohl(shdr->seq) >> 8, be64toh(semres->hdr.seid));
            seq++;
            if (!all)
            break;

         case 6:

            shdr = (struct pfcp_shdr *)buffer;
            shdr->ver = 3;
            shdr->mp = 0;
            shdr->sbit = 1;
            shdr->seq = htonl(seq << 8);
            sedreq = (void *)buffer;

            sedreq->hdr.mt = PFCP_SESSION_DEL_REQ;
            sedreq->hdr.mlen = htons(sizeof(*semreq) - sizeof(struct pfcp_tlv));
            sedreq->hdr.seid = htobe64(seid + gap);

            sendto(sockfd, (void *)sedreq, sizeof(struct pfcp_sedreq),
                            MSG_CONFIRM, (const struct sockaddr *) &servaddr,
                            sizeof(servaddr));
            printf("del req sent seq %u u-seid %lu.\n", ntohl(sedreq->hdr.seq) >> 8, be64toh(sedreq->hdr.seid));

            n = recvfrom(sockfd, (char *)buffer, MAXLINE,
                            MSG_WAITALL, (struct sockaddr *) &servaddr,
                            &len);
            if (n < sizeof(*shdr)) {
                    printf("SIZE err\n");
                    exit(1);
            }

            shdr = (struct pfcp_shdr *)buffer;
            if (shdr->mt !=  PFCP_SESSION_DEL_RSP) {
                    printf("Unexpected err\n");
                    exit(1);
            }
            if(shdr->seq != htonl(seq << 8)) {
                    printf("Unexpected seq no. Sent %u, Recvd %d\n", seq >> 8, ntohl(shdr->seq) >> 8);
                    exit(1);
            }
            sedres = (struct pfcp_sedres *)buffer;
            printf("del resp recvd - seq %u c-seid %lu\n", ntohl(shdr->seq) >> 8, be64toh(sedres->hdr.seid));
            seq++;
            break;
         case 8:
            exit(0);
        }
        seid++;
      }
    } while(1);
	close(sockfd);
	return 0;
}

