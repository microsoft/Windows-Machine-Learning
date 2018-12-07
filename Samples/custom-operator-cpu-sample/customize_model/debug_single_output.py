# this script add debug operators for the given intermediate output

import sys
import os.path as path
import onnx
from onnx import helper


if (len(sys.argv) != 5):
    print('\nusage: `customize_model_debug.py <input onnx model path> <output onnx model path> <png|text> <output name>\n')
    sys.exit()

model_path = path.join(path.dirname(path.abspath(__file__)), sys.argv[1])
modified_path = path.join(path.dirname(path.abspath(__file__)), sys.argv[2])

model = onnx.load(model_path)

inserted_node = model.graph.node.add()
output = sys.argv[4]
valid_output_path = output.replace('/', '-')
inserted_node.CopyFrom(helper.make_node('Debug', [output], ['unused_' + output], file_type=sys.argv[3], file_path=valid_output_path + '\\' + valid_output_path))

onnx.save(model, modified_path)