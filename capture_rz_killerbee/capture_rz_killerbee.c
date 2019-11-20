
#include "../config.h"

#include "rz_killerbee.h"

#include <libusb-1.0/libusb.h>

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "../capture_framework.h"

/* Unique instance data passed around by capframework */
typedef struct {
    libusb_context *libusb_ctx;
    libusb_device_handle *rz_killerbee_handle;

    unsigned int devno, busno;

    pthread_mutex_t usb_mutex;

    /* we don't want to do a channel query every data response, we just want to 
     * remember the last channel used */
    unsigned int channel;

    /*keep track of our errors so we can reset if needed*/
    unsigned char error_ctr;

    bool ready;

    kis_capture_handler_t *caph;
} local_rz_killerbee_t;

/* Most basic of channel definitions */
typedef struct {
    unsigned int channel;
} local_channel_t;

int rz_killerbee_init(kis_capture_handler_t *caph) {
    local_rz_killerbee_t *localrz_killerbee = (local_rz_killerbee_t *) caph->userdata;
    int ret;

printf("ready set to false\n");
    localrz_killerbee->ready = false;

    printf("libusb_reset_device\n");
    //printf("mutexlock");
    pthread_mutex_lock(&(localrz_killerbee->usb_mutex));
    ret = libusb_reset_device(localrz_killerbee->rz_killerbee_handle);
    //printf("mutexunlock");
    pthread_mutex_unlock(&(localrz_killerbee->usb_mutex));
    printf("libusb_reset_device ret:%d\n",ret);
    if(ret < 0)
        return -1; 

    //set the configurationlibusb_set_configuration
    printf("set the configuration\n");
    //printf("mutexlock");
    pthread_mutex_lock(&(localrz_killerbee->usb_mutex));
    ret = libusb_set_configuration(localrz_killerbee->rz_killerbee_handle, 1);
    //printf("mutexunlock");
    pthread_mutex_unlock(&(localrz_killerbee->usb_mutex));
    printf("libusb_set_configuration ret:%d\n",ret);
    if(ret < 0)
        return -1;

    printf("libusb_claim_interface\n");
    //printf("mutexlock");
    pthread_mutex_lock(&(localrz_killerbee->usb_mutex));
    ret = libusb_claim_interface(localrz_killerbee->rz_killerbee_handle, 0);
    //printf("mutexunlock");
    pthread_mutex_unlock(&(localrz_killerbee->usb_mutex));
    printf("libusb_claim_interface ret:%d\n",ret);
    if(ret < 0)
        return -1;

    printf("libusb_set_interface_alt_setting\n");
    //printf("mutexlock");
    pthread_mutex_lock(&(localrz_killerbee->usb_mutex));
    ret = libusb_set_interface_alt_setting(localrz_killerbee->rz_killerbee_handle,0x00,0x00);
    //printf("mutexunlock");
    pthread_mutex_unlock(&(localrz_killerbee->usb_mutex));
    printf("libusb_set_interface_alt_setting ret:%d\n",ret);
    if(ret < 0)
        return -1;

    return ret;
}

int rz_killerbee_set_mode(kis_capture_handler_t *caph, uint8_t mode) {

    local_rz_killerbee_t *localrz_killerbee = (local_rz_killerbee_t *) caph->userdata;
    int ret;

    printf("set mode\n");
    int xfer = 0;
    unsigned char data[2];
    data[0]=RZ_KILLERBEE_SET_MODE;
    data[1]=RZ_KILLERBEE_CMD_MODE_AC;
    //printf("mutexlock");
    pthread_mutex_lock(&(localrz_killerbee->usb_mutex));
    ret = libusb_bulk_transfer(localrz_killerbee->rz_killerbee_handle, RZ_KILLERBEE_CMD_EP, data, sizeof(data), &xfer, RZ_KILLERBEE_CMD_TIMEOUT);
    //printf("mutexunlock");
    pthread_mutex_unlock(&(localrz_killerbee->usb_mutex));
    printf("set mode:%d xfer:%d\n",ret,xfer);
    printf("rz_killerbee_set_mode ret:%d\n",ret);
    return ret;
}

