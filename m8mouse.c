// SPDX-License-Identifier: MIT
#include "m8mouse.h"

static mode _dpi_modes[] = {
    {"1",           0x00},
    {"2",           0x01},
    {"3",           0x02},
    {"4",           0x03},
    {"5",           0x04},
    {"6",           0x05},
    {0}
};

//dpi resolution at #06, #07, 8, 9, a, b
static mode _dpires_modes[] = {
    {"1 (500)",     0x01},
    {"2 (800)",     0x02},
    {"3 (1000)",    0x03},
    {"4 (1200)",    0x04},
    {"5 (1600)",    0x05},
    {"6 (2000)",    0x06},
    {"7 (2400)",    0x07},
    {"8 (3200)",    0x08},
    {"9 (4000)",    0x09},
    {"10 (4800)",   0x0A},
    {"11 (6400)",   0x0B},
    {"12 (8000)",   0x0C},
    {0}
};


static mode _led_modes[] = {
    {"1 (DPI)",             0x01},
    {"2 (Multicolour)",     0x02},
    {"3 (Rainbow)",         0x03},
    {"4 (Flow)",            0x04},
    {"5 (Waltz)",           0x05},
    {"6 (Four Seasons)",    0x06},
    {"7 (Off)",             0x07},
    {0}
};

static mode _led_speeds[] = {
    {"1",               0xE0},
    {"2",               0xC0},
    {"3",               0xA0},
    {"4",               0x80},
    {"5",               0x60},
    {"6",               0x40},
    {"7",               0x20},
    {"8",               0x00},
    {0}
};


static struct devicedata m8device = {
    .active_dpi = -1,
    .active_led = -1,
    .active_speed = -1,
    .modes_dpi = _dpi_modes,
    .modes_led = _led_modes,
    .modes_speed = _led_speeds,
    .modes_dpires = _dpires_modes,
};



packet_buffer pkt_handshake_out[] = {
    {OUT_RID, 0x01,    0,    0,    0, 0, 0, 0},
    {OUT_RID, 0x03,    0,    0,    0, 0, 0, 0}};
packet_buffer pkt_handshake_inn[] = {
    {INN_RID, 0xa6,    0,    0,    0, 0, 0, INN_B7C},
    {INN_RID, 0xaa, 0x0e, 0xfd,    0, 0, 0, INN_B7C}};

packet_buffer pkt_downloadstart_out[] = { 
    {OUT_RID, 0x04, 0x36,    0, 0x02, 0, 0, 0},
    {OUT_RID, 0x05,    0,    0,    0, 0, 0, 0},
    {OUT_RID, 0x04,    0,    0, 0xff, 0, 0, 0}};

packet_buffer pkt_downloadstart_inn[] = {
    {INN_RID, 0xaa, 0x0e, 0xfd, 0, 0, 0, INN_B7C},
    {INN_RID, 0xff, 0xff,    0, 0, 0, 0, INN_B7C},
    {INN_RID, 0xff, 0xff,    0, 0, 0, 0, INN_B7C}};

packet_buffer pkt_download_out =
    {OUT_RID, 0x05, 0, 0, 0, 0, 0, 0};

packet_buffer pkt_download_inn_last =
    {INN_RID, 0xfd, 0xff, 0xff, 0, 0, 0, INN_B7C};


packet_buffer pkt_uploadstart_out[] = { 
    {OUT_RID, 0x06,    0,    0, 0xff, 0, 0, 0},
};

packet_buffer pkt_uploadstart_inn[] = {
    {INN_RID, 0xaa, 0x0e, 0xfd, 0, 0, 0, INN_B7C}
};

packet_buffer pkt_upload_out =
    {OUT_RID, 0x07, 0, 0, 0, 0, 0, 0};

packet_buffer pkt_upload_inn =
    {INN_RID,    0, 0, 0, 0, 0, 0, INN_B7C};

packet_buffer pkt_hangup_out[] = {
    {OUT_RID, 0x08, 0, 0, 0, 0, 0, 0},
    {OUT_RID, 0x02, 0, 0, 0, 0, 0, 0}};

