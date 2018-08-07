import argparse
from pathlib import Path

import onnxmltools


def parse_args():
    parser = argparse.ArgumentParser(description='Convert model to ONNX.')
    parser.add_argument('source', help='source model')
    parser.add_argument('destination', help='destination for the ONNX model')
    parser.add_argument('--name', default='WimMLDashboardGeneratedModel', help='name for the ONNX model')
    return parser.parse_args()


def main(args):
    source_extension = Path(args.source).suffix.lower()
    if source_extension == '.mlmodel':
        import coremltools
        coreml_model = coremltools.utils.load_spec(args.source)
        onnx_model = onnxmltools.convert_coreml(coreml_model, args.name)
    else:
        raise RuntimeError('File {} has unsupported extension'.format(args.source))
    onnxmltools.utils.save_model(onnx_model, args.destination)


if __name__ == '__main__':
    main(parse_args())
