import { IMetadataProps } from '../state';
import { Proto } from './proto';


class ModelProto extends Proto {
    public setMetadata(metadata: IMetadataProps) {
        Proto.getOnnx();
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

    public download() {
        super.download(this.serialize());
    }
}

export const ModelProtoSingleton = new ModelProto();