packet_buffer pkt_hangup_inn[] = {
    {INN_RID, 0xfd, 0xff, 0xff,    0, 0, 0, INN_B7C}, // first unterminated
    {INN_RID, 0xfc,    0,    0, 0x01, 0, 0, INN_B7C}
};





static int format_buffer(char *str, packet_buffer packet){
    int index = 0;
    for(int i=0; i<M8_PKT_SIZE; i++){
        index += sprintf(&str[index], "%02X", packet[i]);
    }
    return index;
}

static void log_packet(char *label, packet_buffer packet){
    char strbuff[64];
    strbuff[0] = 0;
    format_buffer(strbuff, packet);    
    log_debug("%s: %s", label, strbuff);
}

static void log_packet2(char *label, packet_buffer packet, char *label2, packet_buffer packet2){
    char strbuff[64], strbuff2[64];
    strbuff[0] = 0;
    strbuff2[0] = 0;
    format_buffer(strbuff, packet);
    format_buffer(strbuff2, packet2);
    log_debug("%s: %s  -  %s: %s", label, strbuff, label2, strbuff2);
}


static int send_packet(packet_buffer packet){
    
    log_packet("send_packet: SEND", packet);
#ifndef DEBUG_ONLY
    int bytes_sent = hid_send_feature_report(m8device.devhandle, packet, M8_PKT_SIZE);
#else
    int bytes_sent = M8_PKT_SIZE;
#endif
    
    if(bytes_sent < -1 || bytes_sent != M8_PKT_SIZE){
        log_error("send_packet: Error with written data, sent %i, actual: %i", M8_PKT_SIZE, bytes_sent);
        return -1;
    }
    return ENO_SUCCESS;
}

static int recv_packet(packet_buffer recv_buffer, uint8_t rid, packet_buffer expected_buffer){
    
    memset(recv_buffer, 0, M8_PKT_SIZE);
    recv_buffer[0] = rid;
    
#ifndef DEBUG_ONLY
    int bytes_recv = hid_get_feature_report(m8device.devhandle, recv_buffer, M8_PKT_SIZE);
#else
    int bytes_recv = M8_PKT_SIZE;
#endif

    if(bytes_recv < -1 || bytes_recv != M8_PKT_SIZE){
        log_error("recv_packet: Error receiving data, return: %i", bytes_recv);
        return -1;
    }

    //verify buffer received matches what we expect
    if(expected_buffer && memcmp(recv_buffer, expected_buffer, M8_PKT_SIZE)){
        log_packet2("recv_packet: RECV", recv_buffer, "MISMATCH", expected_buffer);
    }else{
        log_packet("recv_packet: RECV", recv_buffer);
    }

    return ENO_SUCCESS;
}



void log_device_mem(){
    
    int memsize = device_mem_size();
    unsigned char *rawmem = device_mem_raw();
    log_trace("log_device_mem: Printing device memory, size: %i", memsize);
    
    char *buffer = malloc(memsize * 8);
    buffer[0] = 0;
    
    char *pointer = buffer;
    int width = LOG_PRINT_MEM_WIDTH;
    for(int i=0; i<memsize; i++){
        if(i%width == 0)
            pointer += sprintf(pointer, "#%02X: ", i/*/width*/);
        pointer += sprintf(pointer, "%02X ", rawmem[i]);
        if(i%width == (width/2)-1)
            pointer += sprintf(pointer, "- ");
        if(i%width == width-1)
            pointer += sprintf(pointer, "\n");
    }
    log_trace("log_device_mem: memory buffer\n%s", buffer);
    log_trace("log_device_mem: done");
    
    free(buffer);
}




int device_mem_size(){
    return m8device.memsize;
}
unsigned char *device_mem_raw(){
    return m8device.memdata;
}


