import { fileFilterToAccept } from "./dialog";

it('converts Array<FileFilter> to HTML accept field', () => {
    expect(fileFilterToAccept([
        { name: 'CoreML model', extensions: [ 'mlmodel' ] },
        { name: 'Keras model', extensions: [ 'keras', 'h5' ] },
        { name: 'ONNX model', extensions: [ 'onnx' ] },
    ])).toBe('.mlmodel,.keras,.h5,.onnx');
});
