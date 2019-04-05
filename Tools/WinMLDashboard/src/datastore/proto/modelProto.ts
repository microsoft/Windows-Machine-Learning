import * as md5 from 'md5';
import * as path from 'path';
import { getLocalDebugDir, mkdir } from '../../native/appData';
import { IDebugNodeMap, IInsertNode, IInsertNodeChild, IMetadataProps, INodeProtoEssential, INodeProtoIO } from '../state';
import { Proto } from './proto';


class ModelProto extends Proto {

    // debug nodes will be added to the model proto only right before serialization
    // now serialization with be parametrized by whether we are serializing the debugged model or not
    private debugNodes: IDebugNodeMap;

    public setDebugNodes(debugNodes: IDebugNodeMap) {
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
        if (debug) {     
            // only need to copy parts of this.proto which are mutated during the debug case
            const nodeClone = this.proto.graph.node.slice();       
            this.proto.graph.node = [ ...this.proto.graph.node, ...this.createDebugProtoNodes() ]
            const writer = Proto.types.ModelProto.encode(this.proto);
            const data = writer.finish();
            this.proto.graph.node = nodeClone;
            return data;
        } else {
            const writer = Proto.types.ModelProto.encode(this.proto);
            return writer.finish();
        }
    }

    public getCurrentModelDebugDir() {
        return path.join(getLocalDebugDir(), md5(this.proto.domain + this.proto.model_version + this.proto.graph.name));
    }

    public insertNode(insertNode: IInsertNode, children: IInsertNodeChild[]) {
        const onnx = Proto.getOnnx();
        if (!onnx || !this.validateInsertAction(insertNode, children)){
            return;
        }

        // construct insert node
        const nodeProps = {
            attribute: [], input: insertNode.inputs, output: insertNode.outputs
        }

        const insertProto = onnx.NodeProto.fromObject(nodeProps);
        const nodeList = this.proto.graph.node as INodeProtoEssential[];

        // get list of children proto
        let insertIdx = 0;
        for (let i = nodeList.length - 1; i >= 0; i--) {
            const currNode = nodeList[i] as INodeProtoIO;
            // find last parent output
            if (insertIdx === 0 && (currNode).output
                                    .filter((value: string) => insertNode.inputs.includes(value))
                                    .length > 0) {
                // topological sort insert location
                insertIdx = i + 1;
                break;
            }
            // see if current node matches and child nodes
            for (const child of children) {
                if (this.nodeIdsEqual(currNode, (child as IInsertNodeChild).nodeDefinition)) {
                    const inputIdx = currNode.input.indexOf((child as IInsertNodeChild).oldInputName)
                    if (inputIdx !== -1) {
                        currNode.input[inputIdx] = child.newInputName
                    }
                }
            }
        }
        // insert node
        nodeList.splice(insertIdx, 0, insertProto)
    }

    private nodeIdsEqual(a: INodeProtoEssential, b: INodeProtoEssential) : boolean {
        return a.name === b.name && a.op_type === b.op_type && a.domain === b.domain
    }

    private validateInsertAction(insertNode: IInsertNode, children: IInsertNodeChild[]) : boolean {
        for (const child of children) {
            if (!insertNode.inputs.includes((child as IInsertNodeChild).oldInputName) &&
                insertNode.outputs.includes((child as IInsertNodeChild).newInputName)) {
                return false;
            }
        }
        return true;
    }

    private createDebugProtoNodes() {
        const onnx = Proto.getOnnx();
        if (!onnx) {
            return [];
        }
        const modelDir = mkdir(this.getCurrentModelDebugDir())
        const nodeProtos = [];
        for (const output of Object.keys(this.debugNodes)) {
            for (const fileType of this.debugNodes[output]) {
                // the detached head of Netron we are using expects a base64 encoded string
                const fileTypeProps = {name: 'file_type', type: 'STRING', s: window.btoa(fileType) };
                const fileTypeAttrProto = onnx.AttributeProto.fromObject(fileTypeProps);
                const separatorRegex = /\\|\//g 
                const nodeDir = mkdir(modelDir, output.replace(separatorRegex, "-"))
                const typeDir = mkdir(nodeDir, fileType);
                const filePathProps = {name: 'file_path', type: 'STRING', s: window.btoa(path.join(typeDir, 'debug'))};
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
