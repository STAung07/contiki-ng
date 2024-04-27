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

// For neighbour discovery, we would like to send message to everyone. We use Broadcast address:
linkaddr_t dest_addr;

/*---------------------------------------------------------------------------*/
typedef struct
{
    unsigned long src_id;
    unsigned long seq;
} neighbour_discovery_packet_struct;

typedef struct
{
    unsigned long src_id;
    unsigned long dest_id;
    unsigned long seq;
} acknowledgment_packet_struct;

typedef struct
{
    unsigned long src_id;
    unsigned long dest_id;
    unsigned long seq;
    int payload_length;
    int payload[60]; // Put light readings in payload field
} data_packet_struct;
/*---------------------------------------------------------------------------*/

// Structure holding the data to be transmitted
static acknowledgment_packet_struct acknowledgment_packet;

// sender timer implemented using rtimer
static struct rtimer rt;

// Protothread variable
static struct pt pt;

// Current time stamp of the node
unsigned long curr_timestamp;

// Neighbour discovery
#define SLOT_TIME RTIMER_SECOND
#define NUM_PRIME 3
volatile int neighbour_discovered = 0;
volatile unsigned long neighbour_id;

// State machine after neighbour discovered
#define BEFORE_RECIEVING_DATA 0
#define RECIEVING_DATA 1
#define NO_MORE_DATA 2
int state;
int num_recv;

// Starts the main contiki neighbour discovery process
PROCESS(task2_B_galvin, "task2_B_galvin");
AUTOSTART_PROCESSES(&task2_B_galvin);

// Function called after reception of a packet
void receive_packet_callback(const void *data, uint16_t len, const linkaddr_t *src, const linkaddr_t *dest)
{
    // Check if the received packet size matches with what we expect it to be

    if (len == sizeof(neighbour_discovery_packet_struct))
    {
        static neighbour_discovery_packet_struct received_packet;

        // Copy the content of packet into the data structure
        memcpy(&received_packet, data, len);

        // Print the details of the received packet
        printf("Recv neighbour discovery packet - src id: %ld seq: %ld rssi: %d\r\n",
               received_packet.src_id, received_packet.seq, (signed short)packetbuf_attr(PACKETBUF_ATTR_RSSI));

        neighbour_discovered = 1;
        neighbour_id = received_packet.src_id;
    }

    // if (len == sizeof(data_packet_struct))
    // {
    //     static data_packet_struct received_packet;

    //     // Copy the content of packet into the data structure
    //     memcpy(&received_packet, data, len);

    //     // Print the details of the received packet
    //     printf("Recv data packet - src id: %ld dest id: %ld seq: %ld payload len: %d rssi: %d\r\n",
    //            received_packet.src_id, received_packet.dest_id, received_packet.seq, received_packet.payload_length, (signed short)packetbuf_attr(PACKETBUF_ATTR_RSSI));

    //     if (received_packet.payload_length < 0)
    //     {
    //         neighbour_discovered = 0;
    //     }
    // }
}

// Scheduler function for the sender of neighbour discovery packets
char sender_scheduler(struct rtimer *t, void *ptr)
{
    // Begin the protothread
    PT_BEGIN(&pt);

    printf("Sender Scheduler called\r\n");

    while (1)
    {
        // radio on
        NETSTACK_RADIO.on();

        curr_timestamp = clock_seconds();
        printf("[%ld] Radio on\r\n", curr_timestamp);

        rtimer_set(t, RTIMER_TIME(t) + SLOT_TIME, 1, (rtimer_callback_t)sender_scheduler, ptr);
        PT_YIELD(&pt);

        if (neighbour_discovered)
        {
            curr_timestamp = clock_seconds();
            printf("[%ld] Detected Node ID: %ld\r\n", curr_timestamp, neighbour_id);
            while (1)
            {
                acknowledgment_packet.dest_id = neighbour_id;
                acknowledgment_packet.seq = num_recv;


                // Initialize the nullnet module with information of packet to be transmitted
                nullnet_buf = (uint8_t *)&acknowledgment_packet; // data transmitted
                nullnet_len = sizeof(acknowledgment_packet);     // length of data transmitted

                curr_timestamp = clock_seconds();
                printf("[%ld] Sending acknowledgement\r\n", curr_timestamp);

                NETSTACK_NETWORK.output(&dest_addr); // Packet transmission
                rtimer_set(t, RTIMER_TIME(t) + SLOT_TIME, 1, (rtimer_callback_t)sender_scheduler, ptr);
                PT_YIELD(&pt);
            }
        }

        curr_timestamp = clock_seconds();
        printf("[%ld] Radio off\r\n", curr_timestamp);
        // radio off
        NETSTACK_RADIO.off();

        rtimer_set(t, RTIMER_TIME(t) + SLOT_TIME * (NUM_PRIME - 1), 1, (rtimer_callback_t)sender_scheduler, ptr);
        PT_YIELD(&pt);
    }

    PT_END(&pt);
}

// Main thread that handles the neighbour discovery process
PROCESS_THREAD(task2_B_galvin, ev, data)
{
    PROCESS_BEGIN();

    // initialize data packet sent for neighbour discovery exchange
    acknowledgment_packet.src_id = node_id; // Initialize the node ID

    nullnet_set_input_callback(receive_packet_callback); // initialize receiver callback
    linkaddr_copy(&dest_addr, &linkaddr_null);

    printf("CC2650 neighbour discoveryr\n");
    printf("Node %d will be sending packet\r\n", node_id);

    // Start sender in one millisecond.
    rtimer_set(&rt, RTIMER_NOW() + (RTIMER_SECOND / 1000), 1, (rtimer_callback_t)sender_scheduler, NULL);

    PROCESS_END();
}
