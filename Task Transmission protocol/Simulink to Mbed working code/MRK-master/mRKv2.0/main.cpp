#include <nrk.h>
#include <include.h>
#include <ulib.h>
#include <stdio.h>
#include <hal.h>
#include <nrk_error.h>
#include <nrk_timer.h>
#include <nrk_stack_check.h>
#include <nrk_stats.h>
#include <string.h>

#include "basic_rf.h"
#include "bmac.h"
#include <ff_basic_sensor.h>
#include <nrk_driver_list.h>
#include <nrk_driver.h>
#include "messageTypes.h"
#include "packetHandler.h"

#define SENDER 1 
#define RECEIVER_NODE 2
// Only require MAC address for address decode
//#define MAC_ADDR    0x0001
// Change this to your group channel
#define MY_CHANNEL 2

#define MAX_MOLES  4
#define MY_ID 2

nrk_task_type RX_TASK;
NRK_STK rx_task_stack[NRK_APP_STACKSIZE];
void rx_task (void);

nrk_task_type TX_TASK;
NRK_STK tx_task_stack[NRK_APP_STACKSIZE];
void tx_task (void);

nrk_task_type RE_TX_TASK;
NRK_STK re_tx_task_stack[NRK_APP_STACKSIZE];
void re_tx_task (void);



void nrk_create_taskset ();

char tx_buf[RF_MAX_PAYLOAD_SIZE];
char rx_buf[RF_MAX_PAYLOAD_SIZE];
char ack_buf[5];
/*************************************************************

Global variables related to code transmission


*************************************************************/
uint8_t data[512];
uint8_t receiveData[512];
char sendNextPacket=0;
char receiverNextPacket = 0;
int sequenceNumber = 0;
int lastSentSequenceNo[MAX_MOLES] ={0};
int lastReceivedSequenceNo[MAX_MOLES]={0};
int lastPacketTransmitted=0;
uint8_t transmittedPacketLength =0;

int main(void)

{
    int r;
    
    nrk_setup_ports();
    nrk_init();
    bmac_task_config();
    nrk_create_taskset();
    bmac_init (MY_CHANNEL);
    bmac_auto_ack_disable();
    for(r=512;r>0;r--)
    {
        data[r-512] = r;

    }

    sendNextPacket=START;
    nrk_start();
    return 0;

}

void rx_task ()
{ 

    uint8_t rssi,len,*local_rx_buf,mole;
    int receivedSeqNo=0,senderNode = 0;
    bmac_set_cca_thresh(DEFAULT_BMAC_CCA);
    bmac_rx_pkt_set_buffer (rx_buf,RF_MAX_PAYLOAD_SIZE);
    //cleaning up the target sector in the flash
    
    while(!bmac_started());
    printf("Receiver node Bmac initialised\n");

    while(1) {
        nrk_wait_until_next_period();

        if( !bmac_rx_pkt_ready())
            continue;
        printf("received packet\n\r");
        nrk_led_toggle(ORANGE_LED);
        local_rx_buf = (uint8_t *)bmac_rx_pkt_get (&len, &rssi);

        //If my own packet then dont receive
        if(local_rx_buf[0]==MY_ID)
        {
            continue;
        }
        //Sample the data
        receivedSeqNo = local_rx_buf[2];
        senderNode = local_rx_buf[1];


        if(len!=0)
        {
            switch(local_rx_buf[3])
            {
            case TASK:
                if(receivedSeqNo>lastReceivedSequenceNo[senderNode])
                {
                    //Process and send the acknowledgement to the receiver
                    lastReceivedSequenceNo[senderNode]=receivedSeqNo;
                    extractPacket(local_rx_buf,receiveData,len);

                    receiverNextPacket= SEND_ACK;
                    ack_buf[DESTINATION_ADDRESS_LOCATION]=senderNode;
                    ack_buf[SOURCE_ADDRESS_LOCATION]= MY_ID;
                    ack_buf[SEQUENCE_NUM_LOCATION] = receivedSeqNo;
                    ack_buf[MESSAGE_TYPE_LOCATION] = TASK_PACKET_ACK;
                    receiverNextPacket= SEND_ACK;
                    if(local_rx_buf[MESSAGE_TYPE_LOCATION]==LAST_PACKET)
                    {
                        for(int i=0;i<1024;i++)
                        {
                            printf("%d \t",receiveData[i]);

                        }
                    }
                }
                else
                {
                    ack_buf[DESTINATION_ADDRESS_LOCATION]=senderNode;
                    ack_buf[SOURCE_ADDRESS_LOCATION]= MY_ID;
                    ack_buf[SEQUENCE_NUM_LOCATION] = lastReceivedSequenceNo[senderNode];
                    ack_buf[MESSAGE_TYPE_LOCATION] = TASK_PACKET_ACK;
                    //Send the acknowledgement to the sender
                    receiverNextPacket= SEND_ACK;

                }

                break;
            case TASK_PACKET_ACK:

                //If it is a new ACK packet then accept it. Else dont bother
                if(lastSentSequenceNo[senderNode]==receivedSeqNo)
                {
                    switch(sendNextPacket)
                    {
                    case WAIT_ACK:
                        sendNextPacket= IN_PROGRESS;
                        break;
                    case LAST_PACKET_ACK:
                        sendNextPacket = END;
                        break;
                    default:
                        printf("Error shouldn't have entered here");
                        break;

                    }
                }


                break;

            }
        }
        bmac_rx_pkt_release ();
    }
    // pointing the function pointer to the copied code in the flash

}



