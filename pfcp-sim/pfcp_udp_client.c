// Client(SMF) side implementation of PFCP
#include "pfcp_common.h"
// Driver code
int main(int argc, char **argv) {
	int sockfd;
    struct sockaddr_in	 servaddr, cliaddr;
    pthread_t thread_id;
    int pass = 0;
	
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
	
    pthread_create(&thread_id, NULL, pfcp_msg_responder, NULL);
	
	int n, len, ch, debug = 1;
    srandom(atoi(argv[2]));
    uint32_t seq = random(), num, duration = 1;
    uint64_t seid, gap = 1000;
    bool all = false, se = false;
    int sent = 0, recv = 0;
    int node_ip = 0x01010101;
    do {
       printf("-----------------------------------------------------\n");
       printf("1 for assoc setup req\n");
       printf("2 for assoc rel req\n");
       printf("3 for heartbeat \n");
       printf("4 for session est req\n");
       printf("5 for session mod req\n");
       printf("6 for session del req\n");
       printf("7 for session est-mod-del req\n");
       printf("8 for all msgs(option 1-6)\n");
       printf("9 for TPS test\n");
       printf("0 for exit\n");
       printf("-----------------------------------------------------\n");
       printf("Enter your choice: ");
       scanf("%d", &ch);
       while ((getchar()) != '\n');
       if (ch == 0) {
          exit(0);
       }
       if ((ch < 1 || ch > 9)) continue;
       all = false;
       se = false;
       printf("Number of msgs: ");
       scanf("%d", &num);
       duration = 1; /* Default duration */
       if (ch == 9) {
         printf("Duration: ");
         scanf("%d", &duration);
         printf("Choose option 1-8: ");
         scanf("%d", &ch);
       }
       seid = atoi(argv[1]);
       printf("\n-----------------------------------------------------\n");
       pass = 0;
       for(int d = 1; d <= duration; d++) {
           sent = 0;
           recv = 0;

           for(int i = 1; i <= num; i++) {
               switch(ch) {
                   case 8: all = true;
                   case 1:
                       asReqResp(sockfd, &servaddr, seq, node_ip, &sent, &recv);
                       seq++;
                       if (!all)
                           break;

                   case 2:
                       arReqResp(sockfd, &servaddr, seq, node_ip, &sent, &recv);
                       seq++;
                       if (!all)
                           break;

                   case 3:
                       hbReqResp(sockfd, &servaddr, seq, &sent, &recv);
                       seq++;
                       if (!all)
                           break;
                   case 7: se = true;
                   case 4:
                       seReqResp(sockfd, &servaddr, seq, node_ip, seid, &sent, &recv);
                       seq++;
                       if (!all && !se)
                           break;
                   case 5:
                       smReqResp(sockfd, &servaddr, seq, seid, gap, &sent, &recv);
                       seq++;
                       if (!all && !se)
                           break;
                   case 6:
                       sdReqResp(sockfd, &servaddr, seq, seid, gap, &sent, &recv);
                       seq++;
                       break;
                   default:
                       break;
               }
               seid++;
           }
           printf(">%d.00 - %d.00:> \t\tSent:\t%d msgs, Recv:\t%d msgs\n", d-1, d,sent,recv);
           if (sent == recv) pass++;
           if(d<duration) sleep(1);
       }
       printf("Done(%u/%u).\n", pass, duration);
    } while(1);
	close(sockfd);
	return 0;
}

