/* Generated by the protocol buffer compiler.  DO NOT EDIT! */
/* Generated from: protobuf/chord.proto */

#ifndef PROTOBUF_C_protobuf_2fchord_2eproto__INCLUDED
#define PROTOBUF_C_protobuf_2fchord_2eproto__INCLUDED

#include <protobuf-c/protobuf-c.h>

PROTOBUF_C__BEGIN_DECLS

#if PROTOBUF_C_VERSION_NUMBER < 1000000
# error This file was generated by a newer version of protoc-c which is incompatible with your libprotobuf-c headers. Please update your headers.
#elif 1003001 < PROTOBUF_C_MIN_COMPILER_VERSION
# error This file was generated by an older version of protoc-c which is incompatible with your libprotobuf-c headers. Please regenerate this file with a newer version of protoc-c.
#endif


typedef struct _Node Node;
typedef struct _NotifyRequest NotifyRequest;
typedef struct _NotifyResponse NotifyResponse;
typedef struct _FindSuccessorRequest FindSuccessorRequest;
typedef struct _FindSuccessorResponse FindSuccessorResponse;
typedef struct _GetPredecessorRequest GetPredecessorRequest;
typedef struct _GetPredecessorResponse GetPredecessorResponse;
typedef struct _CheckPredecessorRequest CheckPredecessorRequest;
typedef struct _CheckPredecessorResponse CheckPredecessorResponse;
typedef struct _GetSuccessorListRequest GetSuccessorListRequest;
typedef struct _GetSuccessorListResponse GetSuccessorListResponse;
typedef struct _ChordMessage ChordMessage;


/* --- enums --- */


/* --- messages --- */

struct  _Node
{
  ProtobufCMessage base;
  uint32_t key;
  uint32_t address;
  uint32_t port;
};
#define NODE__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&node__descriptor) \
    , 0, 0, 0 }


/*
 * Notify
 */
struct  _NotifyRequest
{
  ProtobufCMessage base;
  /*
   * Not necessary, but doesn't hurt
   */
  Node *node;
};
#define NOTIFY_REQUEST__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&notify_request__descriptor) \
    , NULL }


struct  _NotifyResponse
{
  ProtobufCMessage base;
};
#define NOTIFY_RESPONSE__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&notify_response__descriptor) \
     }


/*
 * Find Successor
 */
struct  _FindSuccessorRequest
{
  ProtobufCMessage base;
  uint64_t key;
};
#define FIND_SUCCESSOR_REQUEST__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&find_successor_request__descriptor) \
    , 0 }


struct  _FindSuccessorResponse
{
  ProtobufCMessage base;
  Node *node;
};
#define FIND_SUCCESSOR_RESPONSE__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&find_successor_response__descriptor) \
    , NULL }


/*
 * Get Predecessor
 */
struct  _GetPredecessorRequest
{
  ProtobufCMessage base;
};
#define GET_PREDECESSOR_REQUEST__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&get_predecessor_request__descriptor) \
     }


struct  _GetPredecessorResponse
{
  ProtobufCMessage base;
  Node *node;
};
#define GET_PREDECESSOR_RESPONSE__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&get_predecessor_response__descriptor) \
    , NULL }


/*
 * Check Predecessor
 */
struct  _CheckPredecessorRequest
{
  ProtobufCMessage base;
};
#define CHECK_PREDECESSOR_REQUEST__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&check_predecessor_request__descriptor) \
     }


struct  _CheckPredecessorResponse
{
  ProtobufCMessage base;
};
#define CHECK_PREDECESSOR_RESPONSE__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&check_predecessor_response__descriptor) \
     }


/*
 * Get Successor
 */
struct  _GetSuccessorListRequest
{
  ProtobufCMessage base;
};
#define GET_SUCCESSOR_LIST_REQUEST__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&get_successor_list_request__descriptor) \
     }


struct  _GetSuccessorListResponse
{
  ProtobufCMessage base;
  size_t n_successors;
  Node **successors;
};
#define GET_SUCCESSOR_LIST_RESPONSE__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&get_successor_list_response__descriptor) \
    , 0,NULL }


