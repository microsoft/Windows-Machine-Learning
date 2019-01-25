import * as md5 from 'md5';
import * as path from 'path';
import { mkdir } from '../../native/appData';
import { AttributeType, IDebugNodeMap, IInsertNode, IMetadataProps } from '../state';
import { Proto } from './proto';


class ModelProto extends Proto {

    // debug nodes will be added to the model proto only right before serialization
    // now serialization with be parametrized by whether we are serializing the debugged model or not
    private debugNodes: IDebugNodeMap = {};
    private insertNodes: IInsertNode[] = [];

    public setDebugNodes(debugNodes: IDebugNodeMap) {
        this.debugNodes = debugNodes;
    }

    public setInsertNodes(insertNodes: IInsertNode[]) {
        this.insertNodes = insertNodes;
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
        const needDebug = debug && Object.keys(this.debugNodes).length !== 0;
        const needInsert = this.insertNodes.length !== 0;

        // will need to save temp version of node protobuf
        if (needDebug || needInsert) {
            const nodeClone = this.proto.graph.node.slice();       
            if (needDebug) {
                this.proto.graph.node = [ ...this.proto.graph.node, ...this.createDebugProtoNodes() ];
            }
            if (needInsert) {
                this.proto.graph.node = [ ...this.proto.graph.node, ...this.createInsertProtoNodes() ];
            }
            const writer = Proto.types.ModelProto.encode(this.proto);
            const data = writer.finish();
            this.proto.graph.node = nodeClone;
            return data;
        } else {
            const writer = Proto.types.ModelProto.encode(this.proto);
            return writer.finish();
        }
    }

    private createInsertProtoNodes() {
        const onnx = Proto.getOnnx();
        if (!onnx) {
            return [];
        }
        
        const nodeProtos = [];
        for (const insertNode of this.insertNodes) {
            const nodeProps: any = {input: insertNode.input, opType: insertNode.opType, output: insertNode.output};
            const attrPropsArr = [];
            for (const attr of insertNode.requiredAttributes.concat(insertNode.optionalAttributes)) {
                const attrProps = {name: attr.name, type: attr.type };
                const dataKey = this.getAttributeDataPropName(attr.type);
                attrProps[dataKey] = attr.value;
                const attrProto = onnx.AttributeProto.fromObject(attrProps);
                attrPropsArr.push(attrProto);
            }
            nodeProps.attribute = attrPropsArr;
            nodeProtos.push(onnx.NodeProto.fromObject(nodeProps));
        }
        return nodeProtos;
    }

    private getAttributeDataPropName(type: AttributeType) {
        if (type === AttributeType.float) {
            return 'f';
        } else if (type === AttributeType.integer) {
            return 'i';
        } else {
            return 's';
        }
    }

    private createDebugProtoNodes() {
        const onnx = Proto.getOnnx();
        if (!onnx) {
            return [];
        }
        const nodeProtos = [];
        for (const output of Object.keys(this.debugNodes)) {
            for (const fileType of this.debugNodes[output]) {
                // the detached head of Netron we are using expects a base64 encoded string
                const fileTypeProps = {name: 'file_type', type: 'STRING', s: window.btoa(fileType) };
                const fileTypeAttrProto = onnx.AttributeProto.fromObject(fileTypeProps);
                const parentDebugDir = mkdir(md5(output));
                const debugDir = mkdir(path.join(parentDebugDir, fileType));
                const filePathProps = {name: 'file_path', type: 'STRING', s: window.btoa(debugDir)};
                const filePathAttrProto = onnx.AttributeProto.fromObject(filePathProps);
                
                const nodeProps = {attribute: [fileTypeAttrProto, filePathAttrProto],
                                     input: [output], opType: 'Debug', output: ['unused_' + output + fileType]};
                nodeProtos.push(onnx.NodeProto.fromObject(nodeProps));
            }

           
        }
        return nodeProtos;
    }
}

export const ModelProtoSingleton = new ModelProto();
