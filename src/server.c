/* This is a small demo OPC UA server.  */


#include         "tx_api.h"
#include         "nx_api.h"
#ifdef __PRODUCT_NETXDUO__
#include         "nxd_bsd.h"
#else
#include         "nx_bsd.h"
#endif
#include         <string.h>
#include         <stdlib.h>

#include "nxd_sntp_client.h"



#include <open62541/server.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/server_config_default.h>

#define          DEMO_STACK_SIZE     (16*1024)

#define          SAMPLE_IPV4_ADDRESS IP_ADDRESS(192, 168, 201, 5)
#define          SAMPLE_IPV4_MASK    0xFFFFFF00UL
#define          SAMPLE_IPV4_GATEWAY IP_ADDRESS(192, 168, 201, 1)



/* Define the ThreadX and NetX object control blocks... */

TX_THREAD        thread_server;
NX_PACKET_POOL   bsd_pool;
NX_IP            bsd_ip;


/* Define the counters used in the demo application...  */

ULONG            error_counter;

#ifdef NX_DISABLE_IPV4
/* To send IPv6 packets, define DUO.  */
#define  DUO
#endif


/* Define thread prototypes.  */

VOID        thread_server_entry(ULONG thread_input);
extern VOID _nx_linux_network_driver(NX_IP_DRIVER*);

/* Define main entry point.  */

int main()
{

    /* Enter the ThreadX kernel.  */
    tx_kernel_enter();
}

/* Define what the initial system looks like.  */

void    tx_application_define(void *first_unused_memory)
{
CHAR    *pointer;
UINT    status;

        
    /* Setup the working pointer.  */
    pointer =  (CHAR *) first_unused_memory;

    /* Create a server thread.  */
    tx_thread_create(&thread_server, "Server", thread_server_entry, 0,  
                          pointer, DEMO_STACK_SIZE, 8, 8, TX_NO_TIME_SLICE, TX_AUTO_START);

    pointer =  pointer + DEMO_STACK_SIZE;

    /* Initialize the NetX system.  */
    nx_system_initialize();

    /* Create a BSD packet pool.  */
    status =  nx_packet_pool_create(&bsd_pool, "NetX BSD Packet Pool", 128, pointer, 16384);
    pointer = pointer + 16384;   
    if (status)
    {
        error_counter++;
        printf("Error in creating BSD packet pool\n!");
    }
        
    /* Create an IP instance for BSD.  */
    status = nx_ip_create(&bsd_ip, "BSD IP Instance", SAMPLE_IPV4_ADDRESS, SAMPLE_IPV4_MASK,  &bsd_pool, _nx_linux_network_driver,
                    pointer, 2048, 1);
    pointer =  pointer + 2048;

    if (status)
    {
        error_counter++;
        printf("Error creating BSD IP instance\n!");
    }

    nx_ip_gateway_address_set(&bsd_ip, SAMPLE_IPV4_GATEWAY);
    
#ifndef NX_DISABLE_IPV4
    /* Enable ARP and supply ARP cache memory for BSD IP Instance */
    status =  nx_arp_enable(&bsd_ip, (void *) pointer, 1024);
    pointer = pointer + 1024; 

    /* Check ARP enable status.  */     
    if (status)
    {
        error_counter++;
        printf("Error in Enable ARP and supply ARP cache memory to BSD IP instance\n");
    }   
#endif /* NX_DISABLE_IPV4 */

    /* Enable TCP processing for BSD IP instances.  */

    status = nx_tcp_enable(&bsd_ip);

    /* Check TCP enable status.  */
    if (status)
    {
        error_counter++;
        printf("Error in Enable TCP \n");
    }   

    status = nx_udp_enable(&bsd_ip);

    /* Check UDP enable status.  */
    if (status)
    {
        error_counter++;
        printf("Error in Enable UDP \n");
    }

    status = nx_icmp_enable(&bsd_ip);

    /* Check ICMP enable status.  */
    if (status)
    {
        error_counter++;
        printf("Error in Enable ICMP \n");
    }

    /* Now initialize BSD Scoket Wrapper */
    status = (UINT)nx_bsd_initialize (&bsd_ip, &bsd_pool,pointer, 2048, 2);
}

#define SAMPLE_SNTP_SERVER_ADDRESS        IP_ADDRESS(192, 168, 201, 1)
#define SAMPLE_SNTP_SYNC_MAX              3
#define SAMPLE_SNTP_UPDATE_MAX            10
#define SAMPLE_SNTP_UPDATE_INTERVAL       (NX_IP_PERIODIC_RATE / 2)