typedef enum {
  CHORD_MESSAGE__MSG__NOT_SET = 0,
  CHORD_MESSAGE__MSG_NOTIFY_REQUEST = 2,
  CHORD_MESSAGE__MSG_NOTIFY_RESPONSE = 3,
  CHORD_MESSAGE__MSG_FIND_SUCCESSOR_REQUEST = 4,
  CHORD_MESSAGE__MSG_FIND_SUCCESSOR_RESPONSE = 5,
  CHORD_MESSAGE__MSG_GET_PREDECESSOR_REQUEST = 6,
  CHORD_MESSAGE__MSG_GET_PREDECESSOR_RESPONSE = 7,
  CHORD_MESSAGE__MSG_CHECK_PREDECESSOR_REQUEST = 8,
  CHORD_MESSAGE__MSG_CHECK_PREDECESSOR_RESPONSE = 9,
  CHORD_MESSAGE__MSG_GET_SUCCESSOR_LIST_REQUEST = 10,
  CHORD_MESSAGE__MSG_GET_SUCCESSOR_LIST_RESPONSE = 11
    PROTOBUF_C__FORCE_ENUM_TO_BE_INT_SIZE(CHORD_MESSAGE__MSG)
} ChordMessage__MsgCase;

struct  _ChordMessage
{
  ProtobufCMessage base;
  uint32_t version;
  ChordMessage__MsgCase msg_case;
  union {
    NotifyRequest *notify_request;
    NotifyResponse *notify_response;
    FindSuccessorRequest *find_successor_request;
    FindSuccessorResponse *find_successor_response;
    GetPredecessorRequest *get_predecessor_request;
    GetPredecessorResponse *get_predecessor_response;
    CheckPredecessorRequest *check_predecessor_request;
    CheckPredecessorResponse *check_predecessor_response;
    GetSuccessorListRequest *get_successor_list_request;
    GetSuccessorListResponse *get_successor_list_response;
  };
};
#define CHORD_MESSAGE__INIT \
 { PROTOBUF_C_MESSAGE_INIT (&chord_message__descriptor) \
    , 417u, CHORD_MESSAGE__MSG__NOT_SET, {0} }


/* Node methods */
void   node__init
                     (Node         *message);
size_t node__get_packed_size
                     (const Node   *message);
size_t node__pack
                     (const Node   *message,
                      uint8_t             *out);
size_t node__pack_to_buffer
                     (const Node   *message,
                      ProtobufCBuffer     *buffer);
Node *
       node__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   node__free_unpacked
                     (Node *message,
                      ProtobufCAllocator *allocator);
/* NotifyRequest methods */
void   notify_request__init
                     (NotifyRequest         *message);
size_t notify_request__get_packed_size
                     (const NotifyRequest   *message);
size_t notify_request__pack
                     (const NotifyRequest   *message,
                      uint8_t             *out);
size_t notify_request__pack_to_buffer
                     (const NotifyRequest   *message,
                      ProtobufCBuffer     *buffer);
NotifyRequest *
       notify_request__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   notify_request__free_unpacked
                     (NotifyRequest *message,
                      ProtobufCAllocator *allocator);
/* NotifyResponse methods */
void   notify_response__init
                     (NotifyResponse         *message);
size_t notify_response__get_packed_size
                     (const NotifyResponse   *message);
size_t notify_response__pack
                     (const NotifyResponse   *message,
                      uint8_t             *out);
size_t notify_response__pack_to_buffer
                     (const NotifyResponse   *message,
                      ProtobufCBuffer     *buffer);
NotifyResponse *
       notify_response__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   notify_response__free_unpacked
                     (NotifyResponse *message,
                      ProtobufCAllocator *allocator);
/* FindSuccessorRequest methods */
void   find_successor_request__init
                     (FindSuccessorRequest         *message);
size_t find_successor_request__get_packed_size
                     (const FindSuccessorRequest   *message);
size_t find_successor_request__pack
                     (const FindSuccessorRequest   *message,
                      uint8_t             *out);
size_t find_successor_request__pack_to_buffer
                     (const FindSuccessorRequest   *message,
                      ProtobufCBuffer     *buffer);
