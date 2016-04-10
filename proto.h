#ifndef __PROTO__H__
#define __PROTO__H__

#include <stdint.h>

namespace robo {

//
// We keep it simple here. No endian translation, no bitpacking,
// no serialization, no versioning. Our server/client are on
// the same machine. We simply disable padding and send/recv
// specific size structs.
//
namespace proto {

enum {
    CMD_GET_MAP = 0x01,
    CMD_PING    = 0x02,
    CMD_EXIT    = 0x03,
} COMMANDS;

struct Request
{
    uint32_t trx_id;
    uint32_t cmd;
} __attribute__((packed));;

struct Response
{
    uint32_t trx_id;
    uint32_t cmd;

    // TODO: we do not yet know our response data type/content
    uint64_t data;
} __attribute__((packed));;

} // namespace proto

} // namespace robo

#endif // __PROTO__H__