export interface IMetadataProps {
    [key: string]: string
}

export interface IProperties {
    [key: string]: string
}

export interface IDebugNodeMap {
    [output: string]: DebugFormat[];
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
    selectedNode: string,
}
