/*
 * Copyright (C) 2009 Giacomo Spigler
 * CopyPolicy: Released under the terms of the GNU GPL v3.0.
 */
#include "client.h"

#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

using namespace robo;

// these should goto config.json/yaml
const char *UDS_PATH = "/tmp/robo.vision.s";

int main() {

    int res = 0;

    Client client;

    res = client.initialize(UDS_PATH);
    if (res)
        goto fail;

    proto::Request  request;
    proto::Response response;

    request.trx_id = 1;
    request.cmd = proto::CMD_PING;

    res = client.send_request(request);
    if (res)
        goto fail;
    
    printf("Sent request\n");
    res = client.get_response(response);
    if (res)
        goto fail;

    printf("Got response\n");

    assert(response.trx_id == 1);
    assert(response.cmd == proto::CMD_PING);
    assert(response.data == 0);


    request.trx_id = 2;
    request.cmd = proto::CMD_PING;

    res = client.send_request(request);
    if (res)
        goto fail;
    
    printf("Sent request\n");
    res = client.get_response(response);
    if (res)
        goto fail;

    printf("Got response\n");

    assert(response.trx_id == 2);
    assert(response.cmd == proto::CMD_PING);
    assert(response.data == 0);


    request.trx_id = 3;
    request.cmd = proto::CMD_EXIT;

    printf("Sent exit\n");
    res = client.send_request(request);
    if (res)
        goto fail;

    res = client.get_response(response);
    assert(res == 107);
    if (res && res != 107)
        goto fail;
    printf("Got exit\n");
    res = 0;

fail: 
    printf("Exit code=%d %s", res, strerror(res));
    client.shutdown();
    return res;
}



