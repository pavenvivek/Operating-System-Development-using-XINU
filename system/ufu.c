#include <xinu.h>
#include <ufu.h>

/* struct to keep track of allocated futures */
typedef struct alc_f {
  future_t *f;
  struct alc_f *next;
} alc_f;


ufu_t ufu = {
  .slot = EMPTY,
  .locport = 52743
};


syscall ufu_init(char * ip) {

  if (ufu.slot != EMPTY) {
    errormsg("Ufu was already initialized\n");
    return SYSERR;
  }

  if ((ufu.localip = getlocalip()) == SYSERR) {
    errormsg("Could not obtain a local IP address\n");
    return SYSERR;
  }

  if (dot2ip(ip, &ufu.remip) == SYSERR) {
    errormsg("Invalid IP address: %s\n", ip);
    return SYSERR;
  }

  if ((ufu.slot = udp_register(ufu.remip, UFU_PORT, ufu.locport)) == SYSERR) {
    errormsg("udp_register(%s, %d, %d) failed\n", ip, UFU_PORT, ufu.locport);
    return SYSERR;
  }

  return OK;
}

syscall ufu_finalize() {

  if (ufu.slot == EMPTY) {
    errormsg("Ufu is not initialized\n");
    return SYSERR;
  }

  if (udp_release(ufu.slot) == SYSERR) {
    errormsg("udp_release(%d) failed\n", ufu.slot);
    return SYSERR;
  }

  ufu.slot = EMPTY;

  return OK;
}

/**
 * Functions implementing UFU msg passing
 */

syscall ufu_sendexit() {

  ufumsg_t *umsg; 
  int	retval;
 
  umsg = (ufumsg_t *)getmem(sizeof(ufumsg_t));
  if (umsg == (ufumsg_t *)SYSERR) {
    printf("Insufficient memory");
    return SYSERR;
  }

  umsg->type = UFU_EXIT;
  retval = udp_send(ufu.slot, (char *) umsg, sizeof(ufumsg_t));
    
  if (retval == SYSERR) {
    fprintf(stderr, "Udp client: error sending UDP \n");
    return SYSERR;
  }

  retval = udp_recv(ufu.slot, (char *) umsg, sizeof(ufumsg_t), UFU_TIMEOUT);
  if (retval == TIMEOUT) {
    fprintf(stderr, "Udp client: timeout...\n");
    return SYSERR;
  } 
  else if (retval == SYSERR) {
    fprintf(stderr, "Udp client: error from udp_recv \n");
    udp_release(ufu.slot);
    return SYSERR;
  }

  if (umsg->type == UFU_ACK) {
    return OK;
  }
  else if (umsg->type == UFU_NACK) {
    return SYSERR;
  }

  return SYSERR;
}

future_t* ufu_alloc(future_mode_t mode, uint size, uint nelems) {

  ufumsg_t *umsg = NULL;
  int retval;  

  umsg = (ufumsg_t *)getmem(sizeof(ufumsg_t));
  if (umsg == (ufumsg_t *)SYSERR) {
    printf("Insufficient memory");
    return NULL;
  }

  umsg->type = UFU_ALLOC;
  umsg->value0 = size;
  umsg->value1 = nelems;

  retval = udp_send(ufu.slot, (char *) umsg, sizeof(ufumsg_t));
  if (retval == SYSERR) {
    fprintf(stderr, "Udp client: error sending UDP \n");
    return NULL;
  }
  
  retval = udp_recv(ufu.slot, (char *) umsg, sizeof(ufumsg_t), UFU_TIMEOUT);
  if (retval == SYSERR) {
    fprintf(stderr, "Udp client: error from udp_recv \n");
    return NULL;
  }

  if (umsg->type == UFU_ACK) {
    return umsg->address; // Server returns success
  }
  else if (umsg->type == UFU_NACK) {
    return NULL;   // Server returns failure
  }

  return NULL;
}

syscall ufu_free(future_t *fut) {
  ufumsg_t *umsg = NULL;
  int32 retval;

  umsg = (ufumsg_t *)getmem(sizeof(ufumsg_t));
  if (umsg == (ufumsg_t *)SYSERR) {
    printf("Insufficient memory");
    return SYSERR;
  }

  umsg->type = UFU_FREE;
  umsg->address = fut;

  retval = udp_send(ufu.slot, (char *) umsg, sizeof(ufumsg_t));
  if (retval == SYSERR) {
    fprintf(stderr, "Udp client: error sending UDP \n");
    return SYSERR;
  }
  
  retval = udp_recv(ufu.slot, (char *) umsg, sizeof(ufumsg_t), UFU_TIMEOUT);
  if (retval == SYSERR) {
    fprintf(stderr, "Udp client: error from udp_recv \n");
    return SYSERR;
  }

  if (umsg->type == UFU_ACK) {
    return OK; // Server returns success
  }
  else if (umsg->type == UFU_NACK) {
    return SYSERR;   // Server returns failure
  }

  return SYSERR;
}

