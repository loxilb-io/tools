// PFCP Test Tool
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
  
#define PFCP_PORT    8805 
#define MAXBUF       12000

#define PFCP_HEARTBEAT_REQ                1
#define PFCP_HEARTBEAT_RSP                2
#define PFCP_ASSOCIATION_SETUP_REQ        5
#define PFCP_ASSOCIATION_SETUP_RSP        6
#define PFCP_ASSOCIATION_REL_REQ          9
#define PFCP_ASSOCIATION_REL_RSP          10

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

struct thread_info {
  pthread_t thread_id;
  int       thread_num;
  char      *argv_string;
};

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

volatile int ghb_rcvd = 0;
int wr_socket = -1;
int rd_socket = -1;
int client = 1;

void *rcv_thread(void *arg)
{
  uint8_t buffer[MAXBUF];
  struct sockaddr_in s_addr, c_addr;
  struct pfcp_shdr *shdr;
  struct pfcp_nhdr *nhdr, *pfcp;
  struct pfcp_seres *seres;
  struct pfcp_hbres *hbres;
  struct pfcp_asres *asres;
  struct pfcp_arres *arres;
  int len, n, mlen;

  memset(&c_addr, 0, sizeof(c_addr));

  len = sizeof(c_addr);

  uint64_t seid, gap = 1000, rseid;
  while (1) {
    n = recvfrom(rd_socket, (char *)buffer, MAXBUF,
        MSG_WAITALL, ( struct sockaddr *) &c_addr,
        &len);

    if (n < sizeof(*nhdr)) {
      printf("SIZE err\n");
      continue;
    }

    mlen = 0;
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
      if (client) printf("PFCP HB resp sent.\n");
      break;
    case PFCP_HEARTBEAT_RSP:
      hbres = (struct pfcp_hbres *)buffer;
      if (!client) printf("PFCP HB resp recvd - seq %u rts %u \n",ntohl(hbres->hdr.seq) >> 8, ntohl(hbres->ts.tsv));
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
      //printf("Unexpected msg %u -- HERE\n", pfcp->mt);
      mlen = 0;
      break;
    }

    if (mlen) {
      sendto(rd_socket, (void *)buffer, mlen,
        MSG_CONFIRM, (const struct sockaddr *) &c_addr,
        len);
    }
  }

  return 0;
}

void client_chk_rcv(struct sockaddr_in *sendaddr,
                    int emt, uint32_t eseq, unsigned int *nrmsg)
{
  __u8 buffer[MAXBUF];
  char *hello = "Hello from server";
  struct pfcp_shdr *shdr;
  struct pfcp_nhdr *nhdr, *pfcp;
  struct pfcp_seres *seres;
  struct pfcp_hbres *hbres;
  struct pfcp_asres *asres;
  struct pfcp_arres *arres;
  int len, n, mlen;
  int hb_rcv = 0;
  uint64_t seid, gap = 1000, rseid;
  struct sockaddr_in src_addr = { 0 };
  int do_send = 0;

  len = sizeof(src_addr);

  while (1) {
    memset(buffer, 0, sizeof(buffer));
    n = recvfrom(wr_socket, (char *)buffer, MAXBUF,
                MSG_WAITALL, ( struct sockaddr *) &src_addr,
                &len);

    if (n < sizeof(*nhdr)) {
      printf("SIZE err\n");
      return;
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

    case PFCP_HEARTBEAT_RSP:
    case PFCP_ASSOCIATION_SETUP_RSP:
    case PFCP_ASSOCIATION_REL_RSP:
      nhdr = (struct pfcp_nhdr *)buffer;
      if (nhdr->mt != emt) {
        printf("Unexpected msg %u:%u -- RSP\n", nhdr->mt, emt);
      }
      if(nhdr->seq != htonl(eseq << 8)) {
          printf("Unexpected seq no. Sent %u, Recvd %d\n", eseq, ntohl(nhdr->seq) >> 8);
          exit(1);
      }
      //printf("OK msg %u:%u -- RSP\n", nhdr->mt, emt);
      ++*nrmsg;
      goto out;
    case PFCP_SESSION_EST_RSP:
    case PFCP_SESSION_MOD_RSP:
    case PFCP_SESSION_DEL_RSP:
      shdr = (struct pfcp_shdr *)buffer;
      if (shdr->mt != emt) {
        printf("Unexpected sess msg %u:%u -- RSP\n", shdr->mt, emt);
        goto out;
      }
      if(shdr->seq != htonl(eseq << 8)) {
        printf("Unexpected sess seq no. Sent %u, Recvd %d\n", eseq, ntohl(shdr->seq) >> 8);
        exit(1);
      }
      //printf("OK sess msg %u:%u -- RSP\n", shdr->mt, emt);
      ++*nrmsg;
      goto out;
    default:
      printf("Unexpected msg %u -- %d(%s)\n", pfcp->mt, n, __FUNCTION__);
      return;
    }
  }

out:
  return;
}

