export interface IMetadataProps {
    [key: string]: string
}

export interface IProperties {
    [key: string]: string
}

export default interface IState {
    metadataProps: IMetadataProps,
    nodes: any,
    properties: IProperties,
    selectedNode: string,
}
