/*
 * Copyright (C) 2016 Tolga Ceylan
 *
 * CopyPolicy: Released under the terms of the GNU GPL v3.0.
 */
#include "client.h"
#include "common.h"

#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

namespace robo {

Client::Client()
    :
    m_server_fd(-1)
{
}

Client::~Client()
{
    shutdown();
}

int Client::initialize(const char *uds_path)
{
    assert(uds_path);

    if (m_server_fd != -1)
        return EINVAL;

    int rc = 0;
    struct sockaddr_un address;
    socklen_t address_length = sizeof(address);

    memset(&address, 0, sizeof(address));
    address.sun_family = AF_UNIX;

    rc = snprintf(address.sun_path, sizeof(address.sun_path), uds_path);
    if (rc <= 0 || rc >= sizeof(address.sun_path))
        return EINVAL;

    m_server_fd = ::socket(PF_UNIX, SOCK_STREAM, 0);
    if (m_server_fd < 0)
        goto fail;

    rc = ::connect(m_server_fd, (struct sockaddr *)&address, address_length);
    if (rc)
        goto fail;

    return 0;

fail:
    rc = errno;
    shutdown();
    return rc ? rc : EFAULT;
}

void Client::shutdown()
{
    if (m_server_fd != -1)
        HANDLE_EINTR(::close(m_server_fd));
    m_server_fd = -1;
}

int Client::get_response(proto::Response &response)
{
    if (m_server_fd == -1)
        return ENOTCONN;

    // WARNING: not even memcpy here, we direcly pass response addr to kernel
    char *buf = (char *) &response;
    size_t idx = 0;

    while (idx < sizeof(buf)) {

        ssize_t rc = HANDLE_EINTR(::recv(m_server_fd, buf + idx, sizeof(response) - idx, 0));
        if (rc <= 0) {
            if (rc < 0) {
                rc = errno;
                return rc;
            }
            return ENOTCONN;
        }

        idx += (size_t) rc;
    }

    return 0;
}

int Client::send_request(const proto::Request &request)
{
    if (m_server_fd == -1)
        return ENOTCONN;

    // WARNING: not even memcpy here, no copy
    const char *buf = (const char *) &request;
    size_t idx = 0;

    while (idx < sizeof(buf)) {
        ssize_t rc = HANDLE_EINTR(::send(m_server_fd, buf + idx, sizeof(request) - idx, 0));
        if (rc < 0) {
            rc = errno;
            return rc;
        }
        idx += (size_t) rc;
    }

    return 0;
}

} // namespace robo