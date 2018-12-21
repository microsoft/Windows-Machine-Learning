import * as md5 from 'md5';

export interface IMetadataProps {
    [key: string]: string
}

export interface IProperties {
    [key: string]: string
}

export interface IDebugNode {
    output: string,
    fileType: string,
    getMd5Hash(): string,
}

export class DebugNode implements IDebugNode {
    public output: string;
    public fileType: string;

    constructor(output: string, fileType: string) {
        this.output = output;
        this.fileType = fileType;
    }

    public getMd5Hash() {
        return md5(this.output + this.fileType);
    }
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
