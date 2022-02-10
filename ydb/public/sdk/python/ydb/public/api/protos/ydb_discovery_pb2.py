# -*- coding: utf-8 -*-
# Generated by the protocol buffer compiler.  DO NOT EDIT!
# source: ydb/public/api/protos/ydb_discovery.proto 
"""Generated protocol buffer code."""
from google.protobuf import descriptor as _descriptor
from google.protobuf import message as _message
from google.protobuf import reflection as _reflection
from google.protobuf import symbol_database as _symbol_database
# @@protoc_insertion_point(imports)

_sym_db = _symbol_database.Default()


from ydb.public.api.protos import ydb_operation_pb2 as ydb_dot_public_dot_api_dot_protos_dot_ydb__operation__pb2


DESCRIPTOR = _descriptor.FileDescriptor(
  name='ydb/public/api/protos/ydb_discovery.proto', 
  package='Ydb.Discovery',
  syntax='proto3',
  serialized_options=b'\n\030com.yandex.ydb.discoveryB\017DiscoveryProtos\370\001\001',
  create_key=_descriptor._internal_create_key,
  serialized_pb=b'\n)ydb/public/api/protos/ydb_discovery.proto\x12\rYdb.Discovery\x1a)ydb/public/api/protos/ydb_operation.proto\"9\n\x14ListEndpointsRequest\x12\x10\n\x08\x64\x61tabase\x18\x01 \x01(\t\x12\x0f\n\x07service\x18\x02 \x03(\t\"\xc3\x01\n\x0c\x45ndpointInfo\x12\x0f\n\x07\x61\x64\x64ress\x18\x01 \x01(\t\x12\x0c\n\x04port\x18\x02 \x01(\r\x12\x13\n\x0bload_factor\x18\x03 \x01(\x02\x12\x0b\n\x03ssl\x18\x04 \x01(\x08\x12\x0f\n\x07service\x18\x05 \x03(\t\x12\x10\n\x08location\x18\x06 \x01(\t\x12\x0f\n\x07node_id\x18\x07 \x01(\r\x12\r\n\x05ip_v4\x18\x08 \x03(\t\x12\r\n\x05ip_v6\x18\t \x03(\t\x12 \n\x18ssl_target_name_override\x18\n \x01(\t\"\\\n\x13ListEndpointsResult\x12.\n\tendpoints\x18\x01 \x03(\x0b\x32\x1b.Ydb.Discovery.EndpointInfo\x12\x15\n\rself_location\x18\x02 \x01(\t\"E\n\x15ListEndpointsResponse\x12,\n\toperation\x18\x01 \x01(\x0b\x32\x19.Ydb.Operations.Operation\"\'\n\rWhoAmIRequest\x12\x16\n\x0einclude_groups\x18\x01 \x01(\x08\",\n\x0cWhoAmIResult\x12\x0c\n\x04user\x18\x01 \x01(\t\x12\x0e\n\x06groups\x18\x02 \x03(\t\">\n\x0eWhoAmIResponse\x12,\n\toperation\x18\x01 \x01(\x0b\x32\x19.Ydb.Operations.OperationB.\n\x18\x63om.yandex.ydb.discoveryB\x0f\x44iscoveryProtos\xf8\x01\x01\x62\x06proto3'
  ,
  dependencies=[ydb_dot_public_dot_api_dot_protos_dot_ydb__operation__pb2.DESCRIPTOR,])




