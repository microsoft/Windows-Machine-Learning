import { IMetadataProps } from '../state';
import { Proto } from './proto';


class ModelProto extends Proto {
    public setInputs(inputs: { [key: string]: any }) {
        if (!Proto.getOnnx() || !this.proto) {
            return;
        }
        this.proto.graph.input = inputs;
    }

    public setMetadata(metadata: IMetadataProps) {
        if (!Proto.getOnnx() || !this.proto) {
            return;
        }
        this.proto.metadataProps = Object.keys(metadata).reduce((acc: any[], x: string) => {
            const entry = new Proto.types.StringStringEntryProto();
            entry.key = x;
            entry.value = metadata[x];
            acc.push(entry);
            return acc;
        }, []);
    }

    public serialize() {
        Proto.getOnnx();
        const writer = Proto.types.ModelProto.encode(this.proto);
        return writer.finish();
    }

    public download = () => {
        Proto.download(this.serialize());
    }
}

export const ModelProtoSingleton = new ModelProto();