int rz_killerbee_set_channel(kis_capture_handler_t *caph, uint8_t channel) {
    printf("channel %u\n", channel);
    local_rz_killerbee_t *localrz_killerbee = (local_rz_killerbee_t *) caph->userdata;
    int ret;
    int xfer = 0;
    unsigned char data[2];
    printf("rz_killerbee_set_channel\n");
    data[0]=RZ_KILLERBEE_SET_CHANNEL;
    data[1]=channel;
    //printf("mutexlock");
    pthread_mutex_lock(&(localrz_killerbee->usb_mutex));
    ret = libusb_bulk_transfer(localrz_killerbee->rz_killerbee_handle, RZ_KILLERBEE_CMD_EP, data, sizeof(data), &xfer, RZ_KILLERBEE_CMD_TIMEOUT);
    //printf("mutexunlock");
    pthread_mutex_unlock(&(localrz_killerbee->usb_mutex));
    printf("set channel:%d xfer:%d\n",channel,xfer);

    printf("rz_killerbee_set_channel ret:%d\n",ret); 
    return ret;
}///mutex inside

int rz_killerbee_open_stream(kis_capture_handler_t *caph) {
    int ret;
    int xfer = 0;
    local_rz_killerbee_t *localrz_killerbee = (local_rz_killerbee_t *) caph->userdata;
    unsigned char data[2];
    //open stream
    printf("rz_killerbee_open_stream\n");
    data[0]=RZ_KILLERBEE_OPEN_STREAM;
    //printf("mutexlock");
    pthread_mutex_lock(&(localrz_killerbee->usb_mutex));
    ret = libusb_bulk_transfer(localrz_killerbee->rz_killerbee_handle, RZ_KILLERBEE_CMD_EP, data, sizeof(data)-1, &xfer, RZ_KILLERBEE_CMD_TIMEOUT);
    //printf("mutexunlock");
    pthread_mutex_unlock(&(localrz_killerbee->usb_mutex));
    printf("open stream:%d xfer:%d\n",ret,xfer);

    printf("rz_killerbee_open_stream ret:%d\n",ret);

printf("ready set to true\n");
    localrz_killerbee->ready = true;

    return ret;
}//mutex inside

int rz_killerbee_close_stream(kis_capture_handler_t *caph) {
    int ret;
    int xfer = 0;
    local_rz_killerbee_t *localrz_killerbee = (local_rz_killerbee_t *) caph->userdata;
    unsigned char data[2];

printf("ready set to false\n");
    localrz_killerbee->ready = false;
    //open stream
    printf("rz_killerbee_close_stream\n");
    data[0]=RZ_KILLERBEE_CLOSE_STREAM;
    //printf("mutexlock");
    pthread_mutex_lock(&(localrz_killerbee->usb_mutex));
    ret = libusb_bulk_transfer(localrz_killerbee->rz_killerbee_handle, RZ_KILLERBEE_CMD_EP, data, sizeof(data)-1, &xfer, RZ_KILLERBEE_CMD_TIMEOUT);
    //printf("mutexunlock");
    pthread_mutex_unlock(&(localrz_killerbee->usb_mutex));
    printf("close stream:%d xfer:%d\n",ret,xfer);

    printf("rz_killerbee_close_stream ret:%d\n",ret);

    return ret;
}//mutex inside

