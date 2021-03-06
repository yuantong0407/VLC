#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/dma-mapping.h>
#include <linux/timer.h>

#define ARQ_STOP_N_WAIT

// common definitions
struct packet {
	char *vaddr;
	dma_addr_t handle;
	unsigned int len;
};

extern int dma_write_simple(char* buff, dma_addr_t handle, unsigned int buff_len);

#ifdef ARQ_SELECTIVE_REPEAT
// selective repeat ARQ
// resend if
// get NAK
// get timeout
#define SW 128

static unsigned int seq_start;
static unsigned int seq_end;

void sr_init(void);
void sr_send(char* buff, unsigned int bufflen);
void sr_handle_ack(unsigned char seq);
void sr_handle_nak(unsigned char seq);

static inline void inc(unsigned int *s){
	*s++;
	if (s == SW){
		s = 0;
	}
}

void sr_init(void){
	seq_start = 0;
	seq_end = 0;
}

void sr_send(char* buff, unsigned int bufflen){
	// put seq number at end of buff
}


#endif

// OK lets do stop n' wait
#ifdef ARQ_STOP_N_WAIT

#define WAITQSIZE 512

static spinlock_t lock;
// static int snw_idle; // protected by lock


static struct packet snw_packet_queue[WAITQSIZE]; // never changes
static unsigned int snw_queue_head, snw_queue_tail; // protected by lock

static struct timer_list snw_timer;

#define TIMEOUT 20
// static struct timespec snw_timer_begin, snw_timer_end;

void snw_init(void);
void snw_exit(void);
int snw_send(char* buff, unsigned int bufflen);
int snw_handle_ack(void);
int snw_handle_nak(void);

void fill_snw_queue(char* v, dma_addr_t h, int i){
	snw_packet_queue[i].vaddr = v;
	snw_packet_queue[i].handle = h;
}


static inline void inc(unsigned int *idx){
	(*idx) ++;
	if (*idx == WAITQSIZE){
		*idx = 0;
	}
}

static inline unsigned int dec(unsigned int *idx){
	if (*idx == 0){
		return WAITQSIZE - 1;
	}
	else{
		return (*idx) - 1;
	}
}

static inline void __snw_packet_enqueue(char* buff, unsigned int buff_len){
	memcpy(snw_packet_queue[dec(&snw_queue_tail)].vaddr, buff, buff_len);
	kfree(buff);
	snw_packet_queue[dec(&snw_queue_tail)].len = buff_len;
}

static inline int __snw_tranmit_head(void){
	mod_timer(&snw_timer, jiffies + msecs_to_jiffies(TIMEOUT));
	return dma_write_simple(snw_packet_queue[snw_queue_head].vaddr, 
		snw_packet_queue[snw_queue_head].handle,
		snw_packet_queue[snw_queue_head].len); // need to change this API
}

void snw_handle_timeout(unsigned long useless){
	// retransmit!
	printk("@@ Retransmit!!\n");
	spin_lock(&lock);
	if (snw_queue_head != snw_queue_tail){
		spin_unlock(&lock);
		__snw_tranmit_head();
	}
	else{
		spin_unlock(&lock);
	}
}

void snw_init(void){
	spin_lock_init(&lock);
	snw_queue_head = 0;
	snw_queue_tail = 0; // nothing in queue

	//init timer
	init_timer(&snw_timer);
	setup_timer(&snw_timer, snw_handle_timeout, 0);
	//init snw_packet_queue in dma_init()
}

void snw_exit(void){
	// dalloc dma in dma_exit()
	del_timer(&snw_timer);
}

// static inline int isempty(unsigned int *head, unsigned int *tail){
// 	return *tail == *head;
// }


int snw_send(char* buff, unsigned int bufflen){
	// EFFECTS: send a packet to dma
	// queued if previous packet not sent
	// dropped if queue is full
	// INVARIANTS:
	// 0. snw_idle = queue_empty
	// 1. if queue not empty, then need to queue
	spin_lock(&lock);
	if (snw_queue_head == snw_queue_tail){
		// queue empty && ack replied
		inc(&snw_queue_tail);
		spin_unlock(&lock);
		// write to dma on queue tail (now snw_queue_tail - 1)
		__snw_packet_enqueue(buff, bufflen);
		// send queue head
		__snw_tranmit_head();

	}
	else{
		if (snw_queue_tail + 1 == snw_queue_head){
			// queue full, drop packet
			spin_unlock(&lock);
			return -1;
		}
		else{
			// queued
			inc(&snw_queue_tail);
			spin_unlock(&lock);
			// write to dma on queue tail (now snw_queue_tail - 1)
			__snw_packet_enqueue(buff, bufflen);
		}
	}

	return 0;
}

int snw_handle_ack(void){
	spin_lock(&lock);
	if (snw_queue_head + 1 == snw_queue_tail){
		// nothing on queue
		inc(&snw_queue_head);
		// deactivate timer
		spin_unlock(&lock);
		// del_timer(&snw_timer);
		// done
	}
	else if (snw_queue_head == snw_queue_tail){
		// there must be something wrong
		spin_unlock(&lock);
		return 666666;
	}
	else{
		// queue not empty
		// transmit next packet
		inc(&snw_queue_head);
		spin_unlock(&lock);
		// send queue head (snw_queue_head)
		__snw_tranmit_head();
	}

	return 0;

}

int snw_handle_nak(void){
	// retransmit!
	// send queue head (snw_queue_head)
	return __snw_tranmit_head();
}

#endif