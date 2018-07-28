export interface IMetadataProps {
    [key: string]: string
}

export interface IProperties {
    [key: string]: string
}

export default interface IState {
    inputs: { [key: string]: any },
    metadataProps: IMetadataProps,
    modelInputs: { [key: string]: any },
    modelOutputs: { [key: string]: any },
    nodes: { [key: string]: any },
    outputs: { [key: string]: any },
    properties: IProperties,
    selectedNode: string,
}
