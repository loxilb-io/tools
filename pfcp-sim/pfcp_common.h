#ifndef __PFCP_COMMON_H
#define __PFCP_COMMON_H

#include <stdio.h>

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

void *node_msg_func();

#endif
