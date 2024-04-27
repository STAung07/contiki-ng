/*
* CS4222/5422: Assignment 3b
* Perform neighbour discovery
*/

#include "contiki.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"
#include "net/packetbuf.h"
#include "lib/random.h"
#include "net/linkaddr.h"
#include <string.h>
#include <stdio.h> 
#include "node-id.h"

// Identification information of the node

#define SLOT_TIME RTIMER_SECOND/10
#define NUM_PRIME 3


// For neighbour discovery, we would like to send message to everyone. We use Broadcast address:
linkaddr_t dest_addr;

/*---------------------------------------------------------------------------*/
typedef struct {
  unsigned long src_id;
  unsigned long timestamp;
  unsigned long seq;
  
} data_packet_struct;

/*---------------------------------------------------------------------------*/
// duty cycle = WAKE_TIME / (WAKE_TIME + SLEEP_SLOT * SLEEP_CYCLE)
/*---------------------------------------------------------------------------*/

// sender timer implemented using rtimer
static struct rtimer rt;

// Protothread variable
static struct pt pt;

// Structure holding the data to be transmitted
static data_packet_struct data_packet;

// Current time stamp of the node
unsigned long curr_timestamp;

// Starts the main contiki neighbour discovery process
PROCESS(task1, "task1");
AUTOSTART_PROCESSES(&task1);

// Function called after reception of a packet
void receive_packet_callback(const void *data, uint16_t len, const linkaddr_t *src, const linkaddr_t *dest) 
{


  // Check if the received packet size matches with what we expect it to be

  if(len == sizeof(data_packet)) {

 
    static data_packet_struct received_packet_data;
    
    // Copy the content of packet into the data structure
    memcpy(&received_packet_data, data, len);
    

    // Print the details of the received packet
    printf("Recv packet - src id: %ld seq: %lu rssi: %d\r\n",received_packet_data.src_id, received_packet_data.seq, (signed short)packetbuf_attr(PACKETBUF_ATTR_RSSI));
  }

}

// Scheduler function for the sender of neighbour discovery packets
char sender_scheduler(struct rtimer *t, void *ptr) {
  // Begin the protothread
  PT_BEGIN(&pt);

  printf("Sender Scheduler called\r\n");

  while(1){
    // radio on
    NETSTACK_RADIO.on();

    printf("Sending packet - seq: %lu\r\n", data_packet.seq);
    // Initialize the nullnet module with information of packet to be trasnmitted
    nullnet_buf = (uint8_t *)&data_packet; //data transmitted
    nullnet_len = sizeof(data_packet); //length of data transmitted
    
    data_packet.seq++;
    
    curr_timestamp = clock_time();
    
    data_packet.timestamp = curr_timestamp;

    NETSTACK_NETWORK.output(&dest_addr); //Packet transmission

    rtimer_set(t, RTIMER_TIME(t) + SLOT_TIME, 1, (rtimer_callback_t)sender_scheduler, ptr);
    PT_YIELD(&pt);

    printf("Sending packet - seq: %lu\r\n", data_packet.seq);
    // Initialize the nullnet module with information of packet to be trasnmitted
    nullnet_buf = (uint8_t *)&data_packet; //data transmitted
    nullnet_len = sizeof(data_packet); //length of data transmitted
    
    data_packet.seq++;
    
    curr_timestamp = clock_time();
    
    data_packet.timestamp = curr_timestamp;

    NETSTACK_NETWORK.output(&dest_addr); //Packet transmission
    
    // radio off
    NETSTACK_RADIO.off();

    rtimer_set(t, RTIMER_TIME(t) + SLOT_TIME * (NUM_PRIME - 1), 1, (rtimer_callback_t)sender_scheduler, ptr);
    PT_YIELD(&pt);
  }
  
  PT_END(&pt);
}


// Main thread that handles the neighbour discovery process
PROCESS_THREAD(task1, ev, data)
{

 // static struct etimer periodic_timer;

  PROCESS_BEGIN();

    // initialize data packet sent for neighbour discovery exchange
  data_packet.src_id = node_id; //Initialize the node ID
  data_packet.seq = 0; //Initialize the sequence number of the packet
  
  nullnet_set_input_callback(receive_packet_callback); //initialize receiver callback
  linkaddr_copy(&dest_addr, &linkaddr_null);



  printf("CC2650 neighbour discoveryr\n");
  printf("Node %d will be sending packet of size %d Bytesr\n", node_id, (int)sizeof(data_packet_struct));

  // Start sender in one millisecond.
  rtimer_set(&rt, RTIMER_NOW() + (RTIMER_SECOND / 1000), 1, (rtimer_callback_t)sender_scheduler, NULL);

  

  PROCESS_END();
}
