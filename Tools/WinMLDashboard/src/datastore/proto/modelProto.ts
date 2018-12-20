import { IDebugNode, IMetadataProps } from '../state';
import { Proto } from './proto';

class ModelProto extends Proto {

    // debug nodes will be added to the model proto only right before serialization
    // now serialization with be parametrized by whether we are serializing the debugged model or not
    private debugNodes: IDebugNode[];

    public setDebugNodes(debugNodes: IDebugNode[]) {
        this.debugNodes = debugNodes;
    }

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

    public serialize(debug: boolean) {
        if (!Proto.getOnnx() || !this.proto) {
            return;
        }
        const clone = Proto.types.ModelProto.fromObject(this.proto);
        if (debug) {            
            // Need to actually construct a onnx node rather than our own structure
            clone.graph.node = [ ...clone.graph.node, ...this.createDebugProtoNodes() ]
        }
        const writer = Proto.types.ModelProto.encode(clone);
        return writer.finish();
    }

    private createDebugProtoNodes() {
        const onnx = Proto.getOnnx();
        if (!onnx) {
            return [];
        }
        const nodeProtos = [];
        for (const node of this.debugNodes) {
            const fileTypeProps = {name: 'file_type', type: 'STRING', s: node.fileType };
            const fileTypeAttrProto = onnx.AttributeProto.fromObject(fileTypeProps);

            // const outputFile:string = node.fileType;
            // encoding throws exception if _ is in the name
            const filePathProps = {name: 'file_path', type: 'STRING', s: "tempname"};
            const filePathAttrProto =  onnx.AttributeProto.fromObject(filePathProps);
            
            const nodeProps = {opType: 'Debug', input: [node.output], output: ['unused'], attribute: [fileTypeAttrProto, filePathAttrProto]};
            nodeProtos.push(onnx.NodeProto.fromObject(nodeProps));
        }
        return nodeProtos;
    }
}

export const ModelProtoSingleton = new ModelProto();
