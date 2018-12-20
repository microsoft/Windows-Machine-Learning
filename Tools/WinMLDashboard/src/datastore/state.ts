export interface IMetadataProps {
    [key: string]: string
}

export interface IProperties {
    [key: string]: string
}

export interface IDebugNode {
    output: string,
    fileType: string,
}

export default interface IState {
    showLeft: boolean,
    showRight: boolean,

    file: File,
    saveFileName: string,

    debugNodes: IDebugNode[],
    inputs: { [key: string]: any },
    intermediateOutputs: string[],
    metadataProps: IMetadataProps,
    modelInputs: string[],
    modelOutputs: string[],
    nodes: { [key: string]: any },
    outputs: { [key: string]: any },
    properties: IProperties,
    selectedNode: string,
}
