import os.path as path
import onnx
from onnx import helper


model_path = path.join(path.dirname(path.abspath(__file__)), r'..\..\..\SharedContent\models\SqueezeNet.onnx')
modified_path = path.join(path.dirname(path.abspath(__file__)),  r'..\..\..\SharedContent\models\SqueezeNet_python_modified.onnx')

model = onnx.load(model_path)

debug_node = model.graph.node.add()

debug_node.CopyFrom(helper.make_node('Debug', ['data_0'], ['unused_output'], file_type='png', file_path='outputfolder/outputname'))

onnx.save(model, modified_path)