FindSuccessorRequest *
       find_successor_request__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   find_successor_request__free_unpacked
                     (FindSuccessorRequest *message,
                      ProtobufCAllocator *allocator);
/* FindSuccessorResponse methods */
void   find_successor_response__init
                     (FindSuccessorResponse         *message);
size_t find_successor_response__get_packed_size
                     (const FindSuccessorResponse   *message);
size_t find_successor_response__pack
                     (const FindSuccessorResponse   *message,
                      uint8_t             *out);
size_t find_successor_response__pack_to_buffer
                     (const FindSuccessorResponse   *message,
                      ProtobufCBuffer     *buffer);
FindSuccessorResponse *
       find_successor_response__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   find_successor_response__free_unpacked
                     (FindSuccessorResponse *message,
                      ProtobufCAllocator *allocator);
/* GetPredecessorRequest methods */
void   get_predecessor_request__init
                     (GetPredecessorRequest         *message);
size_t get_predecessor_request__get_packed_size
                     (const GetPredecessorRequest   *message);
size_t get_predecessor_request__pack
                     (const GetPredecessorRequest   *message,
                      uint8_t             *out);
size_t get_predecessor_request__pack_to_buffer
                     (const GetPredecessorRequest   *message,
                      ProtobufCBuffer     *buffer);
GetPredecessorRequest *
       get_predecessor_request__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   get_predecessor_request__free_unpacked
                     (GetPredecessorRequest *message,
                      ProtobufCAllocator *allocator);
/* GetPredecessorResponse methods */
void   get_predecessor_response__init
                     (GetPredecessorResponse         *message);
size_t get_predecessor_response__get_packed_size
                     (const GetPredecessorResponse   *message);
size_t get_predecessor_response__pack
                     (const GetPredecessorResponse   *message,
                      uint8_t             *out);
size_t get_predecessor_response__pack_to_buffer
                     (const GetPredecessorResponse   *message,
                      ProtobufCBuffer     *buffer);
GetPredecessorResponse *
       get_predecessor_response__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   get_predecessor_response__free_unpacked
                     (GetPredecessorResponse *message,
                      ProtobufCAllocator *allocator);
/* CheckPredecessorRequest methods */
void   check_predecessor_request__init
                     (CheckPredecessorRequest         *message);
size_t check_predecessor_request__get_packed_size
                     (const CheckPredecessorRequest   *message);
size_t check_predecessor_request__pack
                     (const CheckPredecessorRequest   *message,
                      uint8_t             *out);
size_t check_predecessor_request__pack_to_buffer
                     (const CheckPredecessorRequest   *message,
                      ProtobufCBuffer     *buffer);
CheckPredecessorRequest *
       check_predecessor_request__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   check_predecessor_request__free_unpacked
                     (CheckPredecessorRequest *message,
                      ProtobufCAllocator *allocator);
/* CheckPredecessorResponse methods */
void   check_predecessor_response__init
                     (CheckPredecessorResponse         *message);
size_t check_predecessor_response__get_packed_size
                     (const CheckPredecessorResponse   *message);
size_t check_predecessor_response__pack
                     (const CheckPredecessorResponse   *message,
                      uint8_t             *out);
size_t check_predecessor_response__pack_to_buffer
                     (const CheckPredecessorResponse   *message,
                      ProtobufCBuffer     *buffer);
CheckPredecessorResponse *
       check_predecessor_response__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   check_predecessor_response__free_unpacked
                     (CheckPredecessorResponse *message,
                      ProtobufCAllocator *allocator);
/* GetSuccessorListRequest methods */
void   get_successor_list_request__init
                     (GetSuccessorListRequest         *message);
size_t get_successor_list_request__get_packed_size
                     (const GetSuccessorListRequest   *message);
size_t get_successor_list_request__pack
                     (const GetSuccessorListRequest   *message,
                      uint8_t             *out);
size_t get_successor_list_request__pack_to_buffer
                     (const GetSuccessorListRequest   *message,
                      ProtobufCBuffer     *buffer);
