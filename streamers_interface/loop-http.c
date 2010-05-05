/*
 *  Copyright (c) 2010 Luca Abeni
 *  Copyright (c) 2010 Csaba Kiraly
 *
 *  This is free software; see gpl-3.0.txt
 */
#include <sys/select.h>
#include <sys/time.h>
#include <time.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include <net_helper.h>
#include <msg_types.h>
#include <peerset.h>
#include <peer.h>

#include "chunk_signaling.h"
#include "streaming.h"
#include "topology.h"
#include "loop.h"
#include "dbg.h"

#define BUFFSIZE 512 * 1024
static struct timeval period = {0, 500000};
static struct timeval tnext;

extern pthread_mutex_t cb_mutex;

void tout_init(struct timeval *tv)
{
  struct timeval tnow;

  if (tnext.tv_sec == 0) {
    gettimeofday(&tnext, NULL);
  }
  gettimeofday(&tnow, NULL);
  if(timercmp(&tnow, &tnext, <)) {
    timersub(&tnext, &tnow, tv);
  } else {
    *tv = (struct timeval){0, 0};
  }
}

void loop(struct nodeID *s, int csize, int buff_size)
{
  int done = 0;
  static uint8_t buff[BUFFSIZE];
  int cnt = 0;
  
  period.tv_sec = csize / 1000000;
  period.tv_usec = csize % 1000000;
  
  sigInit(s);
  peers_init();
  update_peers(NULL, NULL, 0);
  stream_init(buff_size, s);
  while (!done) {
    int len, res;
    struct timeval tv;

    tout_init(&tv);
    res = wait4data(s, &tv);
    if (res > 0) {
      struct nodeID *remote;

      len = recv_from_peer(s, &remote, buff, BUFFSIZE);
      if (len < 0) {
        fprintf(stderr,"Error receiving message. Maybe larger than %d bytes\n", BUFFSIZE);
        nodeid_free(remote);
        continue;
      }
      switch (buff[0] /* Message Type */) {
        case MSG_TYPE_TOPOLOGY:
          update_peers(remote, buff, len);
          break;
        case MSG_TYPE_CHUNK:
          dprintf("Chunk message received:\n");
    pthread_mutex_lock(&cb_mutex);
          received_chunk(remote, buff, len, true);
    pthread_mutex_unlock(&cb_mutex);
          break;
        case MSG_TYPE_SIGNALLING:
          sigParseData(remote, buff, len);
          break;
        default:
          fprintf(stderr, "Unknown Message Type %x\n", buff[0]);
      }
      nodeid_free(remote);
    } else {
      struct timeval tmp;

      //send_chunk();
      send_offer();
      if (cnt++ % 10 == 0) {
        update_peers(NULL, NULL, 0);
      }
      timeradd(&tnext, &period, &tmp);
      tnext = tmp;
    }
  }
}

void source_push_chunk(int chunks)
{
      int i;
        for (i = 0; i < chunks; i++) {	// @TODO: why this cycle?
    pthread_mutex_lock(&cb_mutex);
          send_chunk();
    pthread_mutex_unlock(&cb_mutex);
        }
}

void source_loop(const char *fname, struct nodeID *s, int csize, int chunks, bool loop)
{
  int done = 0;
  static uint8_t buff[BUFFSIZE];
  int cnt = 0;

  period.tv_sec = csize  / 1000000;
  period.tv_usec = csize % 1000000;
  
  sigInit(s);
  peers_init();
  if (source_init(fname, s, loop) < 0) {
    fprintf(stderr,"Cannot initialize source, exiting");
    return;
  }
  while (!done) {
    int len, res;
    struct timeval tv;

//    tout_init(&tv);
    res = wait4data(s, NULL);
    if (res > 0) {
      struct nodeID *remote;

      len = recv_from_peer(s, &remote, buff, BUFFSIZE);
      if (len < 0) {
        fprintf(stderr,"Error receiving message. Maybe larger than %d bytes\n", BUFFSIZE);
        nodeid_free(remote);
        continue;
      }
      dprintf("Received message (%c) from %s\n", buff[0], node_addr(remote));
      switch (buff[0] /* Message Type */) {
        case MSG_TYPE_TOPOLOGY:
          fprintf(stderr, "Top Parse\n");
          update_peers(remote, buff, len);
          break;
        case MSG_TYPE_CHUNK:
          fprintf(stderr, "Some dumb peer pushed a chunk to me!\n");
    pthread_mutex_lock(&cb_mutex);
            received_chunk(remote, buff, len, false);
    pthread_mutex_unlock(&cb_mutex);
            source_push_chunk(chunks);
            if (cnt++ % 10 == 0) {
              update_peers(NULL, NULL, 0);
            }
          break;
        case MSG_TYPE_SIGNALLING:
          sigParseData(remote, buff, len);
          break;
        default:
          fprintf(stderr, "Bad Message Type %x\n", buff[0]);
      }
      nodeid_free(remote);
    } else {
      int res;
      struct timeval tmp, d;

      exit(1);
      d.tv_sec = 0;
    pthread_mutex_lock(&cb_mutex);
      res = generated_chunk(&d.tv_usec);
      if (res) {
        source_push_chunk(chunks);
      }
    pthread_mutex_unlock(&cb_mutex);
      timeradd(&tnext, &d, &tmp);
      tnext = tmp;
    }
  }
}