void tx_task ()
{
    int r;
    uint8_t val;

    nrk_sig_t tx_done_signal;
    while (!bmac_started ())
        nrk_wait_until_next_period ();

    tx_done_signal = bmac_get_tx_done_signal ();
    nrk_signal_register (tx_done_signal);
    tx_buf[0] = MY_ID;
    tx_buf[1] = RECEIVER_NODE;
    while(1){

        switch(sendNextPacket)
        {
        case START:
            tx_buf[2] = ++sequenceNumber;
            lastSentSequenceNo[RECEIVER_NODE]= sequenceNumber;
            r=createNextTaskPacket(tx_buf,data,1024);
            transmittedPacketLength = r+HEADER_SIZE;
            if(val == NRK_OK)
            {
                if(r == MAX_PACKET_SIZE)
                {
                    sendNextPacket = WAIT_ACK;
                    val=bmac_tx_pkt(tx_buf, transmittedPacketLength);
                }
                else
                {
                    sendNextPacket = LAST_PACKET_ACK;
                    val=bmac_tx_pkt(tx_buf, transmittedPacketLength);
                }
            }
            else 
            {
                printf("ERROR IN TRANSMISSION\r\n");
            }
            break;

        case IN_PROGRESS:
            tx_buf[2] = ++sequenceNumber;
            r=createNextTaskPacket(tx_buf,data,1024);
            lastSentSequenceNo[RECEIVER_NODE]= sequenceNumber;
            if(val == NRK_OK)
            {
                if(r == MAX_PACKET_SIZE)
                {
                    sendNextPacket = WAIT_ACK; //Waiting for ack
                    val=bmac_tx_pkt(tx_buf, transmittedPacketLength);
                }
                else
                {
                    sendNextPacket = LAST_PACKET_ACK;//If last packet wait for ack
                    val=bmac_tx_pkt(tx_buf, transmittedPacketLength);
                }
            }
            else 
            {
                printf("ERROR IN TRANSMISSION\r\n");

            }
            break;
        case WAIT_ACK:
        case END:
        default:
            break;



        }

        nrk_wait_until_next_period();
    }

}

void re_tx_task()
{

		uint8_t val;
    nrk_sig_t tx_done_signal;

    while (!bmac_started ())
        nrk_wait_until_next_period ();

    tx_done_signal = bmac_get_tx_done_signal ();
    nrk_signal_register (tx_done_signal);

    while(1)
    {

        switch(sendNextPacket)
        {
        case LAST_PACKET_ACK:
        case WAIT_ACK:
            val=bmac_tx_pkt(tx_buf, transmittedPacketLength);
            if(val == NRK_OK)
            {
                printf("Retransmitted\r\n");
            }
            else
            {
                printf("ERROR IN TRANSMISSION\r\n");

            }
            break;
        default:
            break;
        }

        switch(receiverNextPacket)
        {
        case SEND_ACK:
            val=bmac_tx_pkt(ack_buf,5);
            receiverNextPacket = STOP;
            break;
        default:
            break;

         }


    nrk_wait_until_next_period();



    }
}
    void nrk_create_taskset ()
    {


        RX_TASK.task = rx_task;
        nrk_task_set_stk( &RX_TASK, rx_task_stack, NRK_APP_STACKSIZE);
        RX_TASK.prio = 2;
        RX_TASK.FirstActivation = TRUE;
        RX_TASK.Type = BASIC_TASK;
        RX_TASK.SchType = PREEMPTIVE;
        RX_TASK.period.secs = 100;
        RX_TASK.period.nano_secs = 0;
        RX_TASK.cpu_reserve.secs = 0;
        RX_TASK.cpu_reserve.nano_secs = 30 * NANOS_PER_MS;
        RX_TASK.offset.secs = 0;
        RX_TASK.offset.nano_secs = 0;
        nrk_activate_task (&RX_TASK);

        TX_TASK.task = tx_task;
        nrk_task_set_stk( &TX_TASK, tx_task_stack, NRK_APP_STACKSIZE);
        TX_TASK.prio = 1;
        TX_TASK.FirstActivation = TRUE;
        TX_TASK.Type = BASIC_TASK;
        TX_TASK.SchType = PREEMPTIVE;
        TX_TASK.period.secs = 100;
        TX_TASK.period.nano_secs = 0;
        TX_TASK.cpu_reserve.secs = 0;
        TX_TASK.cpu_reserve.nano_secs = 30 * NANOS_PER_MS;
        TX_TASK.offset.secs = 0;
        TX_TASK.offset.nano_secs = 0;
        nrk_activate_task (&TX_TASK);

        RE_TX_TASK.task = re_tx_task;
        nrk_task_set_stk( &RE_TX_TASK, re_tx_task_stack, NRK_APP_STACKSIZE);
        RE_TX_TASK.prio = 1;
        RE_TX_TASK.FirstActivation = TRUE;
        RE_TX_TASK.Type = BASIC_TASK;
        RE_TX_TASK.SchType = PREEMPTIVE;
        RE_TX_TASK.period.secs = 100;
        RE_TX_TASK.period.nano_secs = 0;
        RE_TX_TASK.cpu_reserve.secs = 0;
        RE_TX_TASK.cpu_reserve.nano_secs = 20 * NANOS_PER_MS;
        RE_TX_TASK.offset.secs = 0;
        RE_TX_TASK.offset.nano_secs = 0;
        nrk_activate_task (&RE_TX_TASK);



        printf ("Create done\r\n");
    }
