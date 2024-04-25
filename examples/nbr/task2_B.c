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

#include "sys/ctimer.h"
#include "board-peripherals.h"

// Identification information of the node
#define NEIGHBOUR_DEVICE_ID 5380 // change this to the device ID of the neighbour
#define CURRENT_DEVICE_ID 9730 // change this to the device ID of the neighbour

// Configures the wake-up timer for neighbour discovery 
#define WAKE_TIME RTIMER_SECOND    // 10 HZ, 0.1s

#define SLEEP_CYCLE  9        	      // 0 for never sleep
#define SLEEP_SLOT RTIMER_SECOND/10   // sleep slot should not be too large to prevent overflow

#define ACK 0
// For neighbour discovery, we would like to send message to everyone. We use Broadcast address:
linkaddr_t dest_addr;

#define NUM_SEND 1 // Keep sending 1 packet while awake
/*---------------------------------------------------------------------------*/
typedef struct {
  unsigned long src_id;
  unsigned long timestamp;
  long seq;
  int light;

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

// CHANGES START
int light_readings[60];
// int num_read = 0;
int sleep_time = 9;

bool is_acked = false;

bool ack_message_sent = false;

// Starts the main contiki neighbour discovery process
PROCESS(task2_B, "cc2650 neighbour discovery process");
AUTOSTART_PROCESSES(&task2_B);


// Function called after reception of a packet; RECEIVE light reading from Node A
void receive_packet_callback(const void *data, uint16_t len, const linkaddr_t *src, const linkaddr_t *dest) 
{
  // Check if the received packet size matches with what we expect it to be
  if(len == sizeof(data_packet)) {

    static data_packet_struct received_packet_data;
    
    // Copy the content of packet into the data structure
    memcpy(&received_packet_data, data, len);
    
    if (received_packet_data.src_id == NEIGHBOUR_DEVICE_ID) {
        
      // Print the details of the received packet
      printf("Received neighbour discovery packet %lu with rssi %d from %ld\r\n", received_packet_data.seq, (signed short)packetbuf_attr(PACKETBUF_ATTR_RSSI),received_packet_data.src_id);
      
      // check seq number of received packet data
      if(received_packet_data.seq == 0){
        is_acked = true;
        nullnet_buf = (uint8_t *)&data_packet;
        nullnet_len = sizeof(data_packet);
        data_packet.seq = 0;
        data_packet.timestamp = clock_time();
        data_packet.src_id = CURRENT_DEVICE_ID;
        data_packet.light = 0;
        NETSTACK_NETWORK.output(&dest_addr);
        // reset sleep time
        //sleep_time = 0;
      } else {
        // store light reading
        light_readings[received_packet_data.seq-1] = received_packet_data.light;
        printf("Light reading %ld stored\n", received_packet_data.seq);
      }
    }
  }

}

// Scheduler function for the sender of neighbour discovery packets
// Sends out one packet every WAKE_TIME
char sender_scheduler(struct rtimer *t, void *ptr) {
 
  static uint16_t i = 0;
  
  static int NumSleep=0;
 
  // Begin the protothread
  PT_BEGIN(&pt);

  // Get the current time stamp
  curr_timestamp = clock_time();

  printf("Start clock %lu ticks, timestamp %3lu.%03lu\n", curr_timestamp, curr_timestamp / CLOCK_SECOND, 
  ((curr_timestamp % CLOCK_SECOND)*1000) / CLOCK_SECOND);

  while(1){

    // radio on
    NETSTACK_RADIO.on();

    // send NUM_SEND number of neighbour discovery beacon packets
    for(i = 0; i < NUM_SEND; i++){
        
      if (is_acked && ack_message_sent == false) {
          // acked, break
          // send ack
          ack_message_sent = true;
          break;
      }

      // Initialize the nullnet module with information of packet to be trasnmitted
      nullnet_buf = (uint8_t *)&data_packet; //data transmitted
      nullnet_len = sizeof(data_packet); //length of data transmitted
      
      curr_timestamp = clock_time();
      
      data_packet.timestamp = curr_timestamp;

      printf("Send seq# %ld  @ %8lu ticks   %3lu.%03lu\n", data_packet.seq, curr_timestamp, curr_timestamp / CLOCK_SECOND, ((curr_timestamp % CLOCK_SECOND)*1000) / CLOCK_SECOND);

      NETSTACK_NETWORK.output(&dest_addr); //Packet transmission
      

      // wait for WAKE_TIME before sending the next packet
      if(i != (NUM_SEND - 1)){

        rtimer_set(t, RTIMER_TIME(t) + WAKE_TIME, 1, (rtimer_callback_t)sender_scheduler, ptr);
        PT_YIELD(&pt);
      
      }
   
    }

    // sleep for a random number of slots
    if(sleep_time != 0){
    
      // radio off
      NETSTACK_RADIO.off();

      // SLEEP_SLOT cannot be too large as value will overflow,
      // to have a large sleep interval, sleep many times instead

      // get a value that is uniformly distributed between 0 and 2*SLEEP_CYCLE
      // the average is SLEEP_CYCLE 
      NumSleep = random_rand() % (2 * SLEEP_CYCLE + 1);  
      printf(" Sleep for %d slots \n",NumSleep);

      // NumSleep should be a constant or static int
      for(i = 0; i < NumSleep; i++){
        rtimer_set(t, RTIMER_TIME(t) + SLEEP_SLOT, 1, (rtimer_callback_t)sender_scheduler, ptr);
        PT_YIELD(&pt);
      }
    }
  }
  
  PT_END(&pt);
}

// Main thread that handles the neighbour discovery process
PROCESS_THREAD(task2_B, ev, data)
{

 // static struct etimer periodic_timer;

  PROCESS_BEGIN();

    // initialize data packet sent for neighbour discovery exchange
  data_packet.src_id = node_id; //Initialize the node ID
  data_packet.seq = -1; //Initialize the sequence number of the packet
  data_packet.light = 0;
  
  nullnet_set_input_callback(receive_packet_callback); //initialize receiver callback; handle storing of light data
  linkaddr_copy(&dest_addr, &linkaddr_null);

  printf("CC2650 neighbour discovery\n");
  printf("Node %d will be sending packet of size %d Bytes\n", node_id, (int)sizeof(data_packet_struct));

  // Start sender in one millisecond.
  rtimer_set(&rt, RTIMER_NOW() + (RTIMER_SECOND / 1000), 1, (rtimer_callback_t)sender_scheduler, NULL);
  
  PROCESS_END();
}