/* Seconds between Unix Epoch (1/1/1970) and NTP Epoch (1/1/1999) */
#define SAMPLE_UNIX_TO_NTP_EPOCH_SECOND   0x83AA7E80

static NX_SNTP_CLIENT   sntp_client;
extern ULONG unix_time_base;

/* Sync up the local time.  */
static UINT sntp_time_sync()
{

    UINT    status;
    UINT    server_status;
    ULONG   sntp_server_address;
    UINT    i;


    printf("SNTP Time Sync...\r\n");

    sntp_server_address = SAMPLE_SNTP_SERVER_ADDRESS;

    /* Create the SNTP Client to run in broadcast mode.. */
    status = nx_sntp_client_create(&sntp_client, &bsd_ip, 0, &bsd_pool,
        NX_NULL,
        NX_NULL,
        NX_NULL /* no random_number_generator callback */);

    /* Check status.  */
    if (status)
    {
        return(status);
    }

    /* Use the IPv4 service to initialize the Client and set the IPv4 SNTP server. */
    status = nx_sntp_client_initialize_unicast(&sntp_client, sntp_server_address);

    /* Check status.  */
    if (status)
    {
        nx_sntp_client_delete(&sntp_client);
        return(status);
    }

    /* Set local time to 0 */
    status = nx_sntp_client_set_local_time(&sntp_client, 0, 0);

    /* Check status.  */
    if (status)
    {
        nx_sntp_client_delete(&sntp_client);
        return(status);
    }

    /* Run Unicast client */
    status = nx_sntp_client_run_unicast(&sntp_client);

    /* Check status.  */
    if (status)
    {
        nx_sntp_client_stop(&sntp_client);
        nx_sntp_client_delete(&sntp_client);
        return(status);
    }

    /* Wait till updates are received */
    for (i = 0; i < SAMPLE_SNTP_UPDATE_MAX; i++)
    {

        /* First verify we have a valid SNTP service running. */
        status = nx_sntp_client_receiving_updates(&sntp_client, &server_status);

        /* Check status.  */
        if ((status == NX_SUCCESS) && (server_status == NX_TRUE))
        {

            /* Server status is good. Now get the Client local time. */
            ULONG sntp_seconds, sntp_fraction;
            ULONG system_time_in_second;

            /* Get the local time.  */
            status = nx_sntp_client_get_local_time(&sntp_client, &sntp_seconds, &sntp_fraction, NX_NULL);

            /* Check status.  */
            if (status != NX_SUCCESS)
            {
                continue;
            }

            /* Get the system time in second.  */
            system_time_in_second = tx_time_get() / TX_TIMER_TICKS_PER_SECOND;

            /* Convert to Unix epoch and minus the current system time.  */
            unix_time_base = (sntp_seconds - (system_time_in_second + SAMPLE_UNIX_TO_NTP_EPOCH_SECOND));

            /* Time sync successfully.  */

            /* Stop and delete SNTP.  */
            nx_sntp_client_stop(&sntp_client);
            nx_sntp_client_delete(&sntp_client);

            return(NX_SUCCESS);
        }

        /* Sleep.  */
        tx_thread_sleep(SAMPLE_SNTP_UPDATE_INTERVAL);
    }

    /* Time sync failed.  */

    /* Stop and delete SNTP.  */
    nx_sntp_client_stop(&sntp_client);
    nx_sntp_client_delete(&sntp_client);

    /* Return success.  */
    return(NX_NOT_SUCCESSFUL);
}


UA_Boolean running = true;

VOID  thread_server_entry(ULONG thread_input)
{
    UA_Server *server = UA_Server_new();
    UA_ServerConfig_setDefault(UA_Server_getConfig(server));

    // add a variable node to the adresspace
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Int32 myInteger = 42;
    UA_Variant_setScalarCopy(&attr.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
    attr.description = UA_LOCALIZEDTEXT_ALLOC("en-US", "the answer");
    attr.displayName = UA_LOCALIZEDTEXT_ALLOC("en-US", "the answer");
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING_ALLOC(1, "the.answer");
    UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME_ALLOC(1, "the answer");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    UA_Server_addVariableNode(server, myIntegerNodeId, parentNodeId,
                              parentReferenceNodeId, myIntegerName,
                              UA_NODEID_NULL, attr, NULL, NULL);

    /* allocations on the heap need to be freed */
    UA_VariableAttributes_clear(&attr);
    UA_NodeId_clear(&myIntegerNodeId);
    UA_QualifiedName_clear(&myIntegerName);

    UA_StatusCode retval = UA_Server_run(server, &running);

    UA_Server_delete(server);
}


