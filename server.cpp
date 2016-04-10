/*
 * Copyright (C) 2016 Tolga Ceylan
 *
 * CopyPolicy: Released under the terms of the GNU GPL v3.0.
 */
#include "server.h"
#include "common.h"

#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

namespace robo {

Server::Server()
    :
    m_uds_path(NULL),
    m_server_fd(-1),
    m_client_fd(-1)
{
}

Server::~Server()
{
    shutdown();
}

int Server::initialize(const char *uds_path)
{
    assert(uds_path);

    if (m_server_fd != -1)
        return EINVAL;

    int rc = 0;
    struct sockaddr_un address;
    socklen_t address_length = sizeof(address);

    m_uds_path = uds_path;

    memset(&address, 0, sizeof(address));
    address.sun_family = AF_UNIX;

    rc = snprintf(address.sun_path, sizeof(address.sun_path), uds_path);
    if (rc <= 0 || rc >= sizeof(address.sun_path))
        return EINVAL;

    ::unlink(m_uds_path);

    m_server_fd = ::socket(PF_UNIX, SOCK_STREAM, 0);
    if (m_server_fd < 0)
        goto fail;

    rc = ::bind(m_server_fd, (struct sockaddr *)&address, address_length);
    if (rc)
        goto fail;

    rc = ::listen(m_server_fd, 1);
    if (rc)
        goto fail;

    logger(LOG_INFO, "Server::initialize fd=%d on %s", m_server_fd, m_uds_path);
    return 0;

fail:
    rc = errno;
    logger(LOG_ERROR, "Server::initialize failed %d %s", rc, strerror(rc));
    shutdown();
    return rc ? rc : EFAULT;
}

void Server::close_client()
{
    if (m_client_fd != -1) {
        HANDLE_EINTR(::close(m_client_fd));
        m_client_fd = -1;
    }
}

int Server::accept_client()
{
    if (m_server_fd == -1)
        return EINVAL;

    int rc = 0;
    struct sockaddr_un address;
    socklen_t address_length = sizeof(address);

    memset(&address, 0, sizeof(address));
    close_client();

    rc = HANDLE_EINTR(::accept(m_server_fd, (struct sockaddr *)&address, &address_length));
    if (rc < 0) {
        rc = errno;
        goto fail;
    }

    m_client_fd = rc;
    return 0;

fail:
    logger(LOG_ERROR, "Server::accept_client failed %d %s", rc, strerror(rc));
    return rc;
}

void Server::shutdown()
{
    logger(LOG_INFO, "Server::shutdown fd=%d", m_server_fd);

    close_client();

    if (m_server_fd != -1)
        HANDLE_EINTR(::close(m_server_fd));
    if (m_uds_path)
        ::unlink(m_uds_path);

    m_server_fd = -1;
    m_uds_path = NULL;
}

int Server::get_request(proto::Request &request)
{
    if (m_client_fd == -1) {
        int ret = accept_client();
        if (ret < 0)
            return ret;
    }

    // WARNING: not even memcpy here, we direcly pass request addr to kernel
    char *buf = (char *) &request;
    size_t idx = 0;

    while (idx < sizeof(buf)) {

        ssize_t rc = HANDLE_EINTR(::recv(m_client_fd, buf + idx, sizeof(request) - idx, 0));
        if (rc <= 0) {

            if (rc < 0) {
                rc = errno;
                logger(LOG_ERROR, "Server::get_request recv failed %d %s", rc, strerror(rc));
            }
            else {
                logger(LOG_ERROR, "Server::get_request connection closed");
            }

            rc = accept_client();
            if (rc < 0)
                return rc;

            // reset our state and go back to listening.
            idx = 0;
            continue;
        }

        idx += (size_t) rc;
    }

    return 0;
}

int Server::send_response(const proto::Response &response)
{
    if (m_client_fd == -1)
        return ENOTCONN;

    // WARNING: not even memcpy here, no copy
    const char *buf = (const char *) &response;
    size_t idx = 0;

    while (idx < sizeof(buf)) {
        ssize_t rc = HANDLE_EINTR(::send(m_client_fd, buf + idx, sizeof(response) - idx, 0));
        if (rc < 0) {
            rc = errno;
            logger(LOG_ERROR, "Server::send_response send failed %d %s", rc, strerror(rc));
            close_client();
            return rc;
        }
        idx += (size_t) rc;
    }

    return 0;
}

} // namespace robo