int server_socket_init()
{
  struct sockaddr_in s_addr, c_addr;
  struct timeval tv;
  tv.tv_sec = 10;
  tv.tv_usec = 100000;

  if ((rd_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
    assert(0);
  }

  memset(&s_addr, 0, sizeof(s_addr));
  memset(&c_addr, 0, sizeof(c_addr));

  // Filling server information
  s_addr.sin_family = AF_INET;
  s_addr.sin_port = htons(PFCP_PORT);
  s_addr.sin_addr.s_addr = inet_addr("10.10.10.1");

  c_addr.sin_family = AF_INET;
  c_addr.sin_addr.s_addr= htonl(INADDR_ANY);
  c_addr.sin_port=htons(PFCP_PORT);
  bind(rd_socket,(struct sockaddr *)&c_addr,sizeof(c_addr));

  if (setsockopt(rd_socket, SOL_SOCKET, SO_RCVTIMEO, &tv,sizeof(tv)) < 0) {
     perror("sock-timeo");
  }

  if ((wr_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
    assert(0);
  }

  memset(&s_addr, 0, sizeof(s_addr));
  memset(&c_addr, 0, sizeof(c_addr));

  s_addr.sin_family = AF_INET;
  s_addr.sin_port = htons(PFCP_PORT);
  s_addr.sin_addr.s_addr = inet_addr("10.10.10.1");

  c_addr.sin_family = AF_INET;
  c_addr.sin_addr.s_addr= htonl(INADDR_ANY);
  c_addr.sin_port=htons(30000);
  bind(wr_socket,(struct sockaddr *)&c_addr,sizeof(c_addr));

  if (connect(wr_socket, (struct sockaddr *)&s_addr, sizeof(s_addr)) < 0)  {
    assert(0);
  }

  if (setsockopt(rd_socket, SOL_SOCKET, SO_RCVTIMEO, &tv,sizeof(tv)) < 0) {
     perror("sock-timeo");
  }

  return 0;
}

int client_socket_init()
{
  struct sockaddr_in s_addr, c_addr;
  struct timeval tv;
  tv.tv_sec = 1;
  tv.tv_usec = 100000;

  if ((wr_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    assert(0);
  }

  memset(&s_addr, 0, sizeof(s_addr));
  memset(&c_addr, 0, sizeof(c_addr));

  s_addr.sin_family = AF_INET;
  s_addr.sin_port = htons(PFCP_PORT);
  s_addr.sin_addr.s_addr = inet_addr("10.10.10.254");

  c_addr.sin_family = AF_INET;
  c_addr.sin_addr.s_addr= htonl(INADDR_ANY);
  c_addr.sin_port=htons(30000);
  bind(wr_socket,(struct sockaddr *)&c_addr,sizeof(c_addr));

  if (connect(wr_socket, (struct sockaddr *)&s_addr, sizeof(s_addr)) < 0)  {
    assert(0); 
  } 

  if (setsockopt(wr_socket, SOL_SOCKET, SO_RCVTIMEO, &tv,sizeof(tv)) < 0) {
     perror("sock-timeo");
  }

  if ((rd_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
    assert(0);
  }

  c_addr.sin_family = AF_INET;
  c_addr.sin_addr.s_addr= htonl(INADDR_ANY);
  c_addr.sin_port=htons(PFCP_PORT);
  bind(rd_socket,(struct sockaddr *)&c_addr,sizeof(c_addr));

  if (setsockopt(rd_socket, SOL_SOCKET, SO_RCVTIMEO, &tv,sizeof(tv)) < 0) {
     perror("sock-timeo");
  }

  return 0;
}

int client_main(int port_hint) 
{
  char buffer[MAXBUF];
  struct pfcp_shdr *shdr;
  struct pfcp_nhdr *nhdr;
  struct pfcp_sereq *sereq;
  struct pfcp_seres *seres;
  struct pfcp_semreq *semreq;
  struct pfcp_semres *semres;
  struct pfcp_sedreq *sedreq;
  struct pfcp_sedres *sedres;
  struct pfcp_asreq *asreq;
  struct pfcp_asres *asres;
  struct pfcp_arreq *arreq;
  struct pfcp_arres *arres;
  struct pfcp_hbreq *hbreq;
  struct pfcp_hbres *hbres;
  struct sockaddr_in s_addr, c_addr;
  int n, len, ch = 0;
  unsigned int nmsg = 0;
  unsigned int nrmsg = 0;
  uint32_t seq = random(), num;
  uint64_t seid, gap = 1000;
  bool all = false;
  bool cont = true;
  uint8_t emt = 0;

  srandom(port_hint);

  do {

    memset(&s_addr, 0, sizeof(s_addr));
    s_addr.sin_family = AF_INET;
    s_addr.sin_port = htons(PFCP_PORT);
    s_addr.sin_addr.s_addr = inet_addr("10.10.10.254");

    if (!cont) {
      printf("\n\n1 for assoc setup req\n");
      printf("2 for assoc rel req\n");
      printf("3 for heartbeat \n");
      printf("4 for session est req\n");
      printf("5 for session mod req\n");
      printf("6 for session del req\n");
      printf("7 for session est-mod-del req\n");
      printf("8 for exit\n");
      scanf("%d", &ch);
      printf("Number of msgs: ");
      scanf("%d", &num);
    } else {
      ch++;
      num = 1000;
    } 
    if (ch >= 7) {
      if (!cont) {
        exit(0);
      } else {
        ch = 1;
        printf("msg sent %u msg rcvd %u\n", nmsg, nrmsg);
        nmsg = 0;
        nrmsg = 0;
        sleep(2);
      }
    }

    all = false;

    seid = port_hint;
    for (int i = 1; i <= num; i++) {
      memset(buffer, 0, sizeof(buffer));
      switch(ch) {
        case 1:
          nhdr = (struct pfcp_nhdr *)buffer;
          nhdr->ver = 1;
          nhdr->mp = 0;
          asreq = (void*) buffer;
          asreq->hdr.mt = PFCP_ASSOCIATION_SETUP_REQ;
          emt = PFCP_ASSOCIATION_SETUP_RSP;
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

          sendto(wr_socket, (void *)asreq, sizeof(struct pfcp_asreq),
                 MSG_CONFIRM, (const struct sockaddr *) &s_addr,
                 sizeof(s_addr));
          //printf("AS Req sent seq %u\n", ntohl(asreq->hdr.seq) >> 8);
          seq++;
          nmsg++;
          break;

        case 2:
          nhdr = (struct pfcp_nhdr *)buffer;
          nhdr->ver = 3;
          nhdr->mp = 0;
          arreq = (void*) buffer;
          arreq->hdr.mt = PFCP_ASSOCIATION_REL_REQ;
          emt = PFCP_ASSOCIATION_REL_RSP;
          arreq->hdr.mlen = htons(sizeof(*arreq) - sizeof(struct pfcp_tlv));
          arreq->hdr.seq = htonl(seq << 8);

          /*Node ID*/
          arreq->node_id.tlv.type = htons(60);
          arreq->node_id.tlv.len = htons(sizeof(struct nodeid_ie) - sizeof(struct pfcp_tlv));
          arreq->node_id.type = 0;
          arreq->node_id.node_ip = 0x01010101;

          sendto(wr_socket, (void *)arreq, sizeof(struct pfcp_arreq),
                 MSG_CONFIRM, (const struct sockaddr *) &s_addr,
                 sizeof(s_addr));
          //printf("AR Req sent seq %u\n", ntohl(arreq->hdr.seq) >> 8);
          seq++;
          nmsg++;
          break;

       case 3:
          nhdr = (struct pfcp_nhdr *)buffer;
          nhdr->ver = 3;
          nhdr->mp = 0;

          hbreq = (void *)buffer;
          hbreq->hdr.mt = PFCP_HEARTBEAT_REQ;
          emt = PFCP_HEARTBEAT_RSP;
          hbreq->hdr.mlen = htons(sizeof(*hbreq) - sizeof(struct pfcp_tlv));
          hbreq->hdr.seq = htonl(seq << 8);

          hbreq->ts.tlv.type = htons(96);
          hbreq->ts.tlv.len = htons(sizeof(struct rects_ie) - sizeof(struct pfcp_tlv));
          hbreq->ts.tsv = htonl(seq);

          sendto(wr_socket, (void *)hbreq, sizeof(struct pfcp_hbreq),
                 MSG_CONFIRM, (const struct sockaddr *) &s_addr,
                 sizeof(s_addr));
          //printf("HB sent seq %u ts %u\n", ntohl(hbreq->hdr.seq) >> 8, ntohl(hbreq->ts.tsv));
          seq++;
          nmsg++;
          break;

        case 4:

          shdr = (struct pfcp_shdr *)buffer;
          shdr->ver = 3;
          shdr->mp = 0;
          shdr->sbit = 0;
          sereq = (void *)buffer;
          sereq->hdr.seq = htonl(seq << 8);
          sereq->hdr.mt = PFCP_SESSION_EST_REQ;
          emt = PFCP_SESSION_EST_RSP;
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

          sendto(wr_socket, (void *)sereq, sizeof(struct pfcp_sereq),
                 MSG_CONFIRM, (const struct sockaddr *) &s_addr,
                 sizeof(s_addr));
          //printf("est sent seq %u f-seid %lu.\n", ntohl(sereq->hdr.seq) >> 8, be64toh(sereq->fseid.seid));
          seq++;
          nmsg++;
          break;

        case 5:

          shdr = (struct pfcp_shdr *)buffer;
          shdr->ver = 3;
          shdr->mp = 0;
          shdr->sbit = 1;
          shdr->seq = htonl(seq << 8);
          semreq = (void *)buffer;
          semreq->hdr.mt = PFCP_SESSION_MOD_REQ;
          emt = PFCP_SESSION_MOD_RSP;
          semreq->hdr.mlen = htons(sizeof(*semreq) - sizeof(struct pfcp_tlv));
          semreq->hdr.seid = htobe64(seid + gap);

          sendto(wr_socket, (void *)semreq, sizeof(struct pfcp_semreq),
                 MSG_CONFIRM, (const struct sockaddr *) &s_addr,
                 sizeof(s_addr));
          //printf("mod req sent seq %u u-seid %lu.\n", ntohl(semreq->hdr.seq) >> 8, be64toh(semreq->hdr.seid));
          seq++;
          nmsg++;
          break;

        case 6:

          shdr = (struct pfcp_shdr *)buffer;
          shdr->ver = 3;
          shdr->mp = 0;
          shdr->sbit = 1;
          shdr->seq = htonl(seq << 8);
          sedreq = (void *)buffer;
          sedreq->hdr.mt = PFCP_SESSION_DEL_REQ;
          emt = PFCP_SESSION_DEL_RSP;
          sedreq->hdr.mlen = htons(sizeof(*semreq) - sizeof(struct pfcp_tlv));
          sedreq->hdr.seid = htobe64(seid + gap);

          sendto(wr_socket, (void *)sedreq, sizeof(struct pfcp_sedreq),
                 MSG_CONFIRM, (const struct sockaddr *) &s_addr,
                sizeof(s_addr));
          //printf("del req sent seq %u u-seid %lu.\n", ntohl(sedreq->hdr.seq) >> 8, be64toh(sedreq->hdr.seid));
          seq++;
          nmsg++;
          break;
        case 8:
        default:
          exit(0);
      }
      client_chk_rcv(&s_addr, emt, seq-1, &nrmsg);
      seid++;
    }
  } while(1);

  return 0;
}

int server2clienthb(int port_hint)
{
  char buffer[MAXBUF];
  struct pfcp_shdr *shdr;
  struct pfcp_nhdr *nhdr;
  struct pfcp_sereq *sereq;
  struct pfcp_seres *seres;
  struct pfcp_semreq *semreq;
  struct pfcp_semres *semres;
  struct pfcp_sedreq *sedreq;
  struct pfcp_sedres *sedres;
  struct pfcp_asreq *asreq;
  struct pfcp_asres *asres;
  struct pfcp_arreq *arreq;
  struct pfcp_arres *arres;
  struct pfcp_hbreq *hbreq;
  struct pfcp_hbres *hbres;
  struct sockaddr_in s_addr, c_addr;
  
  memset(&s_addr, 0, sizeof(s_addr));
    
  int n, len, ch;
  srandom(port_hint);
  uint32_t seq = random(), num;
  uint64_t seid, gap = 1000;
  do { 

    s_addr.sin_family = AF_INET;
    s_addr.sin_port = htons(PFCP_PORT);
    s_addr.sin_addr.s_addr = inet_addr("10.10.10.1");

    seid = random();
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

    sendto(wr_socket, (void *)hbreq, sizeof(struct pfcp_hbreq),
            MSG_CONFIRM, (const struct sockaddr *) &s_addr,
                          sizeof(s_addr));
    printf("PFCP HB sent seq %u ts %u\n", ntohl(hbreq->hdr.seq) >> 8, ntohl(hbreq->ts.tsv));

     n = recvfrom(wr_socket, (char *)buffer, MAXBUF,
                  MSG_WAITALL, (struct sockaddr *) &s_addr, &len);
     if (n < sizeof(*nhdr)) {
        printf("SIZE err\n");
        sleep(2);
        continue;
     }

     nhdr = (struct pfcp_nhdr *)buffer;
     if (nhdr->mt !=  PFCP_HEARTBEAT_RSP) {
        printf("Unexpected msg %u\n", nhdr->mt);
        sleep(2);
        continue;
     }
     hbres = (struct pfcp_hbres *)buffer;
     printf("HB resp recvd - seq %u rts %u \n",ntohl(hbres->hdr.seq) >> 8, ntohl(hbres->ts.tsv));
     seq++;
     sleep(5);
  } while(1);
}

// Main 
int main(int argc, char **argv)
{
  struct thread_info thread;

  if (argc < 2) {
    printf("Invalid option\n");
    exit(0);
  }

  memset(&thread, 0, sizeof(thread));

  if (!strcmp(argv[1], "client")) {
    printf("Starting Client\n");
    client_socket_init();
    pthread_create(&thread.thread_id, NULL, rcv_thread, NULL);
    client_main(atoi(argv[2]));
  } else if (!strcmp(argv[1], "server")) {
    printf("Starting Server\n");
    client = 0;
    server_socket_init();
    pthread_create(&thread.thread_id, NULL, rcv_thread, NULL);
    server2clienthb(atoi(argv[2]));
  }
}