int rz_killerbee_receive_payload(kis_capture_handler_t *caph, uint8_t *rx_buf, size_t rx_max) {
    local_rz_killerbee_t *localrz_killerbee = (local_rz_killerbee_t *) caph->userdata;
    int actual_len, r;
    //printf("rz_killerbee_receive_payload \n");
    //printf("mutexlock");
    pthread_mutex_lock(&(localrz_killerbee->usb_mutex));
    r = libusb_bulk_transfer(localrz_killerbee->rz_killerbee_handle, RZ_KILLERBEE_REP_EP, rx_buf, rx_max, &actual_len, RZ_KILLERBEE_READ_TIMEOUT);
    //printf("mutexunlock");
    pthread_mutex_unlock(&(localrz_killerbee->usb_mutex));
//    printf("rz_killerbee_receive_payload r:%d actual_len:%d\n",r,actual_len);
    if(r == LIBUSB_ERROR_TIMEOUT) {
//        localrz_killerbee->error_ctr++;
//        if(localrz_killerbee->error_ctr >= 50)
//            return r;
//        else
//printf("return 1\n");
            return 1;/*continue on for now*/
    }
        
    if (r < 0)
        return r;
    localrz_killerbee->error_ctr = 0;/*we got something valid so reset*/
    return actual_len;
}//mutex inside

int probe_callback(kis_capture_handler_t *caph, uint32_t seqno, char *definition,
        char *msg, char **uuid, KismetExternal__Command *frame,
        cf_params_interface_t **ret_interface,
        cf_params_spectrum_t **ret_spectrum) {
printf("probe_callback\n"); 
    char *placeholder = NULL;
    int placeholder_len;
    char *interface;
    char errstr[STATUS_MAX];

    *ret_spectrum = NULL;
    *ret_interface = cf_params_interface_new();

    int x;
    int busno = -1, devno = -1;

    libusb_device **libusb_devs = NULL;
    ssize_t libusb_devices_cnt = 0;
    int r;

    int matched_device = 0;

    local_rz_killerbee_t *localrz_killerbee = (local_rz_killerbee_t *) caph->userdata;

    if ((placeholder_len = cf_parse_interface(&placeholder, definition)) <= 0) {
        snprintf(msg, STATUS_MAX, "Unable to find interface in definition"); 
        return 0;
    }

    interface = strndup(placeholder, placeholder_len);

    /* Look for the interface type */
    if (strstr(interface, "rz_killerbee") != interface) {
        free(interface);
        return 0;
    }

    /* Look for interface-bus-dev */
    x = sscanf(interface, "rz_killerbee-%d-%d", &busno, &devno);
    free(interface);

    /* If we don't have a valid busno/devno or malformed interface name */
    if (x != -1 && x != 2) {
        return 0;
    }
    printf("mutexlock");pthread_mutex_lock(&(localrz_killerbee->usb_mutex));
    libusb_devices_cnt = libusb_get_device_list(localrz_killerbee->libusb_ctx, &libusb_devs);

    if (libusb_devices_cnt < 0) {
        return 0;
    }

    for (ssize_t i = 0; i < libusb_devices_cnt; i++) {
        struct libusb_device_descriptor dev;

        r = libusb_get_device_descriptor(libusb_devs[i], &dev);

        if (r < 0) {
            continue;
        }

        if (dev.idVendor == RZ_KILLERBEE_USB_VENDOR && dev.idProduct == RZ_KILLERBEE_USB_PRODUCT) {
            if (busno >= 0) {
                if (busno == libusb_get_bus_number(libusb_devs[i]) &&
                        devno == libusb_get_device_address(libusb_devs[i])) {
                    matched_device = 1;
                    break;
                }
            } else {
                matched_device = 1;
                busno = libusb_get_bus_number(libusb_devs[i]);
                devno = libusb_get_device_address(libusb_devs[i]);
                break;
            }
        }
    }
    libusb_free_device_list(libusb_devs, 1);
    printf("mutexunlock");pthread_mutex_unlock(&(localrz_killerbee->usb_mutex));

    /* Make a spoofed, but consistent, UUID based on the adler32 of the interface name 
     * and the location in the bus */
    snprintf(errstr, STATUS_MAX, "%08X-0000-0000-0000-%06X%06X",
            adler32_csum((unsigned char *) "kismet_cap_rz_killerbee", 
                strlen("kismet_cap_rz_killerbee")) & 0xFFFFFFFF,
            busno, devno);
    *uuid = strdup(errstr);

    /* RZ KILLERBEE */
    (*ret_interface)->channels = (char **) malloc(sizeof(char *) * 16);
    for (int i = 11; i < 27; i++) {
        char chstr[4];
        snprintf(chstr, 4, "%d", i);
        (*ret_interface)->channels[i - 11] = strdup(chstr);
    }

    (*ret_interface)->channels_len = 16;

    return 1;
}/////mutex inside

