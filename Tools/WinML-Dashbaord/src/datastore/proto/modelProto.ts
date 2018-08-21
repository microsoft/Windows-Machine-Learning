import { IMetadataProps } from '../state';
import { Proto } from './proto';


class ModelProto extends Proto {
    public setInputs(inputs: { [key: string]: any }) {
        if (!Proto.getOnnx() || !this.proto) {
            return;
        }
        this.proto.graph.input = Object.keys(inputs).map((name: string) => ({ name, ...inputs[name] }));
    }

    public setMetadata(metadata: IMetadataProps) {
        if (!Proto.getOnnx() || !this.proto) {
            return;
        }
        this.proto.metadataProps = Object.keys(metadata).map((x: string) => {
            const entry = new Proto.types.StringStringEntryProto();
            entry.key = x;
            entry.value = metadata[x];
            return entry;
        });
    }
    public setOutputs(outputs: { [key: string]: any }) {
        if (!Proto.getOnnx() || !this.proto) {
            return;
        }
        this.proto.graph.output = Object.keys(outputs).map((name: string) => ({ name, ...outputs[name] }));
    }

    public serialize() {
        Proto.getOnnx();
        const clone = Proto.types.ModelProto.fromObject(this.proto);
        const writer = Proto.types.ModelProto.encode(clone);
        return writer.finish();
    }
}

export const ModelProtoSingleton = new ModelProto();
