import argparse
from pathlib import Path

import winmltools
import onnxmltools

def parse_args():
    parser = argparse.ArgumentParser(description='Convert model to ONNX.')
    parser.add_argument('source', help='source  model')
    parser.add_argument('framework', help='source framework model comes from')
    parser.add_argument('ONNXVersion', help='which ONNX Version to convert to')
    parser.add_argument('quantizationOption', help='on which OS to quantize' )
    parser.add_argument('outputNames', help='names of output nodes')
    parser.add_argument('destination', help='destination ONNX model (ONNX or prototxt extension)')
    parser.add_argument('--name', default='WimMLDashboardConvertedModel', help='(ONNX output only) model name')
    return parser.parse_args()


def get_extension(path):
    return Path(path).suffix[1:].lower()


def save_onnx(onnx_model, destination):
    destination_extension = get_extension(destination)
    if destination_extension == 'onnx':
        winmltools.utils.save_model(onnx_model, destination)
    elif destination_extension == 'prototxt':
        winmltools.utils.save_text(onnx_model, destination)
    else:
        raise RuntimeError('Conversion to extension {} is not supported'.format(destination_extension))

def get_opset(ONNXVersion):
    if '1.2' == ONNXVersion:
        return 7
    elif '1.3' == ONNXVersion:
        return 8
    else:
        return 7

def get_useDequantize(quantizationOption):
    if '19H1' == quantizationOption:
        return True
    else:
        return False

def coreml_converter(args):
    # When imported, CoreML tools checks for the current version of Keras and TF and prints warnings if they are
    # outside its expected range. We don't want it to import these packages (since they are big and take seconds to
    # load) and we don't want to clutter the console with unrelated Keras warnings when converting from CoreML.
    import sys
    sys.modules['keras'] = None
    import coremltools
    source_model = coremltools.utils.load_spec(args.source)
    onnx_model = winmltools.convert_coreml(source_model, get_opset(args.ONNXVersion), args.name)
    return onnx_model


def keras_converter(args):
    from keras.models import load_model
    source_model = load_model(args.source)
    destination_extension = get_extension(args.destination)
    onnx_model = winmltools.convert_keras(source_model, get_opset(args.ONNXVersion))
    return onnx_model

def scikit_learn_converter(args):
    from sklearn.externals import joblib
    source_model = joblib.load(args.source) 
    from onnxmltools.convert.common.data_types import FloatTensorType
    onnx_model = winmltools.convert_sklearn(source_model, get_opset(args.ONNXVersion),
                                  input_features=[('input', FloatTensorType(source_model.coef_.shape))])
    return onnx_model

def xgboost_converter(args):
    from sklearn.externals import joblib
    source_model = joblib.load(args.source)
    from onnxmltools.convert.common.data_types import FloatTensorType
    onnx_model = winmltools.convert_xgboost(source_model, get_opset(args.ONNXVersion),
                                input_features=[('input', FloatTensorType([1, source_model.feature_importances_.shape[0]]))])
    return onnx_model

def libSVM_converter(args):
    import svmutil
    source_model = svmutil.svm_load_model(args.source)
    from onnxmltools.convert.common.data_types import FloatTensorType
    onnx_model = winmltools.convert_libsvm(source_model, get_opset(args.ONNXVersion),
                                input_features=[('input', FloatTensorType([1, 'None']))])
    return onnx_model

def convert_tensorflow_file(filename, opset, output_names, destination, debug=True):
    from tensorflow.core.framework import graph_pb2
    import tensorflow as tf
    import tf2onnx
    import onnx
    graph_def = graph_pb2.GraphDef()
    with open(filename, 'rb') as file:
        graph_def.ParseFromString(file.read())
    g = tf.import_graph_def(graph_def, name='')
    with tf.Session(graph=g) as sess:
        converted_model = winmltools.convert_tensorflow(sess.graph, opset, continue_on_error=True, verbose=True, output_names=output_names)
        onnx.checker.check_model(converted_model)
    return converted_model
    # if debug:
    #     with open(destination, 'wb') as file:
    #         file.write(converted_model.SerializeToString())
    # tf.reset_default_graph()

def tensorFlow_converter(args):
    convert_tensorflow_file(args.source, get_opset(args.ONNXVersion), args.outputNames.split(), args.destination)

def onnx_converter(args):
    onnx_model = winmltools.load_model(args.source)
    return onnx_model

framework_converters = {
    'coreml': coreml_converter,
    'keras': keras_converter,
    'scikit-learn': scikit_learn_converter,
    'xgboost': xgboost_converter,
    'libsvm': libSVM_converter,
    'tensorflow': tensorFlow_converter
}

suffix_converters = {
    'h5': keras_converter,
    'keras': keras_converter,
    'mlmodel': coreml_converter,
    'onnx': coreml_converter,
}


def main(args):
    # TODO: framework converter check.
    source_extension = get_extension(args.source)
    framework = args.framework.lower()
    frame_converter = framework_converters.get(framework)
    suffix_converter = suffix_converters.get(source_extension) 

    if not (frame_converter or suffix_converter):
        raise RuntimeError('Conversion from extension {} is not supported'.format(source_extension))

    if frame_converter and suffix_converter and (frame_converter != suffix_converter):
        raise RuntimeError('model with extension {} do not come from {}'.format(source_extension, framework))

    onnx_model = None
    if frame_converter:
        onnx_model = frame_converter(args)
    else:
        onnx_model = suffix_converter(args)
    
    if(args.quantizationOption and 'none' != args.quantizationOption):
        onnx_model = winmltools.quantize(onnx_model, use_dequantize_linear=get_useDequantize(args.quantizationOption))
    
    save_onnx(onnx_model, args.destination)


if __name__ == '__main__':
    main(parse_args())
