export interface IMetadataProps {
    [key: string]: string
}

export interface IProperties {
    [key: string]: string
}

export default interface IState {
    showLeft: boolean,
    showRight: boolean,

    file: File,
    saveFileName: string,

    inputs: { [key: string]: any },
    metadataProps: IMetadataProps,
    modelInputs: string[],
    modelOutputs: string[],
    nodes: { [key: string]: any },
    outputs: { [key: string]: any },
    properties: IProperties,
    // quantization options: {'none', 'RS5', '19H1'}
    quatizationOption: string,
    selectedNode: string,
}