int list_callback(kis_capture_handler_t *caph, uint32_t seqno,
        char *msg, cf_params_list_interface_t ***interfaces) {
    /* Basic list of devices */
    typedef struct rz_killerbee_list {
        char *device;
        struct rz_killerbee_list *next;
    } rz_killerbee_list_t; 

printf("list_callback\n");

    rz_killerbee_list_t *devs = NULL;
    size_t num_devs = 0;
    libusb_device **libusb_devs = NULL;
    ssize_t libusb_devices_cnt = 0;
    int r;
    char devname[32];
    unsigned int i;

    local_rz_killerbee_t *localrz_killerbee = (local_rz_killerbee_t *) caph->userdata;
    printf("mutexlock");pthread_mutex_lock(&(localrz_killerbee->usb_mutex));
    libusb_devices_cnt = libusb_get_device_list(localrz_killerbee->libusb_ctx, &libusb_devs);
    printf("mutexunlock");pthread_mutex_unlock(&(localrz_killerbee->usb_mutex));
    if (libusb_devices_cnt < 0) {
        return 0;
    }
    printf("mutexlock");pthread_mutex_lock(&(localrz_killerbee->usb_mutex));
    for (ssize_t i = 0; i < libusb_devices_cnt; i++) {
        struct libusb_device_descriptor dev;

        r = libusb_get_device_descriptor(libusb_devs[i], &dev);

        if (r < 0) {
            continue;
        }

        if (dev.idVendor == RZ_KILLERBEE_USB_VENDOR && dev.idProduct == RZ_KILLERBEE_USB_PRODUCT) {
            snprintf(devname, 32, "rz_killerbee-%u-%u",
                libusb_get_bus_number(libusb_devs[i]),
                libusb_get_device_address(libusb_devs[i]));

            rz_killerbee_list_t *d = (rz_killerbee_list_t *) malloc(sizeof(rz_killerbee_list_t));
            num_devs++;
            d->device = strdup(devname);
            d->next = devs;
            devs = d;
        }
    }
    libusb_free_device_list(libusb_devs, 1);
    printf("mutexunlock");pthread_mutex_unlock(&(localrz_killerbee->usb_mutex));
    if (num_devs == 0) {
        *interfaces = NULL;
        return 0;
    }

    *interfaces = 
        (cf_params_list_interface_t **) malloc(sizeof(cf_params_list_interface_t *) * num_devs);

    i = 0;

    while (devs != NULL) {
        rz_killerbee_list_t *td = devs->next;
        (*interfaces)[i] = (cf_params_list_interface_t *) malloc(sizeof(cf_params_list_interface_t));
        memset((*interfaces)[i], 0, sizeof(cf_params_list_interface_t));

        (*interfaces)[i]->interface = devs->device;
        (*interfaces)[i]->flags = NULL;
        (*interfaces)[i]->hardware = strdup("rz_killerbee");

        free(devs);
        devs = td;

        i++;
    }
    return num_devs;
}///mutex inside

