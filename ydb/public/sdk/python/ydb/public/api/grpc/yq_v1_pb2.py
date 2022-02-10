# -*- coding: utf-8 -*-
# Generated by the protocol buffer compiler.  DO NOT EDIT!
# source: ydb/public/api/grpc/yq_v1.proto 
"""Generated protocol buffer code."""
from google.protobuf import descriptor as _descriptor
from google.protobuf import message as _message
from google.protobuf import reflection as _reflection
from google.protobuf import symbol_database as _symbol_database
# @@protoc_insertion_point(imports)

_sym_db = _symbol_database.Default()


from ydb.public.api.protos import yq_pb2 as ydb_dot_public_dot_api_dot_protos_dot_yq__pb2


DESCRIPTOR = _descriptor.FileDescriptor(
  name='ydb/public/api/grpc/yq_v1.proto', 
  package='YandexQuery.V1',
  syntax='proto3',
  serialized_options=b'\n\023com.yandex.query.v1',
  create_key=_descriptor._internal_create_key,
  serialized_pb=b'\n\x1fydb/public/api/grpc/yq_v1.proto\x12\x0eYandexQuery.V1\x1a\x1eydb/public/api/protos/yq.proto2\xd5\x0e\n\x12YandexQueryService\x12P\n\x0b\x43reateQuery\x12\x1f.YandexQuery.CreateQueryRequest\x1a .YandexQuery.CreateQueryResponse\x12P\n\x0bListQueries\x12\x1f.YandexQuery.ListQueriesRequest\x1a .YandexQuery.ListQueriesResponse\x12V\n\rDescribeQuery\x12!.YandexQuery.DescribeQueryRequest\x1a\".YandexQuery.DescribeQueryResponse\x12Y\n\x0eGetQueryStatus\x12\".YandexQuery.GetQueryStatusRequest\x1a#.YandexQuery.GetQueryStatusResponse\x12P\n\x0bModifyQuery\x12\x1f.YandexQuery.ModifyQueryRequest\x1a .YandexQuery.ModifyQueryResponse\x12P\n\x0b\x44\x65leteQuery\x12\x1f.YandexQuery.DeleteQueryRequest\x1a .YandexQuery.DeleteQueryResponse\x12S\n\x0c\x43ontrolQuery\x12 .YandexQuery.ControlQueryRequest\x1a!.YandexQuery.ControlQueryResponse\x12V\n\rGetResultData\x12!.YandexQuery.GetResultDataRequest\x1a\".YandexQuery.GetResultDataResponse\x12G\n\x08ListJobs\x12\x1c.YandexQuery.ListJobsRequest\x1a\x1d.YandexQuery.ListJobsResponse\x12P\n\x0b\x44\x65scribeJob\x12\x1f.YandexQuery.DescribeJobRequest\x1a .YandexQuery.DescribeJobResponse\x12_\n\x10\x43reateConnection\x12$.YandexQuery.CreateConnectionRequest\x1a%.YandexQuery.CreateConnectionResponse\x12\\\n\x0fListConnections\x12#.YandexQuery.ListConnectionsRequest\x1a$.YandexQuery.ListConnectionsResponse\x12\x65\n\x12\x44\x65scribeConnection\x12&.YandexQuery.DescribeConnectionRequest\x1a\'.YandexQuery.DescribeConnectionResponse\x12_\n\x10ModifyConnection\x12$.YandexQuery.ModifyConnectionRequest\x1a%.YandexQuery.ModifyConnectionResponse\x12_\n\x10\x44\x65leteConnection\x12$.YandexQuery.DeleteConnectionRequest\x1a%.YandexQuery.DeleteConnectionResponse\x12Y\n\x0eTestConnection\x12\".YandexQuery.TestConnectionRequest\x1a#.YandexQuery.TestConnectionResponse\x12V\n\rCreateBinding\x12!.YandexQuery.CreateBindingRequest\x1a\".YandexQuery.CreateBindingResponse\x12S\n\x0cListBindings\x12 .YandexQuery.ListBindingsRequest\x1a!.YandexQuery.ListBindingsResponse\x12\\\n\x0f\x44\x65scribeBinding\x12#.YandexQuery.DescribeBindingRequest\x1a$.YandexQuery.DescribeBindingResponse\x12V\n\rModifyBinding\x12!.YandexQuery.ModifyBindingRequest\x1a\".YandexQuery.ModifyBindingResponse\x12V\n\rDeleteBinding\x12!.YandexQuery.DeleteBindingRequest\x1a\".YandexQuery.DeleteBindingResponseB\x15\n\x13\x63om.yandex.query.v1b\x06proto3'
  ,
  dependencies=[ydb_dot_public_dot_api_dot_protos_dot_yq__pb2.DESCRIPTOR,])



