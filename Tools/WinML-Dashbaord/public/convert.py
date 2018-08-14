import argparse
from pathlib import Path

import onnxmltools


def parse_args():
    parser = argparse.ArgumentParser(description='Convert model to ONNX.')
    parser.add_argument('source', help='source CoreML or Keras model')
    parser.add_argument('destination', help='destination ONNX model (ONNX or prototxt extension)')
    parser.add_argument('--name', default='WimMLDashboardConvertedModel', help='(ONNX output only) model name')
    return parser.parse_args()


def get_extension(path):
    extension = Path(path).suffix
    return extension[1:].lower()


def save_onnx(onnx_model, destination):
    destination_extension = get_extension(destination)
    if destination_extension == 'onnx':
        onnxmltools.utils.save_model(onnx_model, destination)
    elif destination_extension == 'prototxt':
        onnxmltools.utils.save_text(onnx_model, destination)
    else:
        raise RuntimeError('Conversion to extension {} is not supported'.format(destination_extension))


def coreml_converter(args):
    import coremltools
    source_model = coremltools.utils.load_spec(args.source)
    onnx_model = onnxmltools.convert_coreml(source_model, args.name)
    save_onnx(onnx_model, args.destination)


def keras_converter(args):
    from keras.models import load_model
    source_model = load_model(args.source)
    destination_extension = get_extension(args.destination)
    onnx_model = onnxmltools.convert_keras(source_model)
    save_onnx(onnx_model, args.destination)

converters = {
    'h5': keras_converter,
    'keras': keras_converter,
    'mlmodel': coreml_converter,
}


def main(args):
    source_extension = get_extension(args.source)
    destination_extension = get_extension(args.destination)
    if destination_extension not in ('onnx', 'prototxt'):
        raise RuntimeError('Conversion to extension {} is not supported'.format(destination_extension))
    converter = converters.get(source_extension)
    if not converter:
        raise RuntimeError('Conversion from extension {} is not supported'.format(source_extension))
    converter(args)


if __name__ == '__main__':
    main(parse_args())