static int devmem_get_mode_index(int address, uint8_t mask, mode *modes){
    
    if(m8device.memsize <= address)
        return -ENO_GENERAL;
    
    uint8_t ledvalue = m8device.memdata[address] & mask;
    uint8_t ledcheck = m8device.memdata[address + 1] & mask;
    if((ledvalue | ledcheck) != mask ){
        log_debug("LED value and checksum do no match, value: %02x, check: %02x, mask: %02x",
                  ledvalue, ledcheck, mask);
        return -ENO_GENERAL;
    }
    
    //search for the label
    mode *curr = modes;
    for(int i = 0; curr->label; i++, curr++){
        if((curr->value) == ledvalue){
            return i;
        }
    }
    
    return -ENO_GENERAL;
}

static int devmem_get_dpires_index(int address, mode *modes){
    
    if(m8device.memsize <= address)
        return -ENO_GENERAL;
    
    uint8_t dpires = m8device.memdata[address];    
    //search for the label
    mode *curr = modes;
    for(int i = 0; curr->label; i++, curr++){
        if((curr->value) == dpires){
            return i;
        }
    }
    
    return -ENO_GENERAL;
}

static int devmem_set_mode(int address, uint8_t mask, mode *modes, int index){
    
    if(m8device.memsize <= address)
        return -ENO_GENERAL;
    
    //search for mode value
    mode *curr = modes;
    int found = 0;
    for(int i = 0; curr->label; i++, curr++){
        if(i == index){
            found = 1;
            break;
        }
    }
    if(!found){
        return -ENO_GENERAL;
    }
    
    uint8_t value = modes[index].value;
    
    uint8_t currvalue = m8device.memdata[address];
    uint8_t currcheck = m8device.memdata[address+1];
    
    uint8_t newvalue = (currvalue & ~mask)  | (value & mask);
    uint8_t newcheck = (currcheck & ~mask)  | ((~value) & mask);
    
    log_debug("devmem_set_mode_value: setting current %02x to %02x  -  check %02x to %02x", 
              currvalue, newvalue, currcheck, newcheck);
    
    m8device.memdata[address] = newvalue;
    m8device.memdata[address+1] = newcheck;
    
    return ENO_SUCCESS;
}


static int devmem_clear(){
    log_trace("devmem_clear: clearing memory buffer");
    memset(m8device.memdata, 0, M8_DEV_MEMBUFF_SIZE);
    m8device.memsize = 0;
    m8device.memindex = 0;
    return ENO_SUCCESS;
}


static int devmem_store(packet_buffer packet){
    
    int exp_size = M8_PKT_SIZE - 2;
    
    if(m8device.memsize + exp_size > (M8_DEV_MEMBUFF_SIZE)){
        log_warn("store_devmem: skipping due to overflow, memsize: %i, adding %i, max is %i", 
                 m8device.memsize, exp_size, M8_DEV_MEMBUFF_SIZE);
        return -ENO_GENERAL;
    }
    
    memcpy(&m8device.memdata[m8device.memindex], &packet[1], exp_size);
    m8device.memindex += exp_size;
    m8device.memsize = m8device.memindex;
    return ENO_SUCCESS;
}

static int devmem_retrieve(packet_buffer packet, int index){
    
    int exp_size = M8_PKT_SIZE - 2;
    
    if(index > m8device.memsize){
        log_warn("devmem_retrieve: skipping due to overflow, memsize: %i, getting %i from index %i",
                 m8device.memsize, exp_size, index);
        return -ENO_GENERAL;
    }
    
    memcpy(&packet[2], &m8device.memdata[index], exp_size);

    return (index + exp_size);
}



static int command_handshake(){
    unsigned char recv_buffer[M8_PKT_SIZE];

    //send handshake - 2 packets out/2 packets in
    log_info("command_handshake: Sending HANDSHAKE");    
    int count_hands = sizeof(pkt_handshake_out)/sizeof(pkt_handshake_out[0]);
    for(int i = 0; i < count_hands; i++){
        if(send_packet(pkt_handshake_out[i])){
            log_error("command_handshake: Error sending hanshake buffer %i", i);
            return -ENO_SEND;
        }
        //wait time
        usleep(DELAY_AFTER_SET_MS * 1000);
    
        if(recv_packet(recv_buffer, INN_RID, pkt_handshake_inn[i])){
            log_error("command_handshake: Error receiving hanshake buffer %i", i);
            return -ENO_RECV;
        }
    }
    return ENO_SUCCESS;
}

