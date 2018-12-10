# this script add debug operators for the given intermediate output

import sys
import os.path as path
import onnx
from onnx import helper
import argparse

def create_modified_model(args):
    model_path = path.join(path.dirname(path.abspath(__file__)), args.model_path)
    modified_path = path.join(path.dirname(path.abspath(__file__)), args.modified_path)
    model = onnx.load(model_path)

    #insert new debug operator node into graph with given attributes and input
    inserted_node = model.graph.node.add()
    valid_output_path = args.debug_file_path.replace('/', '\\')
    inserted_node.CopyFrom(helper.make_node('Debug', [args.intermediate_output], ['unused_' + args.intermediate_output], file_type=args.debug_file_type, file_path=valid_output_path))

    onnx.save(model, modified_path)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--model_path', help='source model path to add debug nodes', required=True)
    parser.add_argument('--modified_path', help='output model path', required=True)
    parser.add_argument('--debug_file_type', help='file_type attribute of Debug Operator inserted', choices=['png', 'text'], required=True)
    parser.add_argument('--debug_file_path', help='file_path attribute of Debug Operator inserted', required=True)
    parser.add_argument('--intermediate_output', help='the intermediate output this Debug Operator will consume', required=True)
    args = parser.parse_args()
    create_modified_model(args)