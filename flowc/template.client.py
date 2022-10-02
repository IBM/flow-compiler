import logging
import argparse
import sys

import grpc
from google.protobuf.json_format import Parse
from google.protobuf.json_format import MessageToJson

PROTO = {{ENTRIES_PROTO/c}}
CHANNEL_OPTIONS = [('grpc.keepalive_timeout_ms', 600000) ]
RPCS = [ {I:ENTRY_FULL_NAME{{{ENTRY_FULL_NAME/c}}, }I}"" ][:-1]

def run_client(args, channel):
    stub = None
    found = False
{I:ENTRY_FULL_NAME{ 
    if not found and args.entry_name == '{{ENTRY_FULL_NAME}}':
        found = True
        stub = {{NAME}}_pb2_grpc.{{ENTRY_SERVICE_SHORT_NAME}}Stub(channel)
        with open(args.infn, 'r') as infn, open(args.outfn, 'w') as outfn:
            count = 0
            while True:
                count += 1
                text = infn.readline()
                if not text:
                    break
                if text.lstrip().startswith('#') or text.lstrip() == '':
                    print(text.rstrip(), file=outfn)
                    continue                    
                print("sending {}".format(count), file=sys.stderr)
                request = Parse(text, {{NAME}}_pb2.{{ENTRY_INPUT_SHORT_TYPE}}())
                response = stub.{{ENTRY_NAME}}(request, timeout=args.grpc_timeout)
                response_json = MessageToJson(response, including_default_value_fields=False, preserving_proto_field_name=True, indent=0, sort_keys=False, use_integers_for_enums=False).replace('\n', '')
                print(response_json, file=outfn)
}I}
    if not found:
        print('Unknown RPC: {}\nmust be one of: {}'.format(args.entry_name, ', '.join(RPCS)), file=sys.stderr)
        return 1
    return 0

if __name__ == '__main__':
    logging.basicConfig()
    from argparse import RawTextHelpFormatter
    parser = argparse.ArgumentParser(description='{{NAME}} gRPC client\n{{MAIN_FILE_SHORT}} ({{MAIN_FILE_TS}})\n{{FLOWC_NAME}} {{FLOWC_VERSION}} ({{FLOWC_BUILD}})\ngRPC version {}'.format(grpc.__version__), formatter_class=RawTextHelpFormatter)
    parser.add_argument('-a', metavar='HOST:PORT', dest='endpoint', help='gRPC server address', default=None)
    parser.add_argument('-b', '--blocking-calls', action='store_true', dest='blocking_calls', default=False)
    parser.add_argument('-g', '--ignore-grpc-errors', action='store_true', dest='ignore_grpc_errors', default=False, help='do not stop for gRPC errors')
    parser.add_argument('-j', '--ignore-json-errors', action='store_true', dest='ignore_json_errors', default=False, help='ignore requests that fail conversion from JSON to protobuf')
    parser.add_argument('-T', '--timeout', metavar='SECONDS', dest='grpc_timeout', type=int, default=3600, help='Timeout for gRPC calls')
    parser.add_argument('-t', '--time-calls', action='store_true', dest='time_calls', default=False, help='return time information from the server')
    parser.add_argument('-r', '--rpc', metavar='RPC', dest='entry_name', default='{{ENTRY_FULL_NAME}}', help='remote procedure to call, one of: {}'.format(', '.join(RPCS)))
    parser.add_argument('-P', '--proto', dest='show_proto', action='store_true', default=False, help='display proto file')
    parser.add_argument('-i', '--input', metavar='FILE', dest='infn', default='-', help='input file, use - for stdin')
    parser.add_argument('-o', '--output', metavar='FILE', dest='outfn', default='-', help='output file, use - for stdout')
    args = parser.parse_args()
    if args.infn == '-':
        args.infn = '/dev/stdin'
    if args.outfn == '-':
        args.outfn = '/dev/stdout'

    rc = 0
    if args.show_proto:
        print(PROTO)
    elif not args.endpoint:
        print('gRPC server adderess must be set', file=sys.stderr)
        rc = 1
    else:
        try:
            import {{NAME}}_pb2
            import {{NAME}}_pb2_grpc
        except:
            import grpc_tools.protoc
            sdir = os.path.dirname(sys.argv[0])
            if sdir == '':
                sdir = '.'
            pfile = os.path.join(sdir, '{{NAME}}.proto')
            with open(pfile, 'w') as pf:
                print(PROTO, file=pf)
            grpc_tools.protoc.main([f'--proto_path={sdir}', f'--python_out={sdir}', f'--grpc_python_out={sdir}', pfile])
            import {{NAME}}_pb2
            import {{NAME}}_pb2_grpc
        with grpc.insecure_channel(target=args.endpoint, options=CHANNEL_OPTIONS) as channel:
            rc = run_client(args, channel)
    sys.exit(rc)

