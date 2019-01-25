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

export enum InsertDirection {
    Before = "Before",
    After = "After",
}

export interface IInsertNode {
    requiredAttributes: IAttribute[];
    optionalAttributes: IAttribute[];
    readonly opType: InsertOperatorType;
    input: string[];
    output: string[];
}

export class InsertNode implements IInsertNode {
    public requiredAttributes: IAttribute[];
    public optionalAttributes: IAttribute[];
    public readonly opType: InsertOperatorType;
    public input: string[];
    public output: string[];
    constructor(opType: InsertOperatorType) {
        this.opType = opType;
        switch (opType) {
            case InsertOperatorType.Clip:
                this.requiredAttributes = [{name: "max", type: AttributeType.float,  value: null}, 
                                    {name: "min", type: AttributeType.float, value: null}];
                this.optionalAttributes = [];

                break;
            default:
                this.requiredAttributes = [];
                this.optionalAttributes = [];
        }
    }
}

export interface IAttribute {
    name: string;
    type: AttributeType;
    value: any;
}

// for now these are the attributes allowed for supported insert operators
export enum AttributeType {
    float = "FLOAT",
    integer = "INT",
    string = "STRING",
}

// for now these are the supported insert operators
export enum InsertOperatorType {
    Add = "Add",
    Mul = "Mul",
    Clip = "Clip",
}

export default interface IState {
    showLeft: boolean,
    showRight: boolean,

    file: File,
    saveFileName: string,

    debugNodes: IDebugNodeMap,
    inputs: { [key: string]: any },
    insertNodes: IInsertNode[],
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
