# this script will interlace debug operators for every intermediate output of the onnx graph

import sys
import os.path as path
import onnx
from onnx import helper
import argparse

def create_modified_model(args):
    model_path = path.join(path.dirname(path.abspath(__file__)), args.model_path)
    modified_path = path.join(path.dirname(path.abspath(__file__)), args.modified_path)
    model = onnx.load(model_path)

    intermediate_outputs = set()

    # gather each unique intermediate output in the graph
    for node in model.graph.node:
        for output in node.output:
            intermediate_outputs.add(output)

    # create a debug operator that consumes each intermediate output
    # debug operator file path attribute is constructed by the name of the intermediate output
    for output in intermediate_outputs:
        inserted_node = model.graph.node.add()
        valid_output_path = output.replace('/', '\\')
        inserted_node.CopyFrom(helper.make_node('Debug', [output], ['unused_' + output], file_type=args.debug_file_type, file_path=path.join(valid_output_path,  valid_output_path)))

    onnx.save(model, modified_path)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--model_path', help='source model path to add debug nodes', required=True)
    parser.add_argument('--modified_path', help='output model path', required=True)
    parser.add_argument('--debug_file_type', help='file_type attribute of Debug Operator inserted', choices=['png', 'text'], required=True)
    args = parser.parse_args()
    create_modified_model(args)