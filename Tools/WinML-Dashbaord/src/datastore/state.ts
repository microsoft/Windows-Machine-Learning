export interface IMetadataProps {
    [key: string]: string
}

export interface IProperties {
    [key: string]: string
}

export default interface IState {
    file: File,
    inputs: { [key: string]: any },
    metadataProps: IMetadataProps,
    modelInputs: string[],
    modelOutputs: string[],
    nodes: { [key: string]: any },
    outputs: { [key: string]: any },
    properties: IProperties,
    selectedNode: string,
}
