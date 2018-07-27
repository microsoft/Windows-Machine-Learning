const browserGlobal = window as any;

export class Proto {
    public static types: {
        ModelProto: any,
        StringStringEntryProto: any,
    };

    public static getOnnx() {
        if (!this.onnx) {
            if (!browserGlobal.protobuf) {
                throw new Error("Can't find protobuf module");
            }
            if (!browserGlobal.protobuf.roots.onnx) {
                return;
            }
            this.onnx = browserGlobal.protobuf.roots.onnx.onnx;
            this.types = {
                ModelProto: this.onnx.ModelProto,
                StringStringEntryProto: this.onnx.StringStringEntryProto,
            };
        }
        return this.onnx;
    }

    public static download(data: Uint8Array, filename='model.onnx') {
        const blob = new Blob([data], {type: 'application/octet-stream'});
        const anchor = document.createElement('a');
        anchor.href = URL.createObjectURL(blob);
        anchor.download = filename;

        document.body.appendChild(anchor);
        anchor.click();
        document.body.removeChild(anchor);
        URL.revokeObjectURL(anchor.href);
    }

    private static onnx: any;
    public proto: any;
}
