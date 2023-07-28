/* This is a small demo of OPC UA client.  */


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

#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/plugin/log_stdout.h>

#define          DEMO_STACK_SIZE     (16*1024)

#define          SAMPLE_IPV4_ADDRESS IP_ADDRESS(192, 168, 201, 5)
#define          SAMPLE_IPV4_MASK    0xFFFFFF00UL
#define          SAMPLE_IPV4_GATEWAY IP_ADDRESS(192, 168, 201, 1)

#define DISCOVERY_SERVER_ENDPOINT "opc.tcp://192.168.201.1:4840"

/* Define the ThreadX and NetX object control blocks... */

TX_THREAD        thread_client;
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
VOID        thread_client_entry(ULONG thread_input);
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

    /* Create a Client thread.  */
    tx_thread_create(&thread_client, "Client", thread_client_entry, 0,  
                          pointer, DEMO_STACK_SIZE, 16, 16, TX_NO_TIME_SLICE, TX_AUTO_START);

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
    status = nx_ip_create(&bsd_ip, "BSD IP Instance", SAMPLE_IPV4_ADDRESS, SAMPLE_IPV4_MASK, &bsd_pool, _nx_linux_network_driver,
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

VOID  thread_client_entry(ULONG thread_input)
{
    //sntp_time_sync();


    /*
     * Example for calling FindServersOnNetwork
     */

    {
        UA_ServerOnNetwork *serverOnNetwork = NULL;
        size_t serverOnNetworkSize = 0;

        UA_Client *client = UA_Client_new();
        UA_ClientConfig_setDefault(UA_Client_getConfig(client));
        UA_StatusCode retval = UA_Client_findServersOnNetwork(client, DISCOVERY_SERVER_ENDPOINT, 0, 0,
                                                              0, NULL, &serverOnNetworkSize, &serverOnNetwork);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                         "Could not call FindServersOnNetwork service. "
                         "Is the discovery server started? StatusCode %s",
                         UA_StatusCode_name(retval));
            UA_Client_delete(client);
            return;
        }

        // output all the returned/registered servers
        for(size_t i = 0; i < serverOnNetworkSize; i++) {
            UA_ServerOnNetwork *server = &serverOnNetwork[i];
            printf("Server[%lu]: %.*s", (unsigned long) i,
                   (int) server->serverName.length, server->serverName.data);
            printf("\n\tRecordID: %u", server->recordId);
            printf("\n\tDiscovery URL: %.*s", (int) server->discoveryUrl.length,
                   server->discoveryUrl.data);
            printf("\n\tCapabilities: ");
            for(size_t j = 0; j < server->serverCapabilitiesSize; j++) {
                printf("%.*s,", (int) server->serverCapabilities[j].length,
                       server->serverCapabilities[j].data);
            }
            printf("\n\n");
        }

        UA_Array_delete(serverOnNetwork, serverOnNetworkSize,
                        &UA_TYPES[UA_TYPES_SERVERONNETWORK]);
    }

    /* Example for calling FindServers */
    UA_ApplicationDescription *applicationDescriptionArray = NULL;
    size_t applicationDescriptionArraySize = 0;

    UA_StatusCode retval;
    {
        UA_Client *client = UA_Client_new();
        UA_ClientConfig_setDefault(UA_Client_getConfig(client));
        retval = UA_Client_findServers(client, DISCOVERY_SERVER_ENDPOINT, 0, NULL, 0, NULL,
                                       &applicationDescriptionArraySize, &applicationDescriptionArray);
        UA_Client_delete(client);
    }
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Could not call FindServers service. "
                "Is the discovery server started? StatusCode %s", UA_StatusCode_name(retval));
        return;
    }

    // output all the returned/registered servers
    for(size_t i = 0; i < applicationDescriptionArraySize; i++) {
        UA_ApplicationDescription *description = &applicationDescriptionArray[i];
        printf("Server[%lu]: %.*s", (unsigned long) i, (int) description->applicationUri.length,
               description->applicationUri.data);
        printf("\n\tName: %.*s", (int) description->applicationName.text.length,
               description->applicationName.text.data);
        printf("\n\tApplication URI: %.*s", (int) description->applicationUri.length,
               description->applicationUri.data);
        printf("\n\tProduct URI: %.*s", (int) description->productUri.length,
               description->productUri.data);
        printf("\n\tType: ");
        switch(description->applicationType) {
            case UA_APPLICATIONTYPE_SERVER:
                printf("Server");
                break;
            case UA_APPLICATIONTYPE_CLIENT:
                printf("Client");
                break;
            case UA_APPLICATIONTYPE_CLIENTANDSERVER:
                printf("Client and Server");
                break;
            case UA_APPLICATIONTYPE_DISCOVERYSERVER:
                printf("Discovery Server");
                break;
            default:
                printf("Unknown");
        }
        printf("\n\tDiscovery URLs:");
        for(size_t j = 0; j < description->discoveryUrlsSize; j++) {
            printf("\n\t\t[%lu]: %.*s", (unsigned long) j,
                   (int) description->discoveryUrls[j].length,
                   description->discoveryUrls[j].data);
        }
        printf("\n\n");
    }


    /*
     * Now that we have the list of available servers, call get endpoints on all of them
     */

    printf("-------- Server Endpoints --------\n");

    for(size_t i = 0; i < applicationDescriptionArraySize; i++) {
        UA_ApplicationDescription *description = &applicationDescriptionArray[i];
        if(description->discoveryUrlsSize == 0) {
            UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_CLIENT,
                        "[GetEndpoints] Server %.*s did not provide any discovery urls. Skipping.",
                        (int)description->applicationUri.length, description->applicationUri.data);
            continue;
        }

        printf("\nEndpoints for Server[%lu]: %.*s\n", (unsigned long) i,
               (int) description->applicationUri.length, description->applicationUri.data);

        UA_Client *client = UA_Client_new();
        UA_ClientConfig_setDefault(UA_Client_getConfig(client));

        char *discoveryUrl = (char *) UA_malloc(sizeof(char) * description->discoveryUrls[0].length + 1);
        memcpy(discoveryUrl, description->discoveryUrls[0].data, description->discoveryUrls[0].length);
        discoveryUrl[description->discoveryUrls[0].length] = '\0';

        UA_EndpointDescription *endpointArray = NULL;
        size_t endpointArraySize = 0;
        //TODO: adapt to the new async getEndpoint
        retval = UA_Client_getEndpoints(client, discoveryUrl, &endpointArraySize, &endpointArray);
        UA_free(discoveryUrl);
        if(retval != UA_STATUSCODE_GOOD) {
            UA_Client_disconnect(client);
            UA_Client_delete(client);
            break;
        }

        for(size_t j = 0; j < endpointArraySize; j++) {
            UA_EndpointDescription *endpoint = &endpointArray[j];
            printf("\n\tEndpoint[%lu]:", (unsigned long) j);
            printf("\n\t\tEndpoint URL: %.*s", (int) endpoint->endpointUrl.length, endpoint->endpointUrl.data);
            printf("\n\t\tTransport profile URI: %.*s", (int) endpoint->transportProfileUri.length,
                   endpoint->transportProfileUri.data);
            printf("\n\t\tSecurity Mode: ");
            switch(endpoint->securityMode) {
            case UA_MESSAGESECURITYMODE_INVALID:
                printf("Invalid");
                break;
            case UA_MESSAGESECURITYMODE_NONE:
                printf("None");
                break;
            case UA_MESSAGESECURITYMODE_SIGN:
                printf("Sign");
                break;
            case UA_MESSAGESECURITYMODE_SIGNANDENCRYPT:
                printf("Sign and Encrypt");
                break;
            default:
                printf("No valid security mode");
                break;
            }
            printf("\n\t\tSecurity profile URI: %.*s", (int) endpoint->securityPolicyUri.length,
                   endpoint->securityPolicyUri.data);
            printf("\n\t\tSecurity Level: %d", endpoint->securityLevel);
        }

        UA_Array_delete(endpointArray, endpointArraySize, &UA_TYPES[UA_TYPES_ENDPOINTDESCRIPTION]);
        UA_Client_delete(client);
    }

    printf("\n");

    UA_Array_delete(applicationDescriptionArray, applicationDescriptionArraySize,
                    &UA_TYPES[UA_TYPES_APPLICATIONDESCRIPTION]);

}