static int command_hangup(){
    unsigned char recv_buffer[M8_PKT_SIZE];
    
    //hang up - 2 packets out/2 packets in, but sometimes 3 in
    log_info("command_hangup: Sending HANGUP");    
    int count_hup = sizeof(pkt_hangup_out)/sizeof(pkt_hangup_out[0]);
    for(int i = 0; i < count_hup; i++){
        if(send_packet(pkt_hangup_out[i])){
            log_error("command_hangup: Error sending hangup buffer %i", i);
            return -ENO_SEND;
        }
        //wait time
        usleep(DELAY_AFTER_SET_MS * 1000);
    
        if(recv_packet(recv_buffer, INN_RID, pkt_hangup_inn[i])){
            log_error("command_hangup: Error receiving hangup buffer %i", i);
            return -ENO_SEND;
        }
        
        //check if response is terminated
        if(recv_buffer[M8_PKT_SIZE-1] != INN_B7C){
            log_trace("command_hangup: unterminated response during hangup %i, requesting more", i);
            recv_packet(recv_buffer, INN_RID, pkt_hangup_inn[i]);
            if(recv_buffer[M8_PKT_SIZE-1] != INN_B7C){
                log_debug("command_hangup: still unterminated response during hangup %i, moving on", i);
            }
        }
    }
    return ENO_SUCCESS;
}


static int command_download(){
    unsigned char recv_buffer[M8_PKT_SIZE];
    
    //send initiate query - 3 packets out/3 packets in
    log_info("command_download: Sending INIT_DOWNLOAD");
    int count_start = sizeof(pkt_downloadstart_out)/sizeof(pkt_downloadstart_out[0]);
    for(int i = 0; i < count_start; i++){
        if(send_packet(pkt_downloadstart_out[i])){
            log_error("command_download: Error sending init download buffer %i", i);
            return -ENO_SEND;
        }
        //wait time
        usleep(DELAY_AFTER_SET_MS * 1000);
    
        if(recv_packet(recv_buffer, INN_RID, pkt_downloadstart_inn[i])){
            log_error("command_download: Error receiving init download buffer %i", i);
            return -ENO_SEND;
        }
    }
    
    //run query - 43 packets
    log_info("command_download: Sending DOWNLOAD");
    int txcount = M8_PKT_TRANSFER_COUNT;
    for(int i = 0; i < txcount; i++){
        if(send_packet(pkt_download_out)){
            log_error("command_download: Error sending query_get buffer %i", i);
        }
        //wait time
        usleep(DELAY_AFTER_SET_MS * 1000);
    
        if(recv_packet(recv_buffer, INN_RID, NULL)){
            log_error("command_download: Error running queries, stopping at %i", i);
            return -ENO_GENERAL;
        }
        if(recv_buffer[M8_PKT_SIZE-1] == INN_B7E){
            log_info("command_download: unexpected termination, probably smaller mem, stopping at %i", i);
            break;
        }
        //store device memory
        if(devmem_store(recv_buffer)){
            log_error("command_download: Error storing device memory during round %i", i);
            return -ENO_GENERAL;
        }
    }
    
    return ENO_SUCCESS;
}


