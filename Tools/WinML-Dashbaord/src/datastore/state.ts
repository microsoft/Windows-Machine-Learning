export interface IMetadataProps {
    [key: string]: string
}

export interface IProperties {
    [key: string]: string
}

export default interface IState {
    graph: any,
    metadataProps: IMetadataProps,
    properties: IProperties,
}