int open_callback(kis_capture_handler_t *caph, uint32_t seqno, char *definition,
        char *msg, uint32_t *dlt, char **uuid, KismetExternal__Command *frame,
        cf_params_interface_t **ret_interface,
        cf_params_spectrum_t **ret_spectrum) {

printf("open_callback\n");

    char *placeholder = NULL;
    int placeholder_len;
    char *interface;
    char errstr[STATUS_MAX];

    *ret_spectrum = NULL;
    *ret_interface = cf_params_interface_new();

    int x;
    int busno = -1, devno = -1;

    libusb_device **libusb_devs = NULL;
    libusb_device *matched_dev = NULL;
    ssize_t libusb_devices_cnt = 0;
    int r;

    int matched_device = 0;
    char cap_if[32];

    local_rz_killerbee_t *localrz_killerbee = (local_rz_killerbee_t *) caph->userdata;

    if ((placeholder_len = cf_parse_interface(&placeholder, definition)) <= 0) {
        snprintf(msg, STATUS_MAX, "Unable to find interface in definition"); 
        return 0;
    }

    interface = strndup(placeholder, placeholder_len);

    /* Look for the interface type */
    if (strstr(interface, "rz_killerbee") != interface) {
        snprintf(msg, STATUS_MAX, "Unable to find rz killerbee interface"); 
        free(interface);
        return -1;
    }

    /* Look for interface-bus-dev */
    x = sscanf(interface, "rz_killerbee-%d-%d", &busno, &devno);
    free(interface);

    /* If we don't have a valid busno/devno or malformed interface name */
    if (x != -1 && x != 2) {
        snprintf(msg, STATUS_MAX, "Malformed rz_killerbee interface, expected 'rz_killerbee' or "
                "'rz_killerbee-bus#-dev#'"); 
        return -1;
    }
    //printf("mutexlock");
    pthread_mutex_lock(&(localrz_killerbee->usb_mutex));
    libusb_devices_cnt = libusb_get_device_list(localrz_killerbee->libusb_ctx, &libusb_devs);
    //printf("mutexunlock");
    pthread_mutex_unlock(&(localrz_killerbee->usb_mutex));
    if (libusb_devices_cnt < 0) {
        snprintf(msg, STATUS_MAX, "Unable to iterate USB devices"); 
        return -1;
    }
    //printf("mutexlock");
    pthread_mutex_lock(&(localrz_killerbee->usb_mutex));
    for (ssize_t i = 0; i < libusb_devices_cnt; i++) {
        struct libusb_device_descriptor dev;

        r = libusb_get_device_descriptor(libusb_devs[i], &dev);

        if (r < 0) {
            continue;
        }

        if (dev.idVendor == RZ_KILLERBEE_USB_VENDOR && dev.idProduct == RZ_KILLERBEE_USB_PRODUCT) {
            if (busno >= 0) {
                if (busno == libusb_get_bus_number(libusb_devs[i]) &&
                        devno == libusb_get_device_address(libusb_devs[i])) {
                    matched_device = 1;
                    matched_dev = libusb_devs[i];
                    break;
                }
            } else {
                matched_device = 1;
                busno = libusb_get_bus_number(libusb_devs[i]);
                devno = libusb_get_device_address(libusb_devs[i]);
                matched_dev = libusb_devs[i];
                break;
            }
        }
    }

    if (!matched_device) {
        snprintf(msg, STATUS_MAX, "Unable to find rz_killerbee USB device");
        return -1;
    }

    libusb_free_device_list(libusb_devs, 1);
    printf("mutexunlock");pthread_mutex_unlock(&(localrz_killerbee->usb_mutex));

    snprintf(cap_if, 32, "rz_killerbee-%u-%u", busno, devno);

    localrz_killerbee->devno = devno;
    localrz_killerbee->busno = busno;

    /* Make a spoofed, but consistent, UUID based on the adler32 of the interface name 
     * and the location in the bus */
    snprintf(errstr, STATUS_MAX, "%08X-0000-0000-0000-%06X%06X",
            adler32_csum((unsigned char *) "kismet_cap_rz_killerbee", 
                strlen("kismet_cap_rz_killerbee")) & 0xFFFFFFFF,
            busno, devno);
    *uuid = strdup(errstr);

    (*ret_interface)->capif = strdup(cap_if);
    (*ret_interface)->hardware = strdup("rz_killerbee");

    /* RZ KILLERBEE */
    (*ret_interface)->channels = (char **) malloc(sizeof(char *) * 16);
    for (int i = 11; i < 27; i++) {
        char chstr[4];
        snprintf(chstr, 4, "%d", i);
        (*ret_interface)->channels[i - 11] = strdup(chstr);
    }

    (*ret_interface)->channels_len = 16;

    //printf("mutexlock");
    pthread_mutex_lock(&(localrz_killerbee->usb_mutex));
    /* Try to open it */
    r = libusb_open(matched_dev, &localrz_killerbee->rz_killerbee_handle);
    //printf("mutexunlock");
    pthread_mutex_unlock(&(localrz_killerbee->usb_mutex));
    if (r < 0) {
        snprintf(errstr, STATUS_MAX, "Unable to open rz_killerbee USB interface: %s", 
                libusb_strerror((enum libusb_error) r));
        //printf("mutexunlock");
        pthread_mutex_unlock(&(localrz_killerbee->usb_mutex));
        return -1;
    }

    rz_killerbee_init(caph); 
    rz_killerbee_set_mode(caph,RZ_KILLERBEE_CMD_MODE_AC);
    rz_killerbee_open_stream(caph);
    //rz_killerbee_set_channel(caph, 11);
    //rz_killerbee_open_stream(caph);

    return 1;
}///mutex inside

