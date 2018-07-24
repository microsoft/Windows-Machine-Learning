import { IMetadataProps } from '../state';
import { Proto } from './proto';

export class ModelProto extends Proto {
    public static setMetadata(metadata: IMetadataProps) {
        const proto = this.get();
        proto.metadataProps = Object.keys(metadata).reduce((acc: any[], x: string) => {
            const entry = new this.types.StringStringEntryProto();
            entry.key = x;
            entry.value = metadata[x];
            acc.push(entry);
            return acc;
        }, []);
    }

    public static serialize() {
        const writer = this.types.ModelProto.encode(this.get());
        return writer.finish();
    }
}
