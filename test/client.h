/*
 * Copyright (C) 2016 Tolga Ceylan
 *
 * CopyPolicy: Released under the terms of the GNU GPL v3.0.
 */
#ifndef __CLIENT__H__
#define __CLIENT__H__

#include <proto.h>

namespace robo {

// Simple test client
class Client
{
	public:
		Client();
		~Client();

		int initialize(const char *uds_path);
		void shutdown();

		int send_request(const proto::Request &request);
		int get_response(proto::Response &response);

	private:
		int 			m_server_fd;
};

} // namespace robo

#endif // __CLIENT__H__