_LISTENDPOINTSREQUEST = _descriptor.Descriptor(
  name='ListEndpointsRequest',
  full_name='Ydb.Discovery.ListEndpointsRequest',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  create_key=_descriptor._internal_create_key,
  fields=[
    _descriptor.FieldDescriptor(
      name='database', full_name='Ydb.Discovery.ListEndpointsRequest.database', index=0,
      number=1, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=b"".decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='service', full_name='Ydb.Discovery.ListEndpointsRequest.service', index=1,
      number=2, type=9, cpp_type=9, label=3,
      has_default_value=False, default_value=[],
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=103,
  serialized_end=160,
)


_ENDPOINTINFO = _descriptor.Descriptor(
  name='EndpointInfo',
  full_name='Ydb.Discovery.EndpointInfo',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  create_key=_descriptor._internal_create_key,
  fields=[
    _descriptor.FieldDescriptor(
      name='address', full_name='Ydb.Discovery.EndpointInfo.address', index=0,
      number=1, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=b"".decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='port', full_name='Ydb.Discovery.EndpointInfo.port', index=1,
      number=2, type=13, cpp_type=3, label=1,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='load_factor', full_name='Ydb.Discovery.EndpointInfo.load_factor', index=2,
      number=3, type=2, cpp_type=6, label=1,
      has_default_value=False, default_value=float(0),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='ssl', full_name='Ydb.Discovery.EndpointInfo.ssl', index=3,
      number=4, type=8, cpp_type=7, label=1,
      has_default_value=False, default_value=False,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='service', full_name='Ydb.Discovery.EndpointInfo.service', index=4,
      number=5, type=9, cpp_type=9, label=3,
      has_default_value=False, default_value=[],
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='location', full_name='Ydb.Discovery.EndpointInfo.location', index=5,
      number=6, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=b"".decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='node_id', full_name='Ydb.Discovery.EndpointInfo.node_id', index=6,
      number=7, type=13, cpp_type=3, label=1,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='ip_v4', full_name='Ydb.Discovery.EndpointInfo.ip_v4', index=7,
      number=8, type=9, cpp_type=9, label=3,
      has_default_value=False, default_value=[],
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='ip_v6', full_name='Ydb.Discovery.EndpointInfo.ip_v6', index=8,
      number=9, type=9, cpp_type=9, label=3,
      has_default_value=False, default_value=[],
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='ssl_target_name_override', full_name='Ydb.Discovery.EndpointInfo.ssl_target_name_override', index=9,
      number=10, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=b"".decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=163,
  serialized_end=358,
)


_LISTENDPOINTSRESULT = _descriptor.Descriptor(
  name='ListEndpointsResult',
  full_name='Ydb.Discovery.ListEndpointsResult',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  create_key=_descriptor._internal_create_key,
  fields=[
    _descriptor.FieldDescriptor(
      name='endpoints', full_name='Ydb.Discovery.ListEndpointsResult.endpoints', index=0,
      number=1, type=11, cpp_type=10, label=3,
      has_default_value=False, default_value=[],
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='self_location', full_name='Ydb.Discovery.ListEndpointsResult.self_location', index=1,
      number=2, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=b"".decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=360,
  serialized_end=452,
)


_LISTENDPOINTSRESPONSE = _descriptor.Descriptor(
  name='ListEndpointsResponse',
  full_name='Ydb.Discovery.ListEndpointsResponse',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  create_key=_descriptor._internal_create_key,
  fields=[
    _descriptor.FieldDescriptor(
      name='operation', full_name='Ydb.Discovery.ListEndpointsResponse.operation', index=0,
      number=1, type=11, cpp_type=10, label=1,
      has_default_value=False, default_value=None,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=454,
  serialized_end=523,
)


_WHOAMIREQUEST = _descriptor.Descriptor(
  name='WhoAmIRequest',
  full_name='Ydb.Discovery.WhoAmIRequest',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  create_key=_descriptor._internal_create_key,
  fields=[
    _descriptor.FieldDescriptor(
      name='include_groups', full_name='Ydb.Discovery.WhoAmIRequest.include_groups', index=0,
      number=1, type=8, cpp_type=7, label=1,
      has_default_value=False, default_value=False,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=525,
  serialized_end=564,
)


_WHOAMIRESULT = _descriptor.Descriptor(
  name='WhoAmIResult',
  full_name='Ydb.Discovery.WhoAmIResult',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  create_key=_descriptor._internal_create_key,
  fields=[
    _descriptor.FieldDescriptor(
      name='user', full_name='Ydb.Discovery.WhoAmIResult.user', index=0,
      number=1, type=9, cpp_type=9, label=1,
      has_default_value=False, default_value=b"".decode('utf-8'),
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='groups', full_name='Ydb.Discovery.WhoAmIResult.groups', index=1,
      number=2, type=9, cpp_type=9, label=3,
      has_default_value=False, default_value=[],
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=566,
  serialized_end=610,
)


_WHOAMIRESPONSE = _descriptor.Descriptor(
  name='WhoAmIResponse',
  full_name='Ydb.Discovery.WhoAmIResponse',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  create_key=_descriptor._internal_create_key,
  fields=[
    _descriptor.FieldDescriptor(
      name='operation', full_name='Ydb.Discovery.WhoAmIResponse.operation', index=0,
      number=1, type=11, cpp_type=10, label=1,
      has_default_value=False, default_value=None,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=612,
  serialized_end=674,
)

_LISTENDPOINTSRESULT.fields_by_name['endpoints'].message_type = _ENDPOINTINFO
_LISTENDPOINTSRESPONSE.fields_by_name['operation'].message_type = ydb_dot_public_dot_api_dot_protos_dot_ydb__operation__pb2._OPERATION
_WHOAMIRESPONSE.fields_by_name['operation'].message_type = ydb_dot_public_dot_api_dot_protos_dot_ydb__operation__pb2._OPERATION
DESCRIPTOR.message_types_by_name['ListEndpointsRequest'] = _LISTENDPOINTSREQUEST
DESCRIPTOR.message_types_by_name['EndpointInfo'] = _ENDPOINTINFO
DESCRIPTOR.message_types_by_name['ListEndpointsResult'] = _LISTENDPOINTSRESULT
DESCRIPTOR.message_types_by_name['ListEndpointsResponse'] = _LISTENDPOINTSRESPONSE
DESCRIPTOR.message_types_by_name['WhoAmIRequest'] = _WHOAMIREQUEST
DESCRIPTOR.message_types_by_name['WhoAmIResult'] = _WHOAMIRESULT
DESCRIPTOR.message_types_by_name['WhoAmIResponse'] = _WHOAMIRESPONSE
_sym_db.RegisterFileDescriptor(DESCRIPTOR)

ListEndpointsRequest = _reflection.GeneratedProtocolMessageType('ListEndpointsRequest', (_message.Message,), {
  'DESCRIPTOR' : _LISTENDPOINTSREQUEST,
  '__module__' : 'ydb.public.api.protos.ydb_discovery_pb2' 
  # @@protoc_insertion_point(class_scope:Ydb.Discovery.ListEndpointsRequest)
  })
_sym_db.RegisterMessage(ListEndpointsRequest)

EndpointInfo = _reflection.GeneratedProtocolMessageType('EndpointInfo', (_message.Message,), {
  'DESCRIPTOR' : _ENDPOINTINFO,
  '__module__' : 'ydb.public.api.protos.ydb_discovery_pb2' 
  # @@protoc_insertion_point(class_scope:Ydb.Discovery.EndpointInfo)
  })
_sym_db.RegisterMessage(EndpointInfo)

ListEndpointsResult = _reflection.GeneratedProtocolMessageType('ListEndpointsResult', (_message.Message,), {
  'DESCRIPTOR' : _LISTENDPOINTSRESULT,
  '__module__' : 'ydb.public.api.protos.ydb_discovery_pb2' 
  # @@protoc_insertion_point(class_scope:Ydb.Discovery.ListEndpointsResult)
  })
_sym_db.RegisterMessage(ListEndpointsResult)

ListEndpointsResponse = _reflection.GeneratedProtocolMessageType('ListEndpointsResponse', (_message.Message,), {
  'DESCRIPTOR' : _LISTENDPOINTSRESPONSE,
  '__module__' : 'ydb.public.api.protos.ydb_discovery_pb2' 
  # @@protoc_insertion_point(class_scope:Ydb.Discovery.ListEndpointsResponse)
  })
_sym_db.RegisterMessage(ListEndpointsResponse)

WhoAmIRequest = _reflection.GeneratedProtocolMessageType('WhoAmIRequest', (_message.Message,), {
  'DESCRIPTOR' : _WHOAMIREQUEST,
  '__module__' : 'ydb.public.api.protos.ydb_discovery_pb2' 
  # @@protoc_insertion_point(class_scope:Ydb.Discovery.WhoAmIRequest)
  })
_sym_db.RegisterMessage(WhoAmIRequest)

WhoAmIResult = _reflection.GeneratedProtocolMessageType('WhoAmIResult', (_message.Message,), {
  'DESCRIPTOR' : _WHOAMIRESULT,
  '__module__' : 'ydb.public.api.protos.ydb_discovery_pb2' 
  # @@protoc_insertion_point(class_scope:Ydb.Discovery.WhoAmIResult)
  })
_sym_db.RegisterMessage(WhoAmIResult)

WhoAmIResponse = _reflection.GeneratedProtocolMessageType('WhoAmIResponse', (_message.Message,), {
  'DESCRIPTOR' : _WHOAMIRESPONSE,
  '__module__' : 'ydb.public.api.protos.ydb_discovery_pb2' 
  # @@protoc_insertion_point(class_scope:Ydb.Discovery.WhoAmIResponse)
  })
_sym_db.RegisterMessage(WhoAmIResponse)


DESCRIPTOR._options = None
# @@protoc_insertion_point(module_scope)
