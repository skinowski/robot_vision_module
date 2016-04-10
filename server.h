/*
 * Copyright (C) 2016 Tolga Ceylan
 *
 * CopyPolicy: Released under the terms of the GNU GPL v3.0.
 */
#ifndef __SERVER__H__
#define __SERVER__H__

#include <proto.h>

namespace robo {

// We currently only support/use unix domain socket (SOCK_STREAM)
// due to performance/isolation. This is a fully blocking server
// and there are no timeouts. Concurrency is limited to one connected
// client only.
//
// TODO: Consider non-UDS approach for easier testing (across
// network, or distributing processing across more raspberries.)
//
class Server
{
    public:
        Server();
        ~Server();

        int initialize(const char *uds_path);
        void shutdown();

        int get_request(proto::Request &request);
        int send_response(const proto::Response &response);

    private:

        int accept_client();
        void close_client();

    private:

        const char      *m_uds_path;
        int             m_server_fd;
        int             m_client_fd;
};

} // namespace robo

#endif // __SERVER__H__
