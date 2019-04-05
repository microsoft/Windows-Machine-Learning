export interface IMetadataProps {
    [key: string]: string
}

export interface IProperties {
    [key: string]: string
}

export interface IDebugNodeMap {
    [output: string]: DebugFormat[];
}

// Each IInsertNodeChild.oldInputName matches an input in IInsertNode.inputs
// Each IInsertNodeChild.newInputName matches and input an output in IInsertNode.outputs
export interface IInsertNode {
    opName: string,
    inputs: string[],
    outputs: string[],
    children: [IInsertNodeChild],
}

export interface IInsertNodeChild {
    nodeDefinition: INodeProtoEssential,
    oldInputName: string,
    newInputName: string,
}

export interface INodeProtoEssential {
    name: string,
    op_type: string,
    domain: string,
}

export interface INodeProtoIO extends INodeProtoEssential {
    input: string[]
    output: string[]
}

export enum DebugFormat {
    text = "txt",
    png = "png",
}

export default interface IState {
    showLeft: boolean,
    showRight: boolean,

    file: File,
    saveFileName: string,

    debugNodes: IDebugNodeMap,
    inputs: { [key: string]: any },
    metadataProps: IMetadataProps,
    modelInputs: string[],
    modelOutputs: string[],
    nodes: { [key: string]: any },
    outputs: { [key: string]: any },
    properties: IProperties,
    // quantization options: {'none', 'RS5', '19H1'}
    quantizationOption: string,
    selectedNode: string,
}