syscall ufu_set(future_t *fut, char *in) {

  ufumsg_t *umsg = NULL;
  int32 retval;

  umsg = (ufumsg_t *)getmem(sizeof(ufumsg_t));
  if (umsg == (ufumsg_t *)SYSERR) {
    printf("Insufficient memory");
    return SYSERR;
  }

  umsg->type = UFU_SET;
  umsg->address = fut;
  umsg->value0 = *in;

  retval = udp_send(ufu.slot, (char *) umsg, sizeof(ufumsg_t));
  if (retval == SYSERR) {
    fprintf(stderr, "Udp client: error sending UDP \n");
    return SYSERR;
  }
  
  retval = udp_recv(ufu.slot, (char *) umsg, sizeof(ufumsg_t), UFU_TIMEOUT);
  if (retval == SYSERR) {
    fprintf(stderr, "Udp client: error from udp_recv \n");
    return SYSERR;
  }

  if (umsg->type == UFU_ACK) {
    return OK; // Server returns success
  }
  else if (umsg->type == UFU_NACK) {
    return SYSERR;   // Server returns failure
  }

  return SYSERR;
}

syscall ufu_get(future_t *fut, char *out) {

  ufumsg_t *umsg = NULL;
  int32 retval;

  umsg = (ufumsg_t *)getmem(sizeof(ufumsg_t));
  if (umsg == (ufumsg_t *)SYSERR) {
    printf("Insufficient memory");
    return SYSERR;
  }

  umsg->type = UFU_GET;
  umsg->address = fut;

  retval = udp_send(ufu.slot, (char *) umsg, sizeof(ufumsg_t));
  if (retval == SYSERR) {
    fprintf(stderr, "Udp client: error sending UDP \n");
    return SYSERR;
  }
  
  memset(umsg, 0, sizeof(ufumsg_t));
  retval = udp_recv(ufu.slot, (char *) umsg, sizeof(ufumsg_t), UFU_TIMEOUT);
  if (retval == SYSERR) {
    fprintf(stderr, "Udp client: error from udp_recv \n");
    return SYSERR;
  }

  if (umsg->type == UFU_ACK) {
    *out = umsg->value0;
    return OK; // Server returns success
  }
  else if (umsg->type == UFU_NACK) {
    return SYSERR;   // Server returns failure
  }

  return SYSERR;
}

