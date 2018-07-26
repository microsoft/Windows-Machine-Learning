import { IMetadataProps, IProperties } from './state';

export const UPDATE_GRAPH = 'UPDATE_GRAPH';
export const UPDATE_INPUTS = 'UPDATE_INPUTS';
export const UPDATE_OUTPUTS = 'UPDATE_OUTPUTS';
export const UPDATE_METADATA_PROPS = 'UPDATE_METADATA_PROPS';
export const UPDATE_PROPERTIES = 'UPDATE_PROPERTIES';

interface IValueInfo {
    description?: string,
    id: string,
    name: string,
    type: string,
}

export interface IAction {
    type: string,
    graph: any,
    inputs: IValueInfo,
    metadataProps: IMetadataProps,
    outputs: IValueInfo,
    properties: IProperties,
}