static int command_upload(){

    unsigned char send_buffer[M8_PKT_SIZE];
    unsigned char recv_buffer[M8_PKT_SIZE];
    
    //send initiate upload - 1 out/1 in
    log_info("command_upload: Sending INIT_UPLOAD");
    int count_start = sizeof(pkt_uploadstart_out)/sizeof(pkt_uploadstart_out[0]);
    for(int i = 0; i < count_start; i++){
        if(send_packet(pkt_uploadstart_out[i])){
            log_error("command_upload: Error sending init upload buffer %i", i);
            return -ENO_SEND;
        }
        //wait time
        usleep(DELAY_AFTER_SET_MS * 1000);
    
        if(recv_packet(recv_buffer, INN_RID, pkt_uploadstart_inn[i])){
            log_error("command_upload: Error receiving init upload buffer %i", i);
            return -ENO_SEND;
        }
    }
    
    //run upload - 43 packets
    log_info("command_upload: Sending UPLOAD");
    int txcount = M8_PKT_TRANSFER_COUNT;
    int txindex = 0;
    for(int i = 0; i < txcount; i++){
        memcpy(send_buffer, pkt_upload_out, sizeof(pkt_upload_out));
        txindex = devmem_retrieve(send_buffer, txindex);
        
        if(send_packet(send_buffer)){
            log_error("command_upload: Error sending upload buffer %i", i);
            return -ENO_SEND;
        }
        //wait time
        usleep(DELAY_AFTER_SET_MS * 1000);
    
        if(recv_packet(recv_buffer, INN_RID, NULL)){
            log_error("command_upload: Error receiving upload response, stopping at %i", i);
            return -ENO_GENERAL;
        }
        if(recv_buffer[M8_PKT_SIZE-1] != INN_B7C){
            log_debug("command_upload: unexpected received termination, looking for %02x, but got %02x", INN_B7C,
                      recv_buffer[M8_PKT_SIZE-1]);
        }
        if(memcmp(&send_buffer[2], &recv_buffer[1], M8_PKT_SIZE - 2)){
            log_debug("command_upload: sent/recv buffers don't match");
            log_packet2("SENT", send_buffer, "RECV", recv_buffer);
            log_debug("command_upload: stopping at %i", i);
            return -ENO_GENERAL;
        }
    }

    return ENO_SUCCESS;
}





int device_init(){
    log_trace("init_device: Begin");
    struct hid_device_info *first_device;
    struct hid_device_info *hid_device;
    struct hid_device_info *target_dev = NULL;
    unsigned int hid_device_count = 0;

    hid_init();

    first_device = hid_device = hid_enumerate(USB_M8_VID, USB_M8_PID);
    while(hid_device){
        hid_device_count++;        
        if(hid_device->vendor_id == USB_M8_VID && hid_device->product_id == USB_M8_PID){
            target_dev = hid_device;
            break;
        }
        hid_device = hid_device->next;
    }
    
    if(!target_dev){
        log_error("init_device: Error, target device not found");
        return -ENO_DEVICE;
    }
    log_info("init_device: Target Device Found at %s!!", target_dev->path);
    log_debug("init_device: -- HID Devices   path: %s, vendor: %04hX, product: %04hX",
            hid_device->path, hid_device->vendor_id, hid_device->product_id);
    log_debug("init_device:    Manufacturer: %ls, Product: %ls",
            hid_device->manufacturer_string, hid_device->product_string);
    log_debug("init_device:    Interface: %i, Serial: %ls",
            hid_device->interface_number, hid_device->serial_number);
    log_debug("init_device:    Release: %i, Usage: %hi, Usage Page: %hi",
            hid_device->release_number, hid_device->usage, hid_device->usage_page);

    m8device.devhandle = hid_open_path(target_dev->path);
    hid_free_enumeration(first_device);
    
    if(!m8device.devhandle){
        log_error("init_device: Error opening device");
        return -ENO_GENERAL;
    }

    return ENO_SUCCESS;
}


void device_shutdown(){
    if(m8device.devhandle){
        log_info("device_shutdown: Closing device");
        hid_close(m8device.devhandle);
    }
    
    hid_exit();    
}

int device_query(){

    log_info("device_query: downloading program data");
    if(m8device.memindex)
        devmem_clear();

    //send handshake
    if(command_handshake()){
        log_warn("device_query: bad handshake, stopping");
        return -ENO_GENERAL;
    }

    //send download
    if(command_download()){
        log_warn("device_query: bad download, stopping");
        return -ENO_GENERAL;
    }
    
    //send handshake
    if(command_hangup()){
        log_warn("device_query: bad hangup while stopping");
        return -ENO_GENERAL;
    }
    
    
    
    log_info("device_query: download done!");
    //log memory if we are in TRACE level
    if(log_get_level() == LOG_TRACE)
        log_device_mem();
    
    if(device_update_state()){
        log_error("device_query: device doesn't show supported states, not confirmed.");
        m8device.devconfirmed = 0;
        return -ENO_GENERAL;
    }
    
    m8device.devconfirmed = 1;
    return ENO_SUCCESS;
}