void *chantranslate_callback(kis_capture_handler_t *caph, char *chanstr) {
    local_channel_t *ret_localchan;
    unsigned int parsechan;
    char errstr[STATUS_MAX];

printf("chantranslate_callback\n");

    if (sscanf(chanstr, "%u", &parsechan) != 1) {
        snprintf(errstr, STATUS_MAX, "1 unable to parse requested channel '%s'; rz killerbee channels "
                "are from 11 to 26", chanstr);
        cf_send_message(caph, errstr, MSGFLAG_INFO);
        return NULL;
    }

    if (parsechan > 26 || parsechan < 11) {
        snprintf(errstr, STATUS_MAX, "2 unable to parse requested channel '%u'; rz killerbee channels "
                "are from 11 to 26", parsechan);
        cf_send_message(caph, errstr, MSGFLAG_INFO);
        return NULL;
    }


    ret_localchan = (local_channel_t *) malloc(sizeof(local_channel_t));
    ret_localchan->channel = parsechan;
    return ret_localchan;
}///

int chancontrol_callback(kis_capture_handler_t *caph, uint32_t seqno, void *privchan,
        char *msg) {
    local_rz_killerbee_t *localrz_killerbee = (local_rz_killerbee_t *) caph->userdata;
    local_channel_t *channel = (local_channel_t *) privchan;
    int r;

printf("chancontrol_callback\n");

    if (privchan == NULL) {
        return 0;
    }

    rz_killerbee_close_stream(caph);

    r = rz_killerbee_set_channel(caph, channel->channel);

    printf("chancontrol_callback r:%d\n",r);

    if (r < 0)
        return -1;

    localrz_killerbee->channel = channel->channel;
   
    rz_killerbee_open_stream(caph);

    return 1;
}///

