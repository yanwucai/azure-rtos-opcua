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
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>

#define          DEMO_STACK_SIZE     (16*1024)

#define          SAMPLE_IPV4_ADDRESS IP_ADDRESS(192, 168, 201, 5)
#define          SAMPLE_IPV4_MASK    0xFFFFFF00UL
#define          SAMPLE_IPV4_GATEWAY IP_ADDRESS(192, 168, 201, 1)

#define          DISCOVERY_SERVER_ENDPOINT "opc.tcp://192.168.201.1:4840"


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

static UA_StatusCode
readInteger(UA_Server *server, const UA_NodeId *sessionId,
            void *sessionContext, const UA_NodeId *nodeId,
            void *nodeContext, UA_Boolean includeSourceTimeStamp,
            const UA_NumericRange *range, UA_DataValue *value) {
    UA_Int32 *myInteger = (UA_Int32*)nodeContext;
    value->hasValue = true;
    UA_Variant_setScalarCopy(&value->value, myInteger, &UA_TYPES[UA_TYPES_INT32]);

    // we know the nodeid is a string
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Node read %.*s",
                (int)nodeId->identifier.string.length,
                nodeId->identifier.string.data);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "read value %i", *(UA_UInt32 *)myInteger);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
writeInteger(UA_Server *server, const UA_NodeId *sessionId,
             void *sessionContext, const UA_NodeId *nodeId,
             void *nodeContext, const UA_NumericRange *range,
             const UA_DataValue *value) {
    UA_Int32 *myInteger = (UA_Int32*)nodeContext;
    if(value->hasValue && UA_Variant_isScalar(&value->value) &&
       value->value.type == &UA_TYPES[UA_TYPES_INT32] && value->value.data)
        *myInteger = *(UA_Int32 *)value->value.data;

    // we know the nodeid is a string
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "Node written %.*s",
                (int)nodeId->identifier.string.length,
                nodeId->identifier.string.data);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND,
                "written value %i", *(UA_UInt32 *)myInteger);
    return UA_STATUSCODE_GOOD;
}

VOID  thread_server_entry(ULONG thread_input)
{
    UA_Server *server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);
    UA_ServerConfig_setMinimal(config, 4841, NULL);

    UA_String_clear(&config->applicationDescription.applicationUri);
    config->applicationDescription.applicationUri =
        UA_String_fromChars("urn:open62541.example.server_register");

    // See http://www.opcfoundation.org/UA/schemas/1.04/ServerCapabilities.csv
    //config.serverCapabilitiesSize = 1;
    //UA_String caps = UA_String_fromChars("LDS");
    //config.serverCapabilities = &caps;

    /* add a variable node to the address space */
    UA_Int32 myInteger = 42;
    UA_NodeId myIntegerNodeId = UA_NODEID_STRING(1, "the.answer");
    UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME(1, "the answer");
    UA_DataSource dateDataSource;
    dateDataSource.read = readInteger;
    dateDataSource.write = writeInteger;
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    attr.description = UA_LOCALIZEDTEXT("en-US", "the answer");
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "the answer");

    UA_Server_addDataSourceVariableNode(server, myIntegerNodeId,
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                                        UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                                        myIntegerName, UA_NODEID_NULL, attr, dateDataSource,
                                        &myInteger, NULL);

    UA_Client *clientRegister = UA_Client_new();
    UA_ClientConfig_setDefault(UA_Client_getConfig(clientRegister));

    // periodic server register after 10 Minutes, delay first register for 500ms
    UA_UInt64 callbackId;
    UA_StatusCode retval =
        UA_Server_addPeriodicServerRegisterCallback(server, clientRegister, DISCOVERY_SERVER_ENDPOINT,
                                                    10 * 60 * 1000, 500, &callbackId);
    // UA_StatusCode retval = UA_Server_addPeriodicServerRegisterJob(server,
    // "opc.tcp://localhost:4840", 10*60*1000, 500, NULL);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Could not create periodic job for server register. StatusCode %s",
                     UA_StatusCode_name(retval));
        UA_Client_disconnect(clientRegister);
        UA_Client_delete(clientRegister);
        UA_Server_delete(server);
        return;
    }

    retval = UA_Server_run(server, &running);

    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Could not start the server. StatusCode %s",
                     UA_StatusCode_name(retval));
        UA_Client_disconnect(clientRegister);
        UA_Client_delete(clientRegister);
        UA_Server_delete(server);
        return;
    }

    // Unregister the server from the discovery server.
    retval = UA_Server_unregister_discovery(server, clientRegister);
    //retval = UA_Server_unregister_discovery(server, "opc.tcp://localhost:4840" );
    if(retval != UA_STATUSCODE_GOOD)
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Could not unregister server from discovery server. StatusCode %s",
                     UA_StatusCode_name(retval));

    UA_Server_removeCallback(server, callbackId);

    UA_Client_disconnect(clientRegister);
    UA_Client_delete(clientRegister);
    UA_Server_delete(server);
}