int device_update(){

    log_info("device_update: uploading program data");
    
    //did we confirm this is the right device
    if(!m8device.devconfirmed)
        return -ENO_DEVICE;
    
    //send handshake
    if(command_handshake()){
        log_warn("device_update: bad handshake, stopping");
        return -ENO_GENERAL;
    }
    

    //send download
    if(command_upload()){
        log_warn("device_update: bad upload, stopping");
        return -ENO_GENERAL;
    }
    
    //send handshake
    if(command_hangup()){
        log_warn("device_update: bad hangup while stopping");
        return -ENO_GENERAL;
    }
    
    log_info("device_update: upload done!");
    return ENO_SUCCESS;
}

int device_set_modes(int dpi, int led, int speed){
    if(dpi > -1){
        if(devmem_set_mode(M8_DPI_ADDR, M8_DPI_MODE_MASK, _dpi_modes, dpi)){
            log_error("Invalid DPI mode requested");
            return ENO_GENERAL;
        }
    }
    if(led > -1){
        if(devmem_set_mode(M8_LED_ADDR, M8_LED_MODE_MASK, _led_modes, led)){
            log_error("Invalid LED mode requested");
            return 1;
        }
    }
    if(speed > -1){
        if(devmem_set_mode(M8_LED_ADDR, M8_LED_SPEED_MASK, _led_speeds, speed)){
            log_error("Invalid DPI mode requested");
            return 1;
        }
    }
    return ENO_SUCCESS;
   
}


mode *device_get_active_mode(M8_DEVICE_MODES which_mode){
    
    if(which_mode == M8_DEVICE_MODE_DPI && (m8device.active_dpi > -1)){
        return &m8device.modes_dpi[m8device.active_dpi];
    }else if(which_mode == M8_DEVICE_MODE_LED && (m8device.active_led > -1)){
        return &m8device.modes_led[m8device.active_led];
    }else if(which_mode == M8_DEVICE_MODE_SPEED && (m8device.active_speed > -1)){
        return &m8device.modes_speed[m8device.active_speed];
    }
    return NULL;
}

mode *device_get_mode_value(M8_DEVICE_MODES which_mode, int index){
    
    if(which_mode == M8_DEVICE_MODE_DPI_RES && index > -1 && index < M8_DPI_RES_COUNT){
        return &m8device.modes_dpires[m8device.dpires_values[index]];
    }
    return NULL;
}

mode *device_get_all_modes(M8_DEVICE_MODES which_mode){
    if(which_mode == M8_DEVICE_MODE_DPI){
        return m8device.modes_dpi;
    }else if(which_mode == M8_DEVICE_MODE_DPI_RES){
        return m8device.modes_dpires;
    }else if(which_mode == M8_DEVICE_MODE_LED){
        return m8device.modes_led;
    }else if(which_mode == M8_DEVICE_MODE_SPEED){
        return m8device.modes_speed;
    }
    return NULL;
}

int device_update_state(){
    log_trace("device_update_state: refreshing device state");

    m8device.active_dpi = devmem_get_mode_index(M8_DPI_ADDR, M8_DPI_MODE_MASK, _dpi_modes);
    m8device.active_led = devmem_get_mode_index(M8_LED_ADDR, M8_LED_MODE_MASK, _led_modes);
    m8device.active_speed = devmem_get_mode_index(M8_LED_ADDR, M8_LED_SPEED_MASK, _led_speeds);
    
    for(int i=0; i<M8_DPI_RES_COUNT; i++){
        m8device.dpires_values[i] = devmem_get_dpires_index(M8_DPI_RES_ADDR + i, _dpires_modes);
    }
    
    log_trace("device_update_state: active modes are dpi %i, led %i, speed %i",
              m8device.active_dpi, m8device.active_led, m8device.active_speed);

    //check that we understood those states, otherwise it's not a supported device
    if(m8device.active_dpi < 0 || m8device.active_led < 0 || m8device.active_speed < 0)
        return -ENO_GENERAL;

    return ENO_SUCCESS;
}
