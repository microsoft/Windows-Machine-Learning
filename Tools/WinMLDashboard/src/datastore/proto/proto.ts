const browserGlobal = window as any;

export class Proto {
    public static types: {
        ModelProto: any,
        StringStringEntryProto: any,
        TypeProto: any,
        ValueInfoProto: any,
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
            this.types = this.onnx;
        }
        return this.onnx;
    }

    private static onnx: any;
    public proto: any;
}