GetSuccessorListRequest *
       get_successor_list_request__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   get_successor_list_request__free_unpacked
                     (GetSuccessorListRequest *message,
                      ProtobufCAllocator *allocator);
/* GetSuccessorListResponse methods */
void   get_successor_list_response__init
                     (GetSuccessorListResponse         *message);
size_t get_successor_list_response__get_packed_size
                     (const GetSuccessorListResponse   *message);
size_t get_successor_list_response__pack
                     (const GetSuccessorListResponse   *message,
                      uint8_t             *out);
size_t get_successor_list_response__pack_to_buffer
                     (const GetSuccessorListResponse   *message,
                      ProtobufCBuffer     *buffer);
GetSuccessorListResponse *
       get_successor_list_response__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   get_successor_list_response__free_unpacked
                     (GetSuccessorListResponse *message,
                      ProtobufCAllocator *allocator);
/* ChordMessage methods */
void   chord_message__init
                     (ChordMessage         *message);
size_t chord_message__get_packed_size
                     (const ChordMessage   *message);
size_t chord_message__pack
                     (const ChordMessage   *message,
                      uint8_t             *out);
size_t chord_message__pack_to_buffer
                     (const ChordMessage   *message,
                      ProtobufCBuffer     *buffer);
ChordMessage *
       chord_message__unpack
                     (ProtobufCAllocator  *allocator,
                      size_t               len,
                      const uint8_t       *data);
void   chord_message__free_unpacked
                     (ChordMessage *message,
                      ProtobufCAllocator *allocator);
/* --- per-message closures --- */

typedef void (*Node_Closure)
                 (const Node *message,
                  void *closure_data);
typedef void (*NotifyRequest_Closure)
                 (const NotifyRequest *message,
                  void *closure_data);
typedef void (*NotifyResponse_Closure)
                 (const NotifyResponse *message,
                  void *closure_data);
typedef void (*FindSuccessorRequest_Closure)
                 (const FindSuccessorRequest *message,
                  void *closure_data);
typedef void (*FindSuccessorResponse_Closure)
                 (const FindSuccessorResponse *message,
                  void *closure_data);
typedef void (*GetPredecessorRequest_Closure)
                 (const GetPredecessorRequest *message,
                  void *closure_data);
typedef void (*GetPredecessorResponse_Closure)
                 (const GetPredecessorResponse *message,
                  void *closure_data);
typedef void (*CheckPredecessorRequest_Closure)
                 (const CheckPredecessorRequest *message,
                  void *closure_data);
typedef void (*CheckPredecessorResponse_Closure)
                 (const CheckPredecessorResponse *message,
                  void *closure_data);
typedef void (*GetSuccessorListRequest_Closure)
                 (const GetSuccessorListRequest *message,
                  void *closure_data);
typedef void (*GetSuccessorListResponse_Closure)
                 (const GetSuccessorListResponse *message,
                  void *closure_data);
typedef void (*ChordMessage_Closure)
                 (const ChordMessage *message,
                  void *closure_data);

/* --- services --- */


/* --- descriptors --- */

extern const ProtobufCMessageDescriptor node__descriptor;
extern const ProtobufCMessageDescriptor notify_request__descriptor;
extern const ProtobufCMessageDescriptor notify_response__descriptor;
extern const ProtobufCMessageDescriptor find_successor_request__descriptor;
extern const ProtobufCMessageDescriptor find_successor_response__descriptor;
extern const ProtobufCMessageDescriptor get_predecessor_request__descriptor;
extern const ProtobufCMessageDescriptor get_predecessor_response__descriptor;
extern const ProtobufCMessageDescriptor check_predecessor_request__descriptor;
extern const ProtobufCMessageDescriptor check_predecessor_response__descriptor;
extern const ProtobufCMessageDescriptor get_successor_list_request__descriptor;
extern const ProtobufCMessageDescriptor get_successor_list_response__descriptor;
extern const ProtobufCMessageDescriptor chord_message__descriptor;

PROTOBUF_C__END_DECLS


#endif  /* PROTOBUF_C_protobuf_2fchord_2eproto__INCLUDED */