/* Run a standard glib mainloop inside the capture thread */
void capture_thread(kis_capture_handler_t *caph) {
    local_rz_killerbee_t *localrz_killerbee = (local_rz_killerbee_t *) caph->userdata;
    char errstr[STATUS_MAX];

    uint8_t usb_buf[256];

    int buf_rx_len, r;

    while (1) {
        if (caph->spindown) {
            /*shutdown the adapter?*/
            rz_killerbee_close_stream(caph);
            rz_killerbee_set_mode(caph,RZ_KILLERBEE_CMD_MODE_NONE);

            /* close usb */
            if (localrz_killerbee->rz_killerbee_handle) {
                libusb_close(localrz_killerbee->rz_killerbee_handle);
                localrz_killerbee->rz_killerbee_handle = NULL;
            }

            break;
        }
if(localrz_killerbee->ready)
{
        buf_rx_len = rz_killerbee_receive_payload(caph, usb_buf, 256);
        if (buf_rx_len < 0) {
            snprintf(errstr, STATUS_MAX, "RZ KILLERBEE interface 'rz_killerbee-%u-%u' closed "
                    "unexpectedly", localrz_killerbee->busno, localrz_killerbee->devno);
            cf_send_error(caph, 0, errstr);
            cf_handler_spindown(caph);
            break;
        }

        /* Skip runt packets caused by timeouts */
        if (buf_rx_len == 1)
            continue;

        //the devices look to report a 4 byte counter/heartbeat, skip it
        if(buf_rx_len <= 7)
            continue;

        /**/
        if (buf_rx_len > 1) {
            fprintf(stderr, "rz killerbee saw %d ", buf_rx_len);

            for (int bb = 0; bb < buf_rx_len; bb++) {
                fprintf(stderr, "%02X ", usb_buf[bb] & 0xFF);
            }
            fprintf(stderr, "\n");
        }
        /**/

        while (1) {
            struct timeval tv;

            gettimeofday(&tv, NULL);

            if ((r = cf_send_data(caph,
                            NULL, NULL, NULL,
                            tv,
                            0,
                            buf_rx_len, usb_buf)) < 0) {
                cf_send_error(caph, 0, "unable to send DATA frame");
                cf_handler_spindown(caph);
            } else if (r == 0) {
                cf_handler_wait_ringbuffer(caph);
                continue;
            } else {
                break;
            }
        }
}
    }

    cf_handler_spindown(caph);
}///

int main(int argc, char *argv[]) {
    local_rz_killerbee_t localrz_killerbee = {
        .libusb_ctx = NULL,
        .rz_killerbee_handle = NULL,
        .caph = NULL,
        .error_ctr = 0,
    };

    pthread_mutex_init(&(localrz_killerbee.usb_mutex), NULL);

    kis_capture_handler_t *caph = cf_handler_init("rz_killerbee");
    int r;

    if (caph == NULL) {
        fprintf(stderr, "FATAL: Could not allocate basic handler data, your system "
                "is very low on RAM or something is wrong.\n");
        return -1;
    }

    r = libusb_init(&localrz_killerbee.libusb_ctx);
    if (r < 0) {
        return -1;
    }

    libusb_set_debug(localrz_killerbee.libusb_ctx, 3);

    localrz_killerbee.caph = caph;

    /* Set the local data ptr */
    cf_handler_set_userdata(caph, &localrz_killerbee);

    /* Set the callback for opening  */
    cf_handler_set_open_cb(caph, open_callback);

    /* Set the callback for probing an interface */
    cf_handler_set_probe_cb(caph, probe_callback);

    /* Set the list callback */
    cf_handler_set_listdevices_cb(caph, list_callback);

    /* Channel callbacks */
    cf_handler_set_chantranslate_cb(caph, chantranslate_callback);
    cf_handler_set_chancontrol_cb(caph, chancontrol_callback);

    /* Set the capture thread */
    cf_handler_set_capture_cb(caph, capture_thread);

    if (cf_handler_parse_opts(caph, argc, argv) < 1) {
        cf_print_help(caph, argv[0]);
        return -1;
    }

    /* Support remote capture by launching the remote loop */
    cf_handler_remote_capture(caph);

    /* Jail our ns */
    cf_jail_filesystem(caph);

    /* Strip our privs */
    cf_drop_most_caps(caph);

    cf_handler_loop(caph);
    libusb_exit(localrz_killerbee.libusb_ctx);
    
    return 0;
}