void ufu_listen() {

  uint16 echoserverport = 7777;	/* port number for UDP echo */
  char str[40];
  ufumsg_t *umsg = NULL;
  future_t *f = NULL;
  alc_f *head = NULL;
  alc_f *temp = NULL;
  alc_f *prev = NULL;
  int32 retval;
  int valid = -1;

  if (ufu.slot != EMPTY) {
    errormsg("Ufu was already initialized\n");
    return;
  }

  if ((ufu.localip = getlocalip()) == SYSERR) {
    errormsg("Could not obtain a local IP address\n");
    return;
  }

  if ((ufu.slot = udp_register(0, 0, echoserverport)) == SYSERR) {
    errormsg("Udp server: could not reserve UDP port %d\n", echoserverport);
    return;
  }

  head = (alc_f *) getmem(sizeof(alc_f));  
  head->next = NULL; // dummy header

  sprintf(str, "%d.%d.%d.%d",
		(ufu.localip>>24)&0xff, (ufu.localip>>16)&0xff,
		(ufu.localip>>8)&0xff,        ufu.localip&0xff);
  printf("UDP Echo server running on %s:%d\n", str, echoserverport);

  umsg = (ufumsg_t *)getmem(sizeof(ufumsg_t));
  if (umsg == (ufumsg_t *)SYSERR) {
    printf("Insufficient memory");
    return;
  }

  while (TRUE) {
    retval = udp_recvaddr(ufu.slot, &ufu.remip, &ufu.remport, (char *) umsg, sizeof(ufumsg_t), UFU_TIMEOUT);

    if (retval == TIMEOUT) {
      continue;
    } 
    else if (retval == SYSERR) {
      fprintf(stderr, "Udp server: error receiving UDP\n");
      return;
    }

    if (umsg->type == UFU_ALLOC) {

      future_mode_t mode = FUTURE_EXCLUSIVE;

      f = future_alloc(mode, umsg->value0, umsg->value1);
      if (f == (future_t *) SYSERR) {
        printf("future_alloc(%d, %d, %d) failed\n", mode, umsg->value0, umsg->value1);
        umsg->type = UFU_NACK;
      }
      else {
        printf("future_alloc(%d, %d, %d): 0x%08x\n", mode, umsg->value0, umsg->value1, f);
        umsg->type = UFU_ACK;
        umsg->address = f;

        temp = head;
        while (temp->next != NULL) {
          temp = temp->next;
        }

        temp->next = (alc_f *) getmem(sizeof(alc_f));
        temp->next->f = f;
        temp->next->next = NULL;
      }

      retval = udp_sendto(ufu.slot, ufu.remip, ufu.remport, (char *) umsg, sizeof(ufumsg_t));
      if (retval == SYSERR) {
        fprintf(stderr, "Udp server: udp_sendto failed\n");
        return;
      }
    }
    else if (umsg->type == UFU_SET) {

      if (umsg->address == NULL) {
        printf("future_set(0x%08x, %d) failed \n", umsg->address, umsg->value0);
        umsg->type = UFU_NACK;
      }
      else {
        valid = 0;

        temp = head->next;
        while (temp != NULL) {
          if (temp->f == umsg->address) {
            valid = 1;
            break;
          }
          temp = temp->next;
        }

        if (valid == 0) {
          printf("future_set(0x%08x, %d) failed \n", umsg->address, umsg->value0);
          umsg->type = UFU_NACK;
        }
        else {
          retval = future_set(umsg->address, (char *) &umsg->value0);
          if (retval == SYSERR) {
            printf("future_set(0x%08x, %d) failed \n", umsg->address, umsg->value0);
            umsg->type = UFU_NACK;
          }
          else {
            printf("future_set(0x%08x, %d) succeeded \n", umsg->address, umsg->value0);
            umsg->type = UFU_ACK;
          }
        }
      }

      retval = udp_sendto(ufu.slot, ufu.remip, ufu.remport, (char *) umsg, sizeof(ufumsg_t));
      if (retval == SYSERR) {
        fprintf(stderr, "Udp server: udp_sendto failed\n");
        return;
      }
    }
    else if (umsg->type == UFU_GET) {

      if (umsg->address == NULL) {
        printf("future_get(0x%08x) failed\n", umsg->address);
        umsg->type = UFU_NACK;
      }
      else {
        valid = 0;

        temp = head->next;
        while (temp != NULL) {
          if (temp->f == umsg->address) {
            valid = 1;
            break;
          }
          temp = temp->next;
        }

        if (valid == 0) {
          printf("future_get(0x%08x) failed\n", umsg->address);
          umsg->type = UFU_NACK;
        }
        else {
          retval = future_get(umsg->address, (char *) &umsg->value0);
          if (retval == SYSERR) {
            printf("future_get(0x%08x) failed\n", umsg->address);
            umsg->type = UFU_NACK;
          }
          else {
            printf("future_get(0x%08x): %d\n", umsg->address, umsg->value0);
            umsg->type = UFU_ACK;
          }
        }
      }
        
      retval = udp_sendto(ufu.slot, ufu.remip, ufu.remport, (char *) umsg, sizeof(ufumsg_t));
      if (retval == SYSERR) {
        fprintf(stderr, "Udp server: udp_sendto failed\n");
        return;
      }
    }
    else if (umsg->type == UFU_FREE) {

      if (umsg->address == NULL) {
        printf("future_free(0x%08x) failed\n", umsg->address);
        umsg->type = UFU_NACK;
      }
      else {
        valid = 0;

        prev = head;
        temp = head->next;
        while (temp != NULL) {
          if (temp->f == umsg->address) {
            valid = 1;
            prev->next = temp->next;

            break;
          }
          prev = temp;
          temp = temp->next;
        }

        if (valid == 0) {
          printf("future_free(0x%08x) failed\n", umsg->address);
          umsg->type = UFU_NACK;
        }
        else {
          retval = future_free(umsg->address);
          if (retval == SYSERR) {
            printf("future_free(0x%08x) failed\n", umsg->address);
            umsg->type = UFU_NACK;
          }
          else {
            printf("future_free(0x%08x) succeeded\n", umsg->address);
            umsg->type = UFU_ACK;
          }
        }
      }

      retval = udp_sendto(ufu.slot, ufu.remip, ufu.remport, (char *) umsg, sizeof(ufumsg_t));
      if (retval == SYSERR) {
        fprintf(stderr, "Udp server: udp_sendto failed\n");
        return;
      }
    }
    else if (umsg->type == UFU_EXIT) {
      printf("Exiting\n");

      umsg->type = UFU_ACK;
      retval = udp_sendto(ufu.slot, ufu.remip, ufu.remport, (char *) umsg, sizeof(ufumsg_t));
      if (retval == SYSERR) {
        fprintf(stderr, "Udp server: udp_sendto failed\n");
        return;
      }

      if (udp_release(ufu.slot) == SYSERR) {
        errormsg("udp_release(%d) failed\n", ufu.slot);
        return;
      }
      ufu.slot = EMPTY;

      break;    
    }
  }

  return;
}
