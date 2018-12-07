# this script will interlace debug operators for every intermediate output of the onnx graph

import sys
import os.path as path
import onnx
from onnx import helper


if (len(sys.argv) != 4):
    print('\nusage: `python debug_all_outputs.py <input onnx model path> <output onnx model path> <png|text>\n')
    sys.exit()

model_path = path.join(path.dirname(path.abspath(__file__)), sys.argv[1])
modified_path = path.join(path.dirname(path.abspath(__file__)), sys.argv[2])

model = onnx.load(model_path)

intermediate_outputs = set()

for node in model.graph.node:
    for output in node.output:
        intermediate_outputs.add(output)

for output in intermediate_outputs:
    inserted_node = model.graph.node.add()
    valid_output_path = output.replace('/', '-')
    inserted_node.CopyFrom(helper.make_node('Debug', [output], ['unused_' + output], file_type=sys.argv[3], file_path=valid_output_path + '\\' + valid_output_path))

onnx.save(model, modified_path)