_sym_db.RegisterFileDescriptor(DESCRIPTOR)


DESCRIPTOR._options = None

_YANDEXQUERYSERVICE = _descriptor.ServiceDescriptor(
  name='YandexQueryService',
  full_name='YandexQuery.V1.YandexQueryService',
  file=DESCRIPTOR,
  index=0,
  serialized_options=None,
  create_key=_descriptor._internal_create_key,
  serialized_start=84,
  serialized_end=1961,
  methods=[
  _descriptor.MethodDescriptor(
    name='CreateQuery',
    full_name='YandexQuery.V1.YandexQueryService.CreateQuery',
    index=0,
    containing_service=None,
    input_type=ydb_dot_public_dot_api_dot_protos_dot_yq__pb2._CREATEQUERYREQUEST,
    output_type=ydb_dot_public_dot_api_dot_protos_dot_yq__pb2._CREATEQUERYRESPONSE,
    serialized_options=None,
    create_key=_descriptor._internal_create_key,
  ),
  _descriptor.MethodDescriptor(
    name='ListQueries',
    full_name='YandexQuery.V1.YandexQueryService.ListQueries',
    index=1,
    containing_service=None,
    input_type=ydb_dot_public_dot_api_dot_protos_dot_yq__pb2._LISTQUERIESREQUEST,
    output_type=ydb_dot_public_dot_api_dot_protos_dot_yq__pb2._LISTQUERIESRESPONSE,
    serialized_options=None,
    create_key=_descriptor._internal_create_key,
  ),
  _descriptor.MethodDescriptor(
    name='DescribeQuery',
    full_name='YandexQuery.V1.YandexQueryService.DescribeQuery',
    index=2,
    containing_service=None,
    input_type=ydb_dot_public_dot_api_dot_protos_dot_yq__pb2._DESCRIBEQUERYREQUEST,
    output_type=ydb_dot_public_dot_api_dot_protos_dot_yq__pb2._DESCRIBEQUERYRESPONSE,
    serialized_options=None,
    create_key=_descriptor._internal_create_key,
  ),
  _descriptor.MethodDescriptor(
    name='GetQueryStatus',
    full_name='YandexQuery.V1.YandexQueryService.GetQueryStatus',
    index=3,
    containing_service=None,
    input_type=ydb_dot_public_dot_api_dot_protos_dot_yq__pb2._GETQUERYSTATUSREQUEST,
    output_type=ydb_dot_public_dot_api_dot_protos_dot_yq__pb2._GETQUERYSTATUSRESPONSE,
    serialized_options=None,
    create_key=_descriptor._internal_create_key,
  ),
  _descriptor.MethodDescriptor(
    name='ModifyQuery',
    full_name='YandexQuery.V1.YandexQueryService.ModifyQuery',
    index=4,
    containing_service=None,
    input_type=ydb_dot_public_dot_api_dot_protos_dot_yq__pb2._MODIFYQUERYREQUEST,
    output_type=ydb_dot_public_dot_api_dot_protos_dot_yq__pb2._MODIFYQUERYRESPONSE,
    serialized_options=None,
    create_key=_descriptor._internal_create_key,
  ),
  _descriptor.MethodDescriptor(
    name='DeleteQuery',
    full_name='YandexQuery.V1.YandexQueryService.DeleteQuery',
    index=5,
    containing_service=None,
    input_type=ydb_dot_public_dot_api_dot_protos_dot_yq__pb2._DELETEQUERYREQUEST,
    output_type=ydb_dot_public_dot_api_dot_protos_dot_yq__pb2._DELETEQUERYRESPONSE,
    serialized_options=None,
    create_key=_descriptor._internal_create_key,
  ),
  _descriptor.MethodDescriptor(
    name='ControlQuery',
    full_name='YandexQuery.V1.YandexQueryService.ControlQuery',
    index=6,
    containing_service=None,
    input_type=ydb_dot_public_dot_api_dot_protos_dot_yq__pb2._CONTROLQUERYREQUEST,
    output_type=ydb_dot_public_dot_api_dot_protos_dot_yq__pb2._CONTROLQUERYRESPONSE,
    serialized_options=None,
    create_key=_descriptor._internal_create_key,
  ),
  _descriptor.MethodDescriptor(
    name='GetResultData',
    full_name='YandexQuery.V1.YandexQueryService.GetResultData',
    index=7,
    containing_service=None,
    input_type=ydb_dot_public_dot_api_dot_protos_dot_yq__pb2._GETRESULTDATAREQUEST,
    output_type=ydb_dot_public_dot_api_dot_protos_dot_yq__pb2._GETRESULTDATARESPONSE,
    serialized_options=None,
    create_key=_descriptor._internal_create_key,
  ),
  _descriptor.MethodDescriptor(
    name='ListJobs',
    full_name='YandexQuery.V1.YandexQueryService.ListJobs',
    index=8,
    containing_service=None,
    input_type=ydb_dot_public_dot_api_dot_protos_dot_yq__pb2._LISTJOBSREQUEST,
    output_type=ydb_dot_public_dot_api_dot_protos_dot_yq__pb2._LISTJOBSRESPONSE,
    serialized_options=None,
    create_key=_descriptor._internal_create_key,
  ),
  _descriptor.MethodDescriptor(
    name='DescribeJob',
    full_name='YandexQuery.V1.YandexQueryService.DescribeJob',
    index=9,
    containing_service=None,
    input_type=ydb_dot_public_dot_api_dot_protos_dot_yq__pb2._DESCRIBEJOBREQUEST,
    output_type=ydb_dot_public_dot_api_dot_protos_dot_yq__pb2._DESCRIBEJOBRESPONSE,
    serialized_options=None,
    create_key=_descriptor._internal_create_key,
  ),
  _descriptor.MethodDescriptor(
    name='CreateConnection',
    full_name='YandexQuery.V1.YandexQueryService.CreateConnection',
    index=10,
    containing_service=None,
    input_type=ydb_dot_public_dot_api_dot_protos_dot_yq__pb2._CREATECONNECTIONREQUEST,
    output_type=ydb_dot_public_dot_api_dot_protos_dot_yq__pb2._CREATECONNECTIONRESPONSE,
    serialized_options=None,
    create_key=_descriptor._internal_create_key,
  ),
  _descriptor.MethodDescriptor(
    name='ListConnections',
    full_name='YandexQuery.V1.YandexQueryService.ListConnections',
    index=11,
    containing_service=None,
    input_type=ydb_dot_public_dot_api_dot_protos_dot_yq__pb2._LISTCONNECTIONSREQUEST,
    output_type=ydb_dot_public_dot_api_dot_protos_dot_yq__pb2._LISTCONNECTIONSRESPONSE,
    serialized_options=None,
    create_key=_descriptor._internal_create_key,
  ),
  _descriptor.MethodDescriptor(
    name='DescribeConnection',
    full_name='YandexQuery.V1.YandexQueryService.DescribeConnection',
    index=12,
    containing_service=None,
    input_type=ydb_dot_public_dot_api_dot_protos_dot_yq__pb2._DESCRIBECONNECTIONREQUEST,
    output_type=ydb_dot_public_dot_api_dot_protos_dot_yq__pb2._DESCRIBECONNECTIONRESPONSE,
    serialized_options=None,
    create_key=_descriptor._internal_create_key,
  ),
  _descriptor.MethodDescriptor(
    name='ModifyConnection',
    full_name='YandexQuery.V1.YandexQueryService.ModifyConnection',
    index=13,
    containing_service=None,
    input_type=ydb_dot_public_dot_api_dot_protos_dot_yq__pb2._MODIFYCONNECTIONREQUEST,
    output_type=ydb_dot_public_dot_api_dot_protos_dot_yq__pb2._MODIFYCONNECTIONRESPONSE,
    serialized_options=None,
    create_key=_descriptor._internal_create_key,
  ),
  _descriptor.MethodDescriptor(
    name='DeleteConnection',
    full_name='YandexQuery.V1.YandexQueryService.DeleteConnection',
    index=14,
    containing_service=None,
    input_type=ydb_dot_public_dot_api_dot_protos_dot_yq__pb2._DELETECONNECTIONREQUEST,
    output_type=ydb_dot_public_dot_api_dot_protos_dot_yq__pb2._DELETECONNECTIONRESPONSE,
    serialized_options=None,
    create_key=_descriptor._internal_create_key,
  ),
  _descriptor.MethodDescriptor(
    name='TestConnection',
    full_name='YandexQuery.V1.YandexQueryService.TestConnection',
    index=15,
    containing_service=None,
    input_type=ydb_dot_public_dot_api_dot_protos_dot_yq__pb2._TESTCONNECTIONREQUEST,
    output_type=ydb_dot_public_dot_api_dot_protos_dot_yq__pb2._TESTCONNECTIONRESPONSE,
    serialized_options=None,
    create_key=_descriptor._internal_create_key,
  ),
  _descriptor.MethodDescriptor(
    name='CreateBinding',
    full_name='YandexQuery.V1.YandexQueryService.CreateBinding',
    index=16,
    containing_service=None,
    input_type=ydb_dot_public_dot_api_dot_protos_dot_yq__pb2._CREATEBINDINGREQUEST,
    output_type=ydb_dot_public_dot_api_dot_protos_dot_yq__pb2._CREATEBINDINGRESPONSE,
    serialized_options=None,
    create_key=_descriptor._internal_create_key,
  ),
  _descriptor.MethodDescriptor(
    name='ListBindings',
    full_name='YandexQuery.V1.YandexQueryService.ListBindings',
    index=17,
    containing_service=None,
    input_type=ydb_dot_public_dot_api_dot_protos_dot_yq__pb2._LISTBINDINGSREQUEST,
    output_type=ydb_dot_public_dot_api_dot_protos_dot_yq__pb2._LISTBINDINGSRESPONSE,
    serialized_options=None,
    create_key=_descriptor._internal_create_key,
  ),
  _descriptor.MethodDescriptor(
    name='DescribeBinding',
    full_name='YandexQuery.V1.YandexQueryService.DescribeBinding',
    index=18,
    containing_service=None,
    input_type=ydb_dot_public_dot_api_dot_protos_dot_yq__pb2._DESCRIBEBINDINGREQUEST,
    output_type=ydb_dot_public_dot_api_dot_protos_dot_yq__pb2._DESCRIBEBINDINGRESPONSE,
    serialized_options=None,
    create_key=_descriptor._internal_create_key,
  ),
  _descriptor.MethodDescriptor(
    name='ModifyBinding',
    full_name='YandexQuery.V1.YandexQueryService.ModifyBinding',
    index=19,
    containing_service=None,
    input_type=ydb_dot_public_dot_api_dot_protos_dot_yq__pb2._MODIFYBINDINGREQUEST,
    output_type=ydb_dot_public_dot_api_dot_protos_dot_yq__pb2._MODIFYBINDINGRESPONSE,
    serialized_options=None,
    create_key=_descriptor._internal_create_key,
  ),
  _descriptor.MethodDescriptor(
    name='DeleteBinding',
    full_name='YandexQuery.V1.YandexQueryService.DeleteBinding',
    index=20,
    containing_service=None,
    input_type=ydb_dot_public_dot_api_dot_protos_dot_yq__pb2._DELETEBINDINGREQUEST,
    output_type=ydb_dot_public_dot_api_dot_protos_dot_yq__pb2._DELETEBINDINGRESPONSE,
    serialized_options=None,
    create_key=_descriptor._internal_create_key,
  ),
])
_sym_db.RegisterServiceDescriptor(_YANDEXQUERYSERVICE)

DESCRIPTOR.services_by_name['YandexQueryService'] = _YANDEXQUERYSERVICE

# @@protoc_insertion_point(module_scope)
