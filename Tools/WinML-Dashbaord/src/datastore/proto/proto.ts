const browserGlobal = window as any;

export class Proto {
    public static types: {
        ModelProto: any,
        StringStringEntryProto: any,
    };

    public static set(proto: any) {
        if (!this.getOnnx()) {
            throw new Error("Can't find protobuf module");
        }
        this.proto = proto;
    }

    public static get() {
        return this.proto;
    }

    private static onnx: any;
    private static proto: any;

    private static getOnnx() {
        if (!this.onnx) {
            if (!browserGlobal.protobuf) {
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
}
