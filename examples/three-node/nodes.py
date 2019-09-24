#!/bin/env python3
# Server on a port, determined by the first thing that fires below:
#  Command line option '-p'
#  environment variable GRPC_PORT

from __future__ import print_function
import sys
import os
from three_node_pb2_grpc import ThreeNodeExampleServicer
import three_node_pb2_grpc

from three_node_pb2 import SplitResponse
from three_node_pb2 import LabelResponse
from three_node_pb2 import ToiResponse

import json
import re,string

def find_word_boundaries(s, ignore_punctuation=False):
    p = 0
    (wb, we) = ([], [])
    inw = False
    for c in s:
        if c in string.punctuation:
            if inw:
                we.append(p)
            if not ignore_punctuation:
                wb.append(p)
                we.append(p+1)
            inw = False
        elif c in string.whitespace:
            if inw:
                we.append(p)
                inw = False
        else:
            if not inw: 
                wb.append(p)
            inw = True
        p += 1
    if inw and len(we) < len(wb):
        we.append(p)
    return [a for a in zip(wb, we)]

class ThreeNodeExampleServer(ThreeNodeExampleServicer):
    def __init__(self):
        pass

    # rpc split(SplitRequest) returns(SplitResponse) {}
    
    def split(self, request, context):
        text = request.sentence
        max_tokens = request.max_tokens
        result = SplitResponse()
        (lc, ls) = (1, 0)
        for (wb, we) in find_word_boundaries(text):
            res = SplitResponse.Token()
            res.token = text[wb:we]
            res.position = wb
            res.length = we-wb
            res.line_number = lc
            res.line_start = ls
            result.tokens.extend([res])
        return result

    # rpc strtoint(ToiRequest) returns(ToiResponse) {}
    def strtoint(self, request, context):
        word = request.word
        value = request.default_value
        is_number = True
        try:
            value = int(word)
        except ValueError:
            is_number = False
        result = ToiResponse(num_value=value, is_number=is_number)
        return result

    # rpc label(LabelRequest) returns(LabelResponse) {}
    def label(self, request, context):
        word = request.word
        if word.isdecimal():
            label = 'NUM'
        elif word.isalpha():
            label = 'ALPHA'
        elif word.isalnum():
            label = 'ALNUM'
        else:
            label = 'PUNCT'
        result = LabelResponse(label=label)
        return result
    

from concurrent import futures
import time, grpc, sys

from optparse import OptionParser
def serve():
    parser = OptionParser()
    parser.add_option("-p", "--port", dest="port", type="int", default=int(os.environ.get('GRPC_PORT', '50666')))
    (options, args) = parser.parse_args()

    server = grpc.server(futures.ThreadPoolExecutor(max_workers=1))
    three_node_pb2_grpc.add_ThreeNodeExampleServicer_to_server(ThreeNodeExampleServer(), server)
    server.add_insecure_port('[::]:%d' % options.port)
    print("ThreeNodeExample gRPC listening on port %d" % int(options.port))
    server.start()
    try:
        while True:
            # sleep one hour 
            time.sleep(60 * 60)
    except KeyboardInterrupt:
        server.stop(0)

if __name__ == '__main__':
    serve()
