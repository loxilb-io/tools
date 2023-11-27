// Server(UPF) side implementation of PFCP
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include "pfcp_common.h"
	
// Driver code
int main(int argc, char **argv) {
	
    pthread_t thread_id; 
	int sockfd;
	char buffer[MAXLINE];
    struct pfcp_nhdr *nhdr;
    struct pfcp_asreq  *asreq;
    struct pfcp_asres  *asres;
    struct pfcp_arreq  *arreq;
    struct pfcp_arres  *arres;
    struct pfcp_hbreq  *hbreq;
    struct pfcp_hbres  *hbres;

	struct sockaddr_in	 servaddr, cliaddr;
	if (argc != 3) {
        printf("Usage - %s <peer-ip> <seid-seed-value>\n", argv[0]);
        exit(0);
    }
	// Creating socket file descriptor
	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}
	
	memset(&servaddr, 0, sizeof(servaddr));
		
	// Filling server information
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(PORT);
	servaddr.sin_addr.s_addr = inet_addr(argv[1]);

    cliaddr.sin_family = AF_INET;
    //cliaddr.sin_addr.s_addr= htonl(INADDR_ANY);
    cliaddr.sin_port=htons(30000); //source port for outgoing packets
    bind(sockfd,(struct sockaddr *)&cliaddr,sizeof(cliaddr));
	
    pthread_create(&thread_id, NULL, node_msg_func, NULL);
	
	int n, len, ch;
    srandom(atoi(argv[2]));
    uint32_t seq = random(), num;
    uint64_t seid, gap = 1000;
    bool all = false;
    do {
       printf("\n\n1 for assoc setup req\n");
       printf("2 for assoc rel req\n");
       printf("3 for heartbeat \n");
       printf("8 for exit\n");
       scanf("%d", &ch);
       if (ch == 8) {
        exit(0);
       }
       all = false;
       printf("Number of msgs: ");
       scanf("%d", &num);
       seid = atoi(argv[1]);
       for(int i = 1; i <= num; i++) {
            memset(buffer, 0, sizeof(buffer));

       switch(ch) {
        case 1:
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
            asreq->node_id.node_ip = 0x02020202;

            asreq->ts.tlv.type = htons(96);
            asreq->ts.tlv.len = htons(sizeof(struct rects_ie) - sizeof(struct pfcp_tlv));
            asreq->ts.tsv = htonl(seq);

            asreq->features.tlv.type = htons(89);
            asreq->features.tlv.len = htons(sizeof(struct feat_ie) - sizeof(struct pfcp_tlv));
            strcpy(asreq->features.data, "Function features");

            sendto(sockfd, (void *)asreq, sizeof(struct pfcp_asreq),
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
            arreq->node_id.node_ip = 0x02020202;

            sendto(sockfd, (void *)arreq, sizeof(struct pfcp_arreq),
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

         case 8:
            exit(0);
        }
        seid++;
      }
    } while(1);
	close(sockfd);

	return 0;
}